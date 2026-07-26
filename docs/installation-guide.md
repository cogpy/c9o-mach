# c9o-mach Installation Guide

This guide walks you through installing c9o-mach (GNU Mach microkernel) on your computer.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Obtaining Installation Media](#obtaining-installation-media)
3. [Booting the Installer](#booting-the-installer)
4. [Installation Process](#installation-process)
5. [Post-Installation](#post-installation)
6. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| CPU | i686 (Pentium II) or x86_64 compatible |
| RAM | 128 MB minimum, 512 MB recommended |
| Storage | 512 MB minimum, 2 GB recommended |
| Boot | BIOS or UEFI |

### Supported Hardware

- **CPUs**: Intel Pentium II and later, AMD K6 and later
- **Storage**: IDE, SATA (AHCI), SCSI, VirtIO
- **Network**: Most common Ethernet adapters (Intel e1000, RTL8139, VirtIO-net)

## Obtaining Installation Media

### Download ISO Images

Official ISO images are available from the releases page:

```bash
# Live boot ISO (for trying without installation)
wget https://github.com/cogpy/c9o-mach/releases/latest/download/c9o-mach-live-i686.iso

# Installation ISO
wget https://github.com/cogpy/c9o-mach/releases/latest/download/c9o-mach-install-i686.iso
```

### Verify Download

```bash
# Verify checksum
sha256sum -c c9o-mach-install-i686.iso.sha256
```

### Create Bootable USB

Using `dd`:
```bash
sudo dd if=c9o-mach-install-i686.iso of=/dev/sdX bs=4M status=progress
sync
```

Using `cp` (ISO is hybrid-bootable):
```bash
sudo cp c9o-mach-install-i686.iso /dev/sdX
sync
```

## Booting the Installer

### BIOS Boot

1. Insert the installation media (USB or CD)
2. Restart your computer
3. Press the boot menu key (usually F12, F2, or Del)
4. Select the installation media
5. Choose "Install c9o-mach" from the GRUB menu

### UEFI Boot

1. Insert the installation media
2. Access UEFI settings (usually F2 or Del at boot)
3. Ensure Secure Boot is disabled
4. Select the installation media from boot options
5. Choose "Install c9o-mach" from the GRUB menu

### QEMU/Virtual Machine

```bash
# BIOS mode
qemu-system-i386 -m 512 -cdrom c9o-mach-install-i686.iso -boot d

# UEFI mode (requires OVMF)
qemu-system-x86_64 -m 512 -cdrom c9o-mach-install-x86_64.iso \
    -bios /usr/share/OVMF/OVMF_CODE.fd -boot d
```

## Installation Process

### Step 1: Welcome Screen

The installer will display a welcome message explaining what c9o-mach is and what will happen during installation.

**Options:**
- Continue with installation
- Exit to live environment

### Step 2: Disk Selection

The installer will show all available disks:

```
Available disks:

  Device     Model                Size            Type
  -------------------------------------------------------------------------
  hd0        QEMU HARDDISK        8.0 GB          ATA/IDE
  hd1        USB Storage          16.0 GB         SCSI
```

**⚠️ Warning:** All data on the selected disk will be erased!

### Step 3: Partition Configuration

Choose a partitioning method:

1. **Use entire disk (recommended)** - Automatically creates optimal layout
2. **Manual partitioning** - For advanced users

**Automatic layout creates:**
- Root partition (ext2, uses remaining space)
- Swap partition (512 MB by default)

### Step 4: Confirmation

Review your settings before proceeding:

```
Target disk:        hd0
Disk size:          8.0 GB
Partition table:    MBR
Root filesystem:    ext2
Swap partition:     512 MB

WARNING: This will ERASE ALL DATA on hd0!
```

### Step 5: Installation

The installer will:
1. Create partitions
2. Format filesystems
3. Copy system files
4. Install bootloader

Progress will be displayed:

```
[========================>                         ] 50% Creating filesystem...
```

### Step 6: Completion

Upon successful installation:

```
Installation Complete!

To boot into your new system:
  1. Remove the installation media
  2. Reboot your computer
  3. Select c9o-mach from the boot menu
```

## Post-Installation

### First Boot

After installation, your system will boot to a minimal environment. The kernel will load and display:

```
GNU Mach 1.8+git...
Copyright (C) 2024 Free Software Foundation, Inc.

c9o-mach ready.
```

### Setting Up Hurd

c9o-mach is a microkernel. For a complete operating system, you need the GNU Hurd servers:

```bash
# From a running Hurd system or cross-compilation environment
./native-install /mnt/target
```

See the [Hurd installation guide](https://www.gnu.org/software/hurd/hurd/running.html) for details.

### Boot Options

Edit `/boot/grub/grub.cfg` to customize boot options:

```
# Debug mode with verbose output
multiboot /boot/gnumach console=com0 debug verbose

# Single user mode
multiboot /boot/gnumach root=device:hd0s1 -s
```

## Troubleshooting

### Boot Issues

**Problem:** System doesn't boot from installation media
- Solution: Check BIOS boot order, disable Secure Boot for UEFI

**Problem:** GRUB shows "error: unknown filesystem"
- Solution: The ISO might be corrupted. Re-download and verify checksum.

**Problem:** Kernel panic during boot
- Solution: Try adding `noacpi` or `nomodeset` to kernel parameters

### Installation Issues

**Problem:** No disks found
- Solution: Your disk controller might not be supported. Try using VirtIO in VM.

**Problem:** Installation fails at partition creation
- Solution: The disk might have existing partitions. Use `dd if=/dev/zero of=/dev/sdX bs=512 count=1` to clear MBR (WARNING: destroys all data).

**Problem:** Bootloader installation fails
- Solution: Ensure GRUB packages are installed: `apt install grub-pc-bin`

### Getting Help

- GitHub Issues: https://github.com/cogpy/c9o-mach/issues
- IRC: #hurd on Libera.Chat
- Mailing list: bug-hurd@gnu.org

## Advanced Topics

### Custom Kernel Parameters

| Parameter | Description |
|-----------|-------------|
| `console=com0` | Use serial console |
| `debug` | Enable debug output |
| `verbose` | Verbose boot messages |
| `console_timestamps=on` | Add timestamps to logs |
| `single` or `-s` | Single user mode |
| `root=device:hdXsY` | Specify root device |

### Manual Installation

For advanced users who want full control:

```bash
# Boot live environment
# Partition disk manually
fdisk /dev/hd0

# Create filesystem
mke2fs /dev/hd0s1

# Mount and install
mount /dev/hd0s1 /mnt/target
cp -a /boot/gnumach /mnt/target/boot/

# Install GRUB
grub-install --boot-directory=/mnt/target/boot /dev/hd0
```

### Dual Boot

To dual boot with another operating system:

1. Use manual partitioning to preserve existing partitions
2. Install c9o-mach to an empty partition
3. Update your existing bootloader to include c9o-mach

Example GRUB entry for existing bootloader:
```
menuentry "c9o-mach" {
    set root=(hd0,1)
    multiboot /boot/gnumach root=device:hd0s1
    boot
}
```

---

*For the latest documentation, visit: https://github.com/cogpy/c9o-mach/docs*
