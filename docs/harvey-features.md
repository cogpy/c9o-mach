# Harvey OS Feature Port

This document records the features ported into c9o-mach from Harvey OS, a
maintained descendant of Plan 9 from Bell Labs, and the licensing basis for
doing so.

## Provenance and Licensing

Harvey OS (github.com/o9nn/harveyos) ships the Plan 9 sources under the
Plan 9 Foundation license, an MIT-style permissive grant:

> Copyright 2021 Plan 9 Foundation
>
> Permission is hereby granted, free of charge, to any person obtaining a
> copy of this software ... to deal in the Software without restriction,
> including without limitation the rights to use, copy, modify, merge,
> publish, distribute, sublicense, and/or sell copies ...
>
> The above copyright notice and this permission notice shall be included
> in all copies or substantial portions of the Software.

This is GPLv2-compatible, so the code may be copied into GPLv2 c9o-mach.
Every ported file preserves the MIT copyright and permission notice
verbatim, alongside a GPLv2-or-later notice for the adaptation. The Harvey
tree also contains `LICENSE.gpl` and `LICENSE.afpl` covering other
subdirectories (fonts, some commands); none of the ported files fall under
those, and each was checked for a contrary per-file header before porting.

The Harvey sources use Plan 9 typedefs from `<u.h>` (uchar, u32int, vlong,
Rune, nil). These are types, not a runtime; each port replaces them with
`<stdint.h>` fixed-width types so it builds `-ffreestanding -nolibc` in the
c9o-mach test harness.

## Ported Features

### 9P2000 wire codec -- `tests/include/ninep/ninep.h`

A faithful port of `sys/src/libc/9sys/convS2M.c`, `convM2S.c`, and
`sys/include/fcall.h`: `np_sizeS2M`/`np_convS2M` pack an `np_fcall_t` to the
wire, `np_convM2S` unpacks with the original in-place bounds checking. This
makes the "9P server over Mach IPC" mapping in `docs/cogwxp-integration.md`
buildable: the codec is the wire layer that a Mach-IPC 9P server needs.

Proven by `tests/test-ninep.c` (`make run-ninep`): round-trip of five
message types with exact wire sizes, a Tattach carried through a Mach port
and decoded field-for-field, and rejection of truncated, oversized, and
unknown-type input.

### UTF-8 codec -- `tests/include/plan9/utf.h`

A port of `sys/src/libc/port/rune.c`: `p9_chartorune`, `p9_runetochar`,
`p9_runelen`, `p9_fullrune`, with the `P9_RUNEERROR`/`P9_RUNEMAX`/`P9_UTFMAX`
constants. 9P path elements are UTF-8 by specification, so this is the
companion the codec needs to interpret the names inside a Twalk.

### Path canonicalizer -- `tests/include/plan9/cleanname.h`

A port of `sys/src/libc/port/cleanname.c`: `p9_cleanname` collapses repeated
slashes and lexically resolves `.` and `..` in place, the normalization a 9P
walk applies to its path elements. Depends on `plan9/utf.h` and `strcpy`.

### Freestanding quicksort -- `tests/include/plan9/sort.h`

A port of `sys/src/libc/port/qsort.c`: `p9_qsort`, median-of-three, in place,
no allocation. The test harness previously had no sort primitive; any test
that needs to order results (benchmark timings, attention rankings, latency
histograms) can now use it. Note the comparator takes two `void *` element
pointers, the Plan 9 signature, which differs from ISO C `qsort`.

The UTF-8 codec, path canonicalizer, and quicksort are proven together by
`tests/test-plan9util.c` (`make run-plan9util`).

## What Was Not Ported

- **lib9p server framework** (`sys/src/lib9p/`): the canonical 9P *server*
  state machine, but it depends on the Plan 9 thread/channel runtime and
  `rfork`. It is a design reference for a future Mach-IPC 9P server, not
  copyable freestanding.
- **pool.c allocator**: a full debugging malloc; the freestanding harness
  wants a trivial arena, not a 1400-line coalescing allocator.
- **libavl** (`sys/src/libavl/`): an intrusive AVL tree that would give the
  cognitive atomspace an O(log n) ordered index with no per-insert
  allocation. Genuinely useful, but it needs a small allocator for its tree
  and walker handles and carries two debug dependencies (`print`, a spurious
  `bio.h` include). Deferred as a candidate follow-up.

## Validation

- `make run-ninep` and `make run-plan9util` boot the tests in QEMU.
- `scripts/validate-harvey-features.sh [build-dir]` compiles both modules,
  boots them requiring the testlib success marker, and checks that every
  ported header preserves the upstream MIT permission notice.
