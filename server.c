// Robotics Lab server
// Written by Ivan Podmazov, 2018

#include "cmdline.h"
#include "log.h"
#include "broadcast-echo-protocol.h"
#include "bulletin-board-protocol.h"

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <signal.h>

#include <libwebsockets.h>

static int callback_http(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
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
        sizeof(struct per_session_data__broadcast_echo_protocol),
    },
    {
        "bulletin-board-protocol",
        callback_bulletin_board,
        sizeof(struct per_session_data__bulletin_board_protocol),
    },
    { NULL, NULL, 0 } /* terminator */
};

volatile sig_atomic_t done = 0;
static void term(int signo)
{
    done = 1;
    SERVER_LOG_EVENT("Shutting down.");
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
    if (cmdline_parser(argc, argv, &ai) != 0) 
    {
        return 1;
    }

    if ((ai.port_arg < 0) || (ai.port_arg > 0xFFFF)) 
    {
        fprintf(stderr, "Invalid port number %i\n", ai.port_arg);
        fprintf(stderr, "Shutting down.\n");
        return 1;
    }

    if (ai.output_given && (ai.output_arg < 0))
    {

        fprintf(stderr, "Invalid data output limit %i\n", ai.output_arg);
        fprintf(stderr, "Shutting down.\n");
        return 1;
    }
    server_log_data_output_limit = (ai.output_given)? ai.output_arg: -1;

    gettimeofday(&server_start_time, NULL);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = ai.port_arg;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if (context == NULL) 
    {
        fprintf(stderr, "Initialization of the libwebsocket has failed!\n");
        fprintf(stderr, "Shutting down.\n");
        return 2;
    }

    init_broadcast_echo_protocol();
    init_bulletin_board_protocol();

    SERVER_LOG_EVENT("The server has been started.");
    while (!done)
    {
        lws_service(context, 50);
    }

    lws_context_destroy(context);

    deinit_broadcast_echo_protocol();
    deinit_bulletin_board_protocol();

    SERVER_LOG_EVENT("The server has been stopped.");
    return 0;
}

