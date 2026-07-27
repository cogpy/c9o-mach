/*
 * c9o-mach Installer - Main Entry Point
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

#include <mach.h>
#include <mach/mach_port.h>
#include <device/device.h>

/* Global installer state */
static installer_state_t installer_state;

/* Boot parameters parsed from kernel command line */
static int expert_mode = 0;
static int debug_mode = 0;
static char target_device[256] = "";

/*
 * Parse boot parameters for installer configuration
 */
static void parse_boot_params(const char *cmdline)
{
    if (cmdline == NULL)
        return;
    
    /* Check for expert mode */
    if (strstr(cmdline, "expert") != NULL) {
        expert_mode = 1;
        installer_log(LOG_INFO, "Expert mode enabled");
    }
    
    /* Check for debug mode */
    if (strstr(cmdline, "debug") != NULL) {
        debug_mode = 1;
        installer_log(LOG_INFO, "Debug mode enabled");
    }
    
    /* Parse target device if specified */
    const char *target = strstr(cmdline, "target=");
    if (target != NULL) {
        target += 7; /* Skip "target=" */
        int i = 0;
        while (*target && *target != ' ' && i < sizeof(target_device) - 1) {
            target_device[i++] = *target++;
        }
        target_device[i] = '\0';
        installer_log(LOG_INFO, "Target device: %s", target_device);
    }
}

/*
 * Initialize the installer
 */
static int installer_init(void)
{
    installer_log(LOG_INFO, "Initializing c9o-mach installer...");
    
    /* Initialize state */
    memset(&installer_state, 0, sizeof(installer_state));
    installer_state.current_step = STEP_WELCOME;
    installer_state.expert_mode = expert_mode;
    installer_state.debug_mode = debug_mode;
    
    /* Initialize subsystems */
    if (ui_init() != 0) {
        installer_log(LOG_ERROR, "Failed to initialize UI");
        return -1;
    }
    
    if (disk_init() != 0) {
        installer_log(LOG_ERROR, "Failed to initialize disk subsystem");
        return -1;
    }
    
    installer_log(LOG_INFO, "Installer initialized successfully");
    return 0;
}

/*
 * Cleanup installer resources
 */
static void installer_cleanup(void)
{
    installer_log(LOG_INFO, "Cleaning up installer...");
    
    disk_cleanup();
    ui_cleanup();
}

/*
 * Run the installation process
 */
static int installer_run(void)
{
    int result = 0;
    
    /* Welcome screen */
    if (ui_show_welcome() != UI_RESULT_NEXT) {
        return INSTALL_CANCELLED;
    }
    installer_state.current_step = STEP_DISK_SELECT;
    
    /* Disk selection */
    disk_info_t *selected_disk = NULL;
    result = ui_disk_selection(&selected_disk);
    if (result != UI_RESULT_NEXT || selected_disk == NULL) {
        return INSTALL_CANCELLED;
    }
    installer_state.target_disk = selected_disk;
    installer_state.current_step = STEP_PARTITION;
    
    /* Partition configuration */
    partition_config_t partition_config;
    result = ui_partition_config(selected_disk, &partition_config);
    if (result != UI_RESULT_NEXT) {
        return INSTALL_CANCELLED;
    }
    installer_state.partition_config = partition_config;
    installer_state.current_step = STEP_CONFIRM;
    
    /* Confirmation */
    result = ui_show_confirmation(&installer_state);
    if (result != UI_RESULT_NEXT) {
        return INSTALL_CANCELLED;
    }
    installer_state.current_step = STEP_INSTALL;
    
    /* Perform installation */
    result = perform_installation(&installer_state);
    if (result != 0) {
        ui_show_error("Installation failed", get_install_error_msg(result));
        return INSTALL_FAILED;
    }
    installer_state.current_step = STEP_BOOTLOADER;
    
    /* Install bootloader */
    result = install_bootloader(&installer_state);
    if (result != 0) {
        ui_show_error("Bootloader installation failed", get_install_error_msg(result));
        return INSTALL_FAILED;
    }
    installer_state.current_step = STEP_COMPLETE;
    
    /* Completion screen */
    ui_show_completion();
    
    return INSTALL_SUCCESS;
}

/*
 * Main entry point for the installer
 */
int main(int argc, char *argv[])
{
    int result;
    
    installer_log(LOG_INFO, "c9o-mach Installer starting...");
    installer_log(LOG_INFO, "Version: %s", INSTALLER_VERSION);
    
    /* Parse boot parameters */
    extern char *kernel_cmdline;
    parse_boot_params(kernel_cmdline);
    
    /* Override target device from command line if provided */
    if (argc > 1 && argv[1][0] != '-') {
        strncpy(target_device, argv[1], sizeof(target_device) - 1);
    }
    
    /* Initialize installer */
    result = installer_init();
    if (result != 0) {
        installer_log(LOG_ERROR, "Installer initialization failed");
        return 1;
    }
    
    /* Run installation process */
    result = installer_run();
    
    /* Cleanup */
    installer_cleanup();
    
    /* Handle result */
    switch (result) {
        case INSTALL_SUCCESS:
            installer_log(LOG_INFO, "Installation completed successfully");
            installer_log(LOG_INFO, "Please remove installation media and reboot");
            return 0;
            
        case INSTALL_CANCELLED:
            installer_log(LOG_INFO, "Installation cancelled by user");
            return 0;
            
        case INSTALL_FAILED:
            installer_log(LOG_ERROR, "Installation failed");
            return 1;
            
        default:
            installer_log(LOG_ERROR, "Unknown installation result: %d", result);
            return 1;
    }
}
