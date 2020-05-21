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
// TODO Add an help option that lists all the options. // Done
// TODO Add a basic templating system. // Done
// TODO Refactor argument passing and controller function.
// TODO Add an option to create template.
// TODO Add option to delete a note.
// TODO Improve error messages.
// TODO Add comments to functions.

enum request_type {
    NO_REQUEST,
    QUICK_WRITE,
    WRITE_NOTE,
    READ_NOTE,
    LIST_NOTES_FILES,
    USE_TEMPLATE,
    PRINT_HELP,
};

enum note_source {
    NO_SOURCE,
    FROM_CMDLINE,
    FROM_STDIN,
    FROM_FILE,
    FROM_TEMPLATE,
};

struct request {
    enum request_type rt;
    char *filename;
    char *template_filename;
    char *buffer;
};

struct note {
    enum note_source ns;
    bool write_error;
    char *storage_path;
    char *filename;
    char *template_filename;
    char *tmpf_path;
    char *buffer;
};

static void controller(struct request *req);
static void write_handler(struct note *stn);
static void read_note(struct note *stn);
static void list_notes_files(struct note *stn);
static void write_note(struct note *stn);
static void write_template(struct note *stn);
static void check_write(struct note *stn);
static char *read_stdin(void);
static void store_note(struct note *stn);
static void print_help();
static void cleanup(struct note *stn);

int main(int argc, char *argv[]) {
    struct request req;

    req.filename = "temp.txt";
    req.template_filename = "template.txt";

    if (argc == 2) {
        if (strcmp(argv[1], "-e") == 0) {
            req.rt = READ_NOTE;
        } else if (strcmp(argv[1], "-l") == 0) {
            req.rt = LIST_NOTES_FILES;
        } else if (strcmp(argv[1], "-h") == 0) { 
            req.rt = PRINT_HELP;
        } else if (strcmp(argv[1], "-t") == 0) {
            req.rt = USE_TEMPLATE;
        } else {
            req.rt = WRITE_NOTE;
            req.filename = argv[1];
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "-e") == 0) {
            req.rt = READ_NOTE;
            req.filename = argv[2];
        } else if (strcmp(argv[1], "-n") == 0) {
            req.rt = QUICK_WRITE;
            req.buffer = argv[2];
        } else if (strcmp(argv[2], "-t") == 0) {
            req.rt = USE_TEMPLATE;
            req.filename = argv[1];
        }  else {
            req.rt = PRINT_HELP;
        }
    } else if (argc == 4) {
        if (strcmp(argv[2], "-t") == 0) {
            req.rt = USE_TEMPLATE;
            req.filename = argv[1];
            req.template_filename = argv[3];
        } else if (strcmp(argv[2], "-n") == 0) {
            req.rt = QUICK_WRITE;
            req.filename = argv[1];
            req.buffer = argv[3];
        } else {
            req.rt = PRINT_HELP;
        }
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
    stn.template_filename = req->template_filename;
    stn.buffer = req->buffer;

    switch (req->rt) {
        case QUICK_WRITE:
            stn.ns = FROM_CMDLINE;
            write_handler(&stn);
            break;
        case WRITE_NOTE:
            write_handler(&stn);
            break;
        case USE_TEMPLATE:
            stn.ns = FROM_TEMPLATE;
            write_handler(&stn);
            break;
        case READ_NOTE:
            read_note(&stn);
            break;
        case LIST_NOTES_FILES:
            list_notes_files(&stn);
            break;
        case PRINT_HELP:
            print_help();
            break;
        default:
            break;
    }

    cleanup(&stn);
}

static void write_handler(struct note *stn) {
    switch (stn->ns) {
        case NO_SOURCE:
            write_note(stn);
            if (stn->ns == FROM_STDIN)
                stn->buffer = read_stdin();
            break;
        case FROM_TEMPLATE:
            write_template(stn);
            break;
        default:
            break;
    }

    check_write(stn);
    if (!stn->write_error)
        store_note(stn);
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

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        stn->ns = FROM_STDIN;
        stn->tmpf_path = NULL;
        stn->buffer = NULL;
        return;
    }

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    tmp_fn = tmpnam(NULL);
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

static void write_template(struct note *stn) {
    FILE *read_file, *write_file;
    char *template_fpath;
    char *template_dir = "templates/", *tmp_fn, *tmp_str, *editor, *tmpf_path, *command;
    int ch, template_fpath_size;

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL)
        terminate("%s", "$EDITOR environment variable is required for template operation.\n");

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    template_fpath_size = strlen(stn->storage_path) + strlen(template_dir) + strlen(stn->template_filename) + 1;
    template_fpath = malloc_wppr(template_fpath_size, __func__);
    strcpy(template_fpath, stn->storage_path);
    strcat(template_fpath, template_dir);
    strcat(template_fpath, stn->template_filename);

    tmp_fn = tmpnam(NULL);
    tmpf_path = malloc_wppr(strlen(stn->storage_path) + strlen(tmp_fn) + 1, __func__);
    strcpy(tmpf_path, stn->storage_path);
    strcat(tmpf_path, tmp_fn + TMP_NM_SLICE);

    read_file = fopen(template_fpath, "r");
    if (read_file == NULL)
        terminate("%s", "Error: template file could not be opened.\n");

    write_file = fopen(tmpf_path, "w");
    if (write_file == NULL)
        terminate("%s", "Error: temp file could not be opened for writing.\n");

    while ((ch = fgetc(read_file)) != EOF)
        fputc(ch, write_file);

    fclose(write_file);
    fclose(read_file);

    command = malloc_wppr(strlen(editor) + strlen(tmpf_path) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    system(command);

    free(editor);
    free(template_fpath);
    free(command);

    stn->tmpf_path = tmpf_path;
}

static void check_write(struct note *stn) {
    FILE *read_file;
    int ch, i = 0;

    stn->write_error = true;

    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
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
    } else if (stn->ns == FROM_STDIN || stn->ns == FROM_CMDLINE) {
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

    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
        read_file = fopen(stn->tmpf_path, "r");
        if (read_file == NULL)
            terminate("%s", "Error: not able to open temp file for reading.\n");

        while ((ch = fgetc(read_file)) != EOF)
            fputc(ch, write_file);

        fclose(read_file);
    } else if (stn->ns == FROM_STDIN || stn->ns == FROM_CMDLINE) {
        while ((ch = stn->buffer[i++]) != '\0')
            fputc(ch, write_file);
        fputc('\n', write_file);
    }

    fprintf(write_file, "\n");

    fclose(write_file);
    free(notef_path);
}

static void print_help() {
    char *help_msg = 
        "The following options are supported:\n\n"
        "filename:           Append a note to the file specified.\n"
        "-e filename:        Open the file for editing. If filename is missing, temp file is opened.\n" 
        "-l:                 List all the notes files in the notes directory.\n"
        "filename -n \"note\": Add the note to the file. If filename is missing, temp file is used.\n";

    printf("%s", help_msg);
}

static void cleanup(struct note *stn) {
    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
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

