/*
 * Freestanding quicksort for Mach test modules.
 *
 * Faithful port of the Plan 9 / Harvey OS qsort
 * (sys/src/libc/port/qsort.c), adapted to a p9_ namespace and static
 * linkage.  Median-of-three pivot, in-place, no allocation.  The
 * comparator takes two element pointers, as in Plan 9 (note this
 * differs from ISO C qsort's const void* signature).
 *
 * Upstream copyright and permission notice, preserved as required:
 *
 *   Copyright 2021 Plan 9 Foundation
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use, copy,
 *   modify, merge, publish, distribute, sublicense, and/or sell copies
 *   of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT.
 *
 * The GNU Mach adaptation is distributed under the GNU General Public
 * License version 2 or later, consistent with the rest of this tree.
 */

#ifndef _PLAN9_SORT_H_
#define _PLAN9_SORT_H_

#include <stdint.h>

typedef struct p9_sort {
	int (*cmp)(void *, void *);
	void (*swap)(char *, char *, long);
	long es;
} p9_sort_t;

static void p9_swapb(char *i, char *j, long es)
{
	char c;

	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es--;
	} while (es != 0);
}

static void p9_swapi(char *ii, char *ij, long es)
{
	long *i, *j, c;

	i = (long *) ii;
	j = (long *) ij;
	do {
		c = *i;
		*i++ = *j;
		*j++ = c;
		es -= sizeof(long);
	} while (es != 0);
}

static char *p9_pivot(char *a, long n, p9_sort_t *p)
{
	long j;
	char *pi, *pj, *pk;

	j = n / 6 * p->es;
	pi = a + j;		/* 1/6 */
	j += j;
	pj = pi + j;		/* 1/2 */
	pk = pj + j;		/* 5/6 */
	if (p->cmp(pi, pj) < 0) {
		if (p->cmp(pi, pk) < 0) {
			if (p->cmp(pj, pk) < 0)
				return pj;
			return pk;
		}
		return pi;
	}
	if (p->cmp(pj, pk) < 0) {
		if (p->cmp(pi, pk) < 0)
			return pi;
		return pk;
	}
	return pj;
}

static void p9_qsorts(char *a, long n, p9_sort_t *p)
{
	long j, es;
	char *pi, *pj, *pn;

	es = p->es;
	while (n > 1) {
		if (n > 10)
			pi = p9_pivot(a, n, p);
		else
			pi = a + (n >> 1) * es;

		p->swap(a, pi, es);
		pi = a;
		pn = a + n * es;
		pj = pn;
		for (;;) {
			do
				pi += es;
			while (pi < pn && p->cmp(pi, a) < 0);
			do
				pj -= es;
			while (pj > a && p->cmp(pj, a) > 0);
			if (pj < pi)
				break;
			p->swap(pi, pj, es);
		}
		p->swap(a, pj, es);
		j = (pj - a) / es;

		n = n - j - 1;
		if (j >= n) {
			p9_qsorts(a, j, p);
			a += (j + 1) * es;
		} else {
			p9_qsorts(a + (j + 1) * es, n, p);
			n = j;
		}
	}
}

/*
 * Sort n elements of es bytes each at va, ordered by cmp.  cmp returns
 * negative, zero, or positive as its first argument sorts before,
 * equal to, or after its second.
 */
static void p9_qsort(void *va, long n, long es,
		     int (*cmp)(void *, void *))
{
	p9_sort_t s;

	s.cmp = cmp;
	s.es = es;
	s.swap = p9_swapi;
	if (((uintptr_t) va | es) % sizeof(long))
		s.swap = p9_swapb;
	p9_qsorts((char *) va, n, &s);
}

#endif /* _PLAN9_SORT_H_ */
