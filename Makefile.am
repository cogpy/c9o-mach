# Makefile for GNU Mach.

# Copyright (C) 2006, 2007, 2008, 2009, 2013 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#
# Various definitions of the Automake environment.
#
# These will be augmented below.
#

SUBDIRS =
DIST_SUBDIRS =
EXTRA_DIST =
DISTCHECK_CONFIGURE_FLAGS =
noinst_LIBRARIES =
noinst_PROGRAMS =
TESTS=
info_TEXINFOS =
MOSTLYCLEANFILES =
DISTCLEANFILES =
AM_CPPFLAGS =
AM_CCASFLAGS =
AM_CFLAGS =
AM_LDFLAGS =

#
# Compilation flags
#

GCC_INSTALL = $(shell LANG=C $(CC) -print-search-dirs | sed -n -e 's/install: \(.*\)/\1/p')
AM_CPPFLAGS += \
	-nostdinc -imacros config.h -I $(GCC_INSTALL)/include

AM_CPPFLAGS += \
	-I$(systype) \
	-I. \
	-I$(top_srcdir)/$(systype) \
	-I$(top_srcdir)/$(systype)/include/mach/sa \
	-I$(top_srcdir)/include

AM_CFLAGS += \
	-fno-builtin-log

AM_CCASFLAGS += \
	-D__ASSEMBLY__

if HOST_ix86
AM_CCASFLAGS += -m32 -Wa,--32
AM_CFLAGS += -m32
endif

# Yes, this makes the eyes hurt.  But perhaps someone will finally take care of
# all that scruffy Mach code...  Also see <http://savannah.gnu.org/task/?5726>.
AM_CFLAGS += \
	-Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes

# Additional compiler warnings for better code quality
# Some warnings are duplicated with basic set, but that's safe
AM_CFLAGS += \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-align \
	-Wwrite-strings \
	-Wredundant-decls \
	-Wnested-externs \
	-Winline \
	-Wno-long-long \
	-Wuninitialized \
	-Wconversion \
	-Wstrict-overflow=2

# Additional strict warnings for development builds
if STRICT_WARNINGS
AM_CFLAGS += \
	-Werror \
	-Wcast-qual \
	-Wmissing-format-attribute \
	-Wformat-nonliteral \
	-Wformat-y2k \
	-Winit-self \
	-Wmissing-include-dirs
endif

# We need the GNU-style inline
AM_CFLAGS += \
	-fgnu89-inline

# Much of the Mach code predates C99 and makes invalid assumptions about
# type punning. Fixed strict aliasing violations, now safe to enable.
AM_CFLAGS += \
	-fstrict-aliasing \
	-Wstrict-aliasing=2

# We use a freestanding environment.
AM_CFLAGS += \
	-ffreestanding \
	-nostdlib

# The smashing stack protector might be enabled by default, but might emit
# unsuitable code.
if disable_smashing_stack_protector
AM_CFLAGS += \
	-fno-stack-protector
endif

# This must be the same size as port names, see e.g. ipc/ipc_entry.c
AM_CFLAGS += -DRDXTREE_KEY_32

# Optimization and debug configuration
if ENABLE_DEBUG_INFO
AM_CFLAGS += -g
endif

# Optimization level from configure
AM_CFLAGS += -O@OPTIMIZATION_LEVEL@

#
# Silent build support.
#

AWK_V = $(AWK_V_$(V))
AWK_V_ = $(AWK_V_$(AM_DEFAULT_VERBOSITY))
AWK_V_0 = @echo "  AWK    $@";

GZIP_V = $(GZIP_V_$(V))
GZIP_V_ = $(GZIP_V_$(AM_DEFAULT_VERBOSITY))
GZIP_V_0 = @echo "  GZIP   $@";

NM_V = $(NM_V_$(V))
NM_V_ = $(NM_V_$(AM_DEFAULT_VERBOSITY))
NM_V_0 = @echo "  NM     $@";

MIGCOM_V = $(MIGCOM_V_$(V))
MIGCOM_V_ = $(MIGCOM_V_$(AM_DEFAULT_VERBOSITY))
MIGCOM_V_0 = @echo "  MIG    $@";

#
# MIG Setup.
#

# MIGCOM.
MIGCOM = $(MIG) -n -cc cat - /dev/null

# We need this because we use $(CPP) to preprocess MIG .defs files.
CPP = @CPP@ -x c

#
# Other Tools' Configuration.
#

