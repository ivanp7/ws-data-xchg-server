#pragma once

#include "queue.h"

#include <libwebsockets.h>

struct per_session_data__bulletin_board_protocol
{
    struct queue_node *messages_queue;

    char *client_name;

    void *data_buffer;
    size_t data_length;
    size_t buffer_length;
};

void init_bulletin_board_protocol(void);
void deinit_bulletin_board_protocol(void);

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

