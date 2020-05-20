#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <dirent.h>

#include "config.h"
#include "utilities.h"

#define SIZE_HUN 100
#define TMP_NM_SLICE 5

// TODO Get the storage location. // Done
// TODO Build the storage location. // Done
// TODO Split the functions into files. // Done
// TODO Get editor name from environment. // Done
// TODO Add malloc wrapper function. // Done
// TODO Get rid of config files and use the environment for location. // Done
// TODO Add reading and editing options for notes. // Done
// TODO If no note is added terminate the program. // Done
// TODO Temp file after check_write terminates are not deleted. Need fix. // Done
// TODO Refactoring required so that only cleanup frees the resources. // Done
// TODO Add support to view files stored in the notes dir. // Done
// TODO Need to refactor the calls of free and cleanup. // Done
// TODO Improve function signatures. // Done
// TODO Add option to write the note from the command line. // Done
// TODO Improve error messages.
// TODO Add comments to functions.

enum request_type {
    QUICK_WRITE,
    WRITE_NOTE,
    READ_TMP_FILE,
    READ_SPECIFIED_FILE,
    LIST_NOTES_FILES,
};

enum note_source {
    FROM_CMDLINE,
    FROM_STDIN,
    FROM_FILE,
    NO_SOURCE,
};

struct request {
    enum request_type rt;
    char *filename;
    char *buffer;
};

struct note {
    enum note_source ns;
    bool write_error;
    char *storage_path;
    char *filename;
    char *tmpf_path;
    char *buffer;
};

static void controller(struct request *req);
static void read_note(struct note *stn);
static void list_notes_files(struct note *stn);
static void write_note(struct note *stn);
static void check_write(struct note *stn);
static char *read_stdin(void);
static void store_note(struct note *stn);
static void cleanup(struct note *stn);

int main(int argc, char *argv[]) {
    struct request req;

    req.filename = "temp.txt";

    if (argc == 2) {
        if (strcmp(argv[1], "-e") == 0) {
            req.rt = READ_TMP_FILE;
        } else if (strcmp(argv[1], "-l") == 0) {
            req.rt = LIST_NOTES_FILES;
        } else {
            req.rt = WRITE_NOTE;
            req.filename = argv[1];
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "-e") == 0) {
            req.rt = READ_SPECIFIED_FILE;
            req.filename = argv[2];
        } else if (strcmp(argv[1], "-n") == 0) {
            req.rt = QUICK_WRITE;
            req.buffer = argv[2];
        } else {
            terminate("%s", "Invalid command\n");
        }
    } else if (argc == 4 && strcmp(argv[2], "-n") == 0) {
        req.rt = QUICK_WRITE;
        req.filename = argv[1];
        req.buffer = argv[3];
    } else if (argc == 1) {
        req.rt = WRITE_NOTE;
    } else {
        terminate("%s", "Invalid command\n");
    }

    controller(&req);

    return 0;
}

static void controller(struct request *req) {
    struct note stn; 

    stn.ns = NO_SOURCE;
    stn.storage_path = read_config();
    stn.filename = req->filename;

    switch (req->rt) {
        case QUICK_WRITE:
            stn.buffer = req->buffer;
            stn.ns = FROM_CMDLINE;
            store_note(&stn);
            break;
        case WRITE_NOTE:
            write_note(&stn);
            if (stn.ns == FROM_STDIN)
                stn.buffer = read_stdin();
            check_write(&stn);
            if (!stn.write_error)
                store_note(&stn);
            break;
        case READ_TMP_FILE:
        case READ_SPECIFIED_FILE:
            read_note(&stn);
            break;
        case LIST_NOTES_FILES:
            list_notes_files(&stn);
            break;
    }

    cleanup(&stn);
}

static void read_note(struct note *stn) {
    FILE *note_fp;
    char *editor, *command, *note_fn;
    int ch;

    note_fn = malloc_wppr(strlen(stn->storage_path) + strlen(stn->filename) + 1, __func__);
    strcpy(note_fn, stn->storage_path);
    strcat(note_fn, stn->filename);

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

static void list_notes_files(struct note *stn) {
    DIR *dp;
    struct dirent *ep;
    dp = opendir(stn->storage_path);

    if (dp == NULL)
        perror("Storage dir could not be opened.\n");
    else {
        while ((ep = readdir(dp)))
            if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0)
                puts(ep->d_name);

        closedir(dp);
    }
}

static void write_note(struct note *stn) {
    char *tmp_fn, *tmpf_path, *command, *editor, *tmp_str;

    tmp_fn = tmpnam(NULL);

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        stn->ns = FROM_STDIN;
        stn->tmpf_path = NULL;
        stn->buffer = NULL;
        return;
    }

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    tmpf_path = malloc_wppr(strlen(stn->storage_path) + strlen(tmp_fn) + 1, __func__);
    strcpy(tmpf_path, stn->storage_path);
    strcat(tmpf_path, tmp_fn + TMP_NM_SLICE);

    command = malloc_wppr(strlen(editor) + strlen(tmpf_path) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    system(command);

    free(editor);
    free(command);

    stn->ns = FROM_FILE;
    stn->tmpf_path = tmpf_path;
    stn->buffer = NULL;
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

static void check_write(struct note *stn) {
    FILE *read_file;
    int ch, i = 0;

    if (stn->ns == FROM_FILE) {
        read_file = fopen(stn->tmpf_path, "r");
        if (read_file == NULL) {
            stn->write_error = true;
            return;
        }

        while ((ch = fgetc(read_file)) != EOF)
            if (!isspace(ch)) {
                stn->write_error = false;
                break;
            }

        fclose(read_file);
    } else if (stn->ns == FROM_STDIN) {
        while ((ch = stn->buffer[i++]) != '\0') {
            if (!isspace(ch)) {
                stn->write_error = false;
                break;
            }
        }
    }
}

static void store_note(struct note *stn) {
    FILE *read_file, *write_file;
    char *notef_path, date_buffer[100];
    int ch, i = 0;
    time_t current;
    struct tm *t;

    notef_path = malloc_wppr(strlen(stn->storage_path) + strlen(stn->filename) + 1, __func__);
    strcpy(notef_path, stn->storage_path);
    strcat(notef_path, stn->filename);

    write_file = fopen(notef_path, "a");
    if (write_file == NULL)
        terminate("%s", "Error: not able to open note file for writing.\n");

    current = time(NULL);
    t = localtime(&current);

    strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
    fprintf(write_file, "%s\n", date_buffer);

    if (stn->ns == FROM_FILE) {
        read_file = fopen(stn->tmpf_path, "r");
        if (read_file == NULL)
            terminate("%s", "Error: not able to open temp file for reading.\n");

        while ((ch = fgetc(read_file)) != EOF)
            fputc(ch, write_file);

        fclose(read_file);
    } else if (stn->ns == FROM_STDIN || stn->ns == FROM_CMDLINE) {
        while ((ch = stn->buffer[i++]) != '\0')
            fputc(ch, write_file);
    }

    fprintf(write_file, "\n");

    fclose(write_file);
    free(notef_path);
}

static void cleanup(struct note *stn) {
    if (stn->ns == FROM_FILE) {
        if (!stn->write_error)
            remove(stn->tmpf_path);
        free(stn->tmpf_path);
    } else if (stn->ns == FROM_STDIN) {
        free(stn->buffer);
    }

    free(stn->storage_path);

    if (stn->write_error)
        terminate("%s", "Error: write error occured.\n");
}

