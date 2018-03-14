#pragma once

#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdio.h>

struct timeval server_start_time;
double get_server_time();

void print_log_entry_prefix(FILE *out);

#define SERVER_LOG_ERROR SERVER_LOG_EVENT
#define SERVER_LOG_EVENT(args) \
{ \
    print_log_entry_prefix(stdout); \
    fprintf(stdout, args); \
    fprintf(stdout, "\n"); \
}

int server_log_data_output_limit;
void SERVER_LOG_DATA(void *in, size_t len);

