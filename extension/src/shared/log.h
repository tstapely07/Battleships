/*
Shared debugging utility, with logging macros
*/

#include <stdio.h>
#include <stdbool.h>

extern FILE *log_out;
extern FILE *log_err;

extern bool init_logger(const char *filepath);
extern void close_logger(void);

// Macros
// Wrapped in a `do while(0)` to ensure safety inside loops and if statements without braces
#define LOG_INFO(fmt, ...) do { fprintf(log_out, "[INFO] " fmt "\n", ##__VA_ARGS__); fflush(log_out); } while(0)
#define LOG_DEBUG(fmt, ...) do { fprintf(log_out, "[DEBUG] " fmt "\n", ##__VA_ARGS__); fflush(log_out); } while(0)
#define LOG_ERROR(fmt, ...) do { fprintf(log_err, "[ERROR] " fmt "\n", ##__VA_ARGS__); fflush(log_out); } while(0)
