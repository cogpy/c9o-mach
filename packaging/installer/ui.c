/*
 * c9o-mach Installer - Text-based User Interface
 * Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "installer.h"

/* Console dimensions */
#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   25

/* UI colors/attributes (ANSI escape codes) */
#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_UNDERLINE  "\033[4m"
#define ANSI_INVERSE    "\033[7m"
#define ANSI_RED        "\033[31m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_WHITE      "\033[37m"
#define ANSI_CLEAR      "\033[2J\033[H"

/* Simple printf wrapper for console output */
extern void cnputc(char c);
extern int printf(const char *fmt, ...);

/*
 * Clear the screen
 */
static void ui_clear_screen(void)
{
    printf(ANSI_CLEAR);
}

/*
 * Print a horizontal line
 */
static void ui_print_line(void)
{
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        cnputc('-');
    }
    cnputc('\n');
}

/*
 * Print centered text
 */
static void ui_print_centered(const char *text)
{
    int len = strlen(text);
    int padding = (SCREEN_WIDTH - len) / 2;
    
    for (int i = 0; i < padding; i++) {
        cnputc(' ');
    }
    printf("%s\n", text);
}

/*
 * Print a title box
 */
static void ui_print_title(const char *title)
{
    printf(ANSI_BOLD ANSI_CYAN);
    ui_print_line();
    ui_print_centered(title);
    ui_print_line();
    printf(ANSI_RESET);
    printf("\n");
}

/*
 * Wait for user to press Enter
 */
static void ui_wait_for_enter(void)
{
    printf("\n" ANSI_YELLOW "Press Enter to continue..." ANSI_RESET);
    /* In a real implementation, read from console */
    /* For now, just return */
}

/*
 * Simple menu selection
 */
static int ui_menu_select(const char *title, const char **options, int count)
{
    int selected = 0;
    
    printf("\n%s\n\n", title);
    
    for (int i = 0; i < count; i++) {
        printf("  %d) %s\n", i + 1, options[i]);
    }
    
    printf("\nSelect option (1-%d): ", count);
    
    /* In a real implementation, read user input */
    /* For now, return first option */
    selected = 0;
    
    return selected;
}

/*
 * Initialize the UI subsystem
 */
int ui_init(void)
{
    installer_log(LOG_DEBUG, "Initializing UI subsystem");
    ui_clear_screen();
    return 0;
}

/*
 * Cleanup UI resources
 */
void ui_cleanup(void)
{
    installer_log(LOG_DEBUG, "Cleaning up UI subsystem");
    printf(ANSI_RESET);
}

/*
 * Show the welcome screen
 */
int ui_show_welcome(void)
{
    ui_clear_screen();
    
    ui_print_title("Welcome to the c9o-mach Installer");
    
    printf("This installer will guide you through installing c9o-mach\n");
    printf("(GNU Mach microkernel) on your computer.\n");
    printf("\n");
    printf(ANSI_BOLD "What is c9o-mach?" ANSI_RESET "\n\n");
    printf("c9o-mach is a microkernel based on GNU Mach, which provides:\n");
    printf("  * Inter-Process Communication (IPC)\n");
    printf("  * Virtual memory management\n");
    printf("  * Basic device drivers\n");
    printf("  * Foundation for the GNU/Hurd operating system\n");
    printf("\n");
    
    printf(ANSI_YELLOW "WARNING:" ANSI_RESET " Installing will modify your disk.\n");
    printf("Make sure you have backed up any important data.\n");
    printf("\n");
    
    const char *options[] = {
        "Continue with installation",
        "Exit installer"
    };
    
    int choice = ui_menu_select("Please select an option:", options, 2);
    
    if (choice == 1) {
        return UI_RESULT_CANCEL;
    }
    
    return UI_RESULT_NEXT;
}

/*
 * Show disk selection screen
 */
int ui_disk_selection(disk_info_t **selected_disk)
{
    disk_info_t *disks;
    int disk_count = 0;
    
    ui_clear_screen();
    ui_print_title("Select Installation Disk");
    
    /* Enumerate available disks */
    disks = disk_enumerate();
    
    if (disks == NULL) {
        printf(ANSI_RED "No disks found!" ANSI_RESET "\n");
        printf("\nPlease ensure your disk is connected and try again.\n");
        ui_wait_for_enter();
        return UI_RESULT_ERROR;
    }
    
    /* Count and display disks */
    printf("Available disks:\n\n");
    printf("  %-10s %-20s %-15s %s\n", "Device", "Model", "Size", "Type");
    ui_print_line();
    
    for (disk_info_t *d = disks; d != NULL; d = d->next) {
        char size_str[32];
        format_size(d->size_bytes, size_str, sizeof(size_str));
        
        printf("  %-10s %-20.20s %-15s %s\n",
               d->device_name, d->model, size_str,
               disk_type_to_string(d->type));
        disk_count++;
    }
    
    printf("\n");
    printf(ANSI_YELLOW "Note:" ANSI_RESET " All data on the selected disk will be erased!\n\n");
    
    if (disk_count == 0) {
        printf(ANSI_RED "No usable disks found." ANSI_RESET "\n");
        disk_free_list(disks);
        ui_wait_for_enter();
        return UI_RESULT_ERROR;
    }
    
    /* For now, auto-select first disk */
    *selected_disk = disks;
    
    printf("Selected disk: %s (%s)\n", 
           (*selected_disk)->device_name, (*selected_disk)->model);
    
    return UI_RESULT_NEXT;
}

/*
 * Show partition configuration screen
 */
