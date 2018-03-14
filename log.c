#include "log.h"

#include <time.h>
#include <string.h>

double get_server_time()
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    return (current_time.tv_sec - server_start_time.tv_sec) + 
        ((current_time.tv_usec - server_start_time.tv_usec)/1000000.0);
}

void print_log_entry_prefix(FILE *out)
{
    time_t raw_time = time(NULL);
    char *time_str = ctime(&raw_time);
    fprintf(out, "[%.*s | %f] ", (int)strlen(time_str)-1, time_str, get_server_time());
}

void SERVER_LOG_DATA(void *in, size_t len)
{
    if (server_log_data_output_limit < 0)
        return;

    time_t raw_time = time(NULL);
    char *time_str = ctime(&raw_time);
    fprintf(stdout, "[%.*s | %f] ", (int)strlen(time_str)-1, time_str, get_server_time());

    fprintf(stdout, "Received %li bytes", len);

    if (server_log_data_output_limit > 0)
    {
        size_t N = len;
        if (N > server_log_data_output_limit)
            N = server_log_data_output_limit;

        unsigned char c;
        fprintf(stdout, ": {");
        for (int i = 0; i < N; i++)
        {
            c = (unsigned char)((char*)in)[i];
            if ((c < 32) || (c == 127) || (c == 255))
                fprintf(stdout, "~");
            else
                fprintf(stdout, "%c", c);
        }
        fprintf(stdout, "}");
    }
    
    fprintf(stdout, "\n");
}

