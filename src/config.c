#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "utilities.h"

#define PATH_TO_CONFIG "/.config/notes/"
#define CONFIG_FILENAME "config"

#define STORAGE_FN_SIZE 100

static void create_config(char *home_str, char *dir_path, char *config_fn);
static char *build_storage(FILE *config_fp);
static char *get_stgnm(FILE *config_fp);

char *read_config(void) {
    FILE *config_fp;
    char *home_str, *dir_path, *config_fn, *storage_path;

    home_str = getenv("HOME");

    config_fn = malloc(strlen(home_str) + strlen(PATH_TO_CONFIG) + strlen(CONFIG_FILENAME) + 1);
    if (config_fn == NULL)
        terminate("%s", "malloc failed in open_config.\n");

    strcpy(config_fn, home_str);
    strcat(config_fn, PATH_TO_CONFIG);

    dir_path = strdup(config_fn);
    if (dir_path == NULL)
        terminate("%s", "strdup failed in open_config.\n");

    strcat(config_fn, CONFIG_FILENAME);

    config_fp = fopen(config_fn, "r");
    if (config_fp == NULL) {
        create_config(home_str, dir_path, config_fn);

        config_fp = fopen(config_fn, "r");
        if (config_fp == NULL)
            terminate("%s", "Config file could not be read after creating.\n");
    } 

    storage_path = build_storage(config_fp);

    fclose(config_fp);
    free(dir_path);
    free(config_fn);

    return storage_path;
}

static void create_config(char *home_str, char *dir_path, char *config_fn) {
    FILE *config_fp;
    char *storage_dir, *default_dir = "/notes/";

    storage_dir = malloc(strlen(home_str) + strlen(default_dir) + 1);
    if (storage_dir == NULL)
        terminate("%s", "malloc failed in create_config.\n");
    strcpy(storage_dir, home_str);
    strcat(storage_dir, default_dir);

    struct stat st = {0};

    if (stat(dir_path, &st) == -1)
        if (mkdir(dir_path, 0700) != 0)
            terminate("%s", "config folder could not be created\n");

    config_fp = fopen(config_fn, "w");
    if (config_fp == NULL)
        terminate("%s", "Config file could not be created.\n");

    fprintf(config_fp, "%s", "# If you want to change the storage location, you can do it from here.\n");
    fprintf(config_fp, "%s\n", storage_dir);

    fclose(config_fp);
    free(storage_dir);
}

static char *build_storage(FILE *config_fp) {
    char *storage_dn;

    storage_dn = get_stgnm(config_fp);

    struct stat st = {0};

    if (stat(storage_dn, &st) == -1)
        if (mkdir(storage_dn, 0700) != 0)
            terminate("%s", "storage folder could not be created\n");

    return storage_dn;
}

static char *get_stgnm(FILE *config_fp) {
    char *buffer;
    int ch, i = 0, size = STORAGE_FN_SIZE;
    bool skip_line = false, reading_started = false;

    fseek(config_fp, 0L, SEEK_SET);

    buffer = malloc(size);
    if (buffer == NULL)
        terminate("%s", "malloc failed in get_storage_name.\n");

    while ((ch = fgetc(config_fp)) != EOF) {
        if (skip_line && ch == '\n') {
            skip_line = false;
            continue;
        }

        if (skip_line && ch != '\n')
            continue;

        if (ch == '#') {
            skip_line = true;
            continue;
        }

        if (!reading_started && isspace(ch))
            continue;

        if (reading_started && ch == '\n')
            break;

        if (i < size) {
            reading_started = true;
            buffer[i++] = ch;

            if (i == size) {
                size *= 2;
                buffer = realloc(buffer, size);
                if (buffer == NULL)
                    terminate("%s", "realloc failed in get_storage_name.\n");
            }
        }
    }

    buffer[i] = '\0';

    return buffer;
}

