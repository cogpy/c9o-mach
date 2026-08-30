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

/* Global installer state */
static installer_state_t installer_state;

/* Boot parameters parsed from kernel command line */
static int expert_mode = 0;
static int debug_mode = 0;
static int unattended = 0;
static char target_device[BOOT_PARAM_VALUE_MAX] = "";

/*
 * Parse boot parameters for installer configuration.  The command line
 * is parsed with the same helpers as the kernel, see kern/boot_params.c.
 */
static void parse_boot_params(const char *cmdline)
{
    if (cmdline == NULL)
        return;
    
    /* Check for expert mode */
    if (boot_param_flag(cmdline, "expert")) {
        expert_mode = 1;
        installer_log(LOG_INFO, "Expert mode enabled");
    }
    
    /* Check for debug mode */
    if (boot_param_flag(cmdline, "debug")) {
        debug_mode = 1;
        installer_set_log_level(LOG_DEBUG);
        installer_log(LOG_INFO, "Debug mode enabled");
    }
    
    /* Writing to a disk is only allowed when explicitly requested */
    if (boot_param_flag(cmdline, "unattended")) {
        unattended = 1;
        installer_log(LOG_WARNING,
                      "Unattended installation requested, the target disk will be overwritten");
    }
    
    /* Parse target device if specified */
    if (boot_param_option(cmdline, "target",
                          target_device, sizeof(target_device)))
        installer_log(LOG_INFO, "Target device: %s", target_device);
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
    
    /* The installer has no console input yet, so nothing is written to a
       disk unless the boot command line asked for it explicitly.  */
    if (!unattended) {
        installer_log(LOG_INFO,
                      "Stopping before any change is written to %s",
                      installer_state.target_disk->device_name);
        installer_log(LOG_INFO,
                      "Boot with `install unattended target=DEVICE' to perform the installation");
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
int main(int argc, char *argv[], int envc, char *envp[])
{
    int result;
    
    installer_log(LOG_INFO, "c9o-mach Installer starting...");
    installer_log(LOG_INFO, "Version: %s", INSTALLER_VERSION);
    
    /* The kernel boot script passes the host and device master ports
       followed by the kernel command line, see
       packaging/iso/grub.cfg.install.template.  */
    parse_boot_params(argc > 3 ? argv[3] : "");
    
    /* Initialize installer */
    result = installer_init();
    if (result != 0) {
        installer_log(LOG_ERROR, "Installer initialization failed");
        return 1;
    }
    
    /* Announce that the installer is up.  The ISO boot tests wait for
       this line, see scripts/validate-iso.sh.  */
    printf("%s: %s %s\n", INSTALLER_READY_MARKER,
           INSTALLER_NAME, INSTALLER_VERSION);
    
    /* Run installation process */
    result = installer_run();
    
    /* Cleanup */
    installer_cleanup();
    
    /* Handle result */
    switch (result) {
        case INSTALL_SUCCESS:
            installer_log(LOG_INFO, "Installation completed successfully");
            installer_log(LOG_INFO, "Please remove installation media and reboot");
            break;
            
        case INSTALL_CANCELLED:
            installer_log(LOG_INFO, "Installation cancelled");
            break;
            
        case INSTALL_FAILED:
            installer_log(LOG_ERROR, "Installation failed");
            break;
            
        default:
            installer_log(LOG_ERROR, "Unknown installation result: %d", result);
            break;
    }
    
    /* Returning from the bootstrap task reboots the machine, so wait
       instead and let the user power the system down.  */
    while (1)
        msleep(1000);
    
    return result;
}
