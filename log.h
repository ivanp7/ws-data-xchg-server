#pragma once

#include <stdio.h>
#include <time.h>

#define server_log_message(args) \
{ \
    fprintf(stdout, "[%f] ", 1.0 * clock() / CLOCKS_PER_SEC); \
    fprintf(stdout, args); \
    fprintf(stdout, "\n"); \
}
#define server_error_message(args) \
{ \
    fprintf(stderr, "[%f] ", 1.0 * clock() / CLOCKS_PER_SEC); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}

