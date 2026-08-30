/*
 * c9o-mach Installer - Header File
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <testlib.h>
#include <kern/boot_params.h>

#include <device.user.h>
#include <mach.user.h>
#include <mach_port.user.h>

/* Version information */
#define INSTALLER_VERSION "1.0.0"
#define INSTALLER_NAME "c9o-mach Installer"

/* Printed once the installer is up, and looked for by the ISO boot
   tests, see scripts/validate-iso.sh.  */
#define INSTALLER_READY_MARKER "c9o-mach-installer-ready"

/* The installer runs as a bootstrap task without a heap, so the disk
   and partition descriptors come from fixed size pools.  */
#define INSTALLER_MAX_DISKS 8
#define INSTALLER_MAX_PARTITIONS 32

/* Installation result codes */
#define INSTALL_SUCCESS     0
#define INSTALL_CANCELLED   1
#define INSTALL_FAILED      2
#define INSTALL_ERROR_DISK  3
#define INSTALL_ERROR_FS    4
#define INSTALL_ERROR_BOOT  5
#define INSTALL_ERROR_COPY  6

/* Installation steps */
typedef enum {
    STEP_WELCOME = 0,
    STEP_DISK_SELECT,
    STEP_PARTITION,
    STEP_CONFIRM,
    STEP_INSTALL,
    STEP_BOOTLOADER,
    STEP_COMPLETE
} install_step_t;

/* UI result codes */
#define UI_RESULT_NEXT      0
#define UI_RESULT_BACK      1
#define UI_RESULT_CANCEL    2
#define UI_RESULT_ERROR     3

/* Logging levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;

/* Disk types */
typedef enum {
    DISK_TYPE_UNKNOWN = 0,
    DISK_TYPE_ATA,
    DISK_TYPE_AHCI,
    DISK_TYPE_SCSI,
    DISK_TYPE_VIRTIO,
    DISK_TYPE_NVME
} disk_type_t;

/* Partition types */
typedef enum {
    PART_TYPE_EMPTY = 0x00,
    PART_TYPE_FAT32 = 0x0B,
    PART_TYPE_FAT32_LBA = 0x0C,
    PART_TYPE_LINUX = 0x83,
    PART_TYPE_LINUX_SWAP = 0x82,
    PART_TYPE_EFI = 0xEF,
    PART_TYPE_MACH = 0xA7,  /* GNU Mach partition type */
    PART_TYPE_HURD = 0x63   /* GNU HURD partition type */
} partition_type_t;

/* Partition table types */
typedef enum {
    PTABLE_TYPE_MBR = 0,
    PTABLE_TYPE_GPT
} ptable_type_t;

/* Filesystem types */
typedef enum {
    FS_TYPE_NONE = 0,
    FS_TYPE_EXT2,
    FS_TYPE_EXT3,
    FS_TYPE_EXT4,
    FS_TYPE_FAT32,
    FS_TYPE_SWAP
} filesystem_type_t;

/* Disk information structure */
typedef struct disk_info {
    char device_name[64];       /* Device name (e.g., "hd0") */
    char model[128];            /* Device model string */
    disk_type_t type;           /* Disk type */
    uint64_t size_bytes;        /* Total size in bytes */
    uint64_t sector_size;       /* Sector size (usually 512) */
    uint64_t total_sectors;     /* Total number of sectors */
    ptable_type_t ptable_type;  /* Partition table type */
    int partition_count;        /* Number of partitions */
    int removable;              /* Is removable media */
    struct disk_info *next;     /* Next disk in list */
} disk_info_t;

/* Partition information structure */
typedef struct partition_info {
    int number;                 /* Partition number (1-based) */
    uint64_t start_sector;      /* Start sector */
    uint64_t end_sector;        /* End sector */
    uint64_t size_bytes;        /* Size in bytes */
    partition_type_t type;      /* Partition type code */
    filesystem_type_t fs_type;  /* Filesystem type */
    char label[64];             /* Partition label */
    int bootable;               /* Bootable flag */
    struct partition_info *next;/* Next partition in list */
} partition_info_t;

