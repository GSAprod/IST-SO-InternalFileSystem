#include <producer-consumer.h>
#include <stdio.h>



int pcq_create(pc_queue_t *queue, size_t capacity) {
    queue++;
    capacity++;
    return 0;
}



int pcq_destroy(pc_queue_t *queue) {
    queue++;
    return 0;
}



int pcq_enqueue(pc_queue_t *queue, void *elem) {
    queue++;
    elem++;
    return 0;
}



void *pcq_dequeue(pc_queue_t *queue) {
    queue++;
    return NULL;
}