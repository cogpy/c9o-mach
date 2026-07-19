/*
 * 9P2000 message codec for freestanding Mach test modules.
 *
 * This is a faithful port of the Plan 9 / Harvey OS 9P2000 wire codec
 * (sys/src/libc/9sys/convS2M.c, convM2S.c, and sys/include/fcall.h),
 * adapted to fixed-width types and static linkage so it can be built
 * with -ffreestanding -nolibc.  The wire format and the bounds-checking
 * structure are unchanged; only the type spellings and packaging differ.
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
 *   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 * The GNU Mach adaptation is distributed under the GNU General Public
 * License version 2 or later, consistent with the rest of this tree;
 * the MIT terms above continue to govern the ported portions.
 */

#ifndef _NINEP_NINEP_H_
#define _NINEP_NINEP_H_

#include <stdint.h>
#include <string.h>	/* memmove, strlen (resolved from the test link) */

#define NP_VERSION9P	"9P2000"
#define NP_MAXWELEM	16
#define NP_NOTAG	((uint16_t) ~0U)
#define NP_NOFID	((uint32_t) ~0U)
#define NP_IOHDRSZ	24

#define NP_BIT8SZ	1
#define NP_BIT16SZ	2
#define NP_BIT32SZ	4
#define NP_BIT64SZ	8
#define NP_QIDSZ	(NP_BIT8SZ + NP_BIT32SZ + NP_BIT64SZ)

enum np_type {
	NP_Tversion = 100,
	NP_Rversion,
	NP_Tauth = 102,
	NP_Rauth,
	NP_Tattach = 104,
	NP_Rattach,
	NP_Terror = 106,	/* illegal */
	NP_Rerror,
	NP_Tflush = 108,
	NP_Rflush,
	NP_Twalk = 110,
	NP_Rwalk,
	NP_Topen = 112,
	NP_Ropen,
	NP_Tcreate = 114,
	NP_Rcreate,
	NP_Tread = 116,
	NP_Rread,
	NP_Twrite = 118,
	NP_Rwrite,
	NP_Tclunk = 120,
	NP_Rclunk,
	NP_Tremove = 122,
	NP_Rremove,
	NP_Tstat = 124,
	NP_Rstat,
	NP_Twstat = 126,
	NP_Rwstat,
	NP_Tmax,
};

typedef struct np_qid {
	uint64_t path;
	uint32_t vers;
	uint8_t type;
} np_qid_t;

typedef struct np_fcall {
	uint8_t type;
	uint32_t fid;
	uint16_t tag;
	union {
		struct {
			uint32_t msize;		/* Tversion, Rversion */
			char *version;		/* Tversion, Rversion */
		};
		struct {
			uint16_t oldtag;	/* Tflush */
		};
		struct {
			char *ename;		/* Rerror */
		};
		struct {
			np_qid_t qid;		/* Rattach, Ropen, Rcreate */
			uint32_t iounit;	/* Ropen, Rcreate */
		};
		struct {
			np_qid_t aqid;		/* Rauth */
		};
		struct {
			uint32_t afid;		/* Tauth, Tattach */
			char *uname;		/* Tauth, Tattach */
			char *aname;		/* Tauth, Tattach */
		};
		struct {
			uint32_t perm;		/* Tcreate */
			char *name;		/* Tcreate */
			uint8_t mode;		/* Tcreate, Topen */
		};
		struct {
			uint32_t newfid;	/* Twalk */
			uint16_t nwname;	/* Twalk */
			char *wname[NP_MAXWELEM];	/* Twalk */
		};
		struct {
			uint16_t nwqid;		/* Rwalk */
			np_qid_t wqid[NP_MAXWELEM];	/* Rwalk */
		};
		struct {
			int64_t offset;		/* Tread, Twrite */
			uint32_t count;		/* Tread, Twrite, Rread */
			char *data;		/* Twrite, Rread */
		};
		struct {
			uint16_t nstat;		/* Twstat, Rstat */
			uint8_t *stat;		/* Twstat, Rstat */
		};
	};
} np_fcall_t;

/* Little-endian fixed-width accessors, as in Plan 9 fcall.h. */
#define NP_GBIT8(p)	((p)[0])
#define NP_GBIT16(p)	((uint16_t) ((p)[0] | ((p)[1] << 8)))
#define NP_GBIT32(p)	((uint32_t) ((p)[0] | ((p)[1] << 8) \
				     | ((p)[2] << 16) | ((p)[3] << 24)))
