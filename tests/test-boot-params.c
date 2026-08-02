/*
 *  Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program ; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <testlib.h>

#include <kern/boot_params.h>

static void test_boot_modes(void)
{
  ASSERT(boot_param_mode("console=com0") == BOOT_MODE_NORMAL,
         "no mode keyword should select the normal boot mode");
  ASSERT(boot_param_mode("console=com0 live root=ramdisk") == BOOT_MODE_LIVE,
         "live should select the live boot mode");
  ASSERT(boot_param_mode("console=com0 rescue single") == BOOT_MODE_RESCUE,
         "rescue should select the rescue boot mode");
  ASSERT(boot_param_mode("console=com0 install target=hd0") == BOOT_MODE_INSTALL,
         "install should select the install boot mode");
  ASSERT(boot_param_mode("live install") == BOOT_MODE_INSTALL,
         "install should take precedence over live");
  ASSERT(boot_param_mode("live rescue") == BOOT_MODE_RESCUE,
         "rescue should take precedence over live");
  ASSERT(boot_param_mode(NULL) == BOOT_MODE_NORMAL,
         "a missing command line should select the normal boot mode");
}

static void test_flags(void)
{
  ASSERT(boot_param_flag("console=com0 live", "live"),
         "live should be found");
  ASSERT(!boot_param_flag("console=comlive", "live"),
         "a flag should only match a whole word");
  ASSERT(!boot_param_flag("console=com0 nolive", "live"),
         "a flag should not match the end of a word");
  ASSERT(boot_param_flag("live", "live"),
         "a flag should match a single word command line");
  ASSERT(boot_param_flag("  \t live \t ", "live"),
         "a flag should match around extra separators");
  ASSERT(!boot_param_flag("", "live"),
         "an empty command line has no flag");
}

static void test_options(void)
{
  char value[BOOT_PARAM_VALUE_MAX];

  ASSERT(boot_param_option("console=com0 target=hd0s1 live",
                           "target", value, sizeof(value)),
         "target= should be found");
  ASSERT(strcmp(value, "hd0s1") == 0, "target= value should be parsed");

  ASSERT(!boot_param_option("console=com0 live", "target",
                            value, sizeof(value)),
         "a missing option should not be reported");
  ASSERT(value[0] == '\0', "a missing option should clear the value");

  ASSERT(!boot_param_option("console=com0 mytarget=hd0", "target",
                            value, sizeof(value)),
         "an option should only match a whole word");

  ASSERT(boot_param_option("target=hd0s1", "target", value, 4),
         "target= should be found");
  ASSERT(strcmp(value, "hd0") == 0, "a too long value should be truncated");

  ASSERT(boot_param_option("console=com0 target=", "target",
                           value, sizeof(value)),
         "an empty option value should still be reported");
  ASSERT(value[0] == '\0', "an empty option value should be empty");
}

static void test_mode_strings(void)
{
  ASSERT(strcmp(boot_mode_string(BOOT_MODE_NORMAL), "normal") == 0,
         "normal mode name");
  ASSERT(strcmp(boot_mode_string(BOOT_MODE_LIVE), "live") == 0,
         "live mode name");
  ASSERT(strcmp(boot_mode_string(BOOT_MODE_INSTALL), "install") == 0,
         "install mode name");
  ASSERT(strcmp(boot_mode_string(BOOT_MODE_RESCUE), "rescue") == 0,
         "rescue mode name");
}

int main(int argc, char *argv[], int envc, char *envp[])
{
  test_boot_modes();
  test_flags();
  test_options();
  test_mode_strings();

  printf("boot parameter parsing ok\n");
  return 0;
}
