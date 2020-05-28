#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "utilities.h"

#define BUFFER_INIT_SIZE 100

// TODO Refactor.

struct idents {
    char *editor;
    char *notes_dir;
    char *date_fmt;
    bool parse_err;
};

struct line_info {
    char *ident;
    char *value;
    char *token_buffer;
    int storage_status;
    bool ident_read;
    bool assign_read;
    bool reading_value;
    bool value_read;
    bool pair_complete;
};

void parse_controller(void);
void parse_file(struct idents *sti, FILE *fp);
void parse_line(struct idents *sti, char *buffer);
void line_cleanup(struct line_info *stli);
int store_ident_val(struct idents *sti, char *ident, char *value);
void parse_cleanup(struct idents *sti);

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
    parse_cleanup(&sti);
}

void parse_file(struct idents *sti, FILE *fp) {
    int ch, size, i = 0;
    char *buffer;
    bool line_ch_read = false;
    bool skip_line = false;

    size = BUFFER_INIT_SIZE;
    buffer = malloc_wppr(size, __func__);

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
                    terminate("%s", "Error: realloc failed in parse.\n");
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
    int ch, i = 0, j = 0, size = BUFFER_INIT_SIZE;
    struct line_info stli;

    stli.ident_read = stli.assign_read = stli.reading_value = \
    stli.value_read = stli.pair_complete = false;

    stli.token_buffer = malloc_wppr(size, __func__);

    while ((ch = buffer[i++]) != '\0') {
        if (!stli.ident_read) {
            if (!isspace(ch) && ch != '=') {
                stli.token_buffer[j++] = ch;
            } else {
                if (ch == '=') {
                    stli.assign_read = true;
                }

                if (j > 0) {
                    stli.ident_read = true;
                    stli.token_buffer[j] = '\0';
                    stli.ident = strdup(stli.token_buffer);
                    if (stli.ident == NULL) {
                        terminate("%s", "Error: strdup failed in parse line.\n");
                    }
                    j = 0;
                }
            }
        } else if (!stli.assign_read && ch == '=') {
            stli.assign_read = true;
        } else {
            if (isspace(ch) && !stli.reading_value) {
                continue;
            } else if (ch != '\n') {
                stli.token_buffer[j++] = ch;
                stli.reading_value = true;
            } else if (j > 0) {
                stli.value_read = true;
                stli.token_buffer[j] = '\0';
                stli.value = strdup(stli.token_buffer);
                if (stli.value == NULL) {
                    terminate("%s", "Error: strdup failed in parse_line.\n");
                }
                break;
            } 
        }

        if (j == size - 1) {
            size *= 2;
            stli.token_buffer = realloc(stli.token_buffer, size);
            if (stli.token_buffer == NULL) {
                terminate("%s", "Error: realloc failed in pare_line.\n");
            }
        }
    }

    if (stli.ident_read && stli.value_read) {
        stli.storage_status = store_ident_val(sti, stli.ident, stli.value);
        stli.pair_complete = true;
    } else {
        fprintf(stderr, "Error: ident or value could not be read successfully.\n");
        sti->parse_err = true;
    }

    line_cleanup(&stli);
}

void line_cleanup(struct line_info *stli) {
    if (!stli->pair_complete) {
        if (stli->value_read) {
            free(stli->value);
        }
    }

    if (stli->storage_status == 1) {
        free(stli->value);
    }

    if (stli->ident_read) {
        free(stli->ident);
    }

    free(stli->token_buffer);
}

int store_ident_val(struct idents *sti, char *ident, char *val) {
    if (strcmp(ident, "$EDITOR") == 0 && sti->editor == NULL) {
        sti->editor = val;
    } else if (strcmp(ident, "$NOTES_DIR") == 0 && sti->notes_dir == NULL) {
        sti->notes_dir = val;
    } else if (strcmp(ident, "$DATE_FMT") == 0 && sti->date_fmt == NULL) {
        sti->date_fmt = val;
    } else {
        fprintf(stderr, "Error: ident used multiple times or unknown ident used.\n");
        sti->parse_err = true;
        return 1;
    }

    return 0;
}

void parse_cleanup(struct idents *sti) {
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

