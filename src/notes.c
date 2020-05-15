#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_TO_CONFIG "/.config/notes/"
#define CONFIG_FILENAME "config"

#define STORAGE_FN_SIZE 100

// TODO Get the storage location // Done
// TODO Build the storage location // Done
// TODO Add comments to functions

void open_config(void);
void create_config(char *home_str, char *dir_path, char *complete_path);
void terminate(const char *fmt, ...);
void print_storage(FILE *fp);
void build_storage(FILE *fp);
char *get_storage_name(FILE *fp);

int main(void) {
    open_config();
    return 0;
}

void open_config(void) {
    FILE *fp;
    char *home_str, *dir_path, *complete_path;

    home_str = getenv("HOME");

    complete_path = malloc(strlen(home_str) + strlen(PATH_TO_CONFIG) + strlen(CONFIG_FILENAME) + 1);
    if (complete_path == NULL)
        terminate("%s", "malloc failed in open_config.\n");

    strcpy(complete_path, home_str);
    strcat(complete_path, PATH_TO_CONFIG);

    dir_path = strdup(complete_path);
    if (dir_path == NULL)
        terminate("%s", "strdup failed in open_config.\n");

    strcat(complete_path, CONFIG_FILENAME);

    fp = fopen(complete_path, "r");
    if (fp == NULL) {
        // file not found. Create it and add the default location
        create_config(home_str, dir_path, complete_path);

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
    build_storage(fp);
    fclose(fp);

    free(dir_path);
    free(complete_path);
}

void create_config(char *home_str, char *dir_path, char *complete_path) {
    FILE *fp;
    char *default_dir = "/notes";
    char *storage_dir;

    storage_dir = malloc(strlen(home_str) + strlen(default_dir) + 1);
    if (storage_dir == NULL)
        terminate("%s", "malloc failed in create_config.\n");
    strcpy(storage_dir, home_str);
    strcat(storage_dir, default_dir);

    struct stat st = {0};

    if (stat(dir_path, &st) == -1)
        if (mkdir(dir_path, 0700) != 0)
            terminate("%s", "config folder could not be created\n");

    fp = fopen(complete_path, "w");
    if (fp == NULL)
        terminate("%s", "Config file could not be created.\n");

    fprintf(fp, "%s", "# If you want to change the storage location, you can do it from here.\n");
    fprintf(fp, "%s", storage_dir);

    fclose(fp);
    free(storage_dir);
}

void print_storage(FILE *fp) {
    int ch;

    printf("printing file\n");

    while ((ch = fgetc(fp)) != EOF)
        putchar(ch);

    printf("\n");
}

void build_storage(FILE *fp) {
    char *storage_folder_name;

    fseek(fp, 0L, SEEK_SET);
    storage_folder_name = get_storage_name(fp);

    struct stat st = {0};

    if (stat(storage_folder_name, &st) == -1)
        if (mkdir(storage_folder_name, 0700) != 0)
            terminate("%s", "storage folder could not be created\n");

    printf("%s", "storage folder created\n");
}

char *get_storage_name(FILE *fp) {
    int ch, i = 0;
    int size = STORAGE_FN_SIZE;
    char *buffer;
    bool skip_line = false;
    bool reading_started = false;

    buffer = malloc(STORAGE_FN_SIZE);
    if (buffer == NULL)
        terminate("%s", "malloc failed in get_storage_name.\n");

    while ((ch = fgetc(fp)) != EOF) {
        if (skip_line && ch == '\n') {
            skip_line = false;
            continue;
        }

        if (skip_line && ch != '\n')
            continue;

        if (ch == '#' || ch == '\n') {
            skip_line = true;
            continue;
        }

        if (isspace(ch) && !reading_started)
            continue;

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
    printf("storage name: %s\n", buffer);

    return buffer;
}

void terminate(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

