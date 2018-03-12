#pragma once

#include <stdio.h>
#include <time.h>

#define SERVER_LOG_EVENT(args) \
{ \
    fprintf(stdout, "[%f] ", 1.0 * clock() / CLOCKS_PER_SEC); \
    fprintf(stdout, args); \
    fprintf(stdout, "\n"); \
}
#define SERVER_LOG_ERROR(args) \
{ \
    fprintf(stderr, "[%f] ", 1.0 * clock() / CLOCKS_PER_SEC); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}

size_t server_log_data_output_limit;
void SERVER_LOG_DATA(void *in, size_t len);
