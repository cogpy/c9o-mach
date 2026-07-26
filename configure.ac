dnl Configure script for GNU Mach.

dnl Copyright (C) 1997, 1998, 1999, 2004, 2006, 2007, 2008, 2010, 2013 Free
dnl Software Foundation, Inc.

dnl Permission to use, copy, modify and distribute this software and its
dnl documentation is hereby granted, provided that both the copyright
dnl notice and this permission notice appear in all copies of the
dnl software, derivative works or modified versions, and any portions
dnl thereof, and that both notices appear in supporting documentation.
dnl
dnl THE FREE SOFTWARE FOUNDATION ALLOWS FREE USE OF THIS SOFTWARE IN ITS
dnl "AS IS" CONDITION.  THE FREE SOFTWARE FOUNDATION DISCLAIMS ANY
dnl LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
dnl USE OF THIS SOFTWARE.

AC_PREREQ([2.69])

m4_include([version.m4])
AC_INIT([AC_PACKAGE_NAME], [AC_PACKAGE_VERSION], [AC_PACKAGE_BUGREPORT],
  [AC_PACKAGE_TARNAME])
AC_CONFIG_SRCDIR([kern/ipc_kobject.c])

if test -z "${CFLAGS+set}"; then
  # Use these CFLAGS by default if nothing is set.
  CFLAGS="-g -O2"
fi
saved_CFLAGS="$CFLAGS"
# We don't need glibc to compile gnumach so ensure we don't try to use it
# during configure.
CFLAGS="$CFLAGS -ffreestanding -nostdlib"

AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE(
  [1.14]
  [dist-xz]
dnl Don't define `PACKAGE' and `VERSION'.
  [no-define]
dnl Do not clutter the main build directory.
  [subdir-objects]
dnl We require GNU make.
  [-Wall -Wno-portability]
  [foreign]
)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])],
                            [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

#
# Deduce the architecture we're building for.
#
# TODO: Should we also support constructs like `i686_xen-pc-gnu' or
# `i686-pc_xen-gnu'?

AC_CANONICAL_HOST

AC_ARG_ENABLE([platform],
  AS_HELP_STRING([--enable-platform=PLATFORM], [specify the platform to build a
    kernel for.  Defaults to `at' for `i?86'.  The other possibility is
    `xen'.]),
  [host_platform=$enable_platform],
  [host_platform=default])
[# Supported configurations.
case $host_platform:$host_cpu in
  default:i?86)
    host_platform=at;;
  default:x86_64)]
    [host_platform=at;;
  at:i?86 | xen:i?86 | at:x86_64 | xen:x86_64)
    :;;
  *:aarch64)
    ;;
  *)]
    AC_MSG_ERROR([unsupported combination of cpu type `$host_cpu' and platform
      `$host_platform'.])[;;
esac]
AC_SUBST([host_platform])

[# This is used in a few places.
case $host_cpu in
  i?86)
    systype=i386;;
  *)
    systype=$host_cpu;;
esac]
AC_SUBST([systype])

AM_CONDITIONAL([HOST_ix86], [test "$systype" = "i386"])

#
# Programs.
#

AC_PROG_AWK
# Modern cross-compilation support
AC_CANONICAL_BUILD
# Temporarily force cross compiling mode to make sure the configure script
# does not try to run compiled binaries during build tool detection.
save_cross_compiling=$cross_compiling
cross_compiling=yes
AM_PROG_AS
cross_compiling=$save_cross_compiling
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
# Modern replacement for deprecated AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_RANLIB
AC_CHECK_TOOL([AR], [ar])
AC_CHECK_TOOL([LD], [ld])
AC_CHECK_TOOL([NM], [nm])

AC_CHECK_TOOL([MIG], [mig], [no])
AC_ARG_VAR([MIG], [Path to the mig tool])

if test x$MIG = xno
then
    AC_MSG_WARN([mig was not found, we will not be able to build a kernel, only install headers. Install or build mig against them, and run configure again. If you already did so, perhaps you need to specify the path with MIG=])
    MIG=mig
fi

dnl Needed for the Automake option `subdir-objects'.
AM_PROG_CC_C_O

# Check for essential build tools and provide helpful error messages
AC_CHECK_PROG([GZIP], [gzip], [gzip])
AS_IF([test "x$GZIP" = "x"], [
  AC_MSG_ERROR([gzip is required for building GNU Mach])
])

AC_CHECK_TOOL([STRIP], [strip])
AS_IF([test "x$STRIP" = "x"], [
  AC_MSG_ERROR([strip tool is required for building GNU Mach])
])

dnl See below why we need to patch stuff during build...
AC_CHECK_PROG([PATCH], [patch], [patch])
AS_IF([test "x$PATCH" = "x"], [
  AC_MSG_ERROR([patch is required for building GNU Mach])
])

# Check for other essential tools
AC_PROG_SED
AC_CHECK_PROG([AWK_PROG], [awk], [awk])
AS_IF([test "x$AWK_PROG" = "x"], [
  AC_MSG_ERROR([awk is required for building GNU Mach])
])

#
# configure fragments.
#

# Default set of device drivers.
AC_ARG_ENABLE([device-drivers],
  AS_HELP_STRING([--enable-device-drivers=WHICH], [specify WHICH (on `ix86-at'
    one of `default', `qemu', `none') to preset a certain subset of all
    available device drivers, as indicated by the below-metioned ``enabled
    ...'' comments; you can then still use further `--enable-*' or
    `--disable-*' options to refine the selection of drivers to include in
    order to choose only those you actually want to have enabled]))
[case $enable_device_drivers in
  '')
    enable_device_drivers=default;;
  no)
    enable_device_drivers=none;;
  default | none | qemu)
    :;;
  *)]
    AC_MSG_ERROR([invalid choice of]
      [`--enable-device-drivers=$enable_device_drivers'.])
    [;;
esac]

AC_ARG_ENABLE([user32],
  AS_HELP_STRING([--enable-user32], [enable 32-bit user space on a 64-bit kernel]))
[if [ x"$enable_user32" = xyes ]; then]
  AC_DEFINE([USER32], [], [enable 32-bit user on 64-bit kernel])
  AM_CONDITIONAL([enable_user32], [true])
[else]
  AM_CONDITIONAL([enable_user32], [false])
[fi]

# Modern build system options
AC_ARG_ENABLE([optimization],
  AS_HELP_STRING([--enable-optimization@<:@=LEVEL@:>@], 
    [set optimization level (0,1,2,3,s,g) @<:@default=2@:>@]),
  [optimization_level="$enableval"],
  [optimization_level="2"])

AS_CASE(["$optimization_level"],
  [0|1|2|3|s|g], [],
  [AC_MSG_ERROR([invalid optimization level: $optimization_level])])

AC_SUBST([OPTIMIZATION_LEVEL], ["$optimization_level"])

AC_ARG_ENABLE([debug-info],
  AS_HELP_STRING([--enable-debug-info], [include debugging information @<:@default=yes@:>@]),
  [debug_info="$enableval"],
  [debug_info="yes"])

# LTTng-style tracing support
AC_ARG_ENABLE([tracing],
  AS_HELP_STRING([--enable-tracing], [enable LTTng-style kernel tracing @<:@default=yes@:>@]),
  [tracing_enabled="$enableval"],
  [tracing_enabled="yes"])

AS_IF([test "x$tracing_enabled" = xyes], [
  AC_DEFINE([CONFIG_MACH_TRACING], [1], [Enable LTTng-style kernel tracing])
  AC_MSG_NOTICE([LTTng-style tracing support enabled])
], [
  AC_MSG_NOTICE([LTTng-style tracing support disabled])
])

AM_CONDITIONAL([ENABLE_DEBUG_INFO], [test "x$debug_info" = "xyes"])

# Strict compilation mode for development
AC_ARG_ENABLE([strict-warnings],
  AS_HELP_STRING([--enable-strict-warnings], 
    [enable strict compiler warnings for development @<:@default=no@:>@]),
  [strict_warnings="$enableval"],
  [strict_warnings="no"])

AM_CONDITIONAL([STRICT_WARNINGS], [test "x$strict_warnings" = "xyes"])

# Timer optimization options
AC_ARG_ENABLE([tickless],
  AS_HELP_STRING([--enable-tickless], 
    [enable tickless timer operation for reduced overhead @<:@default=no@:>@]),
  [tickless_timer="$enableval"],
  [tickless_timer="no"])

AM_CONDITIONAL([TICKLESS_TIMER], [test "x$tickless_timer" = "xyes"])

if test "x$tickless_timer" = "xyes"; then
  AC_DEFINE([TICKLESS_TIMER], [1], [Define to enable tickless timer operation])
fi

# Packaging options for bootable OS
AC_ARG_ENABLE([installer],
  AS_HELP_STRING([--enable-installer], 
    [build the installer binary for OS installation @<:@default=no@:>@]),
  [enable_installer="$enableval"],
  [enable_installer="no"])

AM_CONDITIONAL([ENABLE_INSTALLER], [test "x$enable_installer" = "xyes"])

AC_ARG_ENABLE([live-iso],
  AS_HELP_STRING([--enable-live-iso], 
    [enable live ISO building capabilities @<:@default=no@:>@]),
  [enable_live_iso="$enableval"],
  [enable_live_iso="no"])

AM_CONDITIONAL([ENABLE_LIVE_ISO], [test "x$enable_live_iso" = "xyes"])

AC_ARG_ENABLE([install-iso],
  AS_HELP_STRING([--enable-install-iso], 
    [enable installation ISO building capabilities @<:@default=no@:>@]),
  [enable_install_iso="$enableval"],
  [enable_install_iso="no"])

AM_CONDITIONAL([ENABLE_INSTALL_ISO], [test "x$enable_install_iso" = "xyes"])

AC_ARG_WITH([rootfs],
  AS_HELP_STRING([--with-rootfs=TYPE], 
    [rootfs type for ISO: minimal or full @<:@default=minimal@:>@]),
  [rootfs_type="$withval"],
  [rootfs_type="minimal"])

AC_SUBST([ROOTFS_TYPE], ["$rootfs_type"])

# Check for ISO building tools if packaging is enabled
if test "x$enable_live_iso" = "xyes" -o "x$enable_install_iso" = "xyes"; then
  AC_CHECK_PROG([GRUB_MKRESCUE], [grub-mkrescue], [grub-mkrescue], [no])
  if test "x$GRUB_MKRESCUE" = "xno"; then
    AC_MSG_WARN([grub-mkrescue not found, ISO building will not be available])
  fi
  
  AC_CHECK_PROG([XORRISO], [xorriso], [xorriso], [no])
  if test "x$XORRISO" = "xno"; then
    AC_MSG_WARN([xorriso not found, ISO building may not work correctly])
  fi
  
  AC_CHECK_PROG([MTOOLS], [mcopy], [yes], [no])
  if test "x$MTOOLS" = "xno"; then
    AC_MSG_WARN([mtools not found, FAT filesystem support in ISOs may not work])
  fi
fi

# Check for installer dependencies
if test "x$enable_installer" = "xyes"; then
  AC_CHECK_PROG([MKE2FS], [mke2fs], [mke2fs], [no])
  if test "x$MKE2FS" = "xno"; then
    AC_MSG_WARN([mke2fs not found, ext2 filesystem creation in installer may be limited])
  fi
fi

# Platform-specific configuration.

# PC AT.
# TODO.  Currently handled in `i386/configfrag.ac'.

# General options.
m4_include([configfrag-first.ac])

# Xen.
m4_include([xen/configfrag.ac])

# Machine-specific configuration.

# ix86.
m4_include([i386/configfrag.ac])

# x86_64
m4_include([x86_64/configfrag.ac])

# aarch64
m4_include([aarch64/configfrag.ac])

# General options.
m4_include([configfrag.ac])

# Linux code snarfed into GNU Mach.
m4_include([linux/configfrag.ac])

# The test suite.
m4_include([tests/configfrag.ac])

#
# Compiler features and capabilities detection.
#

# Modern compiler feature detection using AC_COMPILE_IFELSE
AC_DEFUN([GNU_MACH_CHECK_CC_FLAG], [
  AC_MSG_CHECKING([whether $CC accepts $1])
  saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $1"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([], [])],
    [AC_MSG_RESULT([yes])
     $2=yes],
    [AC_MSG_RESULT([no])
     $2=no])
  CFLAGS=$saved_CFLAGS
])

# Check for stack protector support
GNU_MACH_CHECK_CC_FLAG([-fstack-protector], [ssp_possible])
AM_CONDITIONAL([disable_smashing_stack_protector],
  [test "x$ssp_possible" = "xyes"])

# Check for modern compiler warning capabilities
GNU_MACH_CHECK_CC_FLAG([-Wall], [have_wall])
GNU_MACH_CHECK_CC_FLAG([-Wextra], [have_wextra])
GNU_MACH_CHECK_CC_FLAG([-Wstrict-prototypes], [have_wstrict_prototypes])

# Check freestanding environment support
GNU_MACH_CHECK_CC_FLAG([-ffreestanding], [have_ffreestanding])
AS_IF([test "x$have_ffreestanding" != "xyes"], [
  AC_MSG_ERROR([Compiler must support -ffreestanding for kernel compilation])
])

# Check for other essential compiler features
GNU_MACH_CHECK_CC_FLAG([-nostdlib], [have_nostdlib])
AS_IF([test "x$have_nostdlib" != "xyes"], [
  AC_MSG_ERROR([Compiler must support -nostdlib for kernel compilation])
])

#
# Output.
#

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile version.c])

#
# The remaining ugly, dark corners...
#
# Attention, parents: don't show this to your children...
#

#
# config.status.dep.patch
#
# This is a (ugly --- I admit) bootstrap hack to get to-be-generated files
# created before any other source files are compiled.
#
# See <http://lists.gnu.org/archive/html/automake/2006-05/msg00038.html>.
#
# We don't use `BUILT_SOURCES' (as it was suggested in the follow-up message),
# as we also want things like `make SPECIFIC_TARGET' to work.
#
# This affair is especially ugly because internals are used (the `# dummy'
# tag): internals that may be subject to changes.  That's the reason why a
# real patch is being used here and not some `sed' magic: to make it fail
# loudly in case.
#
# For all shipped source files a dependency file is tried to be created where
# it is simply stated that the respective source file depends on _all_
# to-be-generated files.  Depending on all of them doesn't do any harm, as they
# will nevertheless have to be created, sooner or later.  The problem is, that
# `config.status' doesn't know about the source file of the file it is
# currently creating the dependency file for.  So we have it do an educated
# guess...  Later, when compiling the source files, these dependency files will
# be rewritten to contain the files's actual dependencies.  From then on this
# bootstrap hack will be forgotten.
#

dnl AC_CONFIG_COMMANDS_POST([
dnl   sed -i -e \
dnl     's%#\ dummy%Makefile: $(filter-out $(DIST_SOURCES),$(SOURCES))%' \
dnl     config.status
dnl ])
AC_CONFIG_COMMANDS_POST([
  if "$PATCH" -f < "$srcdir"/config.status.dep.patch > /dev/null 2>&1 ||
     ( cd "$srcdir" &&
       ( "$PATCH" -f < Makefile.in.dep.patch ||
         "$PATCH" -f < Makefile.in.dep.2.patch ||
         grep "Ugly bootstrap hack to get to-be-generated files created" Makefile.in ) ) > /dev/null 2>&1
  then] AC_MSG_NOTICE([Applied a patch to work around a deficiency in]
    [Automake.  See `configure.ac' for details.])
  [else] AC_MSG_ERROR([failed to patch using `config.status.dep.patch'.]
      [You have a serious problem.  Please contact <$PACKAGE_BUGREPORT>.])
  [fi
])


CFLAGS="$saved_CFLAGS"

# Configure compiler flags based on detected capabilities and user options
if test "$GCC" = yes; then
  # Base CFLAGS with optimization and debug settings
  if test "x$debug_info" = "xyes"; then
    CFLAGS="$CFLAGS -g"
  fi
  CFLAGS="$CFLAGS -O$optimization_level"
  
  # Essential warning flags (always enabled)
  test "x$have_wall" = "xyes" && CFLAGS="$CFLAGS -Wall"
  test "x$have_wextra" = "xyes" && CFLAGS="$CFLAGS -Wextra"
  test "x$have_wstrict_prototypes" = "xyes" && CFLAGS="$CFLAGS -Wstrict-prototypes"
  
  # Additional useful warnings
  CFLAGS="$CFLAGS -Wmissing-prototypes -Wmissing-declarations"
  CFLAGS="$CFLAGS -Wformat=2 -Wformat-security"
  CFLAGS="$CFLAGS -Wold-style-definition"
  
  # Disable some warnings that may be too noisy initially
  CFLAGS="$CFLAGS -Wno-unused-parameter -Wno-sign-compare"
  
  # Strict warnings for development (if enabled)
  if test "x$strict_warnings" = "xyes"; then
    CFLAGS="$CFLAGS -Werror -pedantic"
  fi
fi

# Summary of configuration
AC_MSG_NOTICE([])
AC_MSG_NOTICE([GNU Mach configuration summary:])
AC_MSG_NOTICE([  Target architecture: $systype])
AC_MSG_NOTICE([  Platform: $host_platform])
AC_MSG_NOTICE([  Optimization level: $optimization_level])
AC_MSG_NOTICE([  Debug information: $debug_info])
AC_MSG_NOTICE([  Strict warnings: $strict_warnings])
AC_MSG_NOTICE([  Tickless timer: $tickless_timer])
AC_MSG_NOTICE([  Stack protector: $ssp_possible])
AC_MSG_NOTICE([  MIG tool: $MIG])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Packaging options:])
AC_MSG_NOTICE([  Build installer: $enable_installer])
AC_MSG_NOTICE([  Live ISO support: $enable_live_iso])
AC_MSG_NOTICE([  Install ISO support: $enable_install_iso])
AC_MSG_NOTICE([  Rootfs type: $rootfs_type])
AC_MSG_NOTICE([])

#
# Fire.
#

AC_OUTPUT
