#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>
#include <stdbool.h>

struct config {
    char editor[250];
    char notes_dir[250];
    char date_fmt[250];
    bool parse_err;
    time_t cmod_time;
};

void read_config(struct config *conf);

#endif