#define NP_GBIT64(p)	((uint64_t) ((uint32_t) ((p)[0] | ((p)[1] << 8) \
				     | ((p)[2] << 16) | ((p)[3] << 24))) \
			 | ((uint64_t) ((uint32_t) ((p)[4] | ((p)[5] << 8) \
				     | ((p)[6] << 16) | ((p)[7] << 24))) << 32))

#define NP_PBIT8(p, v)	((p)[0] = (v))
#define NP_PBIT16(p, v)	((p)[0] = (v), (p)[1] = (v) >> 8)
#define NP_PBIT32(p, v)	((p)[0] = (v), (p)[1] = (v) >> 8, \
			 (p)[2] = (v) >> 16, (p)[3] = (v) >> 24)
#define NP_PBIT64(p, v)	((p)[0] = (v), (p)[1] = (v) >> 8, \
			 (p)[2] = (v) >> 16, (p)[3] = (v) >> 24, \
			 (p)[4] = (v) >> 32, (p)[5] = (v) >> 40, \
			 (p)[6] = (v) >> 48, (p)[7] = (v) >> 56)

#define NP_NIL	((void *) 0)

/*======================================================================
 * Packing (host struct -> wire): sizeS2M, convS2M
 *====================================================================*/

static uint8_t *np_pstring(uint8_t *p, char *s)
{
	uint32_t n;

	if (s == NP_NIL) {
		NP_PBIT16(p, 0);
		p += NP_BIT16SZ;
		return p;
	}
	n = (uint32_t) strlen(s);
	/* String is moved before the length so a struct can be packed
	   into an existing message in place. */
	memmove(p + NP_BIT16SZ, s, n);
	NP_PBIT16(p, n);
	p += n + NP_BIT16SZ;
	return p;
}

static uint8_t *np_pqid(uint8_t *p, np_qid_t *q)
{
	NP_PBIT8(p, q->type);
	p += NP_BIT8SZ;
	NP_PBIT32(p, q->vers);
	p += NP_BIT32SZ;
	NP_PBIT64(p, q->path);
	p += NP_BIT64SZ;
	return p;
}

static uint32_t np_stringsz(char *s)
{
	if (s == NP_NIL)
		return NP_BIT16SZ;
	return NP_BIT16SZ + (uint32_t) strlen(s);
}

static uint32_t np_sizeS2M(np_fcall_t *f)
{
	uint32_t n;
	int i;

	n = 0;
	n += NP_BIT32SZ;	/* size */
	n += NP_BIT8SZ;		/* type */
	n += NP_BIT16SZ;	/* tag */

	switch (f->type) {
	default:
		return 0;
	case NP_Tversion:
		n += NP_BIT32SZ;
		n += np_stringsz(f->version);
		break;
	case NP_Tflush:
		n += NP_BIT16SZ;
		break;
	case NP_Tauth:
		n += NP_BIT32SZ;
		n += np_stringsz(f->uname);
		n += np_stringsz(f->aname);
		break;
	case NP_Tattach:
		n += NP_BIT32SZ;
		n += NP_BIT32SZ;
		n += np_stringsz(f->uname);
		n += np_stringsz(f->aname);
		break;
	case NP_Twalk:
		n += NP_BIT32SZ;
		n += NP_BIT32SZ;
		n += NP_BIT16SZ;
		for (i = 0; i < f->nwname; i++)
			n += np_stringsz(f->wname[i]);
		break;
	case NP_Topen:
		n += NP_BIT32SZ;
		n += NP_BIT8SZ;
		break;
	case NP_Tcreate:
		n += NP_BIT32SZ;
		n += np_stringsz(f->name);
		n += NP_BIT32SZ;
		n += NP_BIT8SZ;
		break;
	case NP_Tread:
		n += NP_BIT32SZ;
		n += NP_BIT64SZ;
		n += NP_BIT32SZ;
		break;
	case NP_Twrite:
		n += NP_BIT32SZ;
		n += NP_BIT64SZ;
		n += NP_BIT32SZ;
		n += f->count;
		break;
	case NP_Tclunk:
	case NP_Tremove:
		n += NP_BIT32SZ;
		break;
	case NP_Tstat:
		n += NP_BIT32SZ;
		break;
	case NP_Twstat:
		n += NP_BIT32SZ;
		n += NP_BIT16SZ;
		n += f->nstat;
		break;
	case NP_Rversion:
		n += NP_BIT32SZ;
		n += np_stringsz(f->version);
		break;
	case NP_Rerror:
		n += np_stringsz(f->ename);
		break;
	case NP_Rflush:
		break;
	case NP_Rauth:
		n += NP_QIDSZ;
		break;
	case NP_Rattach:
		n += NP_QIDSZ;
		break;
	case NP_Rwalk:
		n += NP_BIT16SZ;
		n += f->nwqid * NP_QIDSZ;
		break;
	case NP_Ropen:
	case NP_Rcreate:
		n += NP_QIDSZ;
		n += NP_BIT32SZ;
		break;
	case NP_Rread:
		n += NP_BIT32SZ;
		n += f->count;
		break;
	case NP_Rwrite:
		n += NP_BIT32SZ;
		break;
	case NP_Rclunk:
		break;
	case NP_Rremove:
		break;
	case NP_Rstat:
		n += NP_BIT16SZ;
		n += f->nstat;
		break;
	case NP_Rwstat:
		break;
	}
	return n;
}

