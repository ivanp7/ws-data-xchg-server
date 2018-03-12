#pragma once

#include <libwebsockets.h>

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

