#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "config.h"
#include "utilities.h"

#define SIZE_HUN 100
#define TMP_NM_SLICE 5

// TODO Use getopt to parse command line arguments. // Done
// TODO Add option to list all the templates. // Done
// TODO Add option to disable date formatting.
// TODO Add a config file parser.
// TODO Support custom date formatting.
// TODO Refactor.

enum request_type {
    NO_REQUEST,
    QUICK_WRITE,
    WRITE_NOTE,
    READ_NOTE,
    USE_TEMPLATE,
    LIST_NOTES,
    LIST_TEMPLATES,
    SAVE_TEMPLATE,
    PRINT_HELP,
    PRINT_VERSION,
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

static void parse_args(struct request *req, int argc, char *argv[]);
static void controller(struct request *req);
static void write_handler(struct note *stn);
static void read_note(struct note *stn);
static void list_files(struct note *stn, int rt);
static void write_note(struct note *stn);
static void write_template(struct note *stn);
static void save_template(struct note *stn);
static void check_write(struct note *stn);
static void store_note(struct note *stn);
static char *read_stdin(void);
static void print_help(void);
static void print_version(void);
static void cleanup(struct note *stn);

int main(int argc, char *argv[]) {
    struct request req;

    parse_args(&req, argc, argv);
    controller(&req);

    return 0;
}

static void parse_args(struct request *req, int argc, char *argv[]) {
    int ch, i, index;
    bool request_set = false;
    bool request_err = false;

    req->rt = NO_REQUEST;
    req->filename = "temp.txt";
    req->template_filename = "template.txt";

    opterr = 0;

    while ((ch = getopt(argc, argv, "n:e:s:t:lLvh")) != -1) {
        if (request_set) {
            continue;
        }

        switch(ch) {
            case 'n':
                req->rt = QUICK_WRITE;
                req->buffer = optarg;
                break;
            case 'e':
                req->rt = READ_NOTE;
                req->filename = optarg;
                break;
            case 's':
                req->rt = SAVE_TEMPLATE;
                req->template_filename = optarg;
                break;
            case 't':
                req->rt = USE_TEMPLATE;
                req->template_filename = optarg;
                break;
            case 'l':
                req->rt = LIST_NOTES;
                break;
            case 'L':
                req->rt = LIST_TEMPLATES;
                break;
            case 'v':
                req->rt = PRINT_VERSION;
                break;
            case 'h':
                req->rt = PRINT_HELP;
                break;
            case '?':
                if (optopt == 'e') {
                    req->rt = READ_NOTE;
                } else if (optopt == 's') {
                    req->rt = SAVE_TEMPLATE;
                } else if (optopt == 't') {
                    req->rt = USE_TEMPLATE;
                } else if (optopt == 'n') {
                    terminate("Error: Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    terminate("Error: Unknown option '-%c'.\n", optopt);
                } else {
                    terminate("Error: Unkown option character '\\x%x'.\n", optopt);
                }
                break;
            default:
                terminate("%s", "Error: Couldn't parse the program's arguments.\n");
        }

        request_set = true;
    }

    if (req->rt == NO_REQUEST) {
        req->rt = WRITE_NOTE;
    }

    i = 0;
    for (index = optind; index < argc; index++) {
        if (i == 0) {
            req->filename = argv[index];
        } else {
            request_err = true;
        }
        i++;
    }

    switch (req->rt) {
        case LIST_NOTES:
        case LIST_TEMPLATES:
        case SAVE_TEMPLATE:
        case PRINT_HELP:
        case PRINT_VERSION:
            if (i > 0) {
                request_err = true;
            }
            break;
        default:
            break;
    }

    if (request_err) {
        terminate("%s", "Error: Invalid option or argument.\n");
    }
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
        case SAVE_TEMPLATE:
            save_template(&stn);
            break;
        case READ_NOTE:
            read_note(&stn);
            break;
        case LIST_NOTES:
        case LIST_TEMPLATES:
            list_files(&stn, req->rt);
            break;
        case PRINT_HELP:
            print_help();
            break;
        case PRINT_VERSION:
            print_version();
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
            if (stn->ns == FROM_STDIN) {
                stn->buffer = read_stdin();
            }
            break;
        case FROM_TEMPLATE:
            write_template(stn);
            break;
        default:
            break;
    }

    check_write(stn);
    if (!stn->write_error) {
        store_note(stn);
    }
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
        if (note_fp == NULL) {
            terminate("%s", "Error: not able to open note file for reading.\n");
        }
        while ((ch = fgetc(note_fp)) != EOF) {
            putchar(ch);
        }
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

static void list_files(struct note *stn, int rt) {
    DIR *dp;
    struct dirent *ep;
    char *templ_dir = "templates/";
    char *path = stn->storage_path;

    if (rt == LIST_TEMPLATES) {
        path = malloc_wppr(strlen(stn->storage_path) + strlen(templ_dir) + 1, __func__);
        strcpy(path, stn->storage_path);
        strcat(path, templ_dir);
    }

    dp = opendir(path);

    if (dp == NULL) {
        perror("Error: notes directory could not be opened.\n");
    } else {
        while ((ep = readdir(dp))) {
            if (strcmp(ep->d_name, ".") != 0
                    && strcmp(ep->d_name, "..") != 0
                    && strcmp (ep->d_name, "templates") != 0) {
                puts(ep->d_name);
            }
        }

        closedir(dp);
    }

    if (rt == LIST_TEMPLATES) {
        free(path);
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

static void write_template(struct note *stn) {
    FILE *read_file, *write_file;
    char *template_fpath, *tmp_fn, *tmp_str, *editor, *tmpf_path, *command;
    char *template_dir = "templates/";
    int ch, template_fpath_size;

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        terminate("%s", "Error: $EDITOR environment variable is required for template operation.\n");
    }

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
    if (read_file == NULL) {
        terminate("%s", "Error: template file could not be opened.\n");
    }

    write_file = fopen(tmpf_path, "w");
    if (write_file == NULL) {
        terminate("%s", "Error: temp file could not be opened for writing.\n");
    }

    while ((ch = fgetc(read_file)) != EOF) {
        fputc(ch, write_file);
    }

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

static void save_template(struct note *stn) {
    char *template_fpath, *tmp_str, *editor, *command, *template_dir = "templates/";
    int template_fpath_size;
    struct stat st = {0};

    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        terminate("%s", "Error: $EDITOR environment variable is required for template operation.\n");
    }

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    template_fpath_size = strlen(stn->storage_path) + strlen(template_dir) + strlen(stn->template_filename) + 1;
    template_fpath = malloc_wppr(template_fpath_size, __func__);
    strcpy(template_fpath, stn->storage_path);
    strcat(template_fpath, template_dir);

    if (stat(template_fpath, &st) == -1) {
        if (mkdir(template_fpath, 0700) != 0) {
            terminate("%s", "Error: template folder could not be created\n");
        }
    }

    strcat(template_fpath, stn->template_filename);

    command = malloc_wppr(strlen(editor) + strlen(template_fpath) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, template_fpath);

    system(command);

    free(editor);
    free(template_fpath);
    free(command);
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

        while ((ch = fgetc(read_file)) != EOF) {
            if (!isspace(ch)) {
                stn->write_error = false;
                break;
            }
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
    if (write_file == NULL) {
        terminate("%s", "Error: not able to open note file for writing.\n");
    }

    current = time(NULL);
    t = localtime(&current);

    strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
    fprintf(write_file, "%s\n", date_buffer);

    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
        read_file = fopen(stn->tmpf_path, "r");
        if (read_file == NULL) {
            terminate("%s", "Error: not able to open temp file for reading.\n");
        }

        while ((ch = fgetc(read_file)) != EOF) {
            fputc(ch, write_file);
        }

        fclose(read_file);
    } else if (stn->ns == FROM_STDIN || stn->ns == FROM_CMDLINE) {
        while ((ch = stn->buffer[i++]) != '\0') {
            fputc(ch, write_file);
        }
        fputc('\n', write_file);
    }

    fprintf(write_file, "\n");

    fclose(write_file);
    free(notef_path);
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
                if (buffer == NULL) {
                    terminate("%s", "Error: realloc failed in read_stdout.\n");
                }
            }
        }
    }

