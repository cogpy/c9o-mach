/*
 * Boot parameter framework.
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *	Parsing of the installation related parameters given on the
 *	kernel command line by the boot loader.  The parsing helpers are
 *	pure string functions so that the bootstrap tasks shipped on the
 *	installation media can reuse them.
 */

#ifndef _KERN_BOOT_PARAMS_H_
#define _KERN_BOOT_PARAMS_H_

#include <mach/boolean.h>
#include <sys/types.h>

/* Maximum length of an option value, including the terminating NUL.  */
#define BOOT_PARAM_VALUE_MAX	64

/* How the system was asked to boot by the boot loader.  */
typedef enum {
	BOOT_MODE_NORMAL = 0,	/* boot the installed system */
	BOOT_MODE_LIVE,		/* RAM based live system */
	BOOT_MODE_INSTALL,	/* run the installer */
	BOOT_MODE_RESCUE	/* minimal rescue environment */
} boot_mode_t;

/*
 * Return TRUE when FLAG appears as a whole word in CMDLINE.  Words are
 * separated by spaces, tabs or newlines, so `live' does not match
 * `console=comlive' nor `nolive'.
 */
extern boolean_t boot_param_flag(const char *cmdline, const char *flag);

/*
 * Look for `NAME=VALUE' in CMDLINE and copy VALUE into the buffer VALUE
 * of LEN bytes (NUL terminated, truncated when too long).  Returns TRUE
 * when the option was found.
 */
extern boolean_t boot_param_option(const char *cmdline, const char *name,
				   char *value, size_t len);

/*
 * Determine the boot mode requested on CMDLINE.  `install' takes
 * precedence over `rescue', which takes precedence over `live'.
 */
extern boot_mode_t boot_param_mode(const char *cmdline);

/* Printable name of MODE, e.g. "live".  */
extern const char *boot_mode_string(boot_mode_t mode);

/*
 * Parse the kernel command line.  Called once during startup, after the
 * console is usable.
 */
extern void boot_params_init(void);

/* Boot mode selected by the boot loader, valid after boot_params_init.  */
extern boot_mode_t boot_mode(void);

/*
 * Installation target device requested with `target=DEV', or an empty
 * string when none was given.
 */
extern const char *boot_target_device(void);

#endif	/* _KERN_BOOT_PARAMS_H_ */
