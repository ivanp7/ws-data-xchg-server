#include "broadcast-echo-protocol.h"
#include "clients-array.h"
#include "message.h"
#include "log.h"

#include <stdlib.h>

// ======================================================================================

#define MAX_BROADCAST_ECHO_CLIENTS (1024)

static struct lws **clients;
static size_t clients_count;

// ======================================================================================

static void register_message(struct lws **clients, size_t clients_count, struct message *msg, struct lws *wsi)
{
    if (msg == NULL)
        return;

    struct per_session_data__broadcast_echo_protocol *psd;

    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            psd = (struct per_session_data__broadcast_echo_protocol*)lws_wsi_user(clients[i]);
            psd->messages_queue = messages_queue_push(psd->messages_queue, msg);
        }
}

static void send_message(struct lws *wsi, struct per_session_data__broadcast_echo_protocol *psd)
{
    struct message *msg = queue_head(psd->messages_queue);
    if (msg == NULL)
        return;

    lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);
    psd->messages_queue = messages_queue_pop(psd->messages_queue);
}

// ======================================================================================

void init_broadcast_echo_protocol(void)
{
    clients_count = 0;
    clients = (struct lws**)malloc(sizeof(struct lws*) * MAX_BROADCAST_ECHO_CLIENTS);
}

void deinit_broadcast_echo_protocol(void)
{
    free(clients);
}

// ======================================================================================

int callback_broadcast_echo(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data__broadcast_echo_protocol *psd;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (!register_client(clients, &clients_count, wsi, MAX_BROADCAST_ECHO_CLIENTS))
            {
                SERVER_LOG_ERROR("A client couldn't connect -- the server is full!");
                return -1;
            }

            psd = (struct per_session_data__broadcast_echo_protocol*)user;
            psd->messages_queue = new_messages_queue();

            SERVER_LOG_EVENT("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            register_message(clients, clients_count, new_message(in, len, LWS_PRE), wsi);
            lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));

            SERVER_LOG_DATA(in, len);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            psd = (struct per_session_data__broadcast_echo_protocol*)user;
            send_message(wsi, psd);
            break;

        case LWS_CALLBACK_CLOSED:
            psd = (struct per_session_data__broadcast_echo_protocol*)user;
            psd->messages_queue = delete_messages_queue(psd->messages_queue);

            forget_client(clients, &clients_count, wsi);

            SERVER_LOG_EVENT("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

