/*
 * c9o-mach Installer - Bootloader Installation
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

/* GRUB stage1 boot code for MBR (minimal bootstrap) */
static const unsigned char grub_stage1[] = {
    0xEB, 0x63,             /* JMP short to boot code */
    0x90,                   /* NOP */
    'G', 'R', 'U', 'B', ' ', /* OEM name */
    0x00, 0x02,             /* Bytes per sector (512) */
    0x01,                   /* Sectors per cluster */
    0x00, 0x00,             /* Reserved sectors */
    /* ... minimal GRUB stage1 would continue here */
};

/*
 * Detect boot mode (BIOS or UEFI)
 */
int bootloader_detect_mode(void)
{
    /* In a real implementation, this would:
     * 1. Check for EFI system table
     * 2. Check for EFI firmware services
     * 3. Look for /sys/firmware/efi on Linux
     * For now, assume BIOS boot
     */
    
    installer_log(LOG_DEBUG, "Detecting boot mode...");
    
    /* Check if we were booted via EFI */
    /* This is a simplified check - real detection is more complex */
    
    installer_log(LOG_INFO, "Boot mode: BIOS (Legacy)");
    return 0;  /* 0 = BIOS, 1 = UEFI */
}

/*
 * Install GRUB bootloader to disk (MBR)
 */
static int install_grub_bios(disk_info_t *disk)
{
    unsigned char mbr[512];
    
    installer_log(LOG_INFO, "Installing GRUB (BIOS) to %s", disk->device_name);
    
    /* Read current MBR to preserve partition table */
    if (disk_read_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to read MBR from %s", disk->device_name);
        return -1;
    }
    
    /* Install minimal GRUB boot code (first 446 bytes only) */
    /* This preserves the partition table (bytes 446-509) and signature (510-511) */
    
    /* In a real implementation, we would:
     * 1. Install GRUB's boot.img to MBR (first 446 bytes)
     * 2. Install core.img to the post-MBR gap or partition
     * 3. Set up the modules directory
     * 
     * For now, we'll create a minimal bootstrap that chainloads
     */
    
    /* Minimal boot code - just displays message and halts */
    unsigned char boot_code[] = {
        0xFA,               /* CLI - disable interrupts */
        0x31, 0xC0,         /* XOR AX, AX */
        0x8E, 0xD8,         /* MOV DS, AX */
        0x8E, 0xC0,         /* MOV ES, AX */
        0x8E, 0xD0,         /* MOV SS, AX */
        0xBC, 0x00, 0x7C,   /* MOV SP, 0x7C00 */
        0xFB,               /* STI - enable interrupts */
        0xBE, 0x1E, 0x7C,   /* MOV SI, message (offset 0x1E) */
        /* Print loop */
        0xAC,               /* LODSB - load byte from [SI] to AL */
        0x08, 0xC0,         /* OR AL, AL - check for null */
        0x74, 0x09,         /* JZ done */
        0xB4, 0x0E,         /* MOV AH, 0x0E - teletype output */
        0xB7, 0x00,         /* MOV BH, 0 - page 0 */
        0xCD, 0x10,         /* INT 0x10 - BIOS video interrupt */
        0xEB, 0xF2,         /* JMP print_loop */
        /* Done - halt */
        0xF4,               /* HLT */
        0xEB, 0xFD,         /* JMP halt_loop */
        /* Message */
        'c', '9', 'o', '-', 'm', 'a', 'c', 'h', ' ',
        'b', 'o', 'o', 't', 'l', 'o', 'a', 'd', 'e', 'r', '\r', '\n',
        'P', 'l', 'e', 'a', 's', 'e', ' ', 'i', 'n', 's', 't', 'a', 'l', 'l', ' ',
        'G', 'R', 'U', 'B', '.', '\r', '\n', 0
    };
    
    /* Copy boot code (limited to 446 bytes to preserve partition table) */
    size_t code_len = sizeof(boot_code);
    if (code_len > 446) {
        code_len = 446;
    }
    memcpy(mbr, boot_code, code_len);
    
    /* Ensure MBR signature is intact */
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    
    /* Write updated MBR */
    if (disk_write_sectors(disk, 0, 1, mbr) != 0) {
        installer_log(LOG_ERROR, "Failed to write MBR to %s", disk->device_name);
        return -1;
    }
    
    installer_log(LOG_INFO, "Minimal bootloader installed to MBR");
    installer_log(LOG_WARNING, "For full GRUB support, run: grub-install %s", 
                  disk->device_name);
    
    return 0;
}

