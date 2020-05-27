#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_INIT_SIZE 100

// TODO Refactor.

struct idents {
    char *editor;
    char *notes_dir;
    char *date_fmt;
    bool parse_err;
};

void parse_controller(void);
void parse_file(struct idents *sti, FILE *fp);
void parse_line(struct idents *sti, char *buffer);
void store_ident_val(struct idents *sti, char *ident, char *value);
void cleanup(struct idents *sti);

int main(void) {
    parse_controller();
    return 0;
}

void parse_controller(void) {
    FILE *fp;
    struct idents sti;

    sti.editor = NULL;
    sti.notes_dir = NULL;
    sti.date_fmt = NULL;

    fp = fopen("dump/config", "r");
    parse_file(&sti, fp);

    fclose(fp);
    cleanup(&sti);
}

void parse_file(struct idents *sti, FILE *fp) {
    int ch, size, i = 0;
    char *buffer;
    bool line_ch_read = false;
    bool skip_line = false;

    size = BUFFER_INIT_SIZE;
    buffer = malloc(BUFFER_INIT_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Error: malloc failed in parse.\n");
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch) && !line_ch_read && !skip_line) {
            continue;
        } else if (ch == '#') { 
            skip_line = true;
        } else if (!skip_line) {
            line_ch_read = true;
            buffer[i++] = ch;

            if (i == size - 2) {
                size *= 2;
                buffer = realloc(buffer, size);
                if (buffer == NULL) {
                    fprintf(stderr, "Error: realloc failed in parse.\n");
                }
            }
        }

        if (ch == '\n') {
            buffer[i++] = ch;
            buffer[i] = '\0';

            if (line_ch_read) {
                parse_line(sti, buffer);
            }

            i = 0;
            line_ch_read = false;
            skip_line = false;
        }
    }

    free(buffer);
}

void parse_line(struct idents *sti, char *buffer) {
    int ch, i = 0, j = 0;
    char token_buffer[100];
    char *ident;
    char *value;

    bool ident_read = false;
    bool assign_read = false;
    bool reading_value = false;
    bool value_read = false;

    while ((ch = buffer[i++]) != '\0') {
        if (!ident_read) {
            if (!isspace(ch) && ch != '=') {
                token_buffer[j++] = ch;
            } else {
                if (ch == '=') {
                    assign_read = true;
                }

                if (j > 0) {
                    ident_read = true;
                    token_buffer[j] = '\0';
                    ident = strdup(token_buffer);
                    if (ident == NULL) {
                        fprintf(stderr, "Error: strdup failed in parse line.\n");
                        exit(EXIT_FAILURE);
                    }
                    j = 0;
                }
            }
        } else if (!assign_read) {
            if (ch == '=') {
                assign_read = true;
            }
        } else {
            if (isspace(ch) && !reading_value) {
                continue;
            } if (ch != '\n') {
                token_buffer[j++] = ch;
                reading_value = true;
            } else {
                if (j > 0) {
                    value_read = true;
                    token_buffer[j] = '\0';
                    value = strdup(token_buffer);
                    if (value == NULL) {
                        fprintf(stderr, "Error: strdup failed in parse line.\n");
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
            } 
        }
    }

    if (ident_read && value_read) {
        store_ident_val(sti, ident, value);
    } else {
        fprintf(stderr, "Error: ident or value could not be read successfully.\n");
        sti->parse_err = true;

        if (ident_read) {
            free(ident);
        }

        if (value_read) {
            free(value);
        }
    }
}

void store_ident_val(struct idents *sti, char *ident, char *val) {
    if (strcmp(ident, "$EDITOR") == 0 && sti->editor == NULL) {
        sti->editor = val;
    } else if (strcmp(ident, "$NOTES_DIR") == 0 && sti->notes_dir == NULL) {
        sti->notes_dir = val;
    } else if (strcmp(ident, "$DATE_FMT") == 0 && sti->date_fmt == NULL) {
        sti->date_fmt = val;
    } else {
        fprintf(stderr, "Error: ident used multiple times.\n");
        sti->parse_err = true;
        free(val);
    }

    free(ident);
}

void cleanup(struct idents *sti) {
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
}