/* Partition configuration for installation */
typedef struct {
    int use_entire_disk;        /* Use entire disk (wipe all) */
    int create_swap;            /* Create swap partition */
    uint64_t swap_size_mb;      /* Swap size in MB */
    uint64_t root_size_mb;      /* Root partition size (0 = remainder) */
    filesystem_type_t root_fs;  /* Filesystem for root */
    ptable_type_t ptable_type;  /* Partition table type to create */
} partition_config_t;

/* Installer state */
typedef struct {
    install_step_t current_step;
    int expert_mode;
    int debug_mode;
    disk_info_t *target_disk;
    partition_config_t partition_config;
    partition_info_t *root_partition;
    partition_info_t *swap_partition;
    partition_info_t *efi_partition;
    char hostname[64];
    char root_password[128];
    int install_bootloader;
} installer_state_t;

/*
 * Logging functions
 */
void installer_log(log_level_t level, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
void installer_set_log_level(log_level_t level);
const char *get_install_error_msg(int error_code);

/*
 * UI functions (ui.c)
 */
int ui_init(void);
void ui_cleanup(void);
int ui_show_welcome(void);
int ui_disk_selection(disk_info_t **selected_disk);
int ui_partition_config(disk_info_t *disk, partition_config_t *config);
int ui_show_confirmation(installer_state_t *state);
int ui_show_progress(const char *message, int percent);
int ui_show_completion(void);
void ui_show_error(const char *title, const char *message);
int ui_prompt_yes_no(const char *question);
int ui_prompt_string(const char *prompt, char *buffer, size_t bufsize);

/*
 * Disk functions (disk.c)
 */
int disk_init(void);
void disk_cleanup(void);
disk_info_t *disk_enumerate(void);
void disk_free_list(disk_info_t *list);
partition_info_t *disk_get_partitions(disk_info_t *disk);
void partition_free_list(partition_info_t *list);
int disk_read_sectors(disk_info_t *disk, uint64_t start, uint64_t count, void *buffer);
int disk_write_sectors(disk_info_t *disk, uint64_t start, uint64_t count, const void *buffer);
const char *disk_type_to_string(disk_type_t type);
const char *partition_type_to_string(partition_type_t type);

/*
 * Partition functions (partition.c)
 */
int partition_create_table(disk_info_t *disk, ptable_type_t type);
int partition_create(disk_info_t *disk, uint64_t start, uint64_t size, partition_type_t type);
int partition_delete(disk_info_t *disk, int partition_num);
int partition_set_bootable(disk_info_t *disk, int partition_num, int bootable);
int partition_auto_layout(disk_info_t *disk, partition_config_t *config);

/*
 * Filesystem functions (filesystem.c)
 */
int fs_create(disk_info_t *disk, int partition_num, filesystem_type_t type, const char *label);
int fs_mount(disk_info_t *disk, int partition_num, const char *mountpoint);
int fs_unmount(const char *mountpoint);
int fs_copy_file(const char *src, const char *dst);
int fs_copy_recursive(const char *src, const char *dst);
const char *fs_type_to_string(filesystem_type_t type);

/*
 * Bootloader functions (bootloader.c)
 */
int bootloader_detect_mode(void);  /* Returns 1 for EFI, 0 for BIOS */
int bootloader_install_grub(disk_info_t *disk, const char *root_mountpoint);
int bootloader_generate_config(const char *root_mountpoint);
int bootloader_update_config(const char *root_mountpoint);

/*
 * Installation functions
 */
int perform_installation(installer_state_t *state);
int install_bootloader(installer_state_t *state);
int copy_system_files(installer_state_t *state, const char *root_mountpoint);

/*
 * Utility functions
 */
char *format_size(uint64_t bytes, char *buffer, size_t bufsize);
uint64_t parse_size(const char *str);

#endif /* _INSTALLER_H_ */
