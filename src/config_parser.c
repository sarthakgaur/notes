#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define BUFFER_INIT_SIZE 100

void parse(FILE *fp);
void parse_line(char *buffer);

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

            line_ch_read = false;
            skip_line = false;

            i = 0;
            if (buffer == NULL) {
                fprintf(stderr, "Error: malloc failed in parse.\n");
            }
        }
    }

    free(buffer);
}

void parse_line(char *buffer) {
    int ch, i = 0, j = 0;
    char word_buffer[100];

    bool ident_read = false;
    bool assign_read = false;
    bool reading_value = false;
    bool value_read = false;

    while ((ch = buffer[i++]) != '\0') {
        if (!ident_read) {
            if (!isspace(ch) && ch != '=') {
                word_buffer[j++] = ch;
            } else {
                ident_read = true;

                if (ch == '=') {
                    assign_read = true;
                }

                if (j > 0) {
                    word_buffer[j] = '\0';
                    printf("ident: %s\n", word_buffer);
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
                    printf("value: %s\n", word_buffer);
                }
                break;
            } 
        }
    }
}
