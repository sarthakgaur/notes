#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#include "config.h"
#include "utilities.h"

#define SIZE_HUN 100
#define TMP_NM_SLICE 5

// TODO Add comments to functions.
// TODO Get the storage location. // Done
// TODO Build the storage location. // Done
// TODO Split the functions into files. // Done
// TODO Get editor name from environment. // Done
// TODO Add malloc wrapper function. // Done
// TODO Get rid of config files and use the environment for location. // Done
// TODO Add reading and editing options for notes. // Done
// TODO If no note is added terminate the program. // Done
// TODO Improve error messages.
// TODO Add support to view files stored in the notes dir.

enum request {
    WRITE_NOTE,
    READ_TMP_FILE,
    READ_SPECIFIED_FILE,
};

enum note_source {
    FROM_STDIN,
    FROM_FILE,
};

struct write_info {
    enum note_source ns;
    char *tmpf_path;
    char *buffer;
};

static void controller(enum request req, char *filename);
static void read_note(char *storage_path, char *filename);
static struct write_info *write_note(char *storage_path);
static void check_write(struct write_info *wi);
static char *read_stdin(void);
static void store_note(struct write_info *wi, char *storage_path, char *filename);
static void cleanup(struct write_info *wi, char *storage_path);

int main(int argc, char *argv[]) {
    char *filename = "temp.txt";
    enum request req;

    if (argc == 2) {
        if (strcmp(argv[1], "-e") == 0) {
            req = READ_TMP_FILE;
        } else {
            filename = argv[1];
            req = WRITE_NOTE;
        }
    } else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        req = READ_SPECIFIED_FILE;
        filename = argv[2];
    } else if (argc == 1) {
        req = WRITE_NOTE;
    } else
        terminate("%s", "invalid command\n");

    controller(req, filename);

    return 0;
}

static void controller(enum request req, char *filename) {
    char *storage_path;
    struct write_info *wi; 

    storage_path = read_config();

    switch (req) {
        case WRITE_NOTE:
            wi = write_note(storage_path);
            if (wi->ns == FROM_STDIN)
                wi->buffer = read_stdin();
            check_write(wi);
            store_note(wi, storage_path, filename);
            cleanup(wi, storage_path);
            break;
        case READ_TMP_FILE:
        case READ_SPECIFIED_FILE:
            read_note(storage_path, filename);
            free(storage_path);
            break;
    }
}

static void read_note(char *storage_path, char *filename) {
    FILE *note_fp;
    char *editor, *command, *note_fn;
    int ch;

    note_fn = malloc_wppr(strlen(storage_path) + strlen(filename) + 1, __func__);
    strcpy(note_fn, storage_path);
    strcat(note_fn, filename);

    editor = getenv("EDITOR");
    if (editor == NULL) {
        note_fp = fopen(note_fn, "r");
        if (note_fp == NULL)
            terminate("%s", "Error: not able to open note file for reading.\n");
        while ((ch = fgetc(note_fp)) != EOF)
            putchar(ch);
        fclose(note_fp);
    } else {
        command = malloc_wppr(strlen(editor) + strlen(note_fn) + 2, __func__);
        strcpy(command, editor);
        strcat(command, " ");
        strcat(command, note_fn);

        system(command);

        free(command);
    }

    free(note_fn);
}

static struct write_info *write_note(char *storage_path) {
    char *tmp_fn, *tmpf_path, *command, *editor, *tmp_str;
    struct write_info *wi; 

    wi = malloc_wppr(sizeof(struct write_info), __func__);

    tmp_fn = tmpnam(NULL);

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        wi->ns = FROM_STDIN;
        wi->tmpf_path = NULL;
        wi->buffer = NULL;
        return wi;
    }

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    tmpf_path = malloc_wppr(strlen(storage_path) + strlen(tmp_fn) + 1, __func__);
    strcpy(tmpf_path, storage_path);
    strcat(tmpf_path, tmp_fn + TMP_NM_SLICE);

    command = malloc_wppr(strlen(editor) + strlen(storage_path) + strlen(tmp_fn) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    system(command);

    free(editor);
    free(command);

    wi->ns = FROM_FILE;
    wi->tmpf_path = tmpf_path;
    wi->buffer = NULL;

    return wi;
}

static char *read_stdin(void) {
    char *buffer;
    int ch, i = 0, size = SIZE_HUN;

    buffer = malloc_wppr(size, __func__);

    printf("Write your note: ");

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (i < size) {
            buffer[i++] = ch;
            if (i == size - 2) {
                size *= 2;

                buffer = realloc(buffer, size);
                if (buffer == NULL)
                    terminate("%s", "realloc failed in read_stdout.\n");
            }
        }
    }

    buffer[i++] = '\n';
    buffer[i] = '\0';

    return buffer;
}

static void check_write(struct write_info *wi) {
    FILE *read_file;
    int ch, i = 0;
    bool write_valid = false;

    if (wi->ns == FROM_FILE) {
        read_file = fopen(wi->tmpf_path, "r");
        if (read_file == NULL)
            terminate("%s", "Error: not able to open temp file for reading.\n");

        while ((ch = fgetc(read_file)) != EOF)
            if (!isspace(ch)) {
                write_valid = true;
                break;
            }

        fclose(read_file);
    } else {
        while ((ch = wi->buffer[i++]) != '\0')
            if (!isspace(ch)) {
                write_valid = true;
                break;
            }
    }

    if (!write_valid)
        terminate("%s", "Error: no note was written.\n");
}

static void store_note(struct write_info *wi, char *storage_path, char *filename) {
    FILE *read_file, *write_file;
    char *notef_path, date_buffer[100];
    int ch, i = 0;
    time_t current;
    struct tm *t;

    notef_path = malloc_wppr(strlen(storage_path) + strlen(filename) + 1, __func__);
    strcpy(notef_path, storage_path);
    strcat(notef_path, filename);

    write_file = fopen(notef_path, "a");
    if (write_file == NULL)
        terminate("%s", "Error: not able to open note file for writing.\n");

    current = time(NULL);
    t = localtime(&current);

    strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
    fprintf(write_file, "%s\n", date_buffer);

    if (wi->ns == FROM_FILE) {
        read_file = fopen(wi->tmpf_path, "r");
        if (read_file == NULL)
            terminate("%s", "Error: not able to open temp file for reading.\n");

        while ((ch = fgetc(read_file)) != EOF)
            fputc(ch, write_file);

        fclose(read_file);
    } else {
        while ((ch = wi->buffer[i++]) != '\0')
            fputc(ch, write_file);
    }

    fprintf(write_file, "\n");

    fclose(write_file);
    free(notef_path);
}

static void cleanup(struct write_info *wi, char *storage_path) {
    if (wi->ns == FROM_FILE) {
        remove(wi->tmpf_path);
        free(wi->tmpf_path);
    } else {
        free(wi->buffer);
    }

    free(wi);
    free(storage_path);
}