static uint32_t np_convS2M(np_fcall_t *f, uint8_t *ap, uint32_t nap)
{
	uint8_t *p;
	uint32_t i, size;

	size = np_sizeS2M(f);
	if (size == 0)
		return 0;
	if (size > nap)
		return 0;

	p = ap;
	NP_PBIT32(p, size);
	p += NP_BIT32SZ;
	NP_PBIT8(p, f->type);
	p += NP_BIT8SZ;
	NP_PBIT16(p, f->tag);
	p += NP_BIT16SZ;

	switch (f->type) {
	default:
		return 0;
	case NP_Tversion:
		NP_PBIT32(p, f->msize);
		p += NP_BIT32SZ;
		p = np_pstring(p, f->version);
		break;
	case NP_Tflush:
		NP_PBIT16(p, f->oldtag);
		p += NP_BIT16SZ;
		break;
	case NP_Tauth:
		NP_PBIT32(p, f->afid);
		p += NP_BIT32SZ;
		p = np_pstring(p, f->uname);
		p = np_pstring(p, f->aname);
		break;
	case NP_Tattach:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT32(p, f->afid);
		p += NP_BIT32SZ;
		p = np_pstring(p, f->uname);
		p = np_pstring(p, f->aname);
		break;
	case NP_Twalk:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT32(p, f->newfid);
		p += NP_BIT32SZ;
		NP_PBIT16(p, f->nwname);
		p += NP_BIT16SZ;
		if (f->nwname > NP_MAXWELEM)
			return 0;
		for (i = 0; i < f->nwname; i++)
			p = np_pstring(p, f->wname[i]);
		break;
	case NP_Topen:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT8(p, f->mode);
		p += NP_BIT8SZ;
		break;
	case NP_Tcreate:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		p = np_pstring(p, f->name);
		NP_PBIT32(p, f->perm);
		p += NP_BIT32SZ;
		NP_PBIT8(p, f->mode);
		p += NP_BIT8SZ;
		break;
	case NP_Tread:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT64(p, f->offset);
		p += NP_BIT64SZ;
		NP_PBIT32(p, f->count);
		p += NP_BIT32SZ;
		break;
	case NP_Twrite:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT64(p, f->offset);
		p += NP_BIT64SZ;
		NP_PBIT32(p, f->count);
		p += NP_BIT32SZ;
		memmove(p, f->data, f->count);
		p += f->count;
		break;
	case NP_Tclunk:
	case NP_Tremove:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		break;
	case NP_Tstat:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		break;
	case NP_Twstat:
		NP_PBIT32(p, f->fid);
		p += NP_BIT32SZ;
		NP_PBIT16(p, f->nstat);
		p += NP_BIT16SZ;
		memmove(p, f->stat, f->nstat);
		p += f->nstat;
		break;
	case NP_Rversion:
		NP_PBIT32(p, f->msize);
		p += NP_BIT32SZ;
		p = np_pstring(p, f->version);
		break;
	case NP_Rerror:
		p = np_pstring(p, f->ename);
		break;
	case NP_Rflush:
		break;
	case NP_Rauth:
		p = np_pqid(p, &f->aqid);
		break;
	case NP_Rattach:
		p = np_pqid(p, &f->qid);
		break;
	case NP_Rwalk:
		NP_PBIT16(p, f->nwqid);
		p += NP_BIT16SZ;
		if (f->nwqid > NP_MAXWELEM)
			return 0;
		for (i = 0; i < f->nwqid; i++)
			p = np_pqid(p, &f->wqid[i]);
		break;
	case NP_Ropen:
	case NP_Rcreate:
		p = np_pqid(p, &f->qid);
		NP_PBIT32(p, f->iounit);
		p += NP_BIT32SZ;
		break;
	case NP_Rread:
		NP_PBIT32(p, f->count);
		p += NP_BIT32SZ;
		memmove(p, f->data, f->count);
		p += f->count;
		break;
	case NP_Rwrite:
		NP_PBIT32(p, f->count);
		p += NP_BIT32SZ;
		break;
	case NP_Rclunk:
		break;
	case NP_Rremove:
		break;
	case NP_Rstat:
		NP_PBIT16(p, f->nstat);
		p += NP_BIT16SZ;
		memmove(p, f->stat, f->nstat);
		p += f->nstat;
		break;
	case NP_Rwstat:
		break;
	}
	if (size != (uint32_t) (p - ap))
		return 0;
	return size;
}

