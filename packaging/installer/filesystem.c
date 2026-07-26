/*
 * c9o-mach Installer - Filesystem Operations
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

/* Ext2 filesystem constants */
#define EXT2_SUPER_MAGIC        0xEF53
#define EXT2_SUPERBLOCK_OFFSET  1024
#define EXT2_SUPERBLOCK_SIZE    1024
#define EXT2_BLOCK_SIZE_MIN     1024
#define EXT2_BLOCK_SIZE_DEFAULT 4096

/* FAT32 filesystem constants */
#define FAT32_SIGNATURE         0xAA55
#define FAT32_BOOT_SIGNATURE    "FAT32   "

/*
 * Ext2 superblock structure (minimal for creation)
 */
struct ext2_super_block {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    /* Extended superblock fields */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    /* More fields exist but we don't need them for basic creation */
} __attribute__((packed));

/*
 * Create an ext2 filesystem on a partition
 */
static int fs_create_ext2(disk_info_t *disk, partition_info_t *part, const char *label)
{
    struct ext2_super_block sb;
    uint64_t partition_bytes;
    uint64_t partition_blocks;
    uint32_t block_size = EXT2_BLOCK_SIZE_DEFAULT;
    uint32_t blocks_per_group = 8 * block_size;  /* bits in a block */
    uint32_t inodes_per_group;
    uint32_t num_groups;
    
    if (disk == NULL || part == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Creating ext2 filesystem on %s partition %d",
                  disk->device_name, part->number);
    
    /* Calculate partition size */
    partition_bytes = part->size_bytes;
    partition_blocks = partition_bytes / block_size;
    
    /* Calculate number of block groups */
    num_groups = (partition_blocks + blocks_per_group - 1) / blocks_per_group;
    if (num_groups == 0)
        num_groups = 1;
    
    /* Calculate inodes per group (roughly 1 inode per 4 blocks) */
    inodes_per_group = blocks_per_group / 4;
    if (inodes_per_group < 16)
        inodes_per_group = 16;
    
    /* Initialize superblock */
    memset(&sb, 0, sizeof(sb));
    
    sb.s_inodes_count = num_groups * inodes_per_group;
    sb.s_blocks_count = partition_blocks;
    sb.s_r_blocks_count = partition_blocks / 20;  /* 5% reserved */
    sb.s_free_blocks_count = partition_blocks - num_groups * 2;  /* Rough estimate */
    sb.s_free_inodes_count = sb.s_inodes_count - 11;  /* Reserved inodes */
    sb.s_first_data_block = (block_size == 1024) ? 1 : 0;
    sb.s_log_block_size = (block_size == 1024) ? 0 : 
                          (block_size == 2048) ? 1 : 2;
    sb.s_log_frag_size = sb.s_log_block_size;
    sb.s_blocks_per_group = blocks_per_group;
    sb.s_frags_per_group = blocks_per_group;
    sb.s_inodes_per_group = inodes_per_group;
    sb.s_mtime = 0;  /* Will be set on mount */
    sb.s_wtime = 0;  /* Current time - set later */
    sb.s_mnt_count = 0;
    sb.s_max_mnt_count = 20;
    sb.s_magic = EXT2_SUPER_MAGIC;
    sb.s_state = 1;  /* Clean */
    sb.s_errors = 1;  /* Continue on errors */
    sb.s_minor_rev_level = 0;
    sb.s_lastcheck = 0;
    sb.s_checkinterval = 0;
    sb.s_creator_os = 0;  /* Linux */
    sb.s_rev_level = 1;  /* Dynamic revision */
    sb.s_def_resuid = 0;
    sb.s_def_resgid = 0;
    sb.s_first_ino = 11;  /* First non-reserved inode */
    sb.s_inode_size = 128;
    sb.s_block_group_nr = 0;
    sb.s_feature_compat = 0;
    sb.s_feature_incompat = 0;
    sb.s_feature_ro_compat = 0;
    
    /* Generate a simple UUID */
    for (int i = 0; i < 16; i++) {
        sb.s_uuid[i] = (uint8_t)(i + part->start_sector + disk->device_name[0]);
    }
    
    /* Set volume label */
    if (label != NULL) {
        strncpy(sb.s_volume_name, label, sizeof(sb.s_volume_name) - 1);
    } else {
        strncpy(sb.s_volume_name, "c9o-mach", sizeof(sb.s_volume_name) - 1);
    }
    
    /* Write superblock at offset 1024 bytes (sector 2 for 512-byte sectors) */
    uint64_t sb_sector = part->start_sector + (EXT2_SUPERBLOCK_OFFSET / disk->sector_size);
    
    /* We need to write 1024 bytes, which is 2 sectors */
    unsigned char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &sb, sizeof(sb));
    
    if (disk_write_sectors(disk, sb_sector, 2, buffer) != 0) {
        installer_log(LOG_ERROR, "Failed to write ext2 superblock");
        return -1;
    }
    
    /* Note: A complete ext2 filesystem would also need:
     * - Block group descriptors
     * - Block bitmaps
     * - Inode bitmaps
     * - Inode tables
     * - Root directory
     * For a production system, use mke2fs instead.
     */
    
    installer_log(LOG_INFO, "Ext2 superblock written (use mke2fs for complete filesystem)");
    installer_log(LOG_WARNING, "Note: This creates a minimal ext2 structure. "
                  "Run mke2fs for a complete filesystem.");
    
    return 0;
}

/*
 * Create a swap area on a partition
 */
