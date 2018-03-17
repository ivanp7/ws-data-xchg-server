#include "server.h"
#include "bulletin-board-protocol.h"
#include "clients-array.h"
#include "queue.h"
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
    if (has_memory_allocation_failed(clients))
        stop_server();
}

void deinit_bulletin_board_protocol(void)
{
    free(clients);
}

// ======================================================================================

static int get_client_list(
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

    *response_buf = new_data_buffer(NULL, *response_len);
    if (has_memory_allocation_failed(*response_buf))
        return 0;
    char *response_buf_data = data_buffer_data(*response_buf);

    response_buf_data[0] = 'L';
    response_buf_data[1] = ':';

    pos = 2;
    for (size_t i = 0; i < clients_count; i++)
        if (clients[i] != wsi)
        {
            p = lws_wsi_user(clients[i]);
            size = strlen(p->client_name);
            memcpy(&response_buf_data[pos], p->client_name, size);
            pos += size;

            if (pos < *response_len)
            {
                response_buf_data[pos] = ',';
                pos++;
            }
        }

    return 1;
}

static int find_client_by_name(
        struct lws **clients, size_t clients_count, const char *name, size_t name_len, struct lws **wsi)
{
    *wsi = NULL;

    struct per_session_data__bulletin_board_protocol *p;

    for (size_t i = 0; i < clients_count; i++)
    {
        p = lws_wsi_user(clients[i]);
        if (memcmp(p->client_name, name, name_len) == 0)
        {
            *wsi = clients[i];
            return 1;
        }
    }

    return 0;
}

static int is_client_name_valid(const char *name, size_t len)
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

static int parse_client_list(
        struct lws **clients, size_t clients_count, const char *list_str, size_t len, 
        struct queue_node **selected_clients)
{
    *selected_clients = NULL;
    if (len == 0)
        return 1;

    struct queue_node *node;

    struct lws *wsi;

    char *token = memchr(list_str, ',', len);
    size_t str_len;
    while (1)
    {
        if (token == NULL)
            str_len = len;
        else
            str_len = token - list_str;

        if (!is_client_name_valid(list_str, str_len))
            return 0;

        find_client_by_name(clients, clients_count, list_str, str_len, &wsi);
        if (wsi != NULL)
        {
            node = queue_push(*selected_clients, wsi);
            if (node == NULL)
                return 0;
            *selected_clients = node;
        }

        if (token == NULL)
            break;
        else
        {
            list_str = token+1;
            len -= str_len+1;
            token = memchr(list_str, ',', len);
        }
    }

    return 1;
}

// ======================================================================================

#define STATUS_LENGTH 1

#define STATUS_OK   "K"
#define STATUS_FAIL "F"

static int respond(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, struct message *msg)
{
    struct queue_node *node = messages_queue_push(psd->messages_queue, msg);
    if (has_memory_allocation_failed(node))
        return 0;
    psd->messages_queue = node;
    
    lws_callback_on_writable(wsi);
    return 1;
}

static int respond_with_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, void *data_buffer, size_t len)
{
    struct message *msg = new_message(data_buffer, len);
    if (has_memory_allocation_failed(msg))
        return 0;

    if (!respond(wsi, psd, msg))
    {
        delete_message(msg);
        return 0;
    }

    return 1;
}

static int respond_with_status(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, const char *s)
{
    void *data_buffer = new_data_buffer(s, STATUS_LENGTH);
    if (data_buffer == NULL)
        return 0;

    return respond_with_data(wsi, psd, data_buffer, STATUS_LENGTH);
}

// ======================================================================================

static int update_client_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd, const void *data, size_t len)
{
    if (len > psd->buffer_length)
    {
        void *new_buffer = realloc(psd->data_buffer, len);
        if (has_memory_allocation_failed(new_buffer))
            return respond_with_status(wsi, psd, STATUS_FAIL);

        psd->data_buffer = new_buffer;
        psd->buffer_length = len;
    }

    memcpy(psd->data_buffer, data, len);
    psd->data_length = len;

    return respond_with_status(wsi, psd, STATUS_OK);
}

static int respond_with_stored_data(
        struct lws *wsi, struct per_session_data__bulletin_board_protocol *psd,
        struct queue_node *selected_clients)
{
    if (selected_clients == NULL)
        return 1;

    struct per_session_data__bulletin_board_protocol *p;

    size_t name_len;
    size_t response_len;
    void *response_buf;
    char *response_buf_data;

    int result;
    while (selected_clients != NULL)
    {
        p = lws_wsi_user(queue_head(selected_clients));

        name_len = strlen(p->client_name);

        response_len = 2 + name_len + 1 + p->data_length;
        response_buf = new_data_buffer(NULL, response_len);
        if (has_memory_allocation_failed(response_buf))
        {
            result = 0;
            break;
        }

        response_buf_data = data_buffer_data(response_buf);
        response_buf_data[0] = 'R';
        response_buf_data[1] = ':';
        memcpy(response_buf_data + 2, p->client_name, name_len);
        response_buf_data[2+name_len] = ':';
        memcpy(response_buf_data + 2+name_len+1, p->data_buffer, p->data_length);

        result = respond_with_data(wsi, psd, response_buf, response_len);
        if (!result)
            break;

        selected_clients = selected_clients->next;
    }

    return result;
}

