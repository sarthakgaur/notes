#ifndef CONFIG_H
#define CONFIG_H

struct idents {
    char *editor;
    char *notes_dir;
    char *date_fmt;
    bool parse_err;
};

void read_config(struct idents *sti);

#endif
