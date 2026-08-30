/*
 * c9o-mach Init System - Service Management
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "init.h"

#include <stdarg.h>

/* Init runs without a heap, so services live in a fixed size table.  */
static service_t service_table[INIT_MAX_SERVICES];
static int service_count = 0;

/*
 * Initialize service management
 */
int services_init(void)
{
    init_log(LOG_DEBUG, "Initializing service management");
    
    /* Register built-in services */
    service_register("console", "/sbin/console", SERVICE_TYPE_DAEMON);
    service_register("disk", "/hurd/storeio", SERVICE_TYPE_DAEMON);
    service_register("network", "/hurd/pfinet", SERVICE_TYPE_DAEMON);
    service_register("auth", "/hurd/auth", SERVICE_TYPE_DAEMON);
    service_register("proc", "/hurd/proc", SERVICE_TYPE_DAEMON);
    
    init_log(LOG_INFO, "Service management initialized with %d services", 
             service_count);
    
    return 0;
}

/*
 * Cleanup service management
 */
void services_cleanup(void)
{
    memset(service_table, 0, sizeof(service_table));
    service_count = 0;
}

/*
 * Register a new service
 */
int service_register(const char *name, const char *command, service_type_t type)
{
    if (name == NULL || command == NULL)
        return -1;
    
    /* Check if already registered */
    if (service_find(name) != NULL) {
        init_log(LOG_WARNING, "Service '%s' already registered", name);
        return -1;
    }
    
    /* Take a free slot in the service table */
    service_t *s = NULL;
    for (int i = 0; i < INIT_MAX_SERVICES; i++) {
        if (!service_table[i].used) {
            s = &service_table[i];
            break;
        }
    }
    if (s == NULL) {
        init_log(LOG_ERROR, "No free service slot for '%s'", name);
        return -1;
    }
    memset(s, 0, sizeof(service_t));
    s->used = 1;
    
    /* Fill in service info */
    strncpy(s->name, name, sizeof(s->name) - 1);
    strncpy(s->command, command, sizeof(s->command) - 1);
    s->type = type;
    s->state = SERVICE_STATE_STOPPED;
    s->restart_on_failure = 0;
    s->priority = service_count;  /* Lower number = earlier start */
    s->task_port = MACH_PORT_NULL;
    
    service_count++;
    
    init_log(LOG_DEBUG, "Registered service: %s", name);
    return 0;
}

/*
 * Unregister a service
 */
int service_unregister(const char *name)
{
    service_t *s = service_find(name);
    
    if (s == NULL)
        return -1;  /* Not found */
    
    /* Stop if running */
    if (s->state == SERVICE_STATE_RUNNING) {
        service_stop(name);
    }
    
    memset(s, 0, sizeof(service_t));
    service_count--;
    
    init_log(LOG_DEBUG, "Unregistered service: %s", name);
    return 0;
}

/*
 * Find a service by name
 */
service_t *service_find(const char *name)
{
    for (int i = 0; i < INIT_MAX_SERVICES; i++) {
        service_t *s = &service_table[i];
        if (s->used && strcmp(s->name, name) == 0) {
            return s;
        }
    }
    return NULL;
}

/*
 * Start a service
 */
int service_start(const char *name)
{
    service_t *s = service_find(name);
    
    if (s == NULL) {
        init_log(LOG_ERROR, "Service '%s' not found", name);
        return -1;
    }
    
    if (s->state == SERVICE_STATE_RUNNING) {
        init_log(LOG_WARNING, "Service '%s' already running", name);
        return 0;
    }
    
    init_log(LOG_INFO, "Starting service: %s", name);
    s->state = SERVICE_STATE_STARTING;
    
    /* In a real implementation, this would:
     * 1. Fork a new task
     * 2. Execute the service command
     * 3. Track the task port
     */
    
    /* For now, simulate starting */
    s->state = SERVICE_STATE_RUNNING;
    init_log(LOG_INFO, "Service '%s' started", name);
    
    return 0;
}

/*
 * Stop a service
 */
int service_stop(const char *name)
{
    service_t *s = service_find(name);
    
    if (s == NULL) {
        init_log(LOG_ERROR, "Service '%s' not found", name);
        return -1;
    }
    
    if (s->state != SERVICE_STATE_RUNNING) {
        init_log(LOG_WARNING, "Service '%s' not running", name);
        return 0;
    }
    
    init_log(LOG_INFO, "Stopping service: %s", name);
    s->state = SERVICE_STATE_STOPPING;
    
    /* In a real implementation, this would:
     * 1. Send termination signal to the task
     * 2. Wait for graceful shutdown
     * 3. Force kill if necessary
     */
    
    /* Clean up task port if any */
    if (s->task_port != MACH_PORT_NULL) {
        /* task_terminate(s->task_port); */
        mach_port_deallocate(mach_task_self(), s->task_port);
        s->task_port = MACH_PORT_NULL;
    }
    
    s->state = SERVICE_STATE_STOPPED;
    init_log(LOG_INFO, "Service '%s' stopped", name);
    
    return 0;
}

