/*
 * c9o-mach Installer - Disk Operations
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

#include <device/device_types.h>

/* Device master port for disk access */
static mach_port_t device_master = MACH_PORT_NULL;

/* List of discovered disks */
static disk_info_t *disk_list = NULL;

/* Descriptor pools, the installer runs without a heap */
static disk_info_t disk_pool[INSTALLER_MAX_DISKS];
static int disk_pool_used = 0;
static partition_info_t partition_pool[INSTALLER_MAX_PARTITIONS];
static int partition_pool_used = 0;

static disk_info_t *disk_alloc(void)
{
    if (disk_pool_used >= INSTALLER_MAX_DISKS)
        return NULL;
    
    return &disk_pool[disk_pool_used++];
}

static partition_info_t *partition_alloc(void)
{
    if (partition_pool_used >= INSTALLER_MAX_PARTITIONS)
        return NULL;
    
    return &partition_pool[partition_pool_used++];
}

/*
 * Known device prefixes for disk enumeration
 */
static const char *disk_prefixes[] = {
    "hd",    /* IDE/ATA disks */
    "sd",    /* SCSI disks */
    "wd",    /* IDE disks (alternate) */
    "vd",    /* Virtio disks */
    "nvme",  /* NVMe disks */
    NULL
};

/*
 * Initialize disk subsystem
 */
int disk_init(void)
{
    installer_log(LOG_INFO, "Initializing disk subsystem...");
    
    /* The device master port is handed to the bootstrap task by the
       kernel boot script, see tests/testlib.c.  */
    device_master = device_priv();
    if (device_master == MACH_PORT_NULL) {
        installer_log(LOG_ERROR, "No device master port available");
        return -1;
    }
    
    installer_log(LOG_INFO, "Disk subsystem initialized");
    return 0;
}

/*
 * Cleanup disk subsystem
 */
void disk_cleanup(void)
{
    if (disk_list != NULL) {
        disk_free_list(disk_list);
        disk_list = NULL;
    }
    
    device_master = MACH_PORT_NULL;
}

/*
 * Try to open a device and get basic information
 */
static disk_info_t *probe_disk_device(const char *device_name)
{
    kern_return_t kr;
    mach_port_t device_port;
    unsigned int count;
    int dev_status[DEV_STATUS_MAX];
    
    /* Try to open the device */
    kr = device_open(device_master, D_READ, (char *)device_name, &device_port);
    if (kr != KERN_SUCCESS) {
        return NULL;  /* Device doesn't exist or can't be opened */
    }
    
    /* Allocate disk info structure */
    disk_info_t *disk = disk_alloc();
    if (disk == NULL) {
        device_close(device_port);
        return NULL;
    }
    memset(disk, 0, sizeof(disk_info_t));
    
    /* Copy device name */
    strncpy(disk->device_name, device_name, sizeof(disk->device_name) - 1);
    
    /* Get device status to determine size */
    count = DEV_STATUS_MAX;
    kr = device_get_status(device_port, DEV_GET_SIZE, dev_status, &count);
    if (kr == KERN_SUCCESS && count >= 2) {
        disk->total_sectors = dev_status[DEV_GET_SIZE_COUNT];
        disk->sector_size = dev_status[DEV_GET_SIZE_RECORD_SIZE];
        if (disk->sector_size == 0)
            disk->sector_size = 512;  /* Default sector size */
        disk->size_bytes = disk->total_sectors * disk->sector_size;
    } else {
        /* Assume defaults */
        disk->sector_size = 512;
        disk->total_sectors = 0;
        disk->size_bytes = 0;
    }
    
    /* Determine disk type from device name */
    if (strncmp(device_name, "hd", 2) == 0 || strncmp(device_name, "wd", 2) == 0) {
        disk->type = DISK_TYPE_ATA;
    } else if (strncmp(device_name, "sd", 2) == 0) {
        disk->type = DISK_TYPE_SCSI;
    } else if (strncmp(device_name, "vd", 2) == 0) {
        disk->type = DISK_TYPE_VIRTIO;
    } else if (strncmp(device_name, "nvme", 4) == 0) {
        disk->type = DISK_TYPE_NVME;
    } else {
        disk->type = DISK_TYPE_UNKNOWN;
    }
    
    /* Set model string */
    snprintf(disk->model, sizeof(disk->model), "%s disk", 
             disk_type_to_string(disk->type));
    
    /* Close device port */
    device_close(device_port);
    
    installer_log(LOG_DEBUG, "Found disk: %s (%s, %llu bytes)",
                  disk->device_name, disk->model, 
                  (unsigned long long)disk->size_bytes);
    
    return disk;
}

