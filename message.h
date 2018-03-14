#pragma once

#include "queue.h"

#include <stddef.h>

struct message
{
    void *buffer;
    size_t buffer_length;

    int in_queues;
};

struct message* new_message(void *in, size_t len, size_t prefix_len);
void delete_message(struct message *msg);
struct queue_node* new_messages_queue();
struct queue_node* delete_messages_queue(struct queue_node *messages_queue);
struct queue_node* messages_queue_push(struct queue_node *messages_queue, struct message *msg);
struct queue_node* messages_queue_pop(struct queue_node *messages_queue);
    
