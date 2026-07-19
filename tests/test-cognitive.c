/*
 * Cognitive Primitives Test
 * Exercises fixed-point PLN truth-value math and a minimal atomspace
 * on bare Mach: atoms live in vm_allocate memory, travel through a
 * mach_msg round trip, and attention updates are exact-integer
 * deterministic.  See docs/cogwxp-integration.md for background.
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
 */

#include <mach/message.h>
#include <mach/vm_param.h>

#include <testlib.h>

#include <mach.user.h>
#include <mach_port.user.h>

#include <cognitive/cogpln.h>
#include <cognitive/cogspace.h>

#define COG_ARENA_BYTES (256 * 1024)
#define COG_MSG_ID 5400		/* reserved cognitive subsystem number */

struct cog_message {
	mach_msg_header_t header;
	mach_msg_type_t type;
	struct cog_atom_wire wire;
};

static struct cogspace cs;

static cog_handle_t h_socrates, h_man, h_mortal;

/* Leg 1: cognitive memory backed by Mach VM. */
static void test_vm_leg(void)
{
	vm_offset_t arena = 0;
	kern_return_t err;
	uint32_t capacity;

	err = vm_allocate(mach_task_self(), &arena, COG_ARENA_BYTES, TRUE);
	ASSERT_RET(err, "vm_allocate cognitive arena");

	capacity = cogspace_init(&cs, (void *) arena, COG_ARENA_BYTES);
	ASSERT(capacity > 0, "cogspace capacity");
	printf("cognitive: arena capacity %d atoms\n", (int) capacity);
}

/* Leg 2: exact fixed-point PLN inference. */
static void test_pln_leg(void)
{
	cog_tv_t tv_man = { 5000, 9000, 20 };
	cog_tv_t tv_mortal = { 6000, 9000, 20 };
	cog_tv_t tv_socrates = { 9000, 9500, 10 };
	cog_tv_t tv_s_man = { 8000, 9000, 10 };
	cog_tv_t tv_man_mortal = { 9000, 9000, 10 };
	cog_tv_t tv_other = { 6000, 0, 30 };
	cog_tv_t deduced, revised, conj, neg;
	cog_handle_t out2[2];
	cog_handle_t l1, l2;

	h_socrates = cogspace_add_node(&cs, COG_CONCEPT_NODE, "socrates",
				       tv_socrates);
	h_man = cogspace_add_node(&cs, COG_CONCEPT_NODE, "man", tv_man);
	h_mortal = cogspace_add_node(&cs, COG_CONCEPT_NODE, "mortal",
				     tv_mortal);
	ASSERT(h_socrates != COG_HANDLE_NULL, "add socrates");
	ASSERT(h_man != COG_HANDLE_NULL, "add man");
	ASSERT(h_mortal != COG_HANDLE_NULL, "add mortal");

	out2[0] = h_socrates;
	out2[1] = h_man;
	l1 = cogspace_add_link(&cs, COG_INHERITANCE_LINK, out2, 2, tv_s_man);
	out2[0] = h_man;
	out2[1] = h_mortal;
	l2 = cogspace_add_link(&cs, COG_INHERITANCE_LINK, out2, 2,
			       tv_man_mortal);
	ASSERT(l1 != COG_HANDLE_NULL && l2 != COG_HANDLE_NULL, "add links");

	deduced = cog_tv_deduction(cogspace_get(&cs, l1)->tv,
				   cogspace_get(&cs, l2)->tv,
				   cogspace_get(&cs, h_man)->tv,
				   cogspace_get(&cs, h_mortal)->tv);
	printf("cognitive: deduction strength %d\n", (int) deduced.strength);
	ASSERT(deduced.strength == 7800, "deduction strength == 7800");
	ASSERT(deduced.confidence == 9000, "deduction confidence == 9000");

	revised = cog_tv_revision(deduced, tv_other);
	printf("cognitive: revision strength %d confidence %d\n",
	       (int) revised.strength, (int) revised.confidence);
	ASSERT(revised.strength == 6450, "revision strength == 6450");
	ASSERT(revised.confidence == 476, "revision confidence == 476");

	conj = cog_tv_and(tv_s_man, tv_man);
	ASSERT(conj.strength == 4000, "conjunction strength == 4000");

	neg = cog_tv_not(tv_s_man);
	ASSERT(neg.strength == 2000, "negation strength == 2000");
}