static int fs_create_swap(disk_info_t *disk, partition_info_t *part, const char *label)
{
    unsigned char header[4096];
    uint64_t partition_pages;
    
    if (disk == NULL || part == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Creating swap area on %s partition %d",
                  disk->device_name, part->number);
    
    /* Initialize header */
    memset(header, 0, sizeof(header));
    
    /* Calculate number of pages (4KB each) */
    partition_pages = part->size_bytes / 4096;
    
    /* Linux swap signature at end of first page */
    memcpy(&header[4096 - 10], "SWAPSPACE2", 10);
    
    /* Swap header structure (simplified) */
    /* version at offset 0 */
    header[0] = 1;  /* Version 1 */
    header[1] = 0;
    header[2] = 0;
    header[3] = 0;
    
    /* last_page at offset 4 */
    uint32_t last_page = partition_pages - 1;
    header[4] = (last_page >> 0) & 0xFF;
    header[5] = (last_page >> 8) & 0xFF;
    header[6] = (last_page >> 16) & 0xFF;
    header[7] = (last_page >> 24) & 0xFF;
    
    /* nr_badpages at offset 8 (0) */
    /* uuid at offset 12 (16 bytes) */
    for (int i = 0; i < 16; i++) {
        header[12 + i] = (uint8_t)(i + part->start_sector);
    }
    
    /* label at offset 28 (16 bytes) */
    if (label != NULL) {
        strncpy((char *)&header[28], label, 15);
    }
    
    /* Write header (first page) */
    uint64_t header_sectors = 4096 / disk->sector_size;
    if (disk_write_sectors(disk, part->start_sector, header_sectors, header) != 0) {
        installer_log(LOG_ERROR, "Failed to write swap header");
        return -1;
    }
    
    installer_log(LOG_INFO, "Swap area created (%llu pages)", 
                  (unsigned long long)partition_pages);
    return 0;
}

/*
 * Create a filesystem on a partition
 */
int fs_create(disk_info_t *disk, int partition_num, filesystem_type_t type, 
              const char *label)
{
    partition_info_t *partitions;
    partition_info_t *target = NULL;
    int result;
    
    if (disk == NULL || partition_num < 1)
        return -1;
    
    /* Get partition list */
    partitions = disk_get_partitions(disk);
    if (partitions == NULL) {
        installer_log(LOG_ERROR, "Failed to get partition list");
        return -1;
    }
    
    /* Find the target partition */
    for (partition_info_t *p = partitions; p != NULL; p = p->next) {
        if (p->number == partition_num) {
            target = p;
            break;
        }
    }
    
    if (target == NULL) {
        installer_log(LOG_ERROR, "Partition %d not found on %s",
                      partition_num, disk->device_name);
        partition_free_list(partitions);
        return -1;
    }
    
    /* Create filesystem based on type */
    switch (type) {
        case FS_TYPE_EXT2:
        case FS_TYPE_EXT3:
        case FS_TYPE_EXT4:
            result = fs_create_ext2(disk, target, label);
            break;
            
        case FS_TYPE_SWAP:
            result = fs_create_swap(disk, target, label);
            break;
            
        case FS_TYPE_FAT32:
            installer_log(LOG_ERROR, "FAT32 filesystem creation not yet implemented");
            result = -1;
            break;
            
        default:
            installer_log(LOG_ERROR, "Unknown filesystem type: %d", type);
            result = -1;
            break;
    }
    
    partition_free_list(partitions);
    return result;
}

/*
 * Mount a filesystem (stub - mounting would be done by Hurd servers)
 */
int fs_mount(disk_info_t *disk, int partition_num, const char *mountpoint)
{
    if (disk == NULL || mountpoint == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Mount request: %s partition %d -> %s",
                  disk->device_name, partition_num, mountpoint);
    
    /* In a full implementation, this would:
     * 1. Create/start the appropriate translator
     * 2. Set up the filesystem server
     * 3. Bind to the mountpoint
     */
    
    installer_log(LOG_WARNING, "Filesystem mounting requires Hurd translators");
    return 0;
}

/*
 * Unmount a filesystem (stub)
 */
int fs_unmount(const char *mountpoint)
{
    if (mountpoint == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Unmount request: %s", mountpoint);
    return 0;
}

/*
 * Copy a single file (stub - would need filesystem support)
 */
int fs_copy_file(const char *src, const char *dst)
{
    if (src == NULL || dst == NULL)
        return -1;
    
    installer_log(LOG_DEBUG, "Copy file: %s -> %s", src, dst);
    
    /* This would require:
     * 1. Mounting source filesystem
     * 2. Reading file contents
     * 3. Writing to destination
     */
    
    return 0;
}

/*
 * Recursively copy directory contents (stub)
 */
int fs_copy_recursive(const char *src, const char *dst)
{
    if (src == NULL || dst == NULL)
        return -1;
    
    installer_log(LOG_INFO, "Recursive copy: %s -> %s", src, dst);
    return 0;
}

/*
 * Convert filesystem type to string
 */
const char *fs_type_to_string(filesystem_type_t type)
{
    switch (type) {
        case FS_TYPE_NONE:  return "None";
        case FS_TYPE_EXT2:  return "ext2";
        case FS_TYPE_EXT3:  return "ext3";
        case FS_TYPE_EXT4:  return "ext4";
        case FS_TYPE_FAT32: return "FAT32";
        case FS_TYPE_SWAP:  return "swap";
        default:            return "Unknown";
    }
}
