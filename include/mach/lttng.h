/*
 * LTTng compatibility layer for GNU Mach
 * Copyright (C) 2024 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _MACH_LTTNG_H_
#define _MACH_LTTNG_H_

#include <mach/boolean.h>
#include <stdint.h>

/* Include standard integer types when available, otherwise define our own */
#ifdef __KERNEL__
/* In kernel space, define our own types to avoid conflicts */
#ifndef __uint8_t_defined
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long uint64_t;
#define __uint8_t_defined
#endif
#else
/* In user space, use standard definitions */
#include <stdint.h>
#endif

/*
 * GNU Mach LTTng-style tracing infrastructure
 * 
 * This provides a lightweight tracing framework compatible with LTTng concepts
 * but adapted for the Mach microkernel architecture. The design focuses on
 * minimal overhead and integration with existing Mach debugging infrastructure.
 */

/* Enable/disable tracing at compile time */
#ifndef CONFIG_MACH_TRACING
#define CONFIG_MACH_TRACING 1
#endif

/* Maximum number of trace events buffered */
#define MACH_TRACE_BUF_SIZE 8192

/* Trace event categories - similar to LTTng subsystems */
typedef enum {
	MACH_TRACE_IPC = 0,      /* Inter-process communication */
	MACH_TRACE_SCHED,        /* Scheduling and threads */
	MACH_TRACE_VM,           /* Virtual memory operations */
	MACH_TRACE_KERN,         /* General kernel events */
	MACH_TRACE_DEBUG,        /* Debug and error events */
	MACH_TRACE_MAX_CATEGORY
} mach_trace_category_t;

/* Trace event levels - similar to LTTng levels */
typedef enum {
	MACH_TRACE_LEVEL_EMERG = 0,
	MACH_TRACE_LEVEL_ALERT,
	MACH_TRACE_LEVEL_CRIT,
	MACH_TRACE_LEVEL_ERR,
	MACH_TRACE_LEVEL_WARNING,
	MACH_TRACE_LEVEL_NOTICE,
	MACH_TRACE_LEVEL_INFO,
	MACH_TRACE_LEVEL_DEBUG
} mach_trace_level_t;

/* Individual trace event structure */
struct mach_trace_event {
	uint32_t timestamp_hi;    /* High 32 bits of timestamp */
	uint32_t timestamp_lo;    /* Low 32 bits of timestamp */
	uint16_t category;        /* Event category */
	uint16_t level;           /* Event level */
	uint32_t event_id;        /* Unique event identifier */
	uint32_t cpu_id;          /* CPU that generated event */
	uint32_t task_id;         /* Task ID (if applicable) */
	uint32_t thread_id;       /* Thread ID (if applicable) */
	char data[64];            /* Event-specific data */
} __attribute__((packed));

/* Trace buffer management - opaque in header, implemented in .c */
extern struct mach_trace_buffer_impl *mach_trace_buf_ptr;
extern boolean_t mach_tracing_enabled;

/* Core tracing functions */
void mach_trace_init(void);
void mach_trace_enable(boolean_t enable);
boolean_t mach_trace_is_enabled(void);
void mach_trace_event(mach_trace_category_t category, 
                      mach_trace_level_t level,
                      uint32_t event_id,
                      const char *fmt, ...);

/* LTTng-style tracepoint macros for minimal overhead */
#if CONFIG_MACH_TRACING

#define MACH_TRACEPOINT_DECLARE(category, name) \
	extern void __mach_trace_##category##_##name(void)

#define MACH_TRACEPOINT_DEFINE(category, name, level, id, fmt, ...) \
	void __mach_trace_##category##_##name(void) { \
		if (mach_tracing_enabled) \
			mach_trace_event(MACH_TRACE_##category, level, id, fmt, ##__VA_ARGS__); \
	}

#define MACH_TRACEPOINT(category, name) \
	do { \
		if (mach_tracing_enabled) \
			__mach_trace_##category##_##name(); \
	} while (0)

/* Convenience macros for common trace categories */
#define TRACE_IPC(name) MACH_TRACEPOINT(IPC, name)
#define TRACE_SCHED(name) MACH_TRACEPOINT(SCHED, name)
#define TRACE_VM(name) MACH_TRACEPOINT(VM, name)
#define TRACE_KERN(name) MACH_TRACEPOINT(KERN, name)
#define TRACE_DEBUG(name) MACH_TRACEPOINT(DEBUG, name)

/* Event ID allocation macros */
#define MACH_TRACE_EVENT_IPC_BASE    0x1000
#define MACH_TRACE_EVENT_SCHED_BASE  0x2000
#define MACH_TRACE_EVENT_VM_BASE     0x3000
#define MACH_TRACE_EVENT_KERN_BASE   0x4000
#define MACH_TRACE_EVENT_DEBUG_BASE  0x5000

#else /* !CONFIG_MACH_TRACING */

/* No-op versions when tracing is disabled */
#define MACH_TRACEPOINT_DECLARE(category, name)
#define MACH_TRACEPOINT_DEFINE(category, name, level, id, fmt, ...)
#define MACH_TRACEPOINT(category, name) do { } while (0)
#define TRACE_IPC(name) do { } while (0)
#define TRACE_SCHED(name) do { } while (0)
#define TRACE_VM(name) do { } while (0)
#define TRACE_KERN(name) do { } while (0)
#define TRACE_DEBUG(name) do { } while (0)

#endif /* CONFIG_MACH_TRACING */

/* User-space interface for reading trace data - simplified for kernel */
struct mach_trace_read_request {
	uint32_t max_events;
	uint32_t timeout_ms;
};

struct mach_trace_read_response {
	uint32_t num_events;
	uint32_t dropped_events;
	struct mach_trace_event events[];
};

/* Syscall interface numbers (to be added to mach.defs) */
#define MACH_TRACE_ENABLE_CALL    3500
#define MACH_TRACE_READ_CALL      3501
#define MACH_TRACE_STATUS_CALL    3502

#endif /* _MACH_LTTNG_H_ */