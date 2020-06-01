#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "utilities.h"

#define BUFFER_INIT_SIZE 100

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

static void build_storage(char *home_str, struct idents *sti);
static void parse_controller(struct idents *sti, char *home_str);
static void parse_file(struct idents *sti, FILE *fp);
static void parse_line(struct idents *sti, char *buffer);
static int parse_str_val(struct line_info *stli);
static int store_ident_val(struct idents *sti, char *ident, char *value);
static void line_cleanup(struct line_info *stli);

void read_config(struct idents *sti) {
    char *home_str, *tmp_str;
    int str_len;

    tmp_str = getenv("HOME");
    if (tmp_str == NULL) {
        terminate("%s", "environment variable HOME not available.\n");
    }
    home_str = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(home_str, tmp_str);

    parse_controller(sti, home_str);

    tmp_str = getenv("NOTES_DIR");
    if (tmp_str != NULL) {
        str_len = strlen(tmp_str);
        sti->notes_dir = realloc(sti->notes_dir, str_len + 2);
        if (sti->notes_dir == NULL) {
            terminate("%s", "Error: realloc failed in read_config.\n");
        }
        strcpy(sti->notes_dir, tmp_str);
        if (sti->notes_dir[str_len - 1] != '/') {
            strcat(sti->notes_dir, "/");
        }
    }

    build_storage(home_str, sti);

    free(home_str);
}

static void build_storage(char *home_str, struct idents *sti) {
    char *storage_dn = "/notes/";

    if (sti->notes_dir == NULL) {
        sti->notes_dir = malloc_wppr(strlen(home_str) + strlen(storage_dn) + 1, __func__);
        strcpy(sti->notes_dir, home_str);
        strcat(sti->notes_dir, storage_dn);
    }

    struct stat st = {0};

    if (stat(sti->notes_dir, &st) == -1) {
        if (mkdir(sti->notes_dir, 0700) != 0) {
            terminate("%s", "storage folder could not be created\n");
        }
    }
}

static void parse_controller(struct idents *sti, char *home_str) {
    char *complete_config_path, *config_path = "/.config/notes/config";
    int path_len;
    FILE *fp;

    sti->editor = NULL;
    sti->notes_dir = NULL;
    sti->date_fmt = NULL;
    sti->parse_err = false;

    path_len = strlen(home_str) + strlen(config_path) + 1;
    complete_config_path = malloc_wppr(path_len, __func__);
    strcpy(complete_config_path, home_str);
    strcat(complete_config_path, config_path);

    fp = fopen(complete_config_path, "r");
    if (fp == NULL) {
        sti->parse_err = true;
    } else {
        parse_file(sti, fp);

        fclose(fp);
        // parse_cleanup(&sti);
    }

    free(complete_config_path);
}

static void parse_file(struct idents *sti, FILE *fp) {
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

static void parse_line(struct idents *sti, char *buffer) {
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

    if (stli.ident_read && stli.value_read && parse_str_val(&stli) == 0) {
        stli.storage_status = store_ident_val(sti, stli.ident, stli.value);
        stli.pair_complete = true;
    } else {
        fprintf(stderr, "Error: ident or value could not be read successfully.\n");
        sti->parse_err = true;
    }

    line_cleanup(&stli);
}

static int parse_str_val(struct line_info *stli) {
    int len, i, end_space_count = 0;
    char ch, *val_buffer;

    if (stli->value[0] != '"') {
        return 1;
    }

    len = strlen(stli->value);
    val_buffer = malloc_wppr(len + 1, __func__);

    i = len - 1;
    while ((ch = stli->value[i--]) == ' ') {
        end_space_count++;
    }

    i = 1;
    while ((ch = stli->value[i]) != '"' && ch != '\0') {
        val_buffer[i - 1] = ch;
        i++;
    }

    if (ch == '"') {
        val_buffer[i - 1] = '\0';
    } else {
        free(val_buffer);
        return 1;
    }

    if (strlen(val_buffer) == (long unsigned int) (len - end_space_count - 2)) {
        free(stli->value);
        stli->value = val_buffer;
    } else {
        free(val_buffer);
        return 1;
    }

    return 0;
}

static int store_ident_val(struct idents *sti, char *ident, char *val) {
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

static void line_cleanup(struct line_info *stli) {
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

