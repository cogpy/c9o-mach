# packaging/installer/Makefile.am - Build system for the installer
# Copyright (C) 2024 Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

EXTRA_DIST = installer.h

# Installer sources
INSTALLER_SOURCES = \
	main.c \
	disk.c \
	partition.c \
	filesystem.c \
	bootloader.c \
	ui.c \
	utils.c

# Build the installer as a freestanding binary
# Similar to test modules, but standalone

INSTALLER_CFLAGS = -static -nostartfiles -nolibc \
	-ffreestanding \
	-I$(top_srcdir)/tests/include \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/$(systype) \
	-I$(top_srcdir)/$(systype)/include/mach/sa \
	-I$(MACH_TESTINCLUDE) \
	-I$(MIG_OUTDIR) \
	-ggdb3 \
	-DMIG_EOPNOTSUPP

if disable_smashing_stack_protector
INSTALLER_CFLAGS += -fno-stack-protector
endif

# Link with test library for console output and Mach IPC
INSTALLER_LIBS = \
	$(top_srcdir)/tests/syscalls.S \
	$(top_srcdir)/tests/start.S \
	$(top_srcdir)/tests/testlib.c \
	$(top_srcdir)/tests/testlib_thread_start.c \
	$(top_srcdir)/i386/i386/strings.c \
	$(top_srcdir)/kern/printf.c \
	$(top_srcdir)/kern/strings.c \
	$(top_srcdir)/util/atoi.c \
	$(top_builddir)/tests/errlist.c \
	$(MIG_GEN_CC)

# Build installer binary
installer: $(INSTALLER_SOURCES) $(INSTALLER_LIBS) $(MACH_TESTINSTALL)
	$(USER_CC) $(USER_CFLAGS) $(INSTALLER_CFLAGS) \
		$(addprefix $(srcdir)/,$(INSTALLER_SOURCES)) \
		$(INSTALLER_LIBS) \
		-o $@

# Clean target
clean-installer:
	rm -f installer

.PHONY: installer clean-installer
