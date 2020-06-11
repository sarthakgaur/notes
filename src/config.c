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

static void build_storage(char *home_str, struct config *conf);
static void parse_controller(struct config *conf, char *home_str);
static void parse_file(struct config *conf, FILE *fp);
static void parse_line(struct config *conf, char *buffer);
static int parse_str_val(struct line_info *stli);
static int store_ident_val(struct config *conf, char *ident, char *value);
static void line_cleanup(struct line_info *stli);

/*
 * Gets the necessary information from the environment or the config file. The precedence
 * of environment variables is higher.
 */
void read_config(struct config *conf) {
    char *home_str, *tmp_str;
    int str_len;

    tmp_str = getenv("HOME");
    if (tmp_str == NULL) {
        terminate("%s", "environment variable HOME not available.\n");
    }
    home_str = malloc_wppr(strlen(tmp_str) + 1, __func__);
    strcpy(home_str, tmp_str);

    parse_controller(conf, home_str);

    tmp_str = getenv("NOTES_DIR");
    if (tmp_str != NULL) {
        str_len = strlen(tmp_str);
        conf->notes_dir = realloc(conf->notes_dir, str_len + 2);
        if (conf->notes_dir == NULL) {
            terminate("%s", "Error: realloc failed in read_config.\n");
        }
        strcpy(conf->notes_dir, tmp_str);
        if (conf->notes_dir[str_len - 1] != '/') {
            strcat(conf->notes_dir, "/");
        }
    }

    tmp_str = getenv("EDITOR");
    if (tmp_str != NULL) {
        str_len = strlen(tmp_str);
        conf->editor = realloc(conf->editor, str_len + 2);
        if (conf->editor == NULL) {
            terminate("%s", "Error: realloc failed in read_config.\n");
        }
        strcpy(conf->editor, tmp_str);
    }

    build_storage(home_str, conf);

    free(home_str);
}

/*
 * Create a default path to the notes file if not available and create the notes directory
 * if not already created.
 */
static void build_storage(char *home_str, struct config *conf) {
    char *storage_dn = "/notes/";

    if (conf->notes_dir == NULL) {
        conf->notes_dir = malloc_wppr(strlen(home_str) + strlen(storage_dn) + 1, __func__);
        strcpy(conf->notes_dir, home_str);
        strcat(conf->notes_dir, storage_dn);
    }

    struct stat st = {0};

    if (stat(conf->notes_dir, &st) == -1) {
        if (mkdir(conf->notes_dir, 0700) != 0) {
            terminate("%s", "storage folder could not be created\n");
        }
    }
}

/*
 * Starts the config file parsing by opening the config file and passing it to parse_file
 */
static void parse_controller(struct config *conf, char *home_str) {
    char *complete_config_path, *config_path = "/.config/notes/config";
    int path_len;
    FILE *fp;

    conf->editor = NULL;
    conf->notes_dir = NULL;
    conf->date_fmt = NULL;
    conf->parse_err = false;

    path_len = strlen(home_str) + strlen(config_path) + 1;
    complete_config_path = malloc_wppr(path_len, __func__);
    strcpy(complete_config_path, home_str);
    strcat(complete_config_path, config_path);

    fp = fopen(complete_config_path, "r");
    if (fp == NULL) {
        conf->parse_err = true;
    } else {
        parse_file(conf, fp);

        fclose(fp);
    }

    free(complete_config_path);
}

/*
 * Parse the config file and send each line to parse_line for further parsing
 */
static void parse_file(struct config *conf, FILE *fp) {
    int ch, size, i = 0;
    char *buffer;
    bool line_ch_read = false;
    bool skip_line = false;

    size = BUFFER_INIT_SIZE;
    buffer = malloc_wppr(size, __func__);

    // The skip_line is checked in the first condition because of a special case. If the
    // skip_line check was not present and line contained only a '#' character, it will skip
    // the newline test at bottom.
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
                parse_line(conf, buffer);
            }

            // Set the flags and 'i' to the default values.
            i = 0;
            line_ch_read = false;
            skip_line = false;
        }
    }

    free(buffer);
}

/*
 * Parse the line provided into identifier and value, and send them for storage if they
 * are valid.
 */
static void parse_line(struct config *conf, char *buffer) {
    int ch, i = 0, j = 0, size = BUFFER_INIT_SIZE;
    struct line_info stli;

    stli.ident_read = stli.assign_read = stli.reading_value = \
    stli.value_read = stli.pair_complete = false;

    stli.token_buffer = malloc_wppr(size, __func__);

    // Store the identifier in sti.ident and value in sti.value. All the spaces preceding
    // and following the identified are ignored. However, only spaces preceding value are
    // ignored. Full value parsing happens in parse_str_val.
    // Example format:
    // $EDITOR = "vim"
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

    // Store only when all the values are present and parse_str_val return 0.
    if (stli.ident_read && stli.value_read && parse_str_val(&stli) == 0) {
        stli.storage_status = store_ident_val(conf, stli.ident, stli.value);
        stli.pair_complete = true;
    } else {
        fprintf(stderr, "Error: ident or value could not be read successfully.\n");
        conf->parse_err = true;
    }

    line_cleanup(&stli);
}

/*
 * Parse the value part of the pair. Return 0 if successful, 1 otherwise.
 */
static int parse_str_val(struct line_info *stli) {
    int len, i, end_space_count = 0;
    char ch, *val_buffer;

    // Return if doesn't start with a quote
    if (stli->value[0] != '"') {
        return 1;
    }

    len = strlen(stli->value);
    val_buffer = malloc_wppr(len + 1, __func__);

    // Count the spaces at the end.
    i = len - 1;
    while (isspace(stli->value[i--])) {
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

    // Remove the end_space_count from len and 2 (for 2 quotes).
    if (strlen(val_buffer) == (long unsigned int) (len - end_space_count - 2)) {
        free(stli->value);
        stli->value = val_buffer;
    } else {
        free(val_buffer);
        return 1;
    }

    return 0;
}

/*
 * Stores the value in struct config. Return 0 if the operation was successful, 1 otherwise.
 */
static int store_ident_val(struct config *conf, char *ident, char *val) {
    if (strcmp(ident, "$EDITOR") == 0 && conf->editor == NULL) {
        conf->editor = val;
    } else if (strcmp(ident, "$NOTES_DIR") == 0 && conf->notes_dir == NULL) {
        conf->notes_dir = val;
    } else if (strcmp(ident, "$DATE_FMT") == 0 && conf->date_fmt == NULL) {
        conf->date_fmt = val;
    } else {
        fprintf(stderr, "Error: ident used multiple times or unknown ident used.\n");
        conf->parse_err = true;
        return 1;
    }

    return 0;
}

/*
 * Free the resources used by parse_line.
 */
static void line_cleanup(struct line_info *stli) {
    if (!stli->pair_complete) {
        if (stli->value_read) {
            free(stli->value);
        }
    }

    // Free stli->value is an error occurred while storing the value in struct config.
    if (stli->storage_status == 1) {
        free(stli->value);
    }

    if (stli->ident_read) {
        free(stli->ident);
    }

    free(stli->token_buffer);
}

