#include <producer-consumer.h>
#include <pthread.h>
#include <stdio.h>



int pcq_create(pc_queue_t *queue, size_t capacity) {
    void *queue_elements[capacity];
    queue->pcq_buffer = queue_elements;

    queue->pcq_capacity = capacity;

    pthread_mutex_init(&(queue->pcq_current_size_lock), NULL);
    queue->pcq_current_size = 0;

    pthread_mutex_init(&(queue->pcq_head_lock), NULL);
    queue->pcq_head = capacity;

    pthread_mutex_init(&(queue->pcq_tail_lock), NULL);
    queue->pcq_tail = 0;

    pthread_mutex_init(&(queue->pcq_pusher_condvar_lock), NULL);
    pthread_cond_init(&(queue->pcq_pusher_condvar), NULL);

    pthread_mutex_init(&(queue->pcq_popper_condvar_lock), NULL);
    pthread_cond_init(&(queue->pcq_popper_condvar), NULL);
    return 0;
}



int pcq_destroy(pc_queue_t *queue) {
    // TODO: Destroy mutexes, condvars, etc. (please wait)
    queue++;
    return 0;
}

int pcq_isEmpty(pc_queue_t *queue) {
    // Add locking for mutexes of head and tail
    int condition = (queue->pcq_head % queue->pcq_capacity == queue->pcq_tail);
    // Add unlocking for mutexes of head and tail
    return condition;
}

int pcq_isFull(pc_queue_t *queue) {
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    int condition = (queue->pcq_capacity == queue->pcq_current_size);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    return condition;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
    while (pcq_isEmpty(queue)) 
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
    
    queue->pcq_buffer[queue->pcq_tail++] = elem;
    queue->pcq_tail = queue->pcq_tail % queue->pcq_capacity;
    queue->pcq_current_size++;
    pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);
    return 0;
}



void *pcq_dequeue(pc_queue_t *queue) {
    queue->pcq_head = queue->pcq_head % queue->pcq_capacity;
    void* elem = queue->pcq_buffer[queue->pcq_head++];
    queue->pcq_current_size--;
    return elem;
}