/*
 * Enumerate all available disks
 */
disk_info_t *disk_enumerate(void)
{
    disk_info_t *head = NULL;
    disk_info_t *tail = NULL;
    char device_name[64];
    int i, j;
    
    installer_log(LOG_INFO, "Enumerating disk devices...");
    
    /* Any previously returned list becomes invalid */
    disk_list = NULL;
    disk_pool_used = 0;
    memset(disk_pool, 0, sizeof(disk_pool));
    
    /* Try each prefix with numbers 0-7 */
    for (i = 0; disk_prefixes[i] != NULL; i++) {
        for (j = 0; j < 8; j++) {
            snprintf(device_name, sizeof(device_name), "%s%d", 
                     disk_prefixes[i], j);
            
            disk_info_t *disk = probe_disk_device(device_name);
            if (disk != NULL) {
                /* Add to list */
                if (head == NULL) {
                    head = disk;
                    tail = disk;
                } else {
                    tail->next = disk;
                    tail = disk;
                }
            }
        }
    }
    
    /* Cache the list */
    disk_list = head;
    
    /* Count disks found */
    int count = 0;
    for (disk_info_t *d = head; d != NULL; d = d->next) {
        count++;
    }
    installer_log(LOG_INFO, "Found %d disk(s)", count);
    
    return head;
}

/*
 * Free a disk info list
 */
void disk_free_list(disk_info_t *list)
{
    if (list == NULL)
        return;
    
    /* The descriptors come from a pool that is only reset as a whole,
       so this must only be called for a complete list.  */
    disk_pool_used = 0;
    memset(disk_pool, 0, sizeof(disk_pool));
}

/*
 * Get partitions for a disk
 */
partition_info_t *disk_get_partitions(disk_info_t *disk)
{
    partition_info_t *head = NULL;
    partition_info_t *tail = NULL;
    kern_return_t kr;
    mach_port_t device_port;
    unsigned char mbr[512];
    
    if (disk == NULL)
        return NULL;
    
    /* Any previously returned partition list becomes invalid */
    partition_pool_used = 0;
    memset(partition_pool, 0, sizeof(partition_pool));
    
    /* Open disk device */
    kr = device_open(device_master, D_READ, disk->device_name, &device_port);
    if (kr != KERN_SUCCESS) {
        installer_log(LOG_ERROR, "Failed to open disk %s for partition scan",
                      disk->device_name);
        return NULL;
    }
    
    /* Read MBR */
    io_buf_ptr_t data;
    unsigned int data_count;
    
    kr = device_read(device_port, 0, 0, 512, &data, &data_count);
    if (kr != KERN_SUCCESS || data_count < 512) {
        installer_log(LOG_ERROR, "Failed to read MBR from %s", disk->device_name);
        device_close(device_port);
        return NULL;
    }
    
    memcpy(mbr, data, 512);
    vm_deallocate(mach_task_self(), (vm_address_t)data, data_count);
    
    /* Check for MBR signature */
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        installer_log(LOG_DEBUG, "No MBR signature found on %s", disk->device_name);
        disk->ptable_type = PTABLE_TYPE_MBR;  /* Assume MBR for new disks */
        device_close(device_port);
        return NULL;
    }
    
    disk->ptable_type = PTABLE_TYPE_MBR;
    
    /* Parse MBR partition table (4 primary partitions at offset 446) */
    for (int i = 0; i < 4; i++) {
        unsigned char *entry = &mbr[446 + i * 16];
        uint8_t type = entry[4];
        
        if (type == PART_TYPE_EMPTY)
            continue;
        
        partition_info_t *part = partition_alloc();
        if (part == NULL)
            continue;
        memset(part, 0, sizeof(partition_info_t));
        
        part->number = i + 1;
        part->type = (partition_type_t)type;
        part->bootable = (entry[0] == 0x80);
        
        /* Extract LBA start and size */
        part->start_sector = entry[8] | (entry[9] << 8) | 
                            (entry[10] << 16) | (entry[11] << 24);
        uint32_t sectors = entry[12] | (entry[13] << 8) | 
                          (entry[14] << 16) | (entry[15] << 24);
        part->end_sector = part->start_sector + sectors - 1;
        part->size_bytes = (uint64_t)sectors * disk->sector_size;
        
        /* Determine filesystem type from partition type */
        switch (part->type) {
            case PART_TYPE_LINUX:
            case PART_TYPE_MACH:
            case PART_TYPE_HURD:
                part->fs_type = FS_TYPE_EXT2;  /* Assume ext2 */
                break;
            case PART_TYPE_FAT32:
            case PART_TYPE_FAT32_LBA:
            case PART_TYPE_EFI:
                part->fs_type = FS_TYPE_FAT32;
                break;
            case PART_TYPE_LINUX_SWAP:
                part->fs_type = FS_TYPE_SWAP;
                break;
            default:
                part->fs_type = FS_TYPE_NONE;
                break;
        }
        
        /* Add to list */
        if (head == NULL) {
            head = part;
            tail = part;
        } else {
            tail->next = part;
            tail = part;
        }
        
        installer_log(LOG_DEBUG, "  Partition %d: type=0x%02X, start=%llu, size=%llu",
                      part->number, part->type,
                      (unsigned long long)part->start_sector,
                      (unsigned long long)part->size_bytes);
    }
    
    device_close(device_port);
    
    /* Update partition count */
    disk->partition_count = 0;
    for (partition_info_t *p = head; p != NULL; p = p->next) {
        disk->partition_count++;
    }
    
    return head;
}

