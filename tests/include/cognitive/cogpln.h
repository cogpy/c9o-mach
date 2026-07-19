/*
 * Fixed-point PLN truth-value formulas for freestanding Mach test modules.
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
 * Provenance: truth-value semantics derived from the published
 * Probabilistic Logic Networks formulas as realized in the CoGWXP-OS9
 * reference corpus (cogwxp/opencog/pln/pln.c).  This is a clean-room
 * fixed-point reimplementation; no code was copied.  Numeric
 * compatibility with floating-point implementations is not claimed;
 * the rounding rules below bound the divergence.
 *
 * Rounding rules:
 *   - cog_fx_mul rounds half up.
 *   - cog_tv_revision truncates its weighted mean and its confidence.
 *   - Intermediate signed terms are clamped to [0, COG_TV_SCALE].
 */

#ifndef _COGNITIVE_COGPLN_H_
#define _COGNITIVE_COGPLN_H_

#include <stdint.h>

/* All strengths and confidences are fixed-point fractions of this scale. */
#define COG_TV_SCALE 10000u

/* PLN default lookahead constant K used in count-to-confidence mapping. */
#define COG_TV_K 800u

typedef struct {
	uint32_t strength;	/* 0 .. COG_TV_SCALE */
	uint32_t confidence;	/* 0 .. COG_TV_SCALE */
	uint32_t count;		/* evidence count, used by revision */
} cog_tv_t;

static inline uint32_t cog_fx_clamp(int64_t v)
{
	if (v < 0)
		return 0;
	if (v > (int64_t) COG_TV_SCALE)
		return COG_TV_SCALE;
	return (uint32_t) v;
}

/* Fixed-point multiply, round half up. */
static inline uint32_t cog_fx_mul(uint32_t a, uint32_t b)
{
	return (uint32_t) (((uint64_t) a * b + COG_TV_SCALE / 2)
			   / COG_TV_SCALE);
}

/* Fixed-point divide (a/b scaled); 0 when b is 0. */
static inline uint32_t cog_fx_div(uint32_t a, uint32_t b)
{
	if (b == 0)
		return 0;
	return (uint32_t) (((uint64_t) a * COG_TV_SCALE) / b);
}

static inline uint32_t cog_fx_min(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

/*
 * Deduction: A->B and B->C entail A->C.
 *   sAC = sAB*sBC + (1 - sAB) * (sC - sB*sBC) / (1 - sB)
 * When 1 - sB is 0 the correction term is undefined; fall back to sC.
 * Confidence is the minimum of the premise confidences.
 */
static inline cog_tv_t cog_tv_deduction(cog_tv_t ab, cog_tv_t bc,
					cog_tv_t b, cog_tv_t c)
{
	cog_tv_t r;
	uint32_t one_m_sab = COG_TV_SCALE - ab.strength;
	uint32_t one_m_sb = COG_TV_SCALE - b.strength;
	int64_t corr;

	if (one_m_sb == 0) {
		r.strength = c.strength;
	} else {
		corr = (int64_t) c.strength
		       - (int64_t) cog_fx_mul(b.strength, bc.strength);
		corr = (corr * (int64_t) COG_TV_SCALE) / (int64_t) one_m_sb;
		corr = ((int64_t) one_m_sab * corr) / (int64_t) COG_TV_SCALE;
		r.strength = cog_fx_clamp(
			(int64_t) cog_fx_mul(ab.strength, bc.strength) + corr);
	}
	r.confidence = cog_fx_min(ab.confidence, bc.confidence);
	r.count = cog_fx_min(ab.count, bc.count);
	return r;
}

/*
 * Revision: merge two estimates of the same statement, weighting each
 * strength by its evidence count.  Confidence follows the PLN
 * count-to-confidence mapping n / (n + K).
 */
static inline cog_tv_t cog_tv_revision(cog_tv_t t1, cog_tv_t t2)
{
	cog_tv_t r;
	uint64_t n = (uint64_t) t1.count + t2.count;

	if (n == 0) {
		r.strength = (t1.strength + t2.strength) / 2;
		r.confidence = 0;
		r.count = 0;
		return r;
	}
	r.strength = (uint32_t) (((uint64_t) t1.count * t1.strength
				  + (uint64_t) t2.count * t2.strength) / n);
	r.confidence = (uint32_t) ((n * COG_TV_SCALE) / (n + COG_TV_K));
	r.count = (uint32_t) n;
	return r;
}

/* Modus ponens: from A and A->B, conclude B. */
static inline cog_tv_t cog_tv_modus_ponens(cog_tv_t impl, cog_tv_t a)
{
	cog_tv_t r;

	r.strength = cog_fx_mul(impl.strength, a.strength);
	r.confidence = cog_fx_mul(impl.confidence, a.confidence);
	r.count = cog_fx_min(impl.count, a.count);
	return r;
}

/* Conjunction under independence. */
static inline cog_tv_t cog_tv_and(cog_tv_t a, cog_tv_t b)
{
	cog_tv_t r;

	r.strength = cog_fx_mul(a.strength, b.strength);
	r.confidence = cog_fx_mul(a.confidence, b.confidence);
	r.count = cog_fx_min(a.count, b.count);
	return r;
}

/* Disjunction under independence. */
static inline cog_tv_t cog_tv_or(cog_tv_t a, cog_tv_t b)
{
	cog_tv_t r;

	r.strength = cog_fx_clamp((int64_t) a.strength + b.strength
				  - cog_fx_mul(a.strength, b.strength));
	r.confidence = cog_fx_mul(a.confidence, b.confidence);
	r.count = cog_fx_min(a.count, b.count);
	return r;
}

/* Negation. */
static inline cog_tv_t cog_tv_not(cog_tv_t a)
{
	cog_tv_t r;

	r.strength = COG_TV_SCALE - a.strength;
	r.confidence = a.confidence;
	r.count = a.count;
	return r;
}

#endif /* _COGNITIVE_COGPLN_H_ */
