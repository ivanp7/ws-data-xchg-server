#pragma once

#include <libwebsockets.h>
#include <cjson/cJSON.h>

#define MAX_CLIENT_NAME_LENGTH (15)

struct per_session_data__json_transmission_protocol
{
    struct queue_node *messages_queue;

    char client_name[MAX_CLIENT_NAME_LENGTH+1];
};

void init_json_transmission_protocol(void);
void deinit_json_transmission_protocol(void);

int callback_json_transmission_protocol(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

