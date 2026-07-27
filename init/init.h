/*
 * c9o-mach Init System - Header File
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _INIT_H_
#define _INIT_H_

#include <mach.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Version information */
#define INIT_VERSION "1.0.0"
#define INIT_NAME "c9o-mach init"

/* Logging levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;

/* Boot modes */
typedef enum {
    BOOT_MODE_TYPE_NORMAL = 0,
    BOOT_MODE_TYPE_LIVE,
    BOOT_MODE_TYPE_INSTALL,
    BOOT_MODE_TYPE_RESCUE
} boot_mode_t;

/* Runlevels */
typedef enum {
    RUNLEVEL_BOOT = 0,      /* System booting */
    RUNLEVEL_SINGLE = 1,    /* Single user mode */
    RUNLEVEL_MULTI = 2,     /* Multi-user mode */
    RUNLEVEL_NETWORK = 3,   /* Multi-user with network */
    RUNLEVEL_GRAPHICAL = 5, /* Graphical mode */
    RUNLEVEL_REBOOT = 6,    /* Reboot */
    RUNLEVEL_SHUTDOWN = 7   /* Shutdown */
} runlevel_t;

/* Service states */
typedef enum {
    SERVICE_STATE_STOPPED = 0,
    SERVICE_STATE_STARTING,
    SERVICE_STATE_RUNNING,
    SERVICE_STATE_STOPPING,
    SERVICE_STATE_FAILED
} service_state_t;

/* Service types */
typedef enum {
    SERVICE_TYPE_SIMPLE = 0,    /* Start once and done */
    SERVICE_TYPE_FORKING,       /* Forks and parent exits */
    SERVICE_TYPE_ONESHOT,       /* Run once, don't track */
    SERVICE_TYPE_DAEMON         /* Long-running daemon */
} service_type_t;

/* Service definition */
typedef struct service {
    char name[64];              /* Service name */
    char description[128];      /* Service description */
    char command[256];          /* Command to start service */
    service_type_t type;        /* Service type */
    service_state_t state;      /* Current state */
    int restart_on_failure;     /* Auto-restart on failure */
    int priority;               /* Start priority (lower = earlier) */
    mach_port_t task_port;      /* Task port if running */
    int pid;                    /* Process ID if applicable */
    struct service *next;       /* Next service in list */
} service_t;

/* Init process state */
typedef struct {
    mach_port_t host_priv_port;     /* Host privileged port */
    mach_port_t device_master_port; /* Device master port */
    runlevel_t runlevel;            /* Current runlevel */
    boot_mode_t boot_mode;          /* Boot mode */
    int single_user;                /* Single user mode flag */
    service_t *services;            /* Registered services */
    int service_count;              /* Number of services */
} init_state_t;

/* Reboot flags */
#define RB_HALT     0x08    /* Halt system */
#define RB_POWEROFF 0x4000  /* Power off */

/*
 * Logging functions
 */
void init_log(log_level_t level, const char *fmt, ...);

/*
 * Service management (services.c)
 */
int services_init(void);
void services_cleanup(void);
int service_register(const char *name, const char *command, service_type_t type);
int service_unregister(const char *name);
service_t *service_find(const char *name);
int service_start(const char *name);
int service_stop(const char *name);
int service_restart(const char *name);
service_state_t service_get_state(const char *name);
void services_start_all(void);
void services_stop_all(void);
void services_process_events(void);
void services_list(void);

/*
 * Shutdown functions
 */
void init_shutdown(int reboot);
void init_reboot(void);
void init_halt(void);
void init_poweroff(void);

/*
 * Utility functions
 */
int spawn_process(const char *command, mach_port_t *task_port);
int wait_for_process(mach_port_t task_port);

#endif /* _INIT_H_ */
