#include "clients-array.h"

int register_client(struct lws **clients, size_t *clients_count, struct lws *wsi, size_t max_clients_count)
{
    if (*clients_count == max_clients_count)
        return 0;

    clients[*clients_count] = wsi;
    (*clients_count)++;
    return 1;
}

int forget_client(struct lws **clients, size_t *clients_count, struct lws *wsi)
{
    if (*clients_count == 0)
        return 0;

    size_t i = 0;
    while ((i < *clients_count) && (clients[i] != wsi))
        i++;

    if (i < *clients_count)
    {
        while (i < (*clients_count)-1)
        {
            clients[i] = clients[i+1];
            i++;
        }
        (*clients_count)--;
        return 1;
    }
    else
        return 0;
}

