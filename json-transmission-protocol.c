#include "server.h"
#include "json-transmission-protocol.h"
#include "clients-array.h"
#include "queue.h"
#include "message.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

// ======================================================================================

#define MAX_JSON_TRANSMISSION_CLIENTS (1024)

static struct lws **clients;
static size_t clients_count;

void init_json_transmission_protocol(void)
{
    clients_count = 0;
    clients = malloc(sizeof(struct lws*) * MAX_JSON_TRANSMISSION_CLIENTS);
    if (has_memory_allocation_failed(clients))
        stop_server();
}

void deinit_json_transmission_protocol(void)
{
    free(clients);
}

// ======================================================================================

static int find_client_by_name(
        struct lws **clients, size_t clients_count, const char *name, size_t name_len, struct lws **wsi)
{
    *wsi = NULL;

    struct per_session_data__json_transmission_protocol *p;

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

// ======================================================================================

static int queue_message(
        struct lws *wsi, struct per_session_data__json_transmission_protocol *psd, struct message *msg)
{
    struct queue_node *node = messages_queue_push(psd->messages_queue, msg);
    if (has_memory_allocation_failed(node))
        return 0;
    psd->messages_queue = node;
    
    lws_callback_on_writable(wsi);
    return 1;
}

static int respond_with_data(
        struct lws *wsi, struct per_session_data__json_transmission_protocol *psd, void *data_buffer, size_t len)
{
    struct message *msg = new_message(data_buffer, len);
    if (has_memory_allocation_failed(msg))
        return 0;

    if (!queue_message(wsi, psd, msg))
    {
        delete_message(msg);
        return 0;
    }

    return 1;
}

enum JSON_TRANSMISSION_PROTOCOL_STATUS
{
    STATUS_OK = 0,
    STATUS_PARSE_ERROR
};

static int respond_with_status(
        struct lws *wsi, struct per_session_data__json_transmission_protocol *psd, 
        enum JSON_TRANSMISSION_PROTOCOL_STATUS s)
{
    int result = 1;

    cJSON *json = cJSON_CreateObject();
    if ((json == NULL) || (cJSON_AddStringToObject(json, "type", "proxy_response") == NULL))
        goto end;
        
    switch (s)
    {
        case STATUS_PARSE_ERROR:
            if (cJSON_AddStringToObject(json, "status", "parse_error") == NULL)
                goto end;
            break;

        case STATUS_OK:
        default:
            goto end;
    }

    char *string = cJSON_PrintUnformatted(json);
    size_t string_len = strlen(string) + 1;
    void *data_buffer = new_data_buffer(string, string_len);
    free(string);
    if (data_buffer == NULL)
        return 0;
    result = respond_with_data(wsi, psd, data_buffer, string_len);

end:
    cJSON_Delete(json);
    return result;
}

// ======================================================================================

static int forward_message(
        struct lws **clients, size_t clients_count, struct lws *wsi, 
        struct per_session_data__json_transmission_protocol *psd, void *in, size_t len)
{
    if (len == 0)
        return 1;

    int result = 1;

    cJSON *json;
    if (((char*)in)[len-1] != '\0')
    {
        char *string = malloc(len + 1);
        strncpy(string, in, len);
        string[len] = '\0';
        json = cJSON_Parse(string);
        free(string);
    }
    else
        json = cJSON_Parse((char*)in);

    if (json == NULL)
    {
        result = respond_with_status(wsi, psd, STATUS_PARSE_ERROR);
        return result;
    }

    const cJSON *from = cJSON_GetObjectItemCaseSensitive(json, "from");
    if (!cJSON_IsString(from) || (from->valuestring == NULL) || 
            (strlen(from->valuestring) == 0))
    {
        /* result = respond_with_status(wsi, psd, STATUS_PARSE_ERROR); */
        /* goto end; */
    }
    else
        strncpy(psd->client_name, from->valuestring, MAX_CLIENT_NAME_LENGTH);

    const cJSON *to = cJSON_GetObjectItemCaseSensitive(json, "to");
    if (!cJSON_IsString(to) || (to->valuestring == NULL) || 
            (strlen(to->valuestring) == 0))
    {
        /* result = respond_with_status(wsi, psd, STATUS_PARSE_ERROR); */
        goto end;
    }

    struct lws *target_wsi;
    if (!find_client_by_name(clients, clients_count, to->valuestring, strlen(to->valuestring), &target_wsi))
        goto end;

    char *response_buf = new_data_buffer(in, len);
    if (has_memory_allocation_failed(response_buf))
    {
        result = 0;
        goto end;
    }
    struct message *msg = new_message(response_buf, len);
    if (msg == NULL)
    {
        result = 0;
        goto end;
    }

    result = queue_message(target_wsi, lws_wsi_user(target_wsi), msg);
    if (!result)
        delete_message(msg);

end:
    cJSON_Delete(json);
    return result;
}

// ======================================================================================

int callback_json_transmission_protocol(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data__json_transmission_protocol *psd = user;

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            if (!register_client(clients, &clients_count, wsi, MAX_JSON_TRANSMISSION_CLIENTS))
            {
                server_log_error("A client couldn't connect: registration error.");
                return -1;
            }

            psd->messages_queue = new_messages_queue();
            memset(psd->client_name, '\0', MAX_CLIENT_NAME_LENGTH+1);

            server_log_event("A client has connected.");
            break;

        case LWS_CALLBACK_RECEIVE:
            server_log_data(in, len);

            if (!forward_message(clients, clients_count, wsi, psd, in, len))
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

            server_log_event("A client has disconnected.");
            break;

        default:
            break;
    }

    return 0;
}

