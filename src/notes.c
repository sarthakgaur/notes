#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO Get the storage location

void open_config(void);
void create_config(char *filename);
void terminate(const char *fmt, ...);

int main(void) {
    open_config();
    return 0;
}

void open_config(void) {
    FILE *fp;
    char *filename = "~/.config/notes/config";

    fp = fopen(filename, "r");
    if (fp == NULL) {
        // file not found. Create it and add the default location
        create_config(filename);
    } else {
        fclose(fp);
    }
}

void create_config(char *filename) {
    FILE *fp;
    char *default_dir = "~/notes";
    char *home_str;
    char *complete_path;
    char *config_path = "/.config/notes";
    char *end_filename = "/config";

    home_str = getenv("HOME");

    complete_path = malloc(strlen(home_str) + strlen(config_path) + strlen(end_filename) + 1);
    if (complete_path == NULL) {
        terminate("%s", "malloc failed in create_config.\n");
    }
    strcpy(complete_path, home_str);
    strcat(complete_path, config_path);

    struct stat st = {0};

    if (stat(complete_path, &st) == -1) {
        mkdir(complete_path, 0700);
    }

    strcat(complete_path, end_filename);

    fp = fopen(complete_path, "w");
    if (fp == NULL) {
        // Critical error. Terminate the program
        terminate("%s", "Config file could not be created.\n");
    }

    fprintf(fp, "%s", default_dir);

    fclose(fp);
    free(complete_path);
}

void terminate(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

