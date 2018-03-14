#include "queue.h"

#include <stdlib.h>

struct queue_node* queue_push(struct queue_node *queue, void *data)
{
    struct queue_node *node = (struct queue_node*)malloc(sizeof(struct queue_node));
    node->data = data;
    node->next = NULL;

    if (queue == NULL)
        return node;
    else
    {
        struct queue_node *cur = queue;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = node;
        return queue;
    }
}

void* queue_head(struct queue_node *queue)
{
    if (queue != NULL)
        return queue->data;
    else
        return NULL;
}

struct queue_node* queue_pop(struct queue_node *queue)
{
    if (queue == NULL)
        return NULL;

    struct queue_node *next = queue->next;
    free(queue);
    return next;
}