/*======================================================================
 * Unpacking (wire -> host struct): convM2S
 *
 * The unpacker mutates the input buffer in place: strings are shifted
 * down over their length prefix to make room for a NUL, so f->version,
 * f->uname, etc. point into ap[].  Callers must keep ap[] alive for as
 * long as they use those pointers.  This matches the Plan 9 contract.
 *====================================================================*/

static uint8_t *np_gstring(uint8_t *p, uint8_t *ep, char **s)
{
	uint32_t n;

	if (p + NP_BIT16SZ > ep)
		return NP_NIL;
	n = NP_GBIT16(p);
	p += NP_BIT16SZ - 1;
	if (p + n + 1 > ep)
		return NP_NIL;
	/* Move the string down, on top of the count, to make room for
	   the terminating NUL. */
	memmove(p, p + 1, n);
	p[n] = '\0';
	*s = (char *) p;
	p += n + 1;
	return p;
}

static uint8_t *np_gqid(uint8_t *p, uint8_t *ep, np_qid_t *q)
{
	if (p + NP_QIDSZ > ep)
		return NP_NIL;
	q->type = NP_GBIT8(p);
	p += NP_BIT8SZ;
	q->vers = NP_GBIT32(p);
	p += NP_BIT32SZ;
	q->path = NP_GBIT64(p);
	p += NP_BIT64SZ;
	return p;
}

