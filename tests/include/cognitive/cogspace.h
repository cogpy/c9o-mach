/*
 * Minimal atomspace for freestanding Mach test modules.
 *
 * Copyright (C) 2026 Free Software Foundation
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program ; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Provenance: semantics derived from the CoGWXP-OS9 reference corpus
 * atomspace model (cogwxp/opencog/atomspace/atomspace.c): typed atoms
 * with names, incoming/outgoing sets, truth values, and short-term
 * importance (STI) attention values.  Clean-room reimplementation for
 * a freestanding environment; no code was copied.
 *
 * Storage is a caller-supplied arena (the test backs it with
 * vm_allocate), there is no allocator and no locking: the intended
 * consumer is a single-threaded Mach user test module built with
 * -ffreestanding -nolibc.
 */

#ifndef _COGNITIVE_COGSPACE_H_
#define _COGNITIVE_COGSPACE_H_

#include <stdint.h>
#include <cognitive/cogpln.h>

typedef uint32_t cog_handle_t;	/* 0 is the invalid handle */

#define COG_HANDLE_NULL		((cog_handle_t) 0)
#define COG_NAME_MAX		32
#define COG_MAX_OUTGOING	4
#define COG_MAX_INCOMING	8

enum cog_atom_type {
	COG_CONCEPT_NODE = 1,
	COG_PREDICATE_NODE = 2,
	COG_INHERITANCE_LINK = 100,
	COG_IMPLICATION_LINK = 101,
	COG_EVALUATION_LINK = 102,
};

struct cog_atom {
	uint16_t type;
	uint16_t in_use;
	uint32_t name_hash;
	char name[COG_NAME_MAX];
	cog_tv_t tv;
	int32_t sti;
	uint32_t out_count;
	cog_handle_t outgoing[COG_MAX_OUTGOING];
	uint32_t in_count;
	cog_handle_t incoming[COG_MAX_INCOMING];
};

struct cogspace {
	struct cog_atom *atoms;
	uint32_t capacity;
	uint32_t count;
};

/*
 * Fixed wire representation for sending one atom through a Mach
 * message body.  Every field is 4 bytes wide, so the layout has no
 * padding under natural alignment on both i686 and x86_64 user ABIs.
 */
struct cog_atom_wire {
	uint32_t type;
	char name[COG_NAME_MAX];
	uint32_t strength;
	uint32_t confidence;
	int32_t sti;
};

/* Freestanding string helpers; no libc dependency. */

static inline int cog_streq(const char *a, const char *b)
{
	uint32_t i;

	for (i = 0; i < COG_NAME_MAX; i++) {
		if (a[i] != b[i])
			return 0;
		if (a[i] == '\0')
			return 1;
	}
	return 1;
}

static inline void cog_strcpy(char *dst, const char *src)
{
	uint32_t i;

	for (i = 0; i < COG_NAME_MAX - 1 && src[i] != '\0'; i++)
		dst[i] = src[i];
	for (; i < COG_NAME_MAX; i++)
		dst[i] = '\0';
}

/* FNV-1a 32-bit hash. */
static inline uint32_t cog_name_hash(const char *s)
{
	uint32_t h = 2166136261u;
	uint32_t i;

	for (i = 0; i < COG_NAME_MAX && s[i] != '\0'; i++) {
		h ^= (uint8_t) s[i];
		h *= 16777619u;
	}
	return h;
}

/*
 * Initialize a cogspace over a caller-supplied arena.
 * Returns the atom capacity, or 0 if the arena is too small.
 */
static inline uint32_t cogspace_init(struct cogspace *cs, void *arena,
				     uint32_t bytes)
{
	uint32_t capacity = bytes / (uint32_t) sizeof(struct cog_atom);
	uint8_t *p = (uint8_t *) arena;
	uint32_t i;

	if (capacity == 0)
		return 0;
	for (i = 0; i < capacity * (uint32_t) sizeof(struct cog_atom); i++)
		p[i] = 0;
	cs->atoms = (struct cog_atom *) arena;
	cs->capacity = capacity;
	cs->count = 0;
	return capacity;
}

static inline struct cog_atom *cogspace_get(struct cogspace *cs,
					    cog_handle_t h)
{
	if (h == COG_HANDLE_NULL || h > cs->capacity)
		return 0;
	if (!cs->atoms[h - 1].in_use)
		return 0;
	return &cs->atoms[h - 1];
}

/*
 * Add a named node.  Duplicate (type, name) insertions revise the
 * stored truth value instead of creating a second atom.
 * Returns the handle, or COG_HANDLE_NULL when the space is full.
 */
