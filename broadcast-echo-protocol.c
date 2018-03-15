#include "server.h"
#include "broadcast-echo-protocol.h"
#include "clients-array.h"
#include "message.h"
#include "log.h"

#include <stdlib.h>

// ======================================================================================

#define MAX_BROADCAST_ECHO_CLIENTS (1024)

static struct lws **clients;
static size_t clients_count;

void init_broadcast_echo_protocol(void)
{
    clients_count = 0;
    clients = malloc(sizeof(struct lws*) * MAX_BROADCAST_ECHO_CLIENTS);
    if (has_memory_allocation_failed(clients, __FILE__, __LINE__))
        stop_server();
}

void deinit_broadcast_echo_protocol(void)
{
    free(clients);
}

// ======================================================================================

static void register_message(struct lws **clients, size_t clients_count, struct lws *wsi, struct message *msg)
{
    if (msg == NULL)
        return;

    struct per_session_data__broadcast_echo_protocol *psd;
    struct queue_node *node;

    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            psd = lws_wsi_user(clients[i]);
            node = messages_queue_push(psd->messages_queue, msg);
            if (has_memory_allocation_failed(node, __FILE__, __LINE__))
            {
                stop_server();
                return;
            }
            psd->messages_queue = node;
        }

    lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));
}

// ======================================================================================

int callback_broadcast_echo(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data__broadcast_echo_protocol *psd = user;
    struct message *msg;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (!register_client(clients, &clients_count, wsi, MAX_BROADCAST_ECHO_CLIENTS))
            {
                server_log_error("A client couldn't connect: registration error.");
                return -1;
            }

            psd->messages_queue = new_messages_queue();

            server_log_event("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            server_log_data(in, len);

            msg = new_message(in, len, LWS_PRE);
            if (has_memory_allocation_failed(msg, __FILE__, __LINE__))
            {
                stop_server();
                break;
            }

            register_message(clients, clients_count, wsi, msg);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            msg = queue_head(psd->messages_queue);
            if (msg == NULL)
                break;

            lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);
            psd->messages_queue = messages_queue_pop(psd->messages_queue);
            break;

        case LWS_CALLBACK_CLOSED:
            forget_client(clients, &clients_count, wsi);
            psd->messages_queue = delete_messages_queue(psd->messages_queue);

            server_log_event("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

