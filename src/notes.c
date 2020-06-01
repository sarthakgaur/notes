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
// TODO Add comments to the project. // src/notes.c comments are done
// TODO Add option to disable adding date to note. // Done
// TODO Add a config file parser. // Done
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
    bool write_date;
};

struct note {
    enum note_source ns;
    bool write_error;
    char *storage_path;
    char *filename;
    char *template_filename;
    char *tmpf_path;
    char *buffer;
    bool write_date;
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
static void cleanup(struct note *stn, struct idents *sti);

int main(int argc, char *argv[]) {
    struct request req;

    // Pass the request set by parse_args to the controller that carries out
    // the operation.
    parse_args(&req, argc, argv);
    controller(&req);

    return 0;
}

/*
 * Parses the command line arguments to build a request.
 */
static void parse_args(struct request *req, int argc, char *argv[]) {
    int ch, i, index;
    bool request_set = false;
    bool request_err = false;

    // Set up the dafaults.
    req->rt = NO_REQUEST;
    req->filename = "temp.txt";
    req->template_filename = "template.txt";
    req->write_date = true;

    opterr = 0;

    // "n" requires an argument. For "est" arguments are optional. "lLvhd" do not require
    // any arguments.
    while ((ch = getopt(argc, argv, "n:e:s:t:lLvhd")) != -1) {
        switch(ch) {
            case 'n':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->buffer = optarg;
                    request_set = true;
                }
                break;
            case 'e':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = READ_NOTE;
                    req->filename = optarg;
                    request_set = true;
                }
                break;
            case 's':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = SAVE_TEMPLATE;
                    req->template_filename = optarg;
                    request_set = true;
                }
                break;
            case 't':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = USE_TEMPLATE;
                    req->template_filename = optarg;
                    request_set = true;
                }
                break;
            case 'l':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = LIST_NOTES;
                    request_set = true;
                }
                break;
            case 'L':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = LIST_TEMPLATES;
                    request_set = true;
                }
                break;
            case 'v':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = PRINT_VERSION;
                    request_set = true;
                }
                break;
            case 'h':
                if (!request_set) {
                    req->rt = QUICK_WRITE;
                    req->rt = PRINT_HELP;
                    request_set = true;
                }
                break;
            case 'd':
                req->write_date = false;
                break;
            case '?':
                if (optopt == 'e' && !request_set) {
                    req->rt = READ_NOTE;
                    request_set = true;
                } else if (optopt == 's' && !request_set) {
                    req->rt = SAVE_TEMPLATE;
                    request_set = true;
                } else if (optopt == 't' && !request_set) {
                    req->rt = USE_TEMPLATE;
                    request_set = true;
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
    }

    // If no option was provided, it must be a WRITE_NOTE request.
    if (req->rt == NO_REQUEST) {
        req->rt = WRITE_NOTE;
    }

    // Only one non-option argument if valid: filename. If more than 1 non-opton
    // argument is provided, set the request_err flag.
    i = 0;
    for (index = optind; index < argc; index++) {
        if (i == 0) {
            req->filename = argv[index];
        } else {
            request_err = true;
        }
        i++;
    }

    // If an incompatible non-option argument is provided, set the request_err flag.
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

/*
 * Calls the appropriate function that can complete the request. Frees the resources
 * in the end.
 */
static void controller(struct request *req) {
    struct note stn; 
    struct idents sti;

    read_config(&sti);

    stn.ns = NO_SOURCE;
    stn.storage_path = sti.notes_dir;
    stn.filename = req->filename;
    stn.template_filename = req->template_filename;
    stn.buffer = req->buffer;
    stn.write_date = req->write_date;

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

    // Free the resources.
    cleanup(&stn, &sti);
}

/* 
 * Handles how to the note should be taken. Calls check_write before before
 * storing the note.
 */
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

/*
 * Opens the note file in the editor or writes its content to stdout.
 */