    buffer[i++] = '\n';
    buffer[i] = '\0';

    return buffer;
}

static void print_help(void) {
    char *help_msg = 
        "The following options are supported:\n\n"
        "filename:             Append a note to the file specified.\n"
        "-e filename:          Open the file for editing. If filename is missing, temp file is opened.\n" 
        "-l:                   List all the notes files in the notes directory.\n"
        "-L:                   List all the templates in the templates directory.\n"
        "filename -n \"note\":   Add the note to the file. If filename is missing, temp file is used.\n"
        "-s template_name:     Create or update a template file in templates directory. If no file is\n"
        "                      Specified template.txt is opened.\n"
        "filename -t template: Add a note using a template. If filename is missing, temp file is used.\n"
        "                      If template name is not provided, \"templates/template.txt\" is used.\n"
        "-v:                   Print version number.\n"
        "-h:                   Print this help message.\n";

    printf("%s", help_msg);
}

static void print_version(void) {
    printf("%s\n", "Notes 0.1.6");
}

static void cleanup(struct note *stn) {
    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
        if (!stn->write_error) {
            remove(stn->tmpf_path);
        }
        free(stn->tmpf_path);
    } else if (stn->ns == FROM_STDIN) {
        free(stn->buffer);
    }

    free(stn->storage_path);

    if (stn->write_error) {
        terminate("%s", "Error: write error occured.\n");
    }
}

