/*
 * In-place path canonicalizer for freestanding Mach test modules.
 *
 * Faithful port of the Plan 9 / Harvey OS cleanname
 * (sys/src/libc/port/cleanname.c), adapted to a p9_ namespace and
 * static linkage.  Collapses repeated slashes and resolves "." and
 * ".." lexically -- the normalization a 9P walk needs on the path
 * elements a Twalk carries.  The algorithm is unchanged.
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

#ifndef _PLAN9_CLEANNAME_H_
#define _PLAN9_CLEANNAME_H_

#include <string.h>	/* strcpy, resolved from the test link */

#include <plan9/utf.h>

#define P9_SEP(x)	((x) == '/' || (x) == 0)

/*
 * Rewrite name in place to compress multiple '/', eliminate ".", and
 * process "..".  Returns name.  The result is never longer than the
 * input, so no buffer growth occurs.
 */
static char *p9_cleanname(char *name)
{
	char *s;	/* source of copy */
	char *d;	/* destination of copy */
	char *d0;	/* start of path after the root name */
	p9_rune_t r;
	int rooted;

	if (name[0] == 0)
		return strcpy(name, ".");
	rooted = 0;
	d0 = name;
	if (d0[0] == '#') {
		if (d0[1] == 0)
			return d0;
		d0 += 1 + p9_chartorune(&r, d0 + 1);	/* ignore slash: #/ */
		while (!P9_SEP(*d0))
			d0 += p9_chartorune(&r, d0);
		if (d0 == 0)
			return name;
		d0++;		/* keep / after #<name> */
		rooted = 1;
	} else if (d0[0] == '/') {
		rooted = 1;
		d0++;
	}

	s = d0;
	if (rooted) {
		/* skip extra '/' at root name */
		for (; *s == '/'; s++)
			;
	}
	/* remove duplicate slashes */
	for (d = d0; *s != 0; s++) {
		*d++ = *s;
		if (*s == '/')
			while (s[1] == '/')
				s++;
	}
	*d = 0;

	d = d0;
	s = d0;
	while (*s != 0) {
		if (s[0] == '.' && P9_SEP(s[1])) {
			if (s[1] == 0)
				break;
			s += 2;
			continue;
		}
		if (s[0] == '.' && s[1] == '.' && P9_SEP(s[2])) {
			if (d == d0) {
				if (rooted) {
					/* /../x -> /x */
					if (s[2] == 0)
						break;
					s += 3;
					continue;
				} else {
					/* ../x -> ../x; never collect ../ */
					d0 += 3;
				}
			}
			if (d > d0) {
				/* a/../x -> x */
				for (d -= 2; d > d0 && d[-1] != '/'; d--)
					;
				if (s[2] == 0)
					break;
				s += 3;
				continue;
			}
		}
		while (!P9_SEP(*s))
			*d++ = *s++;
		if (*s == 0)
			break;
		*d++ = *s++;
	}
	*d = 0;
	if (d - 1 > name && d[-1] == '/')	/* thanks to #/ */
		*--d = 0;
	if (name[0] == 0)
		strcpy(name, ".");
	return name;
}

#endif /* _PLAN9_CLEANNAME_H_ */
