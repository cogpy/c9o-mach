# packaging/iso/Makefile.am - Build system for ISO images
# Copyright (C) 2024 Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

EXTRA_DIST = \
	build-iso.sh \
	grub.cfg.live.template \
	grub.cfg.install.template

# ISO build configuration
ISO_BUILD_DIR = $(builddir)/iso-build
ISO_OUTPUT_DIR = $(top_builddir)

# Version for ISO naming
ISO_VERSION = $(VERSION)

# GRUB and ISO creation tools
GRUB_MKRESCUE = grub-mkrescue
XORRISO = xorriso

#
# ISO build targets
#

# Default ISO (live boot)
iso: iso-live

# Live boot ISO for i686
iso-live-i686: $(GNUMACH) rootfs.img
	$(srcdir)/build-iso.sh \
		--type live \
		--arch i686 \
		--kernel $(GNUMACH) \
		--rootfs $(builddir)/rootfs.img \
		--output $(ISO_OUTPUT_DIR) \
		--build-dir $(ISO_BUILD_DIR) \
		--version $(ISO_VERSION)

# Live boot ISO for x86_64
iso-live-x86_64: $(GNUMACH) rootfs.img
	$(srcdir)/build-iso.sh \
		--type live \
		--arch x86_64 \
		--kernel $(GNUMACH) \
		--rootfs $(builddir)/rootfs.img \
		--output $(ISO_OUTPUT_DIR) \
		--build-dir $(ISO_BUILD_DIR) \
		--version $(ISO_VERSION)

# Installation ISO for i686
iso-install-i686: $(GNUMACH) rootfs.img installer
	$(srcdir)/build-iso.sh \
		--type install \
		--arch i686 \
		--kernel $(GNUMACH) \
		--rootfs $(builddir)/rootfs.img \
		--installer $(builddir)/../installer/installer \
		--output $(ISO_OUTPUT_DIR) \
		--build-dir $(ISO_BUILD_DIR) \
		--version $(ISO_VERSION)

# Installation ISO for x86_64
iso-install-x86_64: $(GNUMACH) rootfs.img installer
	$(srcdir)/build-iso.sh \
		--type install \
		--arch x86_64 \
		--kernel $(GNUMACH) \
		--rootfs $(builddir)/rootfs.img \
		--installer $(builddir)/../installer/installer \
		--output $(ISO_OUTPUT_DIR) \
		--build-dir $(ISO_BUILD_DIR) \
		--version $(ISO_VERSION)

# Convenience aliases
if HOST_ix86
iso-live: iso-live-i686
iso-install: iso-install-i686
else
iso-live: iso-live-x86_64
iso-install: iso-install-x86_64
endif

# Full distribution (both live and install)
iso-full: iso-live iso-install

# All ISOs for both architectures
iso-all: iso-live-i686 iso-live-x86_64 iso-install-i686 iso-install-x86_64

#
# ISO testing targets
#

QEMU_ISO_OPTS = -m 512 -nographic -no-reboot -boot d
QEMU_BIOS = qemu-system-i386
QEMU_EFI = qemu-system-x86_64

# Test live ISO in QEMU (BIOS mode)
iso-test-bios: iso-live
	@echo "Testing live ISO in QEMU (BIOS mode)..."
	$(QEMU_BIOS) $(QEMU_ISO_OPTS) \
		-cdrom $(ISO_OUTPUT_DIR)/c9o-mach-live-*-$(ISO_VERSION).iso

# Test live ISO in QEMU (EFI mode)
iso-test-efi: iso-live
	@echo "Testing live ISO in QEMU (EFI mode)..."
	$(QEMU_EFI) $(QEMU_ISO_OPTS) \
		-bios /usr/share/OVMF/OVMF_CODE.fd \
		-cdrom $(ISO_OUTPUT_DIR)/c9o-mach-live-*-$(ISO_VERSION).iso

# Quick test alias
iso-test: iso-test-bios

#
# Cleanup
#

clean-iso:
	rm -rf $(ISO_BUILD_DIR)
	rm -f $(ISO_OUTPUT_DIR)/c9o-mach-*.iso
	rm -f $(ISO_OUTPUT_DIR)/c9o-mach-*.iso.sha256

.PHONY: iso iso-live iso-install iso-full iso-all \
	iso-live-i686 iso-live-x86_64 \
	iso-install-i686 iso-install-x86_64 \
	iso-test iso-test-bios iso-test-efi \
	clean-iso
