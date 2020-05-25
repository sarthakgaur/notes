#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_INIT_SIZE 100

void parse(FILE *fp);
void parse_line(char *buffer);
void check_ident(char *ident);

int main(void) {
    FILE *fp;

    fp = fopen("dump/config", "r");
    parse(fp);

    fclose(fp);

    return 0;
}

void parse(FILE *fp) {
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
                parse_line(buffer);
            }

            i = 0;
            line_ch_read = false;
            skip_line = false;
        }
    }

    free(buffer);
}

void parse_line(char *buffer) {
    int ch, i = 0, j = 0;
    char word_buffer[100];
    char *ident;
    char *value;

    bool ident_read = false;
    bool assign_read = false;
    bool reading_value = false;
    bool value_read = false;

    while ((ch = buffer[i++]) != '\0') {
        if (!ident_read) {
            if (!isspace(ch) && ch != '=') {
                word_buffer[j++] = ch;
            } else {
                if (ch == '=') {
                    assign_read = true;
                }

                if (j > 0) {
                    ident_read = true;
                    word_buffer[j] = '\0';
                    ident = strdup(word_buffer);
                    if (ident == NULL) {
                        fprintf(stderr, "Error: strdup failed in parse line.\n");
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
                word_buffer[j++] = ch;
                reading_value = true;
            } else {
                if (j > 0) {
                    value_read = true;
                    word_buffer[j] = '\0';
                    value = strdup(word_buffer);
                    if (value == NULL) {
                        fprintf(stderr, "Error: strdup failed in parse line.\n");
                    }
                }
                break;
            } 
        }
    }

    if (ident_read && value_read) {
        check_ident(ident);
    } else {
        fprintf(stderr, "Error: ident or value could not be read successfully.\n");
    }

    if (ident_read) {
        free(ident);
    }

    if (value_read) {
        free(value);
    }
}

void check_ident(char *ident) {
    char *ident_list[] = {
        "$EDITOR",
        "$NOTES_DIR",
        "$DATE_FMT"
    };

    int list_size = sizeof(ident_list) / sizeof(ident_list[0]);

    for (int i = 0; i < list_size; i++) {
        if (strcmp(ident, ident_list[i]) == 0) {
            printf("Ident %s is valid.\n", ident);
            return;
        }
    }

    printf("Ident %s is not valid.\n", ident);
}
