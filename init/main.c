/*
 * c9o-mach Init System - Main Entry Point
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The init process is the first userspace process started by the kernel.
 * It receives privileged ports and is responsible for:
 * - Starting essential system services
 * - Managing the service lifecycle
 * - Handling system shutdown/reboot
 */

#include "init.h"

#include <mach.h>
#include <mach/mach_port.h>
#include <device/device.h>

/* Global state */
static init_state_t init_state;

/* Boot mode detection strings */
static const char *BOOT_MODE_LIVE = "live";
static const char *BOOT_MODE_INSTALL = "install";
static const char *BOOT_MODE_RESCUE = "rescue";
static const char *BOOT_MODE_NORMAL = "normal";

/*
 * Parse boot mode from kernel command line
 */
static boot_mode_t parse_boot_mode(const char *cmdline)
{
    if (cmdline == NULL)
        return BOOT_MODE_TYPE_NORMAL;
    
    if (strstr(cmdline, BOOT_MODE_INSTALL) != NULL)
        return BOOT_MODE_TYPE_INSTALL;
    
    if (strstr(cmdline, BOOT_MODE_LIVE) != NULL)
        return BOOT_MODE_TYPE_LIVE;
    
    if (strstr(cmdline, BOOT_MODE_RESCUE) != NULL)
        return BOOT_MODE_TYPE_RESCUE;
    
    return BOOT_MODE_TYPE_NORMAL;
}

/*
 * Parse single-user mode flag
 */
static int parse_single_user(const char *cmdline)
{
    if (cmdline == NULL)
        return 0;
    
    if (strstr(cmdline, "single") != NULL || strstr(cmdline, "-s") != NULL)
        return 1;
    
    return 0;
}

/*
 * Initialize the init process
 */
static int init_initialize(mach_port_t host_port, mach_port_t device_port)
{
    init_log(LOG_INFO, "c9o-mach init starting...");
    init_log(LOG_INFO, "Version: %s", INIT_VERSION);
    
    /* Initialize state */
    memset(&init_state, 0, sizeof(init_state));
    init_state.host_priv_port = host_port;
    init_state.device_master_port = device_port;
    init_state.runlevel = RUNLEVEL_BOOT;
    
    /* Parse boot parameters */
    extern char *kernel_cmdline;
    init_state.boot_mode = parse_boot_mode(kernel_cmdline);
    init_state.single_user = parse_single_user(kernel_cmdline);
    
    /* Log boot mode */
    const char *mode_str;
    switch (init_state.boot_mode) {
        case BOOT_MODE_TYPE_LIVE:
            mode_str = "Live";
            break;
        case BOOT_MODE_TYPE_INSTALL:
            mode_str = "Installation";
            break;
        case BOOT_MODE_TYPE_RESCUE:
            mode_str = "Rescue";
            break;
        default:
            mode_str = "Normal";
            break;
    }
    init_log(LOG_INFO, "Boot mode: %s%s", mode_str, 
             init_state.single_user ? " (single user)" : "");
    
    /* Initialize service management */
    if (services_init() != 0) {
        init_log(LOG_ERROR, "Failed to initialize service management");
        return -1;
    }
    
    init_log(LOG_INFO, "Init initialized successfully");
    return 0;
}

/*
 * Enter single-user mode
 */
static void enter_single_user_mode(void)
{
    init_log(LOG_INFO, "Entering single-user mode");
    init_state.runlevel = RUNLEVEL_SINGLE;
    
    /* Start only essential services */
    service_start("console");
    
    /* Start a shell on the console */
    init_log(LOG_INFO, "Starting single-user shell...");
    /* spawn_shell() would be called here */
}

/*
 * Enter multi-user mode
 */
static void enter_multi_user_mode(void)
{
    init_log(LOG_INFO, "Entering multi-user mode");
    init_state.runlevel = RUNLEVEL_MULTI;
    
    /* Start all configured services */
    services_start_all();
}

/*
 * Handle live boot mode
 */
static void handle_live_boot(void)
{
    init_log(LOG_INFO, "Starting live environment...");
    
    /* Mount ramdisk as root */
    init_log(LOG_INFO, "Mounting ramdisk root filesystem");
    
    /* Start console service */
    service_start("console");
    
    /* Start networking (DHCP) */
    service_start("network");
    
    /* Start live shell */
    init_log(LOG_INFO, "Starting live shell...");
    init_log(LOG_INFO, "Welcome to c9o-mach Live Environment");
    init_log(LOG_INFO, "Type 'help' for available commands");
}

