#include "server.h"
#include "bulletin-board-protocol.h"
#include "clients-array.h"
#include "message.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

// ======================================================================================

#define MAX_BULLETIN_BOARD_CLIENTS (1024)

static struct lws **clients;
static size_t clients_count;

void init_bulletin_board_protocol(void)
{
    clients_count = 0;
    clients = malloc(sizeof(struct lws*) * MAX_BULLETIN_BOARD_CLIENTS);
    if (is_memory_allocation_failed(clients, __FILE__, __LINE__))
        stop_server();
}

void deinit_bulletin_board_protocol(void)
{
    free(clients);
}

// ======================================================================================

#define STATUS_OK 'O'
#define STATUS_FAIL 'F'

static void response(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, void *data, size_t len)
{
    struct message *msg = new_message(data, len, LWS_PRE);
    if (msg != NULL)
    {
        struct queue_node *node = messages_queue_push(psd->messages_queue, msg);
        if (node != NULL)
        {
            psd->messages_queue = node;
            lws_callback_on_writable(wsi);
        }
    }
}

static void response_with_status(struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, char s)
{
    char *buf = malloc(1);
    if (is_memory_allocation_failed(buf, __FILE__, __LINE__))
    {
        stop_server();
        return;
    }

    buf[0] = s;
    response(wsi, psd, buf, 1);
}

static int is_client_name_valid(char *name, size_t len)
{
    if ((name == NULL) || (len == 0))
        return 0;

    char c;
    for (int i = 0; i < len; i++)
    {
        c = name[i];
        if (! (((c >= 'A') && (c <= 'Z')) || 
               ((c >= 'a') && (c <= 'z')) ||
               ((c >= '0') && (c <= '9')) || 
               (c == '-') || (c == '_')))
            return 0;
    }

    return 1;
}

static void accept_client_name(
        struct lws **clients, size_t *clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (is_client_name_valid((char*)in, len) && 
            register_client(clients, clients_count, wsi, MAX_BULLETIN_BOARD_CLIENTS))
    {
        psd->client_name = malloc(len+1);
        if (is_memory_allocation_failed(psd->client_name, __FILE__, __LINE__))
        {
            forget_client(clients, clients_count, wsi);
            stop_server();
            return;
        }

        memcpy(psd->client_name, in, len);
        psd->client_name[len] = '\0';

        server_log_event("A client '%s' has registered.", psd->client_name);
        response_with_status(wsi, psd, STATUS_OK);
    }
    else
    {
        server_log_error("A client couldn't register!");
        response_with_status(wsi, psd, STATUS_FAIL);
    }
}

static void response_with_client_list(
        struct lws **clients, size_t clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    struct per_session_data__bulletin_board_protocol *p;

    size_t response_len = 2;
    char *response_buf;

    size_t pos, size;

    if (clients_count > 1)
        response_len += clients_count-2;

    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            p = lws_wsi_user(clients[i]);
            response_len += strlen(p->client_name);
        }

    response_buf = malloc(response_len);
    if (is_memory_allocation_failed(response_buf, __FILE__, __LINE__))
    {
        stop_server();
        return;
    }

    response_buf[0] = 'L';
    response_buf[1] = ':';

    pos = 2;
    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            p = lws_wsi_user(clients[i]);
            size = strlen(p->client_name);
            memcpy(&response_buf[pos], p->client_name, size);
            pos += size;

            if (pos < response_len)
            {
                response_buf[pos] = ',';
                pos++;
            }
        }

    response(wsi, psd, response_buf, response_len);
}

static void update_client_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if ((len < 2) || (((char*)in)[1] != ':'))
    {
        response_with_status(wsi, psd, STATUS_FAIL);
        return;
    }

    len -= 2;
    in = (char*)in + 2;

    if (len > psd->buffer_length)
    {
        psd->data_buffer = realloc(psd->data_buffer, len);
        if (is_memory_allocation_failed(psd->data_buffer, __FILE__, __LINE__))
        {
            stop_server();
            return;
        }

        psd->buffer_length = len;
    }

    memcpy(psd->data_buffer, in, len);
    psd->data_length = len;

    response_with_status(wsi, psd, STATUS_OK);
}

static void process_request(
        struct lws **clients, size_t *clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (len == 0)
        return;

    if (psd->client_name == NULL)
    {
        accept_client_name(clients, clients_count, wsi, psd, in, len);
        return;
    }

    switch (((char*)in)[0])
    {
        case 'l':
        case 'L':
            response_with_client_list(clients, *clients_count, wsi, psd, in, len);
            break;

        case 'p':
        case 'P':
            update_client_data(wsi, psd, in, len);
            break;

        case 'r':
        case 'R':
            break;

        case 's':
        case 'S':
            break;

        default:
            response_with_status(wsi, psd, STATUS_FAIL);
            break;
    }
}

// ======================================================================================

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data__bulletin_board_protocol *psd;
    struct message *msg;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (clients_count == MAX_BULLETIN_BOARD_CLIENTS)
            {
                server_log_error("A client couldn't connect: the server is full!");
                return -1;
            }

            psd = user;
            psd->messages_queue = new_messages_queue();
            psd->client_name = NULL;
            psd->data_buffer = NULL;
            psd->data_length = 0;
            psd->buffer_length = 0;

            server_log_event("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            server_log_data(in, len);

            psd = user;
            process_request(clients, &clients_count, wsi, psd, in, len);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            psd = user;
            if (psd->client_name == NULL)
                break;

            msg = queue_head(psd->messages_queue);
            if (msg == NULL)
                break;

            lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);
            psd->messages_queue = messages_queue_pop(psd->messages_queue);
            break;

        case LWS_CALLBACK_CLOSED:
            psd = user;
            if (psd->client_name != NULL)
                forget_client(clients, &clients_count, wsi);

            psd->messages_queue = delete_messages_queue(psd->messages_queue);
            free(psd->client_name);
            free(psd->data_buffer);

            server_log_event("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

