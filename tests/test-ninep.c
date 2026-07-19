/*
 * 9P2000 Codec Test
 * Exercises the ported Plan 9 / Harvey 9P2000 wire codec on bare Mach:
 * round-trip marshalling of representative message types, a 9P message
 * carried through a Mach port, and the codec's bounds rejection of
 * truncated input. See docs/ninep-codec.md and tests/include/ninep.
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

#include <testlib.h>

#include <mach.user.h>
#include <mach_port.user.h>

#include <ninep/ninep.h>

#define NP_BUF 512
#define NP_MSG_ID 9000

struct np_message {
	mach_msg_header_t header;
	mach_msg_type_t type;
	uint8_t body[NP_BUF];
};

/* Leg 1: pack then unpack representative message types. */
static void test_roundtrip_leg(void)
{
	uint8_t buf[NP_BUF];
	np_fcall_t tx, rx;
	uint32_t n;
	char *names[2];

	/* Tversion */
	tx.type = NP_Tversion;
	tx.tag = NP_NOTAG;
	tx.msize = 8192;
	tx.version = NP_VERSION9P;
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n == 19, "Tversion wire size == 19");
	ASSERT(np_convM2S(buf, n, &rx) == n, "Tversion decode size");
	ASSERT(rx.type == NP_Tversion, "Tversion type");
	ASSERT(rx.tag == NP_NOTAG, "Tversion tag");
	ASSERT(rx.msize == 8192, "Tversion msize");
	ASSERT((strcmp(rx.version, "9P2000") == 0), "Tversion version");

	/* Twalk with two path elements */
	names[0] = "usr";
	names[1] = "bin";
	tx.type = NP_Twalk;
	tx.tag = 5;
	tx.fid = 1;
	tx.newfid = 2;
	tx.nwname = 2;
	tx.wname[0] = names[0];
	tx.wname[1] = names[1];
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n != 0, "Twalk encodes");
	ASSERT(np_convM2S(buf, n, &rx) == n, "Twalk decode size");
	ASSERT(rx.fid == 1 && rx.newfid == 2, "Twalk fids");
	ASSERT(rx.nwname == 2, "Twalk nwname");
	ASSERT((strcmp(rx.wname[0], "usr") == 0), "Twalk wname0");
	ASSERT((strcmp(rx.wname[1], "bin") == 0), "Twalk wname1");

	/* Tread carries a 64-bit offset */
	tx.type = NP_Tread;
	tx.tag = 7;
	tx.fid = 3;
	tx.offset = (int64_t) 0x1122334455667788LL;
	tx.count = 512;
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n == 23, "Tread wire size == 23");
	ASSERT(np_convM2S(buf, n, &rx) == n, "Tread decode size");
	ASSERT(rx.offset == (int64_t) 0x1122334455667788LL, "Tread offset");
	ASSERT(rx.count == 512, "Tread count");

	/* Rwalk carries qids */
	tx.type = NP_Rwalk;
	tx.tag = 9;
	tx.nwqid = 1;
	tx.wqid[0].type = 0x80;
	tx.wqid[0].vers = 42;
	tx.wqid[0].path = (uint64_t) 0xdeadbeefcafef00dULL;
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n != 0, "Rwalk encodes");
	ASSERT(np_convM2S(buf, n, &rx) == n, "Rwalk decode size");
	ASSERT(rx.nwqid == 1, "Rwalk nwqid");
	ASSERT(rx.wqid[0].type == 0x80, "Rwalk qid type");
	ASSERT(rx.wqid[0].vers == 42, "Rwalk qid vers");
	ASSERT(rx.wqid[0].path == (uint64_t) 0xdeadbeefcafef00dULL,
	       "Rwalk qid path");

	/* Rerror string */
	tx.type = NP_Rerror;
	tx.tag = 11;
	tx.ename = "permission denied";
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n != 0, "Rerror encodes");
	ASSERT(np_convM2S(buf, n, &rx) == n, "Rerror decode size");
	ASSERT((strcmp(rx.ename, "permission denied") == 0),
	       "Rerror ename");

	printf("ninep: round-trip of 5 message types exact\n");
}