static void read_note(struct note *stn) {
    FILE *note_fp;
    char *editor, *command, *note_fn;
    int ch;

    // Build the path to the note file.
    note_fn = malloc_wppr(strlen(stn->storage_path) + strlen(stn->filename) + 1, __func__);
    strcpy(note_fn, stn->storage_path);
    strcat(note_fn, stn->filename);

    // If the $EDITOR environment variable is not set, write the note content to stdout.
    // If environment vairable is set, open the file in the editor.
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
        // Build the command string to open an editor.
        command = malloc_wppr(strlen(editor) + strlen(note_fn) + 2, __func__);
        strcpy(command, editor);
        strcat(command, " ");
        strcat(command, note_fn);

        // Open the editor
        system(command);

        free(command);
    }

    free(note_fn);
}

/*
 * List the notes files or the templates files according to the request.
 */
static void list_files(struct note *stn, int rt) {
    DIR *dp;
    struct dirent *ep;
    char *templ_dir = "templates/";
    char *path = stn->storage_path;

    // If request if to list templates, build the path to the templates directory.
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

    // Call free only if the path to template was build.
    if (rt == LIST_TEMPLATES) {
        free(path);
    }
}

/*
 * If the $EDITOR environment variable is set, open editor for writing note. Otherwise,
 * set the note source to stdin and return.
 */
static void write_note(struct note *stn) {
    char *tmp_fn, *tmpf_path, *command, *editor, *tmp_str;

    // If the $EDITOR environment variable is not set. Note will be read from stdin.
    tmp_str = getenv("EDITOR");
    if (tmp_str == NULL) {
        stn->ns = FROM_STDIN;
        stn->tmpf_path = NULL;
        stn->buffer = NULL;
        return;
    }

    editor = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(editor, tmp_str);

    // Build the path to temp file.
    tmp_fn = tmpnam(NULL);
    tmpf_path = malloc_wppr(strlen(stn->storage_path) + strlen(tmp_fn) + 1, __func__);
    strcpy(tmpf_path, stn->storage_path);
    strcat(tmpf_path, tmp_fn + TMP_NM_SLICE);

    // Build the command string to open an editor.
    command = malloc_wppr(strlen(editor) + strlen(tmpf_path) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    // Open the editor.
    system(command);

    free(editor);
    free(command);

    // Set the note source and the pointer to the temp file path string.
    stn->ns = FROM_FILE;
    stn->tmpf_path = tmpf_path;
    stn->buffer = NULL;
}

/*
 * Write the contents of the template to a temp file and open it for editing.
 */
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

    // Build the path to the template filename.
    template_fpath_size = strlen(stn->storage_path) + strlen(template_dir) + strlen(stn->template_filename) + 1;
    template_fpath = malloc_wppr(template_fpath_size, __func__);
    strcpy(template_fpath, stn->storage_path);
    strcat(template_fpath, template_dir);
    strcat(template_fpath, stn->template_filename);

    // Build the path to a temp file.
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

    // Write the contents of the template file to the temporary file.
    while ((ch = fgetc(read_file)) != EOF) {
        fputc(ch, write_file);
    }

    fclose(write_file);
    fclose(read_file);

    // Build the command string to open an editor.
    command = malloc_wppr(strlen(editor) + strlen(tmpf_path) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, tmpf_path);

    // Open the editor.
    system(command);

    free(editor);
    free(template_fpath);
    free(command);

    // Store the pointer to the temp file path in the note struct.
    stn->tmpf_path = tmpf_path;
}

