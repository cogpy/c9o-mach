/*
 * c9o-mach Installer - Common Utilities
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

#include <stdarg.h>

/* Log level names */
static const char *log_level_names[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

/* Current log level (can be changed at runtime) */
static log_level_t current_log_level = LOG_INFO;

/*
 * Set the minimum log level
 */
void installer_set_log_level(log_level_t level)
{
    current_log_level = level;
}

/*
 * Log a message
 */
void installer_log(log_level_t level, const char *fmt, ...)
{
    va_list args;
    
    if (level < current_log_level) {
        return;
    }
    
    /* Print log level prefix */
    switch (level) {
        case LOG_DEBUG:
            printf("[DEBUG] ");
            break;
        case LOG_INFO:
            printf("[INFO] ");
            break;
        case LOG_WARNING:
            printf("[WARNING] ");
            break;
        case LOG_ERROR:
            printf("[ERROR] ");
            break;
    }
    
    /* Print the message */
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

/*
 * Get error message for installation error code
 */
const char *get_install_error_msg(int error_code)
{
    switch (error_code) {
        case INSTALL_SUCCESS:
            return "Installation completed successfully";
        case INSTALL_CANCELLED:
            return "Installation was cancelled by user";
        case INSTALL_FAILED:
            return "Installation failed";
        case INSTALL_ERROR_DISK:
            return "Disk operation failed";
        case INSTALL_ERROR_FS:
            return "Filesystem operation failed";
        case INSTALL_ERROR_BOOT:
            return "Bootloader installation failed";
        case INSTALL_ERROR_COPY:
            return "File copy operation failed";
        default:
            return "Unknown error";
    }
}

/*
 * Format a byte size into human-readable string
 */
char *format_size(uint64_t bytes, char *buffer, size_t bufsize)
{
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    uint64_t size = bytes;
    uint64_t remainder = 0;
    
    /* No floating point is available in the bootstrap environment, so
       the fractional digit is computed from the remainder.  */
    while (size >= 1024 && unit_index < 4) {
        remainder = size % 1024;
        size /= 1024;
        unit_index++;
    }
    
    if (unit_index == 0)
        snprintf(buffer, bufsize, "%llu %s",
                 (unsigned long long) size, units[unit_index]);
    else
        snprintf(buffer, bufsize, "%llu.%llu %s",
                 (unsigned long long) size,
                 (unsigned long long) (remainder * 10 / 1024),
                 units[unit_index]);
    
    return buffer;
}

/*
 * Parse an unsigned decimal number, returning a pointer to the first
 * character that is not part of it.  The bootstrap environment has no
 * C library, so this replaces strtoull().
 */
static uint64_t parse_u64(const char *str, const char **endptr)
{
    uint64_t value = 0;
    
    while (*str >= '0' && *str <= '9') {
        value = value * 10 + (uint64_t) (*str - '0');
        str++;
    }
    
    if (endptr != NULL)
        *endptr = str;
    
    return value;
}

/*
 * Parse a size string (e.g., "512M", "1G", "1024K") into bytes
 */
uint64_t parse_size(const char *str)
{
    uint64_t value = 0;
    uint64_t multiplier = 1;
    const char *endptr;
    
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    /* Parse the numeric part */
    value = parse_u64(str, &endptr);
    
    /* Check for size suffix */
    if (endptr != NULL && *endptr != '\0') {
        switch (*endptr) {
            case 'k':
            case 'K':
                multiplier = 1024ULL;
                break;
            case 'm':
            case 'M':
                multiplier = 1024ULL * 1024;
                break;
            case 'g':
            case 'G':
                multiplier = 1024ULL * 1024 * 1024;
                break;
            case 't':
            case 'T':
                multiplier = 1024ULL * 1024 * 1024 * 1024;
                break;
            default:
                /* Unknown suffix, assume bytes */
                break;
        }
    }
    
    return value * multiplier;
}

/*
 * Perform the installation process
 */
int perform_installation(installer_state_t *state)
{
    int result;
    
    if (state == NULL || state->target_disk == NULL) {
        return INSTALL_FAILED;
    }
    
    installer_log(LOG_INFO, "Starting installation to %s", 
                  state->target_disk->device_name);
    
    /* Step 1: Create partition layout */
    ui_show_progress("Creating partitions...", 10);
    result = partition_auto_layout(state->target_disk, &state->partition_config);
    if (result != 0) {
        installer_log(LOG_ERROR, "Failed to create partitions");
        return INSTALL_ERROR_DISK;
    }
    
    /* Step 2: Get partition information */
    ui_show_progress("Reading partition table...", 20);
    partition_info_t *parts = disk_get_partitions(state->target_disk);
    if (parts == NULL) {
        installer_log(LOG_ERROR, "Failed to read partition table");
        return INSTALL_ERROR_DISK;
    }
    
    /* Find root partition (first non-swap partition) */
    for (partition_info_t *p = parts; p != NULL; p = p->next) {
        if (p->type != PART_TYPE_LINUX_SWAP) {
            state->root_partition = p;
            break;
        }
    }
    
    if (state->root_partition == NULL) {
        installer_log(LOG_ERROR, "No root partition found");
        partition_free_list(parts);
        return INSTALL_ERROR_DISK;
    }
    
    /* Step 3: Create filesystems */
    ui_show_progress("Creating filesystem...", 40);
    result = fs_create(state->target_disk, state->root_partition->number,
                       state->partition_config.root_fs, "c9o-mach");
    if (result != 0) {
        installer_log(LOG_ERROR, "Failed to create root filesystem");
        partition_free_list(parts);
        return INSTALL_ERROR_FS;
    }
    
    /* Create swap if requested */
    if (state->partition_config.create_swap) {
        ui_show_progress("Creating swap space...", 50);
        for (partition_info_t *p = parts; p != NULL; p = p->next) {
            if (p->type == PART_TYPE_LINUX_SWAP) {
                state->swap_partition = p;
                fs_create(state->target_disk, p->number, FS_TYPE_SWAP, "swap");
                break;
            }
        }
    }
    
    /* Step 4: Mount target filesystem */
    ui_show_progress("Mounting target filesystem...", 60);
    result = fs_mount(state->target_disk, state->root_partition->number, "/mnt/target");
    if (result != 0) {
        installer_log(LOG_WARNING, "Could not mount target (may need Hurd servers)");
        /* Continue anyway - manual installation possible */
    }
    
    /* Step 5: Copy system files */
    ui_show_progress("Copying system files...", 70);
    result = copy_system_files(state, "/mnt/target");
    if (result != 0) {
        installer_log(LOG_WARNING, "File copy incomplete (may need manual completion)");
        /* Continue with bootloader installation */
    }
    
    ui_show_progress("Installation complete", 100);
    
    installer_log(LOG_INFO, "Installation to %s completed",
                  state->target_disk->device_name);
    
    return INSTALL_SUCCESS;
}

/*
 * Install the bootloader
 */
int install_bootloader(installer_state_t *state)
{
    int result;
    
    if (state == NULL || state->target_disk == NULL) {
        return INSTALL_ERROR_BOOT;
    }
    
    installer_log(LOG_INFO, "Installing bootloader...");
    
    result = bootloader_install_grub(state->target_disk, "/mnt/target");
    if (result != 0) {
        installer_log(LOG_ERROR, "Bootloader installation failed");
        return INSTALL_ERROR_BOOT;
    }
    
    installer_log(LOG_INFO, "Bootloader installed successfully");
    return INSTALL_SUCCESS;
}

/*
 * Copy system files to target
 */
int copy_system_files(installer_state_t *state, const char *root_mountpoint)
{
    installer_log(LOG_INFO, "Copying system files to %s", root_mountpoint);
    
    /* In a real implementation, this would:
     * 1. Copy the kernel to /boot/gnumach
     * 2. Copy Hurd servers to /hurd/
     * 3. Copy essential libraries to /lib/
     * 4. Copy configuration files
     * 5. Set up /etc/fstab
     * 6. Create device nodes in /dev/
     */
    
    /* For now, just log what would be copied */
    installer_log(LOG_INFO, "Would copy: /boot/gnumach (kernel)");
    installer_log(LOG_INFO, "Would copy: /hurd/* (Hurd servers)");
    installer_log(LOG_INFO, "Would copy: /lib/* (libraries)");
    installer_log(LOG_INFO, "Would copy: /etc/* (configuration)");
    
    installer_log(LOG_WARNING, 
                  "Manual file copy required - use live environment to complete");
    
    return 0;
}
