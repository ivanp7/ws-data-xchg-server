#pragma once

#include <stdlib.h>

void stop_server();

#define has_memory_allocation_failed(mem) check_and_log_memory_allocation_fail(mem, __FILE__, __LINE__)
int check_and_log_memory_allocation_fail(void *mem, const char *file, int line);

void *new_data_buffer(const void *data, size_t len);
void *extend_data_buffer(void *data_buffer, size_t new_len);
void *data_buffer_data(void *data_buffer);