/*
 * Create a new template file or update an existing one.
 */
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

    // Build the path to the template directory.
    template_fpath_size = strlen(stn->storage_path) + strlen(template_dir) + strlen(stn->template_filename) + 1;
    template_fpath = malloc_wppr(template_fpath_size, __func__);
    strcpy(template_fpath, stn->storage_path);
    strcat(template_fpath, template_dir);

    // Create the templates directory is not present.
    if (stat(template_fpath, &st) == -1) {
        if (mkdir(template_fpath, 0700) != 0) {
            terminate("%s", "Error: template folder could not be created\n");
        }
    }

    // Build the path to the template filename.
    strcat(template_fpath, stn->template_filename);

    // Build the command string to pass to system. The command string contains the editor name and
    // the path to the template file. Example "vim /home/john/documents/notes/templates/template.txt".
    command = malloc_wppr(strlen(editor) + strlen(template_fpath) + 2, __func__);
    strcpy(command, editor);
    strcat(command, " ");
    strcat(command, template_fpath);

    // Open the editor.
    system(command);

    free(editor);
    free(template_fpath);
    free(command);
}

/*
 * If the contents of the temp file or the buffer only contains space characters
 * check_write sets the write_error flag in struct note passed.
 */
static void check_write(struct note *stn) {
    FILE *read_file;
    int ch, i = 0;

    stn->write_error = true;

    // If the source is a file, check the contents of the file.
    // If the source is stdin or command line, check the buffer.
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

/*
 * Read the note from a file or buffer and append it to the note file specified.
 */
static void store_note(struct note *stn) {
    FILE *read_file, *write_file;
    char *notef_path, date_buffer[100];
    int ch, i = 0;
    time_t current;
    struct tm *t;

    // Build the path to the note file.
    notef_path = malloc_wppr(strlen(stn->storage_path) + strlen(stn->filename) + 1, __func__);
    strcpy(notef_path, stn->storage_path);
    strcat(notef_path, stn->filename);

    write_file = fopen(notef_path, "a");
    if (write_file == NULL) {
        terminate("%s", "Error: not able to open note file for writing.\n");
    }

    current = time(NULL);
    t = localtime(&current);

    // Build the date string and write it to the file if write_date is true.
    if (stn->write_date) {
        strftime(date_buffer, sizeof(date_buffer), "%A, %F %H:%M", t);
        fprintf(write_file, "%s\n", date_buffer);
    }

    // If the source is a temp file, write the contents from the temp file.
    // If the source is stdin or the command line, write the contents of the buffer. 
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

/*
 * Read the note from the stdin and store it in a buffer.
 */
static char *read_stdin(void) {
    char *buffer;
    int ch, i = 0, size = SIZE_HUN;

    buffer = malloc_wppr(size, __func__);

    printf("Write your note: ");

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (i < size) {
            buffer[i++] = ch;

            // Double the size and call realloc.
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

/*
 * Prints out all the supported command line options and arguments.
 */ 
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
        "-d:                   The date string is not written to the note file.\n"
        "-v:                   Print version number.\n"
        "-h:                   Print this help message.\n";

    printf("%s", help_msg);
}

/*
 * Prints the version number.
 */
static void print_version(void) {
    printf("%s\n", "Notes 0.1.9");
}

/*
 * Frees the resources according to the usage.
 */
static void cleanup(struct note *stn, struct idents *sti) {
    // Free the temp file path if note souce was a file. If the source was stdin, clear the
    // buffer.
    if (stn->ns == FROM_FILE || stn->ns == FROM_TEMPLATE) {
        if (!stn->write_error) {
            remove(stn->tmpf_path);
        }
        free(stn->tmpf_path);
    } else if (stn->ns == FROM_STDIN) {
        free(stn->buffer);
    }

    if (sti->editor != NULL) {
        printf("$EDITOR = %s\n", sti->editor);
        free(sti->editor);
    }

    if (sti->notes_dir != NULL) {
        printf("$NOTES_DIR = %s\n", sti->notes_dir);
        free(sti->notes_dir);
    }

    if (sti->date_fmt != NULL) {
        printf("$DATE_FMT = %s\n", sti->date_fmt);
        free(sti->date_fmt);
    }

    if (stn->write_error) {
        terminate("%s", "Error: write error occured.\n");
    }
}