int ui_partition_config(disk_info_t *disk, partition_config_t *config)
{
    ui_clear_screen();
    ui_print_title("Partition Configuration");
    
    printf("Target disk: %s (%s)\n\n", disk->device_name, disk->model);
    
    /* Show current partitions if any */
    partition_info_t *parts = disk_get_partitions(disk);
    
    if (parts != NULL) {
        printf("Current partitions:\n\n");
        printf("  %-4s %-12s %-15s %-12s\n", "#", "Type", "Size", "Filesystem");
        ui_print_line();
        
        for (partition_info_t *p = parts; p != NULL; p = p->next) {
            char size_str[32];
            format_size(p->size_bytes, size_str, sizeof(size_str));
            
            printf("  %-4d %-12s %-15s %-12s\n",
                   p->number,
                   partition_type_to_string(p->type),
                   size_str,
                   fs_type_to_string(p->fs_type));
        }
        partition_free_list(parts);
        printf("\n");
    }
    
    /* Partitioning options */
    const char *options[] = {
        "Use entire disk (recommended)",
        "Manual partitioning",
        "Go back"
    };
    
    int choice = ui_menu_select("Partitioning method:", options, 3);
    
    if (choice == 2) {
        return UI_RESULT_BACK;
    }
    
    /* Set default configuration */
    config->use_entire_disk = (choice == 0);
    config->ptable_type = PTABLE_TYPE_MBR;
    config->create_swap = 1;
    config->swap_size_mb = 512;  /* 512MB swap */
    config->root_size_mb = 0;    /* Use remaining space */
    config->root_fs = FS_TYPE_EXT2;
    
    if (!config->use_entire_disk) {
        /* Manual partitioning would go here */
        printf("\nManual partitioning is not yet implemented.\n");
        printf("Using automatic partitioning instead.\n");
        config->use_entire_disk = 1;
        ui_wait_for_enter();
    }
    
    return UI_RESULT_NEXT;
}

/*
 * Show installation confirmation screen
 */
int ui_show_confirmation(installer_state_t *state)
{
    char size_str[32];
    
    ui_clear_screen();
    ui_print_title("Confirm Installation");
    
    printf("Please review the following settings:\n\n");
    
    printf("  Target disk:        %s\n", state->target_disk->device_name);
    format_size(state->target_disk->size_bytes, size_str, sizeof(size_str));
    printf("  Disk size:          %s\n", size_str);
    printf("  Partition table:    %s\n", 
           state->partition_config.ptable_type == PTABLE_TYPE_GPT ? "GPT" : "MBR");
    printf("  Root filesystem:    %s\n", 
           fs_type_to_string(state->partition_config.root_fs));
    
    if (state->partition_config.create_swap) {
        printf("  Swap partition:     %llu MB\n", 
               (unsigned long long)state->partition_config.swap_size_mb);
    } else {
        printf("  Swap partition:     No\n");
    }
    
    printf("\n");
    printf(ANSI_RED ANSI_BOLD "WARNING:" ANSI_RESET " This will " 
           ANSI_RED "ERASE ALL DATA" ANSI_RESET " on %s!\n\n",
           state->target_disk->device_name);
    
    const char *options[] = {
        "Yes, proceed with installation",
        "No, go back"
    };
    
    int choice = ui_menu_select("Are you sure you want to continue?", options, 2);
    
    if (choice == 1) {
        return UI_RESULT_BACK;
    }
    
    return UI_RESULT_NEXT;
}

/*
 * Show installation progress
 */
int ui_show_progress(const char *message, int percent)
{
    static int last_percent = -1;
    
    if (percent != last_percent) {
        printf("\r[");
        
        int bar_width = 50;
        int filled = (percent * bar_width) / 100;
        
        for (int i = 0; i < bar_width; i++) {
            if (i < filled) {
                cnputc('=');
            } else if (i == filled) {
                cnputc('>');
            } else {
                cnputc(' ');
            }
        }
        
        printf("] %3d%% %s", percent, message ? message : "");
        
        last_percent = percent;
    }
    
    if (percent >= 100) {
        printf("\n");
    }
    
    return 0;
}

/*
 * Show installation completion screen
 */
int ui_show_completion(void)
{
    ui_clear_screen();
    ui_print_title("Installation Complete!");
    
    printf(ANSI_GREEN "c9o-mach has been successfully installed.\n\n" ANSI_RESET);
    
    printf("To boot into your new system:\n\n");
    printf("  1. Remove the installation media\n");
    printf("  2. Reboot your computer\n");
    printf("  3. Select c9o-mach from the boot menu\n");
    printf("\n");
    
    printf("For more information and documentation, visit:\n");
    printf("  https://github.com/cogpy/c9o-mach\n");
    printf("\n");
    
    const char *options[] = {
        "Reboot now",
        "Continue to live environment"
    };
    
    int choice = ui_menu_select("What would you like to do?", options, 2);
    
    if (choice == 0) {
        printf("\nRebooting...\n");
        /* host_reboot() would be called here */
    }
    
    return UI_RESULT_NEXT;
}

/*
 * Show error message
 */
void ui_show_error(const char *title, const char *message)
{
    printf("\n" ANSI_RED ANSI_BOLD "Error: %s" ANSI_RESET "\n", title);
    
    if (message != NULL) {
        printf("%s\n", message);
    }
    
    ui_wait_for_enter();
}

/*
 * Prompt for yes/no answer
 */
int ui_prompt_yes_no(const char *question)
{
    printf("%s (y/n): ", question);
    
    /* In real implementation, read user input */
    /* For now, return yes */
    printf("y\n");
    
    return 1;  /* yes */
}

/*
 * Prompt for string input
 */
int ui_prompt_string(const char *prompt, char *buffer, size_t bufsize)
{
    if (buffer == NULL || bufsize == 0)
        return -1;
    
    printf("%s: ", prompt);
    
    /* In real implementation, read from console */
    buffer[0] = '\0';
    
    return 0;
}
