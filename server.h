#pragma once

void stop_server();

#define has_memory_allocation_failed(mem) check_and_log_memory_allocation_fail(mem, __FILE__, __LINE__)
int check_and_log_memory_allocation_fail(void *mem, const char *file, int line);