/*
 * Restart a service
 */
int service_restart(const char *name)
{
    init_log(LOG_INFO, "Restarting service: %s", name);
    
    service_stop(name);
    return service_start(name);
}

/*
 * Get service state
 */
service_state_t service_get_state(const char *name)
{
    service_t *s = service_find(name);
    
    if (s == NULL)
        return SERVICE_STATE_STOPPED;
    
    return s->state;
}

/*
 * Start all services in priority order
 */
void services_start_all(void)
{
    init_log(LOG_INFO, "Starting all services...");
    
    /* Simple priority-based start (lower priority = earlier) */
    for (int priority = 0; priority < service_count; priority++) {
        for (int i = 0; i < INIT_MAX_SERVICES; i++) {
            service_t *s = &service_table[i];
            if (s->used && s->priority == priority
                && s->state == SERVICE_STATE_STOPPED) {
                service_start(s->name);
            }
        }
    }
    
    init_log(LOG_INFO, "All services started");
}

/*
 * Stop all services in reverse priority order
 */
void services_stop_all(void)
{
    init_log(LOG_INFO, "Stopping all services...");
    
    /* Stop in reverse priority order */
    for (int priority = service_count - 1; priority >= 0; priority--) {
        for (int i = 0; i < INIT_MAX_SERVICES; i++) {
            service_t *s = &service_table[i];
            if (s->used && s->priority == priority
                && s->state == SERVICE_STATE_RUNNING) {
                service_stop(s->name);
            }
        }
    }
    
    init_log(LOG_INFO, "All services stopped");
}

/*
 * Process service events (check for terminated services, etc.)
 */
void services_process_events(void)
{
    /* In a real implementation, this would:
     * 1. Check for terminated tasks
     * 2. Handle restart_on_failure
     * 3. Process any pending service requests
     */
    
    for (int i = 0; i < INIT_MAX_SERVICES; i++) {
        service_t *s = &service_table[i];
        if (s->used && s->state == SERVICE_STATE_RUNNING
            && s->task_port != MACH_PORT_NULL) {
            /* Check if task still exists */
            /* If terminated and restart_on_failure, restart */
        }
    }
}

/*
 * List all services
 */
void services_list(void)
{
    printf("%-20s %-15s %-10s\n", "NAME", "STATE", "TYPE");
    printf("%-20s %-15s %-10s\n", "----", "-----", "----");
    
    for (int i = 0; i < INIT_MAX_SERVICES; i++) {
        service_t *s = &service_table[i];
        const char *state_str;
        const char *type_str;
        
        if (!s->used)
            continue;
        
        switch (s->state) {
            case SERVICE_STATE_STOPPED:  state_str = "stopped"; break;
            case SERVICE_STATE_STARTING: state_str = "starting"; break;
            case SERVICE_STATE_RUNNING:  state_str = "running"; break;
            case SERVICE_STATE_STOPPING: state_str = "stopping"; break;
            case SERVICE_STATE_FAILED:   state_str = "failed"; break;
            default:                     state_str = "unknown"; break;
        }
        
        switch (s->type) {
            case SERVICE_TYPE_SIMPLE:  type_str = "simple"; break;
            case SERVICE_TYPE_FORKING: type_str = "forking"; break;
            case SERVICE_TYPE_ONESHOT: type_str = "oneshot"; break;
            case SERVICE_TYPE_DAEMON:  type_str = "daemon"; break;
            default:                   type_str = "unknown"; break;
        }
        
        printf("%-20s %-15s %-10s\n", s->name, state_str, type_str);
    }
    
    printf("\nTotal: %d services\n", service_count);
}

/*
 * Logging function
 */
void init_log(log_level_t level, const char *fmt, ...)
{
    va_list args;
    
    /* Print log level prefix */
    switch (level) {
        case LOG_DEBUG:
            printf("[INIT:DEBUG] ");
            break;
        case LOG_INFO:
            printf("[INIT:INFO] ");
            break;
        case LOG_WARNING:
            printf("[INIT:WARNING] ");
            break;
        case LOG_ERROR:
            printf("[INIT:ERROR] ");
            break;
    }
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}
