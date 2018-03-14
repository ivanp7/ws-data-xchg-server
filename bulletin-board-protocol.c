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

// ======================================================================================

void init_bulletin_board_protocol(void)
{
    clients_count = 0;
    clients = (struct lws**)malloc(sizeof(struct lws*) * MAX_BULLETIN_BOARD_CLIENTS);
}

void deinit_bulletin_board_protocol(void)
{
    free(clients);
}

// ======================================================================================

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_RECEIVE:
            /* memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len ); */
            /* received_payload.len = len; */
            /* lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) ); */
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            /* lws_write( wsi, &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], received_payload.len, LWS_WRITE_TEXT ); */
            break;

        default:
            break;
    }

    return 0;
}