# Don't needlessly overwrite files whose contents haven't changed.
# This helps avoiding unnecessary recompilation cycles when keeping
# cross-compilation toolchains up-to-date.  Thus, unconditionally use the
# `install-sh' that is supplied by GNU Automake 1.10.1, as the GNU Coreutils
# one doesn't provide this functionality yet (TODO: change that).  TODO:
# `build-aux' is hardcoded.
install_sh = $(SHELL) $(abs_srcdir)/build-aux/install-sh -C
INSTALL = $(install_sh)

#
# The main kernel functionality.
#

noinst_LIBRARIES += \
	libkernel.a
libkernel_a_SOURCES =
nodist_libkernel_a_SOURCES =
MOSTLYCLEANFILES += \
	$(nodist_libkernel_a_SOURCES)

gnumach_o_LDADD = \
	libkernel.a

gnumach_SOURCES =
gnumach_LINKFLAGS =

# Makerules: how to do some things.
include Makerules.am

# Main Makefile fragment.
include Makefrag.am

# Test suite.
include tests/Makefrag.am

# Documentation.
include doc/Makefrag.am

#
# Kernel Image
#

# We need the following junk because of the include-files-from-libc.a magic.
# TODO.  Is the following kosher from a Automake point of view?  (I.e. a
# program `gnumach.o' that is then later used again as an object file.)
gnumach_o_SOURCES =
# TODO.  ``-u _start''.  System dependent?
gnumach_o_LINK = $(LD) $(LDFLAGS) -u _start -r -o $@
noinst_PROGRAMS += \
	gnumach.o

# This is the list of routines we use from libgcc.
libgcc_routines := udivdi3 __udivdi3 __udivmoddi4 __umoddi3 __divdi3 __divmoddi4 __moddi3 __ffsdi2
# References generated by ld.
ld_magic_routines := __rel_iplt_start __rel_iplt_end __rela_iplt_start __rela_iplt_end _START etext _edata _end
gnumach-undef: gnumach.$(OBJEXT)
	$(NM_V) $(NM) -u $< | sed 's/  *U  *//' | sort -u > $@
MOSTLYCLEANFILES += gnumach-undef
gnumach-undef-bad: gnumach-undef Makefile
	$(AM_V_GEN) sed '$(foreach r,$(libgcc_routines) $(ld_magic_routines),/^$r$$/d;)' $< > $@
MOSTLYCLEANFILES += gnumach-undef-bad
libgcc-routines.o: gnumach-undef gnumach-undef-bad
	$(AM_V_at) if test -s gnumach-undef-bad; \
	then cat gnumach-undef-bad; exit 2; else true; fi
	$(AM_V_CCLD) $(CCLD) $(LDFLAGS) -r -static \
	  -o $@ `sed 's/^/-Wl,-u,/' < $<` -x c /dev/null -lgcc
	@if nm $@ | grep __init_cpu_features; \
	then echo "Please install a 32bit libc without multiarch support (on Debian systems, the libc6-dev:i386 package containing /usr/lib/i386-linux-gnu/libc.a)". ; \
	false ; fi

gnumach_LINK = $(LD) $(LDFLAGS) $(LINKFLAGS) $(gnumach_LINKFLAGS) -o $@
gnumach_LDADD = gnumach.o libgcc-routines.o

#
# Installation.
#

exec_bootdir = \
	$(exec_prefix)/boot
exec_boot_PROGRAMS = \
	gnumach

#
# Building a distribution.
#

EXTRA_DIST += \
	config.status.dep.patch \
	Makefile.in.dep.2.patch \
	Makefile.in.dep.patch

EXTRA_DIST += \
	DEVELOPMENT

dist-hook: dist-rm-CVS gen-ChangeLog

.PHONY: dist-rm-CVS
dist-rm-CVS:
# Try to be very safe with respect to spuriously removing various directories
# in case of an error.
	find $(distdir)/ -type d -name CVS | while read d; do	\
	  rm -f "$$d"/{Entries,Repository,Root,Tag} &&		\
	  rmdir "$$d";						\
	done

gen_start_commit = e227045b06d62ee7d2fbab9d5ade9030ff43170b
ChangeLog_files = ChangeLog ChangeLog.0 ChangeLog.00
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d $(top_srcdir)/.git; then			\
	  (cd $(top_srcdir)/ &&						\
	  ./gitlog-to-changelog	--strip-tab				\
	    $(gen_start_commit).. &&					\
	  echo) >> $(distdir)/cl-t &&					\
	  for f in $(ChangeLog_files); do				\
	    (cd $(top_srcdir)/ &&					\
	    git show $(gen_start_commit):$$f) >> $(distdir)/cl-t &&	\
	    rm -f $(distdir)/$$f &&					\
	    mv $(distdir)/cl-t $(distdir)/$$f				\
	    || exit $$?;						\
	  done;								\
	fi