/*
 * Handle installation boot mode
 */
static void handle_install_boot(void)
{
    init_log(LOG_INFO, "Starting installation mode...");
    
    /* Start console service */
    service_start("console");
    
    /* Start the installer */
    init_log(LOG_INFO, "Launching installer...");
    /* exec_installer() would be called here */
    
    init_log(LOG_INFO, "Installer will guide you through the installation process");
}

/*
 * Handle rescue boot mode
 */
static void handle_rescue_boot(void)
{
    init_log(LOG_INFO, "Starting rescue mode...");
    
    /* Minimal environment - just console */
    service_start("console");
    
    init_log(LOG_INFO, "Rescue mode active");
    init_log(LOG_INFO, "Essential services only - use 'service start <name>' to start others");
}

/*
 * Handle normal boot mode
 */
static void handle_normal_boot(void)
{
    init_log(LOG_INFO, "Starting normal boot...");
    
    /* Check root filesystem */
    init_log(LOG_INFO, "Checking filesystems...");
    /* fsck would be run here */
    
    /* Mount root filesystem read-write */
    init_log(LOG_INFO, "Mounting root filesystem...");
    
    /* Check if single-user mode requested */
    if (init_state.single_user) {
        enter_single_user_mode();
    } else {
        enter_multi_user_mode();
    }
}

/*
 * Main init loop
 */
static void init_main_loop(void)
{
    /* Handle boot mode */
    switch (init_state.boot_mode) {
        case BOOT_MODE_TYPE_LIVE:
            handle_live_boot();
            break;
            
        case BOOT_MODE_TYPE_INSTALL:
            handle_install_boot();
            break;
            
        case BOOT_MODE_TYPE_RESCUE:
            handle_rescue_boot();
            break;
            
        default:
            handle_normal_boot();
            break;
    }
    
    /* Main event loop */
    init_log(LOG_INFO, "Init entering main loop");
    
    while (1) {
        /* Wait for events:
         * - Service termination
         * - Shutdown requests
         * - Signal handling (if applicable)
         */
        
        /* Process service events */
        services_process_events();
        
        /* Small delay to prevent busy loop */
        /* In real implementation, would use mach_msg with timeout */
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

/*
 * Shutdown the system
 */
void init_shutdown(int reboot)
{
    init_log(LOG_INFO, "System %s requested", reboot ? "reboot" : "shutdown");
    
    /* Change runlevel */
    init_state.runlevel = RUNLEVEL_SHUTDOWN;
    
    /* Stop all services in reverse order */
    init_log(LOG_INFO, "Stopping services...");
    services_stop_all();
    
    /* Sync filesystems */
    init_log(LOG_INFO, "Syncing filesystems...");
    /* sync() would be called here */
    
    /* Unmount filesystems */
    init_log(LOG_INFO, "Unmounting filesystems...");
    /* umount_all() would be called here */
    
    /* Perform shutdown/reboot */
    if (reboot) {
        init_log(LOG_INFO, "Rebooting...");
        host_reboot(init_state.host_priv_port, 0);
    } else {
        init_log(LOG_INFO, "System halted.");
        host_reboot(init_state.host_priv_port, RB_HALT);
    }
    
    /* Should never reach here */
    while (1)
        ;
}

/*
 * Main entry point for init
 * Called by the kernel bootstrap mechanism with privileged ports
 */
int main(int argc, char *argv[])
{
    mach_port_t host_port = MACH_PORT_NULL;
    mach_port_t device_port = MACH_PORT_NULL;
    
    /* Get privileged ports from bootstrap */
    /* In Mach, these are typically passed as arguments or through
     * the bootstrap port mechanism */
    
    if (argc >= 3) {
        /* Parse port names from arguments */
        host_port = (mach_port_t)atoi(argv[1]);
        device_port = (mach_port_t)atoi(argv[2]);
    } else {
        /* Try to get ports through bootstrap */
        extern mach_port_t host_priv(void);
        extern mach_port_t device_priv(void);
        host_port = host_priv();
        device_port = device_priv();
    }
    
    /* Initialize */
    if (init_initialize(host_port, device_port) != 0) {
        init_log(LOG_ERROR, "Init initialization failed!");
        return 1;
    }
    
    /* Enter main loop */
    init_main_loop();
    
    /* Should never reach here */
    return 0;
}
