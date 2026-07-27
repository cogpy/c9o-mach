/*
 * c9o-mach Installer - Partition Management
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

/* MBR constants */
#define MBR_SIGNATURE       0xAA55
#define MBR_PARTITION_OFFSET 446
#define MBR_PARTITION_SIZE   16
#define MBR_MAX_PARTITIONS   4

/* GPT constants */
#define GPT_SIGNATURE       "EFI PART"
#define GPT_HEADER_SIZE     92
#define GPT_ENTRY_SIZE      128
#define GPT_MAX_PARTITIONS  128

/* Alignment for partitions (1MB boundary) */
#define PARTITION_ALIGNMENT_SECTORS (2048)  /* 1MB with 512-byte sectors */

/*
 * Create a new partition table on the disk
 */
int partition_create_table(disk_info_t *disk, ptable_type_t type)
{
    unsigned char mbr[512];
    
    if (disk == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Creating %s partition table on %s",
                  type == PTABLE_TYPE_GPT ? "GPT" : "MBR", disk->device_name);
    
    /* For now, only support MBR */
    if (type == PTABLE_TYPE_GPT) {
        installer_log(LOG_WARNING, "GPT support not yet implemented, using MBR");
        type = PTABLE_TYPE_MBR;
    }
    
    /* Initialize MBR */
    memset(mbr, 0, sizeof(mbr));
    
    /* Set boot code (minimal - just halt) */
    mbr[0] = 0xFA;  /* CLI */
    mbr[1] = 0xF4;  /* HLT */
    mbr[2] = 0xEB;  /* JMP */
    mbr[3] = 0xFD;  /* -3 (loop) */
    
    /* Set MBR signature */
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    
    /* Write MBR to disk */
    if (disk_write_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to write MBR to %s", disk->device_name);
        return -1;
    }
    
    disk->ptable_type = type;
    disk->partition_count = 0;
    
    installer_log(LOG_INFO, "Partition table created successfully");
    return 0;
}

/*
 * Create a new partition
 */
int partition_create(disk_info_t *disk, uint64_t start, uint64_t size, 
                     partition_type_t type)
{
    unsigned char mbr[512];
    int slot = -1;
    
    if (disk == NULL)
        return -1;
    
    /* Read current MBR */
    if (disk_read_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to read MBR from %s", disk->device_name);
        return -1;
    }
    
    /* Verify MBR signature */
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        installer_log(LOG_ERROR, "Invalid MBR signature on %s", disk->device_name);
        return -1;
    }
    
    /* Find an empty partition slot */
    for (int i = 0; i < MBR_MAX_PARTITIONS; i++) {
        unsigned char *entry = &mbr[MBR_PARTITION_OFFSET + i * MBR_PARTITION_SIZE];
        if (entry[4] == PART_TYPE_EMPTY) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        installer_log(LOG_ERROR, "No empty partition slot available on %s",
                      disk->device_name);
        return -1;
    }
    
    /* Align start to 1MB boundary if not already aligned */
    if (start % PARTITION_ALIGNMENT_SECTORS != 0) {
        start = ((start / PARTITION_ALIGNMENT_SECTORS) + 1) * PARTITION_ALIGNMENT_SECTORS;
    }
    
    /* Calculate sectors */
    uint64_t sectors = size / disk->sector_size;
    if (sectors == 0)
        sectors = 1;
    
    /* Align size to sector boundary */
    sectors = (sectors / PARTITION_ALIGNMENT_SECTORS) * PARTITION_ALIGNMENT_SECTORS;
    if (sectors == 0)
        sectors = PARTITION_ALIGNMENT_SECTORS;
    
    installer_log(LOG_INFO, "Creating partition %d: type=0x%02X, start=%llu, sectors=%llu",
                  slot + 1, type, (unsigned long long)start, (unsigned long long)sectors);
    
    /* Fill in partition entry */
    unsigned char *entry = &mbr[MBR_PARTITION_OFFSET + slot * MBR_PARTITION_SIZE];
    
    /* Status (0x00 = not bootable, 0x80 = bootable) */
    entry[0] = 0x00;
    
    /* CHS start (use LBA mode, set to max) */
    entry[1] = 0xFE;  /* Head */
    entry[2] = 0xFF;  /* Sector + Cylinder high bits */
    entry[3] = 0xFF;  /* Cylinder low bits */
    
    /* Partition type */
    entry[4] = (unsigned char)type;
    
    /* CHS end (use LBA mode, set to max) */
    entry[5] = 0xFE;  /* Head */
    entry[6] = 0xFF;  /* Sector + Cylinder high bits */
    entry[7] = 0xFF;  /* Cylinder low bits */
    
    /* LBA start (little-endian) */
    entry[8] = (start >> 0) & 0xFF;
    entry[9] = (start >> 8) & 0xFF;
    entry[10] = (start >> 16) & 0xFF;
    entry[11] = (start >> 24) & 0xFF;
    
    /* LBA size (little-endian) */
    entry[12] = (sectors >> 0) & 0xFF;
    entry[13] = (sectors >> 8) & 0xFF;
    entry[14] = (sectors >> 16) & 0xFF;
    entry[15] = (sectors >> 24) & 0xFF;
    
    /* Write updated MBR */
    if (disk_write_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to write updated MBR to %s", disk->device_name);
        return -1;
    }
    
    disk->partition_count++;
    
    installer_log(LOG_INFO, "Partition %d created successfully", slot + 1);
    return slot + 1;  /* Return partition number (1-based) */
}

