#pragma once

#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdio.h>

struct timeval server_start_time;

void server_log_event(const char *format, ...);
void server_log_error(const char *format, ...);

int server_log_data_output_limit;
void server_log_data(void *in, size_t len);

