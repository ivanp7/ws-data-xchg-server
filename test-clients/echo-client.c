#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>

#include <libwebsockets.h>

static struct lws *web_socket = NULL;

#define EXAMPLE_RX_BUFFER_BYTES (10)

char client_symbol = 'X';
static int callback_broadcast_echo(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            /* Handle incomming messages here. */
            printf("Received %li bytes", len);
            unsigned char c;
            printf(": {");
            for (int i = 0; i < len; i++)
            {
                c = (unsigned char)((char*)in)[i];
                if ((c < 32) || (c == 127) || (c == 255))
                    printf("~");
                else
                    printf("%c", c);
            }
            printf("}\n");
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            unsigned char buf[LWS_PRE + EXAMPLE_RX_BUFFER_BYTES];
            unsigned char *p = &buf[LWS_PRE];
            size_t n = sprintf((char*)p, "%u", rand()%1000);
            p[0] = client_symbol;
            lws_write(wsi, p, n, LWS_WRITE_BINARY);
            break;
        }

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            web_socket = NULL;
            break;

        default:
            break;
    }

    return 0;
}

enum protocols
{
    PROTOCOL_BROADCAST_ECHO = 0,
    PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
    {
        "13", // backward compatibility with the old server
        callback_broadcast_echo,
        0
    },
    { NULL, NULL, 0 } /* terminator */
};

volatile sig_atomic_t done = 0;
static void term(int signo)
{
    done = 1;
    printf("Shutting down.\n");
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    int port = 60000;
    if (argc > 1)
        client_symbol = argv[1][0];
    if (argc > 2)
        port = atoi(argv[2]);
    printf("Client symbol: %c\n", client_symbol);
    printf("Connecting to port number %i\n", port);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);

    {
        struct lws_client_connect_info ccinfo = {0};
        ccinfo.context = context;
        ccinfo.address = "localhost";
        ccinfo.port = port;
        ccinfo.path = "/";
        ccinfo.host = lws_canonical_hostname(context);
        ccinfo.origin = "origin";
        ccinfo.protocol = protocols[PROTOCOL_BROADCAST_ECHO].name;
        web_socket = lws_client_connect_via_info(&ccinfo);
    }

    time_t old = 0;
    while (!done && web_socket)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        if (tv.tv_sec != old)
        {
            /* Send a random number to the server every second. */
            lws_callback_on_writable(web_socket);
            old = tv.tv_sec;
        }

        lws_service(context, 50);
    }

    lws_context_destroy(context);

    return 0;
}

