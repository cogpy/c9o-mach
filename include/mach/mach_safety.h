/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */

/*
 * mach_safety.h - Safety checking macros for GNU Mach
 * 
 * Cross-phase infrastructure improvements for robustness
 * Addresses critical bugs mentioned in GNU Mach development roadmap
 */

#ifndef _MACH_MACH_SAFETY_H_
#define _MACH_MACH_SAFETY_H_

#include <mach/kern_return.h>
#include <stdint.h>

/* Integer type maximum values for overflow checking */
#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFFU
#endif
#ifndef INT32_MAX 
#define INT32_MAX 0x7FFFFFFF
#endif
#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif

/*
 * Integer overflow protection macros
 * Prevents arithmetic overflow vulnerabilities
 */

/* Check if addition would overflow */
#define MACH_SAFE_ADD_CHECK(a, b, type) \
    ((a) > type##_MAX - (b))

/* Check if multiplication would overflow */
#define MACH_SAFE_MUL_CHECK(a, b, type) \
    ((a) != 0 && (b) > type##_MAX / (a))

/* Safe addition with overflow check */
#define MACH_SAFE_ADD(a, b, result, type) \
    (MACH_SAFE_ADD_CHECK(a, b, type) ? KERN_INVALID_ARGUMENT : \
     (*(result) = (a) + (b), KERN_SUCCESS))

/* Safe multiplication with overflow check */
#define MACH_SAFE_MUL(a, b, result, type) \
    (MACH_SAFE_MUL_CHECK(a, b, type) ? KERN_INVALID_ARGUMENT : \
     (*(result) = (a) * (b), KERN_SUCCESS))

/*
 * Bounds checking macros
 * Prevents buffer overruns and invalid memory access
 */

/* Check if offset is within bounds */
#define MACH_BOUNDS_CHECK(base, offset, size, limit) \
    ((offset) < (size) && (base) + (offset) <= (limit))

/* Check if range is valid and within bounds */
#define MACH_RANGE_CHECK(start, length, limit) \
    ((start) <= (limit) && (length) <= (limit) - (start))

/*
 * Resource validation macros
 * Improves resource management robustness
 */

/* Validate pointer is not NULL and within expected range */
#define MACH_VALIDATE_PTR(ptr, min_addr, max_addr) \
    ((ptr) != NULL && (uintptr_t)(ptr) >= (uintptr_t)(min_addr) && \
     (uintptr_t)(ptr) < (uintptr_t)(max_addr))

/*
 * Word-size-aware upper bound for MACH_VALIDATE_PTR.  A literal
 * 0xFFFFFFFF rejects every valid kernel pointer on 64-bit targets
 * (kernel addresses are 0xffffffff8xxxxxxx and above), so callers
 * validating "any plausible kernel pointer" must use this instead.
 * On 32-bit targets it is identical to 0xFFFFFFFF.
 */
#define MACH_PTR_MAX_ADDR ((uintptr_t) -1)

/* Validate memory region doesn't wrap around */
#define MACH_VALIDATE_REGION(addr, size) \
    ((size) > 0 && (uintptr_t)(addr) + (size) > (uintptr_t)(addr))

/*
 * VM map safety checks
 * Prevents VM map kernel bugs
 */

/* Check VM map entry validity */
#define MACH_VM_ENTRY_VALID(entry) \
    ((entry) != NULL && (entry)->start <= (entry)->end)

/* Check VM map region alignment */
#define MACH_VM_ALIGNED(addr, align) \
    (((uintptr_t)(addr) & ((align) - 1)) == 0)

#endif /* _MACH_MACH_SAFETY_H_ */