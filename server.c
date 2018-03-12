// Robotics Lab server
// Written by Ivan Podmazov, 2018

#include <stdlib.h>
#include <locale.h>
#include <signal.h>

#include <libwebsockets.h>

#include "cmdline.h"
#include "log.h"
#include "broadcast-echo-protocol.h"
#include "bulletin-board-protocol.h"

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_HTTP:
            lws_serve_http_file(wsi, "index.html", "text/html", NULL, 0);
            break;
        default:
            break;
    }

    return 0;
}

enum protocols
{
    PROTOCOL_HTTP = 0,
    PROTOCOL_BROADCAST_ECHO,
    PROTOCOL_BULLETIN_BOARD,
    PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
    /* The first protocol must always be the HTTP handler */
    {
        "http-only",   /* name */
        callback_http, /* callback */
        0,             /* No per session data. */
    },
    {
        "13", // backward compatibility with the old server
        callback_broadcast_echo,
        0,
    },
    {
        "bulletin-board-protocol",
        callback_bulletin_board,
        0,
    },
    { NULL, NULL, 0 } /* terminator */
};

volatile sig_atomic_t done = 0;
static void term(int signo)
{
    done = 1;
    server_log_message("Shutting down.");
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    struct gengetopt_args_info ai;
    if (cmdline_parser(argc, argv, &ai) != 0) {
        return 1;
    }

    if ((ai.port_arg < 0) || (ai.port_arg > 0xFFFF)) {
        fprintf(stderr, "Invalid port number %i\n", ai.port_arg);
        fprintf(stderr, "Shutting down.\n");
        return 1;
    }

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = ai.port_arg;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);

    if (context == NULL) {
        fprintf(stderr, "Initialization of the libwebsocket has failed!\n");
        fprintf(stderr, "Shutting down.\n");
        return 2;
    }

    server_log_message("The server has been started.");
    while (!done)
    {
        /* lws_service(context, 50); */
    }

    lws_context_destroy(context);

    server_log_message("The server has been stopped.");
    return 0;
}

