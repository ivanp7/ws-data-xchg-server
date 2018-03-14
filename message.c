#include "message.h"

#include <stdlib.h>
#include <string.h>

struct message* new_message(void *in, size_t len, size_t prefix_len)
{
    struct message *msg = malloc(sizeof(struct message));

    msg->buffer_length = len;
    msg->buffer = malloc(prefix_len + len);
    memcpy(&((unsigned char*)msg->buffer)[prefix_len], in, len);
    msg->in_queues = 0;

    return msg;
}

void delete_message(struct message *msg)
{
    free(msg->buffer);
    free(msg);
}

struct queue_node* new_messages_queue()
{
    return NULL;
}

struct queue_node* delete_messages_queue(struct queue_node *messages_queue)
{
    while (messages_queue != NULL)
        messages_queue = messages_queue_pop(messages_queue);

    return messages_queue;
}

struct queue_node* messages_queue_push(struct queue_node *messages_queue, struct message *msg)
{
    msg->in_queues++;
    return queue_push(messages_queue, msg);
}

struct queue_node* messages_queue_pop(struct queue_node *messages_queue)
{
    struct message *msg = queue_head(messages_queue);
    if (msg != NULL)
    {
        msg->in_queues--;
        if (msg->in_queues == 0)
            delete_message(msg);
    }

    return queue_pop(messages_queue);
}

