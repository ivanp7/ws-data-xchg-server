#pragma once

#include <libwebsockets.h>

int callback_broadcast_echo(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

