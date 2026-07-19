/*
 * UTF-8 codec for freestanding Mach test modules.
 *
 * Faithful port of the Plan 9 / Harvey OS rune library
 * (sys/src/libc/port/rune.c), adapted to fixed-width types, a p9_
 * namespace, and static linkage so it builds with -ffreestanding.
 * The decoding and encoding logic is unchanged.
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

#ifndef _PLAN9_UTF_H_
#define _PLAN9_UTF_H_

#include <stdint.h>

typedef uint32_t p9_rune_t;

enum {
	P9_UTFMAX = 4,			/* maximum bytes per rune */
	P9_RUNESYNC = 0x80,		/* cannot represent part of a UTF sequence (<) */
	P9_RUNESELF = 0x80,		/* rune and UTF sequences are the same (<) */
	P9_RUNEERROR = 0xFFFD,		/* decoding error in UTF */
	P9_RUNEMAX = 0x10FFFF,		/* maximum rune value */
};

#define P9_Bit(i)	(7 - (i))
#define P9_T(i)		(((1 << (P9_Bit(i) + 1)) - 1) ^ 0xFF)
#define P9_RuneX(i)	((1 << (P9_Bit(i) + ((i) - 1) * P9_Bitx)) - 1)

enum {
	P9_Bitx = P9_Bit(1),
	P9_Tx = P9_T(1),		/* 1000 0000 */
	P9_Rune1 = (1 << (P9_Bit(0) + 0 * P9_Bitx)) - 1,
	P9_Maskx = (1 << P9_Bitx) - 1,	/* 0011 1111 */
	P9_Testx = P9_Maskx ^ 0xFF,	/* 1100 0000 */
	P9_SurrogateMin = 0xD800,
	P9_SurrogateMax = 0xDFFF,
};

/* Decode one UTF-8 sequence at str into *rune; return bytes consumed. */
static int p9_chartorune(p9_rune_t *rune, char *str)
{
	int c[P9_UTFMAX], i;
	p9_rune_t l;

	c[0] = *(uint8_t *) (str);
	if (c[0] < P9_Tx) {
		*rune = c[0];
		return 1;
	}
	l = c[0];

	for (i = 1; i < P9_UTFMAX; i++) {
		c[i] = *(uint8_t *) (str + i);
		c[i] ^= P9_Tx;
		if (c[i] & P9_Testx)
			goto bad;
		l = (l << P9_Bitx) | c[i];
		if (c[0] < P9_T(i + 2)) {
			l &= P9_RuneX(i + 1);
			if (i == 1) {
				if (c[0] < P9_T(2) || l <= P9_Rune1)
					goto bad;
			} else if (l <= P9_RuneX(i) || l > P9_RUNEMAX)
				goto bad;
			if (i == 2 && P9_SurrogateMin <= l
			    && l <= P9_SurrogateMax)
				goto bad;
			*rune = l;
			return i + 1;
		}
	}
bad:
	*rune = P9_RUNEERROR;
	return 1;
}

/* Encode *rune into str (up to P9_UTFMAX bytes); return bytes written. */
static int p9_runetochar(char *str, p9_rune_t *rune)
{
	int i, j;
	p9_rune_t c;

	c = *rune;
	if (c <= P9_Rune1) {
		str[0] = c;
		return 1;
	}
	for (i = 2; i < P9_UTFMAX + 1; i++) {
		if (i == 3) {
			if (c > P9_RUNEMAX)
				c = P9_RUNEERROR;
			if (P9_SurrogateMin <= c && c <= P9_SurrogateMax)
				c = P9_RUNEERROR;
		}
		if (c <= P9_RuneX(i) || i == P9_UTFMAX) {
			str[0] = P9_T(i) | (c >> (i - 1) * P9_Bitx);
			for (j = 1; j < i; j++)
				str[j] = P9_Tx
					| ((c >> (i - j - 1) * P9_Bitx)
					   & P9_Maskx);
			return i;
		}
	}
	return P9_UTFMAX;
}

/* Number of UTF-8 bytes needed to encode rune value c. */
static int p9_runelen(long c)
{
	p9_rune_t rune;
	char str[10];

	rune = c;
	return p9_runetochar(str, &rune);
}

/* True if str holds at least one complete rune within n bytes. */
static int p9_fullrune(char *str, int n)
{
	int i;
	p9_rune_t c;

	if (n <= 0)
		return 0;
	c = *(uint8_t *) str;
	if (c < P9_Tx)
		return 1;
	for (i = 3; i < P9_UTFMAX + 1; i++)
		if (c < P9_T(i))
			return n >= i - 1;
	return n >= P9_UTFMAX;
}

#endif /* _PLAN9_UTF_H_ */
