#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>

#include <libwebsockets.h>
#include <cjson/cJSON.h>

static struct lws *web_socket = NULL;

#define BUFFER_SIZE (10000)
char buffer[LWS_PRE+BUFFER_SIZE+1] = {'\0'};

int ready = 0;

static int callback_json_transmission_protocol(
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
            printf(": <<");
            for (int i = 0; i < len; i++)
            {
                c = (unsigned char)((char*)in)[i];
                if ((c < 32) || (c == 127) || (c == 255))
                    printf("~");
                else
                    printf("%c", c);
            }
            printf(">>\n");

            ready = 1;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: ; //empty statement
            char *p = &buffer[LWS_PRE];
            size_t plen = strlen(p);
            if (plen > BUFFER_SIZE+1)
                plen = BUFFER_SIZE+1;

            if (plen > 0)
            {
                lws_write(wsi, (unsigned char*)p, plen, LWS_WRITE_BINARY);
                printf("Sent %li bytes\n", plen);
            }
            break;

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
    PROTOCOL_JSON_TRANSMISSION = 0,
    PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
    {
        "json-transmission-protocol",
        callback_json_transmission_protocol,
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

void read_string(char *string, size_t string_len)
{
    char *pos;
    fgets(string, string_len, stdin);
    if ((pos = memchr(string, '\n', string_len+1)) != NULL)
        *pos = '\0';
    else
        string[string_len] = '\0';
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = term;
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGINT,  &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    int port = 60000;
    if (argc > 1)
        port = atoi(argv[1]);
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
        ccinfo.protocol = protocols[PROTOCOL_JSON_TRANSMISSION].name;
        web_socket = lws_client_connect_via_info(&ccinfo);
    }
    printf("Connected to the server.\n");

    // Main loop
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t old = tv.tv_sec;

    const size_t string_len = 1000;
    char string[string_len + 1];

    while (!done && web_socket)
    {
        lws_service(context, 50);
        gettimeofday(&tv, NULL);

        cJSON *json = cJSON_CreateObject();
        if (ready)
        {
            printf("From> ");
            read_string(string, string_len);
            cJSON_AddStringToObject(json, "from", string);
            printf("To> ");
            read_string(string, string_len);
            cJSON_AddStringToObject(json, "to", string);
            printf("Message> ");
            read_string(string, string_len);
            cJSON_AddStringToObject(json, "message", string);

            cJSON_PrintPreallocated(json, &buffer[LWS_PRE], BUFFER_SIZE, 0);

            if (strlen(&buffer[LWS_PRE]) > 0)
            {
                ready = 0;
                old = tv.tv_sec;
            }
            lws_callback_on_writable(web_socket);
        }
        else
        {
            if (tv.tv_sec - old > 2)
                ready = 1;
        }
        cJSON_Delete(json);
    }
    printf("Disconnected from the server.\n");

    lws_context_destroy(context);

    return 0;
}

