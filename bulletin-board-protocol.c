#include "bulletin-board-protocol.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

// ======================================================================================

#define EXAMPLE_RX_BUFFER_BYTES (10)
struct payload
{
    unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
    size_t len;
} received_payload;

int callback_bulletin_board(
        struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_RECEIVE:
            /* memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len ); */
            /* received_payload.len = len; */
            lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            /* lws_write( wsi, &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], received_payload.len, LWS_WRITE_TEXT ); */
            break;

        default:
            break;
    }

    return 0;
}

