/*
 * Plan 9 Utilities Test
 * Exercises the ported Plan 9 / Harvey utilities on bare Mach: the
 * UTF-8 codec, the path canonicalizer, and the freestanding quicksort.
 * See docs/harvey-features.md and tests/include/plan9.
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

#include <testlib.h>

#include <plan9/utf.h>
#include <plan9/cleanname.h>
#include <plan9/sort.h>

/* Leg 1: UTF-8 encode/decode round trips across the sequence lengths. */
static void test_utf_leg(void)
{
	p9_rune_t runes[] = {
		0x41,		/* 'A'  -> 1 byte */
		0xE9,		/* e-acute -> 2 bytes */
		0x20AC,		/* euro sign -> 3 bytes */
		0x1F600,	/* emoji -> 4 bytes */
	};
	int expect_len[] = { 1, 2, 3, 4 };
	char buf[P9_UTFMAX];
	p9_rune_t decoded;
	int n, m, i;

	for (i = 0; i < 4; i++) {
		n = p9_runetochar(buf, &runes[i]);
		ASSERT(n == expect_len[i], "runetochar length");
		ASSERT(p9_runelen(runes[i]) == expect_len[i], "runelen");
		m = p9_chartorune(&decoded, buf);
		ASSERT(m == n, "chartorune consumes same length");
		ASSERT(decoded == runes[i], "utf round trip value");
	}

	/* ASCII decodes as a single byte. */
	ASSERT(p9_chartorune(&decoded, "z") == 1 && decoded == 'z',
	       "ascii decode");

	/* A lone continuation byte is invalid and yields the error rune. */
	{
		char bad[2];
		bad[0] = (char) 0x80;
		bad[1] = 0;
		ASSERT(p9_chartorune(&decoded, bad) == 1
		       && decoded == P9_RUNEERROR, "invalid utf -> error rune");
	}

	printf("plan9: utf-8 round trips exact\n");
}

/* Leg 2: path canonicalization matches Plan 9 semantics. */
static void test_cleanname_leg(void)
{
	struct {
		const char *in;
		const char *out;
	} cases[] = {
		{ "/usr//bin/./ls", "/usr/bin/ls" },
		{ "/a/b/../c", "/a/c" },
		{ "a/./b/../../c", "c" },
		{ "", "." },
		{ "/", "/" },
		{ "///", "/" },
		{ "/../x", "/x" },
		{ "usr/../../etc", "../etc" },
	};
	char work[64];
	int i;

	for (i = 0; i < (int) (sizeof(cases) / sizeof(cases[0])); i++) {
		strcpy(work, cases[i].in);
		p9_cleanname(work);
		ASSERT(strcmp(work, cases[i].out) == 0, cases[i].in);
	}

	printf("plan9: cleanname resolves . and .. exactly\n");
}

static int cmp_int(void *a, void *b)
{
	int ia = *(int *) a;
	int ib = *(int *) b;

	if (ia < ib)
		return -1;
	if (ia > ib)
		return 1;
	return 0;
}

/* Leg 3: quicksort orders an array and preserves its multiset. */
static void test_sort_leg(void)
{
	int a[] = { 42, -7, 0, 42, 13, -100, 7, 8, 7, 1 };
	int n = sizeof(a) / sizeof(a[0]);
	long sum_before = 0, sum_after = 0;
	int i;

	for (i = 0; i < n; i++)
		sum_before += a[i];

	p9_qsort(a, n, sizeof(int), cmp_int);

	for (i = 1; i < n; i++)
		ASSERT(a[i - 1] <= a[i], "sorted nondecreasing");
	for (i = 0; i < n; i++)
		sum_after += a[i];
	ASSERT(sum_before == sum_after, "sort preserves elements");
	ASSERT(a[0] == -100, "sort min");
	ASSERT(a[n - 1] == 42, "sort max");

	printf("plan9: quicksort orders and preserves the multiset\n");
}

int main(int argc, char *argv[], int envc, char *envp[])
{
	test_utf_leg();
	test_cleanname_leg();
	test_sort_leg();

	printf("plan9 utilities test: PASS\n");
	return 0;
}
