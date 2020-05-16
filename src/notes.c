#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"
#include "utilities.h"

// TODO Get the storage location // Done
// TODO Build the storage location // Done
// TODO Add comments to functions
// TODO Split the functions into files
// TODO Get editor name from environment

static char *write_note(char *storage_path);
static void store_note(char *tmpf_path, char *storage_path, char *filename);
static void cleanup(char *tmpf_path, char *storage_path);

int main(int argc, char *argv[]) {
    char *storage_path, *tmpf_path, *filename = "temp.txt";

    if (argc == 2)
        filename = argv[1];

    storage_path = read_config();
    tmpf_path = write_note(storage_path);
    store_note(tmpf_path, storage_path, filename);

    cleanup(tmpf_path, storage_path);

    return 0;
}

static char *write_note(char *storage_path) {
    char *tmp_fn, *tmpf_path, *command, *editor = "vim";;

    tmp_fn = tmpnam(NULL);

    tmpf_path = malloc(strlen(storage_path) + strlen(tmp_fn) + 1);
    if (tmpf_path == NULL)
        terminate("%s", "malloc failure in write_note.\n");

    strcpy(tmpf_path, storage_path);
    strcat(tmpf_path, tmp_fn + 5);

    command = malloc(strlen(editor) + 1 + strlen(storage_path) + strlen(tmp_fn) + 1);
    if (command == NULL)
        terminate("%s", "malloc failure in write_note.\n");
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    system(command);

    free(command);

    return tmpf_path;
}

static void store_note(char *tmpf_path, char *storage_path, char *filename) {
    FILE *read_file, *write_file;
    char *notef_path, date_buffer[100];
    int ch;
    time_t current;
    struct tm *t;

    read_file = fopen(tmpf_path, "r");
    if (read_file == NULL)
        terminate("%s", "Error: not able to open temp file for reading.\n");

    notef_path = malloc(strlen(storage_path) + strlen(filename) + 1);
    if (notef_path == NULL)
        terminate("%s", "malloc failure in store_note.\n");
    strcpy(notef_path, storage_path);
    strcat(notef_path, filename);

    write_file = fopen(notef_path, "a");
    if (write_file == NULL)
        terminate("%s", "Error: not able to open temp file for reading.\n");

    current = time(NULL);
    t = localtime(&current);

    strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
    fprintf(write_file, "%s\n", date_buffer);

    while ((ch = fgetc(read_file)) != EOF)
        fputc(ch, write_file);

    fprintf(write_file, "\n\n");

    fclose(read_file);
    fclose(write_file);
    free(notef_path);
}

static void cleanup(char *tmpf_path, char *storage_path) {
    remove(tmpf_path);
    free(tmpf_path);
    free(storage_path);
}