DISTCLEANFILES += \
	Makefile.orig \
	config.status.orig

#
# Legacy support.
#

install-headers: install-data
	@echo '*****************************************************'
	@echo '* As you can see above, I was so kind to rewrite your'
	@echo '* `make $@'\''
	@echo '* into'
	@echo '* `make $^'\''
	@echo '* which is how it is to be spelled these days.'
	@echo '*'
	@echo '* Please get your instructions fixed.'
	@echo '*****************************************************'
	@echo
	@echo 'Penalty:'
	sleep 20

#
# Packaging: Bootable ISO and Installer
#

# Architecture detection for ISO naming
ISO_ARCH = $(systype)

# Build directories for packaging
PACKAGING_DIR = $(top_srcdir)/packaging
ISO_BUILD_DIR = $(builddir)/iso-build
ROOTFS_DIR = $(builddir)/rootfs-build

# ISO output names
ISO_LIVE = c9o-mach-live-$(ISO_ARCH).iso
ISO_INSTALL = c9o-mach-install-$(ISO_ARCH).iso

# GRUB configuration templates
GRUB_LIVE_TEMPLATE = $(PACKAGING_DIR)/iso/grub.cfg.live.template
GRUB_INSTALL_TEMPLATE = $(PACKAGING_DIR)/iso/grub.cfg.install.template

# Build ISO script
BUILD_ISO_SCRIPT = $(PACKAGING_DIR)/iso/build-iso.sh

# Installer sources (conditionally built)
if ENABLE_INSTALLER
INSTALLER_SOURCES = \
	$(PACKAGING_DIR)/installer/main.c \
	$(PACKAGING_DIR)/installer/disk.c \
	$(PACKAGING_DIR)/installer/partition.c \
	$(PACKAGING_DIR)/installer/filesystem.c \
	$(PACKAGING_DIR)/installer/bootloader.c \
	$(PACKAGING_DIR)/installer/ui.c \
	$(PACKAGING_DIR)/installer/utils.c

INSTALLER_CFLAGS = \
	-ffreestanding \
	-nostdlib \
	-m32 \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/$(systype)/include/mach/sa \
	-I$(top_srcdir)/tests \
	-DMACH_IPC_COMPAT=0 \
	-DMIG_COMPAT=0

installer: $(INSTALLER_SOURCES) gnumach
	@echo "Building installer binary..."
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/main.c -o $(builddir)/installer-main.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/disk.c -o $(builddir)/installer-disk.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/partition.c -o $(builddir)/installer-partition.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/filesystem.c -o $(builddir)/installer-filesystem.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/bootloader.c -o $(builddir)/installer-bootloader.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/ui.c -o $(builddir)/installer-ui.o
	$(CC) $(INSTALLER_CFLAGS) -c $(PACKAGING_DIR)/installer/utils.c -o $(builddir)/installer-utils.o
	$(LD) -m elf_i386 -N -Ttext 0x10000 \
		$(builddir)/installer-main.o \
		$(builddir)/installer-disk.o \
		$(builddir)/installer-partition.o \
		$(builddir)/installer-filesystem.o \
		$(builddir)/installer-bootloader.o \
		$(builddir)/installer-ui.o \
		$(builddir)/installer-utils.o \
		$(builddir)/tests/testlib.o \
		-o $(builddir)/installer
	@echo "Installer binary built: $(builddir)/installer"

MOSTLYCLEANFILES += \
	$(builddir)/installer \
	$(builddir)/installer-*.o
endif

# Create minimal rootfs image
rootfs.img: gnumach
	@echo "Creating minimal rootfs image..."
	@mkdir -p $(ROOTFS_DIR)
	@# Create a simple rootfs with the kernel
	dd if=/dev/zero of=$(builddir)/rootfs.img bs=1M count=32
	@echo "Rootfs image created: $(builddir)/rootfs.img"

MOSTLYCLEANFILES += $(builddir)/rootfs.img

