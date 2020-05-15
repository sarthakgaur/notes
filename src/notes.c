#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_TO_CONFIG "/.config/notes/"
#define CONFIG_FILENAME "config"

// TODO Get the storage location

void open_config(void);
void create_config(char *dir_path, char *complete_path);
void terminate(const char *fmt, ...);
void print_storage(FILE *fp);

int main(void) {
    open_config();
    return 0;
}

void open_config(void) {
    FILE *fp;
    char *home_str, *dir_path, *complete_path;

    home_str = getenv("HOME");

    complete_path = malloc(strlen(home_str) + strlen(PATH_TO_CONFIG) + strlen(CONFIG_FILENAME) + 1);
    if (complete_path == NULL) {
        terminate("%s", "malloc failed in open_config.\n");
    }
    strcpy(complete_path, home_str);
    strcat(complete_path, PATH_TO_CONFIG);

    dir_path = strdup(complete_path);
    if (dir_path == NULL) {
        terminate("%s", "strdup failed in open_config.\n");
    }

    strcat(complete_path, CONFIG_FILENAME);

    fp = fopen(complete_path, "r");
    if (fp == NULL) {
        // file not found. Create it and add the default location
        create_config(dir_path, complete_path);

        fp = fopen(complete_path, "r");
        if (fp == NULL) {
            // second attempt failed
            terminate("%s", "Config file could not be read after creating.\n");
        }

        printf("2nd attempt success\n");

    } else {
        printf("1st attempt success\n");
    }

    print_storage(fp);
    fclose(fp);

    free(dir_path);
    free(complete_path);
}

void create_config(char *dir_path, char *complete_path) {
    FILE *fp;
    char *default_dir = "/notes";

    struct stat st = {0};

    if (stat(dir_path, &st) == -1) {
        mkdir(dir_path, 0700);
    }

    fp = fopen(complete_path, "w");
    if (fp == NULL) {
        // Critical error. Terminate the program
        terminate("%s", "Config file could not be created.\n");
    }

    fprintf(fp, "%s", default_dir);

    fclose(fp);
}

void terminate(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void print_storage(FILE *fp) {
    int ch;

    printf("printing file\n");

    while ((ch = fgetc(fp)) != EOF)
        putchar(ch);

    printf("\n");
}