/*
 * Install GRUB bootloader to disk (UEFI)
 */
static int install_grub_uefi(disk_info_t *disk, const char *esp_mountpoint)
{
    installer_log(LOG_INFO, "Installing GRUB (UEFI) to %s", disk->device_name);
    
    /* In a real implementation, we would:
     * 1. Copy GRUB EFI binary to ESP/EFI/BOOT/BOOTX64.EFI (or BOOTIA32.EFI)
     * 2. Copy GRUB modules to ESP/EFI/c9o-mach/
     * 3. Create grub.cfg in the modules directory
     * 4. Optionally create NVRAM boot entry
     */
    
    if (esp_mountpoint == NULL) {
        installer_log(LOG_ERROR, "No EFI System Partition mountpoint specified");
        return -1;
    }
    
    installer_log(LOG_WARNING, "UEFI GRUB installation not yet implemented");
    installer_log(LOG_INFO, "Please manually install GRUB using: grub-install --target=x86_64-efi");
    
    return 0;
}

/*
 * Install GRUB bootloader
 */
int bootloader_install_grub(disk_info_t *disk, const char *root_mountpoint)
{
    int boot_mode;
    int result;
    
    if (disk == NULL)
        return -1;
    
    /* Detect boot mode */
    boot_mode = bootloader_detect_mode();
    
    if (boot_mode == 1) {
        /* UEFI boot */
        result = install_grub_uefi(disk, NULL);
    } else {
        /* BIOS boot */
        result = install_grub_bios(disk);
    }
    
    if (result == 0) {
        /* Generate configuration */
        result = bootloader_generate_config(root_mountpoint);
    }
    
    return result;
}

/*
 * Generate GRUB configuration file
 */
int bootloader_generate_config(const char *root_mountpoint)
{
    installer_log(LOG_INFO, "Generating GRUB configuration...");
    
    /* The GRUB configuration that would be generated */
    const char *grub_cfg = 
        "# GRUB configuration for c9o-mach\n"
        "# Generated by c9o-mach installer\n"
        "\n"
        "set timeout=5\n"
        "set default=0\n"
        "\n"
        "menuentry \"c9o-mach\" {\n"
        "    multiboot /boot/gnumach root=device:hd0s1\n"
        "    module /hurd/ext2fs.static ext2fs \\\n"
        "        --multiboot-command-line='${kernel-command-line}' \\\n"
        "        --host-priv-port='${host-port}' \\\n"
        "        --device-master-port='${device-port}' \\\n"
        "        --exec-server-task='${exec-task}' \\\n"
        "        -T typed '${root}' '$(task-create)' '$(task-resume)'\n"
        "    module /lib/ld.so.1 exec /hurd/exec '$(exec-task=task-create)'\n"
        "    boot\n"
        "}\n"
        "\n"
        "menuentry \"c9o-mach (Single User)\" {\n"
        "    multiboot /boot/gnumach root=device:hd0s1 -s\n"
        "    module /hurd/ext2fs.static ext2fs \\\n"
        "        --multiboot-command-line='${kernel-command-line}' \\\n"
        "        --host-priv-port='${host-port}' \\\n"
        "        --device-master-port='${device-port}' \\\n"
        "        --exec-server-task='${exec-task}' \\\n"
        "        -T typed '${root}' '$(task-create)' '$(task-resume)'\n"
        "    module /lib/ld.so.1 exec /hurd/exec '$(exec-task=task-create)'\n"
        "    boot\n"
        "}\n";
    
    /* In a real implementation, we would:
     * 1. Detect the root partition UUID
     * 2. Find the kernel and Hurd servers
     * 3. Write the configuration to /boot/grub/grub.cfg
     */
    
    installer_log(LOG_INFO, "GRUB configuration would be written to /boot/grub/grub.cfg");
    installer_log(LOG_DEBUG, "Config:\n%s", grub_cfg);
    
    return 0;
}

/*
 * Update existing GRUB configuration
 */
int bootloader_update_config(const char *root_mountpoint)
{
    installer_log(LOG_INFO, "Updating GRUB configuration...");
    
    /* In a real implementation, this would:
     * 1. Re-scan for kernels
     * 2. Re-generate grub.cfg
     * 3. Equivalent to running 'update-grub'
     */
    
    return bootloader_generate_config(root_mountpoint);
}
