#pragma once

#include <libwebsockets.h>

struct per_session_data__broadcast_echo_protocol
{
    struct queue_node *messages_queue;
};

void init_broadcast_echo_protocol(void);
void deinit_broadcast_echo_protocol(void);

int callback_broadcast_echo(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