/*
 * Free a partition list
 */
void partition_free_list(partition_info_t *list)
{
    if (list == NULL)
        return;
    
    partition_pool_used = 0;
    memset(partition_pool, 0, sizeof(partition_pool));
}

/*
 * Read sectors from disk
 */
int disk_read_sectors(disk_info_t *disk, uint64_t start, uint64_t count, void *buffer)
{
    kern_return_t kr;
    mach_port_t device_port;
    
    if (disk == NULL || buffer == NULL || count == 0)
        return -1;
    
    kr = device_open(device_master, D_READ, disk->device_name, &device_port);
    if (kr != KERN_SUCCESS)
        return -1;
    
    io_buf_ptr_t data;
    unsigned int data_count;
    uint64_t bytes = count * disk->sector_size;
    
    kr = device_read(device_port, 0, (recnum_t)start, (int)bytes, &data, &data_count);
    if (kr == KERN_SUCCESS) {
        memcpy(buffer, data, data_count);
        vm_deallocate(mach_task_self(), (vm_address_t)data, data_count);
    }
    
    device_close(device_port);
    return (kr == KERN_SUCCESS) ? 0 : -1;
}

/*
 * Write sectors to disk
 */
int disk_write_sectors(disk_info_t *disk, uint64_t start, uint64_t count, const void *buffer)
{
    kern_return_t kr;
    mach_port_t device_port;
    
    if (disk == NULL || buffer == NULL || count == 0)
        return -1;
    
    kr = device_open(device_master, D_READ | D_WRITE, disk->device_name, &device_port);
    if (kr != KERN_SUCCESS)
        return -1;
    
    int bytes_written;
    uint64_t bytes = count * disk->sector_size;
    
    kr = device_write(device_port, 0, (recnum_t)start, (io_buf_ptr_t)buffer, 
                      (unsigned int)bytes, &bytes_written);
    
    device_close(device_port);
    return (kr == KERN_SUCCESS && bytes_written == bytes) ? 0 : -1;
}

/*
 * Convert disk type to string
 */
const char *disk_type_to_string(disk_type_t type)
{
    switch (type) {
        case DISK_TYPE_ATA:     return "ATA/IDE";
        case DISK_TYPE_AHCI:    return "AHCI/SATA";
        case DISK_TYPE_SCSI:    return "SCSI";
        case DISK_TYPE_VIRTIO:  return "VirtIO";
        case DISK_TYPE_NVME:    return "NVMe";
        default:                return "Unknown";
    }
}

/*
 * Convert partition type to string
 */
const char *partition_type_to_string(partition_type_t type)
{
    switch (type) {
        case PART_TYPE_EMPTY:       return "Empty";
        case PART_TYPE_FAT32:       return "FAT32";
        case PART_TYPE_FAT32_LBA:   return "FAT32 LBA";
        case PART_TYPE_LINUX:       return "Linux";
        case PART_TYPE_LINUX_SWAP:  return "Linux Swap";
        case PART_TYPE_EFI:         return "EFI System";
        case PART_TYPE_MACH:        return "GNU Mach";
        case PART_TYPE_HURD:        return "GNU Hurd";
        default:                    return "Unknown";
    }
}
