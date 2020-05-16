#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utilities.h"

void terminate(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void *malloc_wppr(int bytes, const char *func_name) {
    void *ptr;
    ptr = malloc(bytes);
    if (ptr == NULL)
        terminate("malloc failed in %s.\n", func_name);
    return ptr;
}

