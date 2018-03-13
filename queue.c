#include "queue.h"

#include <stdlib.h>

struct queue_node* queue_push(struct queue_node *queue_head, void *data)
{
    struct queue_node *node = (struct queue_node*)malloc(sizeof(struct queue_node));
    node->data = data;
    node->next = NULL;

    if (queue_head == NULL)
        return node;
    else
    {
        struct queue_node *cur = queue_head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = node;
        return queue_head;
    }
}

void* queue_head_data(struct queue_node *queue_head)
{
    if (queue_head != NULL)
        return queue_head->data;
    else
        return NULL;
}

struct queue_node* queue_pop(struct queue_node *queue_head)
{
    if (queue_head == NULL)
        return NULL;

    struct queue_node *next = queue_head->next;
    free(queue_head);
    return next;
}