static int respond_with_sent_data(
        struct per_session_data__bulletin_board_protocol *psd, struct queue_node *selected_clients, 
        void *data, size_t len)
{
    if (selected_clients == NULL)
        return 1;

    struct lws *wsi;
    struct per_session_data__bulletin_board_protocol *p;

    size_t name_len = strlen(psd->client_name);
    size_t response_len = 2 + name_len + 1 + len;
    char *response_buf = new_data_buffer(NULL, response_len);
    if (has_memory_allocation_failed(response_buf))
        return 0;
    char *response_buf_data = data_buffer_data(response_buf);

    response_buf_data[0] = 'S';
    response_buf_data[1] = ':';
    memcpy(response_buf_data + 2, psd->client_name, name_len);
    response_buf_data[2+name_len] = ':';
    memcpy(response_buf_data + 2+name_len+1, data, len);

    struct message *msg = new_message(response_buf, response_len);

    int result;
    while (selected_clients != NULL)
    {
        wsi = queue_head(selected_clients);
        p = lws_wsi_user(wsi);

        result = respond(wsi, p, msg);
        if (!result)
        {
            delete_message(msg);
            break;
        }

        selected_clients = selected_clients->next;
    }

    return result;
}

// ======================================================================================

static int accept_client_name(
        struct lws **clients, size_t *clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (is_client_name_valid((char*)in, len) && 
            register_client(clients, clients_count, wsi, MAX_BULLETIN_BOARD_CLIENTS))
    {
        memcpy(psd->client_name, in, len);

        server_log_event("A client '%s' has registered.", psd->client_name);
        return respond_with_status(wsi, psd, STATUS_OK);
    }
    else
    {
        server_log_error("A client couldn't register!");
        return respond_with_status(wsi, psd, STATUS_FAIL);
    }
}

static int process_request(
        struct lws **clients, size_t *clients_count, struct lws *wsi, 
        struct per_session_data__bulletin_board_protocol *psd, void *in, size_t len)
{
    if (len == 0)
        return respond_with_status(wsi, psd, STATUS_FAIL);

    if (strlen(psd->client_name) == 0)
        return accept_client_name(clients, clients_count, wsi, psd, in, len);

    struct queue_node *selected_clients;

    int result;
    switch (((char*)in)[0])
    {
        /* case 'l': */
        case 'L': ; // empty statement
            if (len > 1)
                return respond_with_status(wsi, psd, STATUS_FAIL);

            char *response_buf;
            size_t response_len;
            if (!get_client_list(clients, *clients_count, wsi, &response_buf, &response_len))
                return 0;

            result = respond_with_data(wsi, psd, response_buf, response_len);
            return result;

        /* case 'p': */
        case 'P':
            if ((len < 2) || (((char*)in)[1] != ':'))
                return respond_with_status(wsi, psd, STATUS_FAIL);

            return update_client_data(wsi, psd, (char*)in + 2, len-2);

        /* case 'r': */
        case 'R':
            if ((len < 2) || (((char*)in)[1] != ':'))
                return respond_with_status(wsi, psd, STATUS_FAIL);

            if (parse_client_list(clients, *clients_count, (char*)in + 2, len-2, &selected_clients))
                result = respond_with_stored_data(wsi, psd, selected_clients);
            else
                result = respond_with_status(wsi, psd, STATUS_FAIL);

            while (selected_clients != NULL)
                selected_clients = queue_pop(selected_clients);
            return result;

        /* case 's': */
        case 'S':
            if ((len < 3) || (((char*)in)[1] != ':'))
                return respond_with_status(wsi, psd, STATUS_FAIL);

            void *delim = memchr((char*)in + 2, ':', len-2);
            int pos = (delim != NULL)? (char*)delim - ((char*)in + 2): -1;
            if (pos < 0)
                return respond_with_status(wsi, psd, STATUS_FAIL);

            if (parse_client_list(clients, *clients_count, (char*)in + 2, pos, &selected_clients))
                result = respond_with_sent_data(psd, selected_clients, (char*)in + 2+pos+1, len-2-pos-1);
            else
                result = respond_with_status(wsi, psd, STATUS_FAIL);

            while (selected_clients != NULL)
                selected_clients = queue_pop(selected_clients);
            return result;

        default:
            return respond_with_status(wsi, psd, STATUS_FAIL);
    }
}

// ======================================================================================

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data__bulletin_board_protocol *psd = user;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (clients_count == MAX_BULLETIN_BOARD_CLIENTS)
            {
                server_log_error("A client couldn't connect: the server is full!");
                return -1;
            }

            psd->messages_queue = new_messages_queue();
            memset(psd->client_name, '\0', MAX_CLIENT_NAME_LENGTH+1);
            psd->data_buffer = NULL;
            psd->data_length = 0;
            psd->buffer_length = 0;

            server_log_event("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            server_log_data(in, len);

            if (!process_request(clients, &clients_count, wsi, psd, in, len))
                stop_server();
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (strlen(psd->client_name) == 0)
                break;

            if (psd->messages_queue == NULL)
                break;

            struct message *msg = queue_head(psd->messages_queue);
            lws_write(wsi, &((unsigned char*)msg->buffer)[LWS_PRE], msg->buffer_length, LWS_WRITE_BINARY);
            psd->messages_queue = messages_queue_pop(psd->messages_queue);
            if (psd->messages_queue != NULL)
                lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLOSED:
            if (psd->client_name != NULL)
                forget_client(clients, &clients_count, wsi);

            psd->messages_queue = delete_messages_queue(psd->messages_queue);
            free(psd->data_buffer);

            server_log_event("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

