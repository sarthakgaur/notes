#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "utilities.h"
#include "cache.h"

static void build_cache(char *cache_path);
static int read_cache(struct config *conf, char *cache_path);
static int is_config_updated(time_t config_mtime, time_t cache_mtime);

int cache_controller(struct config *conf, char *home_str, char *config_path, char *cache_path) {
    struct stat attr;
    int read_status;
    time_t config_mtime, cache_mtime;

    build_cache(home_str);
    read_status = read_cache(conf, cache_path);

    if (read_status == 1) {
        return 1;
    }
    
    stat(config_path, &attr);
    config_mtime = attr.st_mtime;
    cache_mtime = conf->cmod_time;

    return is_config_updated(config_mtime, cache_mtime);
}

void write_cache(struct config *conf, char *cache_path) {
    FILE *config_fptr;

    config_fptr = fopen(cache_path, "wb");
    if (config_fptr == NULL) {
        terminate("%s", "Error: Cannot open cache file for writing.\n");
    }

    fwrite(conf, sizeof(struct config), 1, config_fptr);
    fclose(config_fptr);
}

static void build_cache(char *home_str) {
    struct stat st = {0};
    char *cache_dir_path, *cache_path = "/.cache/notes/";
    int cache_path_len;

    // Build cache dir path
    cache_path_len = strlen(home_str) + strlen(cache_path) + 1;
    cache_dir_path = malloc_wppr(cache_path_len, __func__);
    strcpy(cache_dir_path, home_str);
    strcat(cache_dir_path, cache_path);

    if (stat(cache_dir_path, &st) == -1) {
        if (mkdir(cache_dir_path, 0700) != 0) {
            terminate("%s", "Error: cache folder could not be created\n");
        }
    }

    free(cache_dir_path);
}

static int read_cache(struct config *conf, char *cache_path) {
    FILE *config_fptr;

    config_fptr = fopen(cache_path, "rb");
    if (config_fptr == NULL) {
        return 1;
    }

    fread(conf, sizeof(struct config), 1, config_fptr);
    fclose(config_fptr);

    return 0;
}

static int is_config_updated(time_t config_mtime, time_t cache_mtime) {
    double time_diff;

    time_diff = difftime(config_mtime, cache_mtime);

    return time_diff == 0.00f ? 0 : 1;
}

