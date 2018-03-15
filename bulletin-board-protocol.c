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
    if (has_memory_allocation_failed(clients, __FILE__, __LINE__))
        stop_server();
}

void deinit_bulletin_board_protocol(void)
{
    free(clients);
}

// ======================================================================================

#define MAX_CLIENT_NAME_LENGTH (16-1)

static int is_client_name_valid(char *name, size_t len)
{
    if ((name == NULL) || (len == 0) || (len > MAX_CLIENT_NAME_LENGTH))
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

static void get_client_list(
        struct lws **clients, size_t clients_count, struct lws *wsi, char **response_buf, size_t *response_len)
{
    struct per_session_data__bulletin_board_protocol *p;
    size_t pos, size;

    *response_len = 2;
    if (clients_count > 1)
        *response_len += clients_count-2;

    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            p = lws_wsi_user(clients[i]);
            *response_len += strlen(p->client_name);
        }

    *response_buf = malloc(*response_len);
    if (*response_buf == NULL)
        return;

    (*response_buf)[0] = 'L';
    (*response_buf)[1] = ':';

    pos = 2;
    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            p = lws_wsi_user(clients[i]);
            size = strlen(p->client_name);
            memcpy(&(*response_buf)[pos], p->client_name, size);
            pos += size;

            if (pos < *response_len)
            {
                (*response_buf)[pos] = ',';
                pos++;
            }
        }
}

static void parse_client_list(
        struct lws **clients, size_t clients_count, const char *list_str, size_t len, 
        struct lws ***selected_clients, int *num_clients)
{
    // TODO
}

static void find_client_by_name(
        struct lws **clients, size_t clients_count, const char *name, 
        struct lws **wsi, struct per_session_data__bulletin_board_protocol **psd)
{
    *wsi = NULL;
    *psd = NULL;

    struct per_session_data__bulletin_board_protocol *p;

    for (size_t i = 0; i < clients_count; i++)
    {
        p = lws_wsi_user(clients[i]);
        if (strcmp(p->client_name, name) == 0)
        {
            *wsi = clients[i];
            *psd = p;
            return;
        }
    }
}

// ======================================================================================

#define STATUS_OK   'O'
#define STATUS_FAIL 'F'

static void response(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, void *data, size_t len)
{
    struct message *msg = new_message(data, len, LWS_PRE);
    if (has_memory_allocation_failed(msg, __FILE__, __LINE__))
    {
        stop_server();
        return;
    }

    struct queue_node *node = messages_queue_push(psd->messages_queue, msg);
    if (has_memory_allocation_failed(node, __FILE__, __LINE__))
    {
        delete_message(msg);
        stop_server();
        return;
    }

    psd->messages_queue = node;
    lws_callback_on_writable(wsi);
}

static void response_with_status(struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, char s)
{
    char *buf = malloc(1);
    if (has_memory_allocation_failed(buf, __FILE__, __LINE__))
    {
        stop_server();
        return;
    }

    buf[0] = s;
    response(wsi, psd, buf, 1);
}

// ======================================================================================

static void accept_client_name(
        struct lws **clients, size_t *clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (is_client_name_valid((char*)in, len) && 
            register_client(clients, clients_count, wsi, MAX_BULLETIN_BOARD_CLIENTS))
    {
        psd->client_name = malloc(len+1);
        if (has_memory_allocation_failed(psd->client_name, __FILE__, __LINE__))
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

static void update_client_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (len > psd->buffer_length)
    {
        void *new_buffer = realloc(psd->data_buffer, len);
        if (has_memory_allocation_failed(new_buffer, __FILE__, __LINE__))
        {
            response_with_status(wsi, psd, STATUS_FAIL);
            return;
        }

        psd->data_buffer = new_buffer;
        psd->buffer_length = len;
    }

    memcpy(psd->data_buffer, in, len);
    psd->data_length = len;

    response_with_status(wsi, psd, STATUS_OK);
}

static void response_with_stored_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd,
        struct lws **selected_clients, int num_clients)
{
    // TODO
}

static void response_with_sent_data(
        struct lws **selected_clients, int num_clients, void *data, size_t len)
{
    // TODO
}

// ======================================================================================

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

    char *response_buf = NULL;
    size_t response_len = 0;

    size_t pos;

    struct lws **selected_clients;
    int num_clients;

    switch (((char*)in)[0])
    {
        /* case 'l': */
        case 'L':
            get_client_list(clients, *clients_count, wsi, &response_buf, &response_len);
            if (has_memory_allocation_failed(response_buf, __FILE__, __LINE__))
            {
                stop_server();
                break;
            }

            response(wsi, psd, response_buf, response_len);
            break;

        /* case 'p': */
        case 'P':
            if ((len < 2) || (((char*)in)[1] != ':'))
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            update_client_data(wsi, psd, (char*)in + 2, len-2);
            break;

        /* case 'r': */
        case 'R':
            if ((len < 2) || (((char*)in)[1] != ':'))
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            parse_client_list(clients, *clients_count, (char*)in + 2, len-2, &selected_clients, &num_clients);
            if (num_clients < 0)
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            response_with_stored_data(wsi, psd, selected_clients, num_clients);
            free(selected_clients);
            break;

        /* case 's': */
        case 'S':
            if ((len < 2) || (((char*)in)[1] != ':'))
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            pos = 1337; // TODO
            if (pos < 0)
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            parse_client_list(clients, *clients_count, (char*)in + 2, pos-2, &selected_clients, &num_clients);
            if (num_clients < 0)
            {
                response_with_status(wsi, psd, STATUS_FAIL);
                break;
            }

            response_with_sent_data(selected_clients, num_clients, (char*)in + pos+1, len-pos-1);
            free(selected_clients);
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
    struct per_session_data__bulletin_board_protocol *psd = user;
    struct message *msg;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (clients_count == MAX_BULLETIN_BOARD_CLIENTS)
            {
                server_log_error("A client couldn't connect: the server is full!");
                return -1;
            }

            psd->messages_queue = new_messages_queue();
            psd->client_name = NULL;
            psd->data_buffer = NULL;
            psd->data_length = 0;
            psd->buffer_length = 0;

            server_log_event("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            server_log_data(in, len);

            process_request(clients, &clients_count, wsi, psd, in, len);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (psd->client_name == NULL)
                break;

            msg = queue_head(psd->messages_queue);
            if (msg == NULL)
                break;

            lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);
            psd->messages_queue = messages_queue_pop(psd->messages_queue);
            break;

        case LWS_CALLBACK_CLOSED:
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