/* Leg 3: one atom through a Mach message round trip. */
static void test_ipc_leg(void)
{
	mach_port_t port;
	kern_return_t err;
	struct cog_message msg;
	struct cog_atom *orig;
	struct cog_atom *copy;
	cog_handle_t h_copy;
	uint32_t i;

	err = mach_port_allocate(mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE, &port);
	ASSERT_RET(err, "mach_port_allocate");

	cogspace_stimulate(&cs, h_socrates, 300);
	orig = cogspace_get(&cs, h_socrates);
	ASSERT(orig != 0, "socrates lookup");
	cog_atom_pack(orig, &msg.wire);

	msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
	msg.header.msgh_remote_port = port;
	msg.header.msgh_local_port = MACH_PORT_NULL;
	msg.header.msgh_id = COG_MSG_ID;
	msg.header.msgh_size = sizeof(msg);
	msg.type.msgt_name = MACH_MSG_TYPE_BYTE;
	msg.type.msgt_size = 8;
	msg.type.msgt_number = sizeof(struct cog_atom_wire);
	msg.type.msgt_inline = TRUE;
	msg.type.msgt_longform = FALSE;
	msg.type.msgt_deallocate = FALSE;
	msg.type.msgt_unused = 0;

	err = mach_msg(&msg.header, MACH_SEND_MSG, msg.header.msgh_size,
		       0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
		       MACH_PORT_NULL);
	ASSERT_RET(err, "mach_msg send atom");

	for (i = 0; i < sizeof(msg.wire); i++)
		((char *) &msg.wire)[i] = 0;

	err = mach_msg(&msg.header, MACH_RCV_MSG, 0, sizeof(msg), port,
		       MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	ASSERT_RET(err, "mach_msg receive atom");
	ASSERT(msg.header.msgh_id == COG_MSG_ID, "message id preserved");

	/* Wire vs original. */
	ASSERT(msg.wire.type == COG_CONCEPT_NODE, "wire type");
	ASSERT(cog_streq(msg.wire.name, "socrates"), "wire name");
	ASSERT(msg.wire.strength == orig->tv.strength, "wire strength");
	ASSERT(msg.wire.confidence == orig->tv.confidence,
	       "wire confidence");
	ASSERT(msg.wire.sti == 300, "wire sti");

	/* Wire vs reconstructed atom.  A fresh name forces insertion. */
	cog_strcpy(msg.wire.name, "socrates_rx");
	h_copy = cog_atom_unpack(&msg.wire, &cs);
	copy = cogspace_get(&cs, h_copy);
	ASSERT(copy != 0, "unpacked atom lookup");
	ASSERT(copy->type == COG_CONCEPT_NODE, "unpacked type");
	ASSERT(copy->tv.strength == orig->tv.strength, "unpacked strength");
	ASSERT(copy->tv.confidence == orig->tv.confidence,
	       "unpacked confidence");
	ASSERT(copy->sti == 300, "unpacked sti");

	/* Neutralize the copy's STI so the attention leg stays exact. */
	cogspace_stimulate(&cs, h_copy, -300);

	printf("cognitive: atom crossed mach_msg intact\n");
}

/* Leg 4: deterministic attention dynamics. */
static void test_attention_leg(void)
{
	cog_handle_t top[3];
	uint32_t n;

	/* socrates already has 300 from the IPC leg. */
	cogspace_stimulate(&cs, h_man, 200);
	cogspace_stimulate(&cs, h_mortal, 100);

	n = cogspace_top_sti(&cs, top, 3);
	ASSERT(n == 3, "top-sti count");
	ASSERT(top[0] == h_socrates, "top-sti first");
	ASSERT(top[1] == h_man, "top-sti second");
	ASSERT(top[2] == h_mortal, "top-sti third");

	cogspace_decay(&cs, 100);
	ASSERT(cogspace_get(&cs, h_socrates)->sti == 270, "decay socrates");
	ASSERT(cogspace_get(&cs, h_man)->sti == 180, "decay man");
	ASSERT(cogspace_get(&cs, h_mortal)->sti == 90, "decay mortal");

	printf("cognitive: attention order and decay exact\n");
}

int main(int argc, char *argv[], int envc, char *envp[])
{
	test_vm_leg();
	test_pln_leg();
	test_ipc_leg();
	test_attention_leg();

	printf("cognitive primitives test: PASS\n");
	return 0;
}