static inline cog_handle_t cogspace_add_node(struct cogspace *cs,
					     uint16_t type,
					     const char *name, cog_tv_t tv)
{
	uint32_t hash = cog_name_hash(name);
	uint32_t i;
	struct cog_atom *a;

	for (i = 0; i < cs->capacity; i++) {
		a = &cs->atoms[i];
		if (a->in_use && a->type == type && a->name_hash == hash
		    && cog_streq(a->name, name)) {
			a->tv = cog_tv_revision(a->tv, tv);
			return (cog_handle_t) (i + 1);
		}
	}
	if (cs->count >= cs->capacity)
		return COG_HANDLE_NULL;
	a = &cs->atoms[cs->count];
	a->type = type;
	a->in_use = 1;
	a->name_hash = hash;
	cog_strcpy(a->name, name);
	a->tv = tv;
	a->sti = 0;
	a->out_count = 0;
	a->in_count = 0;
	cs->count++;
	return (cog_handle_t) cs->count;
}

/*
 * Add a link over existing atoms.  Each target records the link in its
 * incoming set.  Returns COG_HANDLE_NULL when a target is invalid or a
 * capacity limit is reached.
 */
static inline cog_handle_t cogspace_add_link(struct cogspace *cs,
					     uint16_t type,
					     const cog_handle_t *outgoing,
					     uint32_t n, cog_tv_t tv)
{
	struct cog_atom *a;
	struct cog_atom *t;
	cog_handle_t h;
	uint32_t i;

	if (n > COG_MAX_OUTGOING)
		return COG_HANDLE_NULL;
	for (i = 0; i < n; i++) {
		t = cogspace_get(cs, outgoing[i]);
		if (t == 0 || t->in_count >= COG_MAX_INCOMING)
			return COG_HANDLE_NULL;
	}
	if (cs->count >= cs->capacity)
		return COG_HANDLE_NULL;

	a = &cs->atoms[cs->count];
	a->type = type;
	a->in_use = 1;
	a->name_hash = 0;
	a->name[0] = '\0';
	a->tv = tv;
	a->sti = 0;
	a->out_count = n;
	a->in_count = 0;
	cs->count++;
	h = (cog_handle_t) cs->count;

	for (i = 0; i < n; i++) {
		a->outgoing[i] = outgoing[i];
		t = cogspace_get(cs, outgoing[i]);
		t->incoming[t->in_count++] = h;
	}
	return h;
}

/* Add short-term importance to one atom. */
static inline void cogspace_stimulate(struct cogspace *cs, cog_handle_t h,
				      int32_t amount)
{
	struct cog_atom *a = cogspace_get(cs, h);

	if (a != 0)
		a->sti += amount;
}

/* Decay every atom's STI by rate_permille/1000. */
static inline void cogspace_decay(struct cogspace *cs,
				  uint32_t rate_permille)
{
	uint32_t i;
	int64_t s;

	if (rate_permille > 1000)
		rate_permille = 1000;
	for (i = 0; i < cs->capacity; i++) {
		if (!cs->atoms[i].in_use)
			continue;
		s = (int64_t) cs->atoms[i].sti * (1000 - (int32_t) rate_permille);
		cs->atoms[i].sti = (int32_t) (s / 1000);
	}
}

/*
 * Select the top-k atoms by STI into out[], highest first.
 * Ties break toward the lower handle.  Returns how many were written.
 */
static inline uint32_t cogspace_top_sti(struct cogspace *cs,
					cog_handle_t *out, uint32_t k)
{
	uint32_t written = 0;
	uint32_t i, j;
	cog_handle_t best;
	int32_t best_sti;
	int taken;

	while (written < k) {
		best = COG_HANDLE_NULL;
		best_sti = 0;
		for (i = 0; i < cs->capacity; i++) {
			if (!cs->atoms[i].in_use)
				continue;
			taken = 0;
			for (j = 0; j < written; j++)
				if (out[j] == (cog_handle_t) (i + 1))
					taken = 1;
			if (taken)
				continue;
			if (best == COG_HANDLE_NULL
			    || cs->atoms[i].sti > best_sti) {
				best = (cog_handle_t) (i + 1);
				best_sti = cs->atoms[i].sti;
			}
		}
		if (best == COG_HANDLE_NULL)
			break;
		out[written++] = best;
	}
	return written;
}

/* Pack one atom into its wire form. */
static inline void cog_atom_pack(const struct cog_atom *a,
				 struct cog_atom_wire *w)
{
	uint32_t i;

	w->type = a->type;
	for (i = 0; i < COG_NAME_MAX; i++)
		w->name[i] = a->name[i];
	w->strength = a->tv.strength;
	w->confidence = a->tv.confidence;
	w->sti = a->sti;
}

/*
 * Unpack a wire atom into the space as a node, returning its handle.
 * The wire carries no evidence count; unpacked atoms start at count 1.
 */
static inline cog_handle_t cog_atom_unpack(const struct cog_atom_wire *w,
					   struct cogspace *cs)
{
	cog_tv_t tv;
	cog_handle_t h;
	struct cog_atom *a;

	tv.strength = w->strength;
	tv.confidence = w->confidence;
	tv.count = 1;
	h = cogspace_add_node(cs, (uint16_t) w->type, w->name, tv);
	a = cogspace_get(cs, h);
	if (a != 0)
		a->sti = w->sti;
	return h;
}

#endif /* _COGNITIVE_COGSPACE_H_ */
