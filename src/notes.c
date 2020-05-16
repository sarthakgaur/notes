#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_TO_CONFIG "/.config/notes/"
#define CONFIG_FILENAME "config"

#define STORAGE_FN_SIZE 100

// TODO Get the storage location // Done
// TODO Build the storage location // Done
// TODO Add comments to functions

char *write_note(char *storage_path);
void store_note(char *tmp_path, char *storage_path, char *filename);

char *open_config(void);
void create_config(char *home_str, char *dir_path, char *complete_path);
void terminate(const char *fmt, ...);
char *build_storage(FILE *fp);
char *get_storage_name(FILE *fp);

int main(int argc, char *argv[]) {
    char *storage_path, *tmp_path;
    char *filename = "temp.txt";

    if (argc > 1)
        filename = argv[1];

    storage_path = open_config();
    tmp_path = write_note(storage_path);
    store_note(tmp_path, storage_path, filename);

    free(storage_path);
    free(tmp_path);
    return 0;
}

char *write_note(char *storage_path) {
    char *tmp_fn, *tmp_path, *command;
    char *editor = "vim";

    tmp_fn = tmpnam(NULL);

    tmp_path = malloc(strlen(storage_path) + strlen(tmp_fn) + 1);
    if (tmp_path == NULL)
        terminate("%s", "malloc failure in write_note.\n");

    strcpy(tmp_path, storage_path);
    strcat(tmp_path, tmp_fn + 5);

    command = malloc(strlen(editor) + 1 + strlen(storage_path) + strlen(tmp_fn) + 1);
    if (command == NULL)
        terminate("%s", "malloc failure in write_note.\n");

    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmp_path);

    system(command);

    free(command);

    return tmp_path;
}

void store_note(char *tmp_path, char *storage_path, char *filename) {
    FILE *read_file, *write_file;
    char *complete_path;
    char date_buffer[100];
    int ch;
    time_t current;
    struct tm *t;

    read_file = fopen(tmp_path, "r");
    if (read_file == NULL)
        terminate("%s", "Error: not able to open temp file for reading.\n");

    complete_path = malloc(strlen(storage_path) + strlen(filename) + 1);
    if (complete_path == NULL)
        terminate("%s", "malloc failure in store_note.\n");

    strcpy(complete_path, storage_path);
    strcat(complete_path, filename);

    write_file = fopen(complete_path, "a");
    if (write_file == NULL)
        terminate("%s", "Error: not able to open temp file for reading.\n");

    current = time(NULL);
    t = localtime(&current);

    strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
    fprintf(write_file, "%s\n", date_buffer);

    while ((ch = fgetc(read_file)) != EOF)
        fputc(ch, write_file);

    fprintf(write_file, "\n\n");

    remove(tmp_path);
    fclose(read_file);
    fclose(write_file);
    free(complete_path);
}

char *open_config(void) {
    FILE *fp;
    char *home_str, *dir_path, *complete_path;
    char *storage_path;

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
    } 

    storage_path = build_storage(fp);

    fclose(fp);
    free(dir_path);
    free(complete_path);

    return storage_path;
}

void create_config(char *home_str, char *dir_path, char *complete_path) {
    FILE *fp;
    char *default_dir = "/notes/";
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
    fprintf(fp, "%s\n", storage_dir);

    fclose(fp);
    free(storage_dir);
}

char *build_storage(FILE *fp) {
    char *storage_folder_name;

    fseek(fp, 0L, SEEK_SET);
    storage_folder_name = get_storage_name(fp);

    struct stat st = {0};

    if (stat(storage_folder_name, &st) == -1)
        if (mkdir(storage_folder_name, 0700) != 0)
            terminate("%s", "storage folder could not be created\n");

    return storage_folder_name;
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

void terminate(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

