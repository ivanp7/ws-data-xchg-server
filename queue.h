#pragma once

struct queue_node
{
    void *data;
    struct queue_node *next;
};

struct queue_node* queue_push(struct queue_node *queue_head, void *data);
void* queue_head_data(struct queue_node *queue_head);
struct queue_node* queue_pop(struct queue_node *queue_head);

