#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "utilities.h"

static char *build_storage(char *home_str, char *storage_path);

char *read_config(void) {
    char *home_str, *storage_path, *tmp_str;
    int str_len;

    tmp_str = getenv("HOME");
    if (tmp_str == NULL)
        terminate("%s", "environment variable HOME not available.\n");
    home_str = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(home_str, tmp_str);

    tmp_str = getenv("NOTES_DIR");
    if (tmp_str == NULL) {
        storage_path = NULL;
    } else {
        str_len = strlen(tmp_str);
        storage_path = malloc_wppr(str_len + 2, __func__);
        strcpy(storage_path, tmp_str);
        if (storage_path[str_len - 1] != '/')
            strcat(storage_path, "/");
    }

    storage_path = build_storage(home_str, storage_path);

    free(home_str);

    return storage_path;
}

static char *build_storage(char *home_str, char *storage_path) {
    char *storage_dn = "/notes/";

    if (storage_path == NULL) {
        storage_path = malloc_wppr(strlen(home_str) + strlen(storage_dn) + 1, __func__);
        strcpy(storage_path, home_str);
        strcat(storage_path, storage_dn);
    }

    struct stat st = {0};

    if (stat(storage_path, &st) == -1)
        if (mkdir(storage_path, 0700) != 0)
            terminate("%s", "storage folder could not be created\n");

    return storage_path;
}