static uint32_t np_convM2S(uint8_t *ap, uint32_t nap, np_fcall_t *f)
{
	uint8_t *p, *ep;
	uint32_t i, size;

	p = ap;
	ep = p + nap;

	if (p + NP_BIT32SZ + NP_BIT8SZ + NP_BIT16SZ > ep)
		return 0;
	size = NP_GBIT32(p);
	p += NP_BIT32SZ;
	if (size < NP_BIT32SZ + NP_BIT8SZ + NP_BIT16SZ)
		return 0;

	f->type = NP_GBIT8(p);
	p += NP_BIT8SZ;
	f->tag = NP_GBIT16(p);
	p += NP_BIT16SZ;

	switch (f->type) {
	default:
		return 0;
	case NP_Tversion:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->msize = NP_GBIT32(p);
		p += NP_BIT32SZ;
		p = np_gstring(p, ep, &f->version);
		break;
	case NP_Tflush:
		if (p + NP_BIT16SZ > ep)
			return 0;
		f->oldtag = NP_GBIT16(p);
		p += NP_BIT16SZ;
		break;
	case NP_Tauth:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->afid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		p = np_gstring(p, ep, &f->uname);
		if (p == NP_NIL)
			break;
		p = np_gstring(p, ep, &f->aname);
		if (p == NP_NIL)
			break;
		break;
	case NP_Tattach:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->afid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		p = np_gstring(p, ep, &f->uname);
		if (p == NP_NIL)
			break;
		p = np_gstring(p, ep, &f->aname);
		if (p == NP_NIL)
			break;
		break;
	case NP_Twalk:
		if (p + NP_BIT32SZ + NP_BIT32SZ + NP_BIT16SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->newfid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->nwname = NP_GBIT16(p);
		p += NP_BIT16SZ;
		if (f->nwname > NP_MAXWELEM)
			return 0;
		for (i = 0; i < f->nwname; i++) {
			p = np_gstring(p, ep, &f->wname[i]);
			if (p == NP_NIL)
				break;
		}
		break;
	case NP_Topen:
		if (p + NP_BIT32SZ + NP_BIT8SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->mode = NP_GBIT8(p);
		p += NP_BIT8SZ;
		break;
	case NP_Tcreate:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		p = np_gstring(p, ep, &f->name);
		if (p == NP_NIL)
			break;
		if (p + NP_BIT32SZ + NP_BIT8SZ > ep)
			return 0;
		f->perm = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->mode = NP_GBIT8(p);
		p += NP_BIT8SZ;
		break;
	case NP_Tread:
		if (p + NP_BIT32SZ + NP_BIT64SZ + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->offset = (int64_t) NP_GBIT64(p);
		p += NP_BIT64SZ;
		f->count = NP_GBIT32(p);
		p += NP_BIT32SZ;
		break;
	case NP_Twrite:
		if (p + NP_BIT32SZ + NP_BIT64SZ + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->offset = (int64_t) NP_GBIT64(p);
		p += NP_BIT64SZ;
		f->count = NP_GBIT32(p);
		p += NP_BIT32SZ;
		if (p + f->count > ep)
			return 0;
		f->data = (char *) p;
		p += f->count;
		break;
	case NP_Tclunk:
	case NP_Tremove:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		break;
	case NP_Tstat:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		break;
	case NP_Twstat:
		if (p + NP_BIT32SZ + NP_BIT16SZ > ep)
			return 0;
		f->fid = NP_GBIT32(p);
		p += NP_BIT32SZ;
		f->nstat = NP_GBIT16(p);
		p += NP_BIT16SZ;
		if (p + f->nstat > ep)
			return 0;
		f->stat = p;
		p += f->nstat;
		break;
	case NP_Rversion:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->msize = NP_GBIT32(p);
		p += NP_BIT32SZ;
		p = np_gstring(p, ep, &f->version);
		break;
	case NP_Rerror:
		p = np_gstring(p, ep, &f->ename);
		break;
	case NP_Rflush:
		break;
	case NP_Rauth:
		p = np_gqid(p, ep, &f->aqid);
		if (p == NP_NIL)
			break;
		break;
	case NP_Rattach:
		p = np_gqid(p, ep, &f->qid);
		if (p == NP_NIL)
			break;
		break;
	case NP_Rwalk:
		if (p + NP_BIT16SZ > ep)
			return 0;
		f->nwqid = NP_GBIT16(p);
		p += NP_BIT16SZ;
		if (f->nwqid > NP_MAXWELEM)
			return 0;
		for (i = 0; i < f->nwqid; i++) {
			p = np_gqid(p, ep, &f->wqid[i]);
			if (p == NP_NIL)
				break;
		}
		break;
	case NP_Ropen:
	case NP_Rcreate:
		p = np_gqid(p, ep, &f->qid);
		if (p == NP_NIL)
			break;
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->iounit = NP_GBIT32(p);
		p += NP_BIT32SZ;
		break;
	case NP_Rread:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->count = NP_GBIT32(p);
		p += NP_BIT32SZ;
		if (p + f->count > ep)
			return 0;
		f->data = (char *) p;
		p += f->count;
		break;
	case NP_Rwrite:
		if (p + NP_BIT32SZ > ep)
			return 0;
		f->count = NP_GBIT32(p);
		p += NP_BIT32SZ;
		break;
	case NP_Rclunk:
	case NP_Rremove:
		break;
	case NP_Rstat:
		if (p + NP_BIT16SZ > ep)
			return 0;
		f->nstat = NP_GBIT16(p);
		p += NP_BIT16SZ;
		if (p + f->nstat > ep)
			return 0;
		f->stat = p;
		p += f->nstat;
		break;
	case NP_Rwstat:
		break;
	}

	if (p == NP_NIL || p > ep)
		return 0;
	if (ap + size == p)
		return size;
	return 0;
}

#endif /* _NINEP_NINEP_H_ */
