#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"
#include "utilities.h"

#define SIZE_HUN 100

// TODO Add comments to functions
// TODO Get the storage location // Done
// TODO Build the storage location // Done
// TODO Split the functions into files // Done
// TODO Get editor name from environment // Done
// TODO Add reading and editing options for notes.
// TODO Add malloc wrapper function. // Done

enum note_source {
    FROM_STDIN,
    FROM_FILE,
};

struct write_info {
    enum note_source ns;
    char *tmpf_path;
    char *buffer;
};

static struct write_info *write_note(char *storage_path);
static char *read_stdin(void);
static void store_note(struct write_info *wi, char *storage_path, char *filename);
static void cleanup(struct write_info *wi, char *storage_path);

int main(int argc, char *argv[]) {
    char *storage_path, *filename = "temp.txt";
    struct write_info *wi; 

    if (argc == 2)
        filename = argv[1];

    storage_path = read_config();
    wi = write_note(storage_path);

    if (wi->ns == FROM_STDIN)
        wi->buffer = read_stdin();

    store_note(wi, storage_path, filename);

    cleanup(wi, storage_path);

    return 0;
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
    strcat(tmpf_path, tmp_fn + 5);

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

    fprintf(write_file, "\n\n");

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

