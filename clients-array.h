#pragma once

#include <libwebsockets.h>

int register_client(struct lws **clients, size_t *clients_count, struct lws *wsi, size_t max_clients_count);
int forget_client(struct lws **clients, size_t *clients_count, struct lws *wsi);

