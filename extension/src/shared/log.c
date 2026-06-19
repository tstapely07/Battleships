#include "shared/log.h"
#include <stdbool.h>
#include <stdio.h>

FILE *log_out = NULL;
FILE *log_err = NULL;

static FILE *log_file = NULL;

bool init_logger(const char *filepath) {
    if (filepath != NULL) {
        // Open the file
        log_file = fopen(filepath, "a");

        if (log_file != NULL) {
            // Route outputs to log file
            log_out = log_file;
            log_err = log_file;
            return true;
        } else {
            // If file fails to open, we can just use the terminal
            fprintf(
                stderr,
                "[ERROR] Failed to open log file, using terminal instead.\n");
        }
    }

    log_out = stdout;
    log_err = stderr;
    return true;
}

void close_logger(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}