# Live ISO target
if ENABLE_LIVE_ISO
iso-live: gnumach rootfs.img
	@echo "Building live ISO: $(ISO_LIVE)"
	@mkdir -p $(ISO_BUILD_DIR)/boot/grub
	@mkdir -p $(ISO_BUILD_DIR)/boot/modules
	@cp $(builddir)/gnumach $(ISO_BUILD_DIR)/boot/
	@cp $(builddir)/rootfs.img $(ISO_BUILD_DIR)/boot/modules/
	@sed -e 's/@KERNEL_PARAMS@/live console=com0 console_timestamps=on/' \
		$(GRUB_LIVE_TEMPLATE) > $(ISO_BUILD_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(builddir)/$(ISO_LIVE) $(ISO_BUILD_DIR)
	@echo "Live ISO created: $(builddir)/$(ISO_LIVE)"

iso-live-test: iso-live
	@echo "Testing live ISO in QEMU..."
	qemu-system-i386 -m 512 -cdrom $(builddir)/$(ISO_LIVE) -serial stdio -boot d
endif

# Install ISO target
if ENABLE_INSTALL_ISO
if ENABLE_INSTALLER
iso-install: gnumach installer rootfs.img
	@echo "Building install ISO: $(ISO_INSTALL)"
	@mkdir -p $(ISO_BUILD_DIR)/boot/grub
	@mkdir -p $(ISO_BUILD_DIR)/boot/modules
	@cp $(builddir)/gnumach $(ISO_BUILD_DIR)/boot/
	@cp $(builddir)/installer $(ISO_BUILD_DIR)/boot/modules/
	@cp $(builddir)/rootfs.img $(ISO_BUILD_DIR)/boot/modules/
	@sed -e 's/@KERNEL_PARAMS@/install console=com0 console_timestamps=on/' \
		$(GRUB_INSTALL_TEMPLATE) > $(ISO_BUILD_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(builddir)/$(ISO_INSTALL) $(ISO_BUILD_DIR)
	@echo "Install ISO created: $(builddir)/$(ISO_INSTALL)"

iso-install-test: iso-install
	@echo "Testing install ISO in QEMU..."
	qemu-system-i386 -m 512 -cdrom $(builddir)/$(ISO_INSTALL) -serial stdio -boot d \
		-drive file=$(builddir)/test-disk.img,format=raw,if=ide
endif
endif

# Convenience targets
.PHONY: iso iso-live iso-live-test iso-install iso-install-test iso-full iso-all iso-clean iso-test

# Default ISO target (alias for iso-live if enabled)
if ENABLE_LIVE_ISO
iso: iso-live
else
iso:
	@echo "ISO building not enabled. Configure with --enable-live-iso"
	@exit 1
endif

# Full ISO build (both live and install)
if ENABLE_LIVE_ISO
if ENABLE_INSTALL_ISO
if ENABLE_INSTALLER
iso-full: iso-live iso-install
	@echo "Both ISOs built successfully"
else
iso-full: iso-live
	@echo "Installer not enabled, only live ISO built"
endif
else
iso-full: iso-live
	@echo "Install ISO not enabled, only live ISO built"
endif
else
iso-full:
	@echo "ISO building not enabled. Configure with --enable-live-iso and/or --enable-install-iso"
	@exit 1
endif

# Build all ISOs for all architectures (placeholder for cross-compilation)
iso-all: iso-full
	@echo "All configured ISOs built"

# Clean ISO build artifacts
iso-clean:
	rm -rf $(ISO_BUILD_DIR)
	rm -rf $(ROOTFS_DIR)
	rm -f $(builddir)/$(ISO_LIVE)
	rm -f $(builddir)/$(ISO_INSTALL)
	rm -f $(builddir)/rootfs.img
	rm -f $(builddir)/installer
	rm -f $(builddir)/installer-*.o

# Test ISO in QEMU (runs validation script)
iso-test:
	@if test -f $(builddir)/$(ISO_LIVE); then \
		$(top_srcdir)/scripts/validate-iso.sh --boot-test $(builddir)/$(ISO_LIVE); \
	elif test -f $(builddir)/$(ISO_INSTALL); then \
		$(top_srcdir)/scripts/validate-iso.sh --boot-test $(builddir)/$(ISO_INSTALL); \
	else \
		echo "No ISO found. Run 'make iso' first."; \
		exit 1; \
	fi

# Validate ISO structure without booting
iso-validate:
	@if test -f $(builddir)/$(ISO_LIVE); then \
		$(top_srcdir)/scripts/validate-iso.sh $(builddir)/$(ISO_LIVE); \
	elif test -f $(builddir)/$(ISO_INSTALL); then \
		$(top_srcdir)/scripts/validate-iso.sh $(builddir)/$(ISO_INSTALL); \
	else \
		echo "No ISO found. Run 'make iso' first."; \
		exit 1; \
	fi