/* Leg 2: a 9P message crosses a Mach port. */
static void test_ipc_leg(void)
{
	mach_port_t port;
	kern_return_t err;
	struct np_message msg;
	np_fcall_t tx, rx;
	uint32_t n;

	err = mach_port_allocate(mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE, &port);
	ASSERT_RET(err, "mach_port_allocate");

	tx.type = NP_Tattach;
	tx.tag = 1;
	tx.fid = 0;
	tx.afid = NP_NOFID;
	tx.uname = "glenda";
	tx.aname = "main";
	n = np_convS2M(&tx, msg.body, sizeof(msg.body));
	ASSERT(n != 0, "Tattach encodes into message body");

	msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
	msg.header.msgh_remote_port = port;
	msg.header.msgh_local_port = MACH_PORT_NULL;
	msg.header.msgh_id = NP_MSG_ID;
	msg.header.msgh_size = sizeof(msg);
	msg.type.msgt_name = MACH_MSG_TYPE_BYTE;
	msg.type.msgt_size = 8;
	msg.type.msgt_number = sizeof(msg.body);
	msg.type.msgt_inline = TRUE;
	msg.type.msgt_longform = FALSE;
	msg.type.msgt_deallocate = FALSE;
	msg.type.msgt_unused = 0;

	err = mach_msg(&msg.header, MACH_SEND_MSG, msg.header.msgh_size,
		       0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
		       MACH_PORT_NULL);
	ASSERT_RET(err, "mach_msg send 9P");

	err = mach_msg(&msg.header, MACH_RCV_MSG, 0, sizeof(msg), port,
		       MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	ASSERT_RET(err, "mach_msg receive 9P");
	ASSERT(msg.header.msgh_id == NP_MSG_ID, "9P message id preserved");

	ASSERT(np_convM2S(msg.body, n, &rx) == n, "decode received 9P");
	ASSERT(rx.type == NP_Tattach, "received Tattach type");
	ASSERT(rx.fid == 0, "received fid");
	ASSERT(rx.afid == NP_NOFID, "received afid");
	ASSERT((strcmp(rx.uname, "glenda") == 0), "received uname");
	ASSERT((strcmp(rx.aname, "main") == 0), "received aname");

	printf("ninep: 9P Tattach crossed mach_msg intact\n");
}

/* Leg 3: the decoder rejects truncated and malformed input. */
static void test_bounds_leg(void)
{
	uint8_t buf[NP_BUF];
	np_fcall_t tx, rx;
	uint32_t n;

	tx.type = NP_Twalk;
	tx.tag = 1;
	tx.fid = 1;
	tx.newfid = 2;
	tx.nwname = 2;
	tx.wname[0] = "usr";
	tx.wname[1] = "bin";
	n = np_convS2M(&tx, buf, sizeof(buf));
	ASSERT(n != 0, "Twalk encodes for truncation test");

	/* Any prefix shorter than the whole message must be rejected. */
	ASSERT(np_convM2S(buf, n - 1, &rx) == 0, "reject truncated by 1");
	ASSERT(np_convM2S(buf, 3, &rx) == 0, "reject header-only");
	ASSERT(np_convM2S(buf, 0, &rx) == 0, "reject empty");

	/* A declared size larger than the buffer must be rejected. */
	NP_PBIT32(buf, n + 64);
	ASSERT(np_convM2S(buf, n, &rx) == 0, "reject oversized declared");

	/* An unknown type must be rejected. */
	NP_PBIT32(buf, n);
	buf[NP_BIT32SZ] = 200;
	ASSERT(np_convM2S(buf, n, &rx) == 0, "reject unknown type");

	printf("ninep: decoder rejects malformed input\n");
}

int main(int argc, char *argv[], int envc, char *envp[])
{
	test_roundtrip_leg();
	test_ipc_leg();
	test_bounds_leg();

	printf("ninep codec test: PASS\n");
	return 0;
}
