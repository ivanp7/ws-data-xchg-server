#include "log.h"

#include <libwebsockets.h>

void SERVER_LOG_DATA(void *in, size_t len)
{
    if (server_log_data_output_limit == 0)
        return;

    fprintf(stdout, "[%f] ", 1.0 * clock() / CLOCKS_PER_SEC);

    size_t N = len - LWS_PRE;
    if (server_log_data_output_limit >= 0)
    {
        if (N > server_log_data_output_limit)
            N = server_log_data_output_limit;
    }

    unsigned char c;
    fprintf(stdout, "{");
    for (int i = 0; i < N; i++)
    {
        c = (unsigned char)((char*)in)[LWS_PRE + i];
        if ((c < 32) || (c == 127) || (c == 255))
            fprintf(stdout, "~");
        else
            fprintf(stdout, "%c", c);
    }
    fprintf(stdout, "}\n");
}