/*
 * Delete a partition
 */
int partition_delete(disk_info_t *disk, int partition_num)
{
    unsigned char mbr[512];
    
    if (disk == NULL || partition_num < 1 || partition_num > MBR_MAX_PARTITIONS)
        return -1;
    
    /* Read current MBR */
    if (disk_read_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to read MBR from %s", disk->device_name);
        return -1;
    }
    
    /* Clear partition entry */
    unsigned char *entry = &mbr[MBR_PARTITION_OFFSET + (partition_num - 1) * MBR_PARTITION_SIZE];
    memset(entry, 0, MBR_PARTITION_SIZE);
    
    /* Write updated MBR */
    if (disk_write_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to write updated MBR to %s", disk->device_name);
        return -1;
    }
    
    disk->partition_count--;
    
    installer_log(LOG_INFO, "Partition %d deleted from %s", partition_num, disk->device_name);
    return 0;
}

/*
 * Set bootable flag on a partition
 */
int partition_set_bootable(disk_info_t *disk, int partition_num, int bootable)
{
    unsigned char mbr[512];
    
    if (disk == NULL || partition_num < 1 || partition_num > MBR_MAX_PARTITIONS)
        return -1;
    
    /* Read current MBR */
    if (disk_read_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to read MBR from %s", disk->device_name);
        return -1;
    }
    
    /* Clear bootable flag on all partitions first */
    for (int i = 0; i < MBR_MAX_PARTITIONS; i++) {
        mbr[MBR_PARTITION_OFFSET + i * MBR_PARTITION_SIZE] = 0x00;
    }
    
    /* Set bootable flag on requested partition */
    if (bootable) {
        mbr[MBR_PARTITION_OFFSET + (partition_num - 1) * MBR_PARTITION_SIZE] = 0x80;
    }
    
    /* Write updated MBR */
    if (disk_write_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to write updated MBR to %s", disk->device_name);
        return -1;
    }
    
    installer_log(LOG_INFO, "Set partition %d as %sbootable on %s",
                  partition_num, bootable ? "" : "non-", disk->device_name);
    return 0;
}

/*
 * Automatically create partition layout for installation
 */
int partition_auto_layout(disk_info_t *disk, partition_config_t *config)
{
    uint64_t disk_sectors;
    uint64_t swap_sectors = 0;
    uint64_t root_sectors = 0;
    uint64_t current_sector;
    int root_part, swap_part = 0;
    
    if (disk == NULL || config == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Creating automatic partition layout on %s", disk->device_name);
    
    /* Calculate disk size in sectors */
    disk_sectors = disk->total_sectors;
    if (disk_sectors == 0) {
        disk_sectors = disk->size_bytes / disk->sector_size;
    }
    
    /* Create fresh partition table if using entire disk */
    if (config->use_entire_disk) {
        if (partition_create_table(disk, config->ptable_type) != 0) {
            return -1;
        }
    }
    
    /* Start after first 1MB (for MBR and alignment) */
    current_sector = PARTITION_ALIGNMENT_SECTORS;
    
    /* Calculate swap size */
    if (config->create_swap && config->swap_size_mb > 0) {
        swap_sectors = (config->swap_size_mb * 1024 * 1024) / disk->sector_size;
        /* Align to 1MB */
        swap_sectors = (swap_sectors / PARTITION_ALIGNMENT_SECTORS) * PARTITION_ALIGNMENT_SECTORS;
    }
    
    /* Calculate root size */
    if (config->root_size_mb > 0) {
        root_sectors = (config->root_size_mb * 1024 * 1024) / disk->sector_size;
    } else {
        /* Use remaining space */
        root_sectors = disk_sectors - current_sector - swap_sectors;
    }
    
    /* Align root size */
    root_sectors = (root_sectors / PARTITION_ALIGNMENT_SECTORS) * PARTITION_ALIGNMENT_SECTORS;
    
    /* Create root partition */
    root_part = partition_create(disk, current_sector, 
                                 root_sectors * disk->sector_size, PART_TYPE_MACH);
    if (root_part < 0) {
        installer_log(LOG_ERROR, "Failed to create root partition");
        return -1;
    }
    current_sector += root_sectors;
    
    /* Create swap partition if requested */
    if (config->create_swap && swap_sectors > 0) {
        swap_part = partition_create(disk, current_sector,
                                     swap_sectors * disk->sector_size, PART_TYPE_LINUX_SWAP);
        if (swap_part < 0) {
            installer_log(LOG_WARNING, "Failed to create swap partition (continuing)");
            swap_part = 0;
        }
    }
    
    /* Set root partition as bootable */
    partition_set_bootable(disk, root_part, 1);
    
    installer_log(LOG_INFO, "Partition layout created:");
    installer_log(LOG_INFO, "  Root partition: %d (%llu MB)", 
                  root_part, (unsigned long long)(root_sectors * disk->sector_size / 1024 / 1024));
    if (swap_part > 0) {
        installer_log(LOG_INFO, "  Swap partition: %d (%llu MB)",
                      swap_part, (unsigned long long)(swap_sectors * disk->sector_size / 1024 / 1024));
    }
    
    return 0;
}
