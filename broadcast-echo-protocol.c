#include "broadcast-echo-protocol.h"
#include "clients-array.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

// ======================================================================================

#define MAX_BROADCAST_ECHO_CLIENTS (1024)

static struct lws **clients;
static size_t clients_count;

// ======================================================================================

struct message
{
    void *buffer;
    size_t buffer_length;
    
    int in_queues;
};

static struct message* new_message(void *in, size_t len)
{
    struct message *msg = (struct message*)malloc(sizeof(struct message));
    msg->in_queues = 0;
    msg->buffer_length = len;
    msg->buffer = malloc(LWS_PRE + len);
    memcpy(&((unsigned char*)msg->buffer)[LWS_PRE], in, len);
    return msg;
}

static void delete_message(struct message *msg)
{
    free(msg->buffer);
    free(msg);
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
    struct message *msg;
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
            psd->messages_queue = NULL;

            SERVER_LOG_EVENT("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            SERVER_LOG_DATA(in, len);

            msg = new_message(in, len);

            for (int i = 0; i < clients_count; i++)
                if (clients[i] != wsi)
                {
                    psd = (struct per_session_data__broadcast_echo_protocol*)lws_wsi_user(clients[i]);
                    psd->messages_queue = queue_push(psd->messages_queue, msg);
                    msg->in_queues++;
                }

            lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            psd = (struct per_session_data__broadcast_echo_protocol*)user;
            msg = queue_head_data(psd->messages_queue);
            if (msg == NULL)
                break;
            
            lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);

            psd->messages_queue = queue_pop(psd->messages_queue);
            msg->in_queues--;
            if (msg->in_queues == 0)
                delete_message(msg);
            break;

        case LWS_CALLBACK_CLOSED:
            psd = (struct per_session_data__broadcast_echo_protocol*)user;
            while (psd->messages_queue != NULL)
            {
                msg = queue_head_data(psd->messages_queue);
                psd->messages_queue = queue_pop(psd->messages_queue);

                msg->in_queues--;
                if (msg->in_queues == 0)
                    delete_message(msg);
            }

            forget_client(clients, &clients_count, wsi);

            SERVER_LOG_EVENT("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

