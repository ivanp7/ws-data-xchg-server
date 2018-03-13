#pragma once

#include <sys/time.h>
#include <stddef.h>
#include <stdio.h>

struct timeval server_start_time;
double get_server_time();

#define SERVER_LOG_EVENT(args) \
{ \
    fprintf(stdout, "[%f] ", get_server_time()); \
    fprintf(stdout, args); \
    fprintf(stdout, "\n"); \
}
#define SERVER_LOG_ERROR(args) \
{ \
    fprintf(stderr, "[%f] ", get_server_time()); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}

int server_log_data_output_limit;
void SERVER_LOG_DATA(void *in, size_t len);

