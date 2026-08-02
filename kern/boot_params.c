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

#include <string.h>

#include <kern/boot_params.h>
#include <kern/printf.h>

/*
 * Defined by the platform code (i386/i386at/model_dep.c) for the kernel,
 * and left NULL in the user space builds which reuse the parsing helpers.
 */
extern char *kernel_cmdline;

static boot_mode_t boot_mode_selected = BOOT_MODE_NORMAL;
static char boot_target[BOOT_PARAM_VALUE_MAX] = "";

static boolean_t
is_separator(char c)
{
	return c == '\0' || c == ' ' || c == '\t' || c == '\n';
}

/*
 * Return the start of the first word of CMDLINE matching NAME, or NULL.
 * A word matches when it is exactly NAME, or when it starts with NAME
 * followed by `=' and OPTION is TRUE.
 */
static const char *
find_word(const char *cmdline, const char *name, boolean_t option)
{
	size_t len = strlen(name);
	const char *p = cmdline;

	if (cmdline == NULL || len == 0)
		return NULL;

	while (*p != '\0') {
		while (*p == ' ' || *p == '\t' || *p == '\n')
			p++;
		if (*p == '\0')
			break;

		if (strncmp(p, name, len) == 0) {
			if (option) {
				if (p[len] == '=')
					return p;
			} else if (is_separator(p[len]))
				return p;
		}

		while (!is_separator(*p))
			p++;
	}

	return NULL;
}

boolean_t
boot_param_flag(const char *cmdline, const char *flag)
{
	return find_word(cmdline, flag, FALSE) != NULL;
}

boolean_t
boot_param_option(const char *cmdline, const char *name,
		  char *value, size_t len)
{
	const char *p;
	size_t i = 0;

	if (value == NULL || len == 0)
		return FALSE;

	value[0] = '\0';

	p = find_word(cmdline, name, TRUE);
	if (p == NULL)
		return FALSE;

	p += strlen(name) + 1;		/* skip `NAME='  */
	while (!is_separator(*p) && i < len - 1)
		value[i++] = *p++;
	value[i] = '\0';

	return TRUE;
}

boot_mode_t
boot_param_mode(const char *cmdline)
{
	if (boot_param_flag(cmdline, "install"))
		return BOOT_MODE_INSTALL;
	if (boot_param_flag(cmdline, "rescue"))
		return BOOT_MODE_RESCUE;
	if (boot_param_flag(cmdline, "live"))
		return BOOT_MODE_LIVE;

	return BOOT_MODE_NORMAL;
}

const char *
boot_mode_string(boot_mode_t mode)
{
	switch (mode) {
	case BOOT_MODE_LIVE:
		return "live";
	case BOOT_MODE_INSTALL:
		return "install";
	case BOOT_MODE_RESCUE:
		return "rescue";
	case BOOT_MODE_NORMAL:
	default:
		return "normal";
	}
}

void
boot_params_init(void)
{
	boot_mode_selected = boot_param_mode(kernel_cmdline);
	(void) boot_param_option(kernel_cmdline, "target",
				 boot_target, sizeof(boot_target));

	printf("boot mode: %s", boot_mode_string(boot_mode_selected));
	if (boot_target[0] != '\0')
		printf(", installation target: %s", boot_target);
	printf("\n");
}

boot_mode_t
boot_mode(void)
{
	return boot_mode_selected;
}

const char *
boot_target_device(void)
{
	return boot_target;
}
