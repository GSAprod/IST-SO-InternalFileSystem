#include <producer-consumer.h>
#include "../utils/wire_protocol.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int pcq_create(pc_queue_t *queue, size_t capacity) {

    queue->pcq_buffer = malloc(capacity * sizeof(void*));
    for (int i=0; i<5; i++) {
        queue->pcq_buffer[i] = malloc(capacity * sizeof(void));
    }

    queue->pcq_capacity = capacity;

    pthread_mutex_init(&(queue->pcq_current_size_lock), NULL);
    queue->pcq_current_size = 0;

    pthread_mutex_init(&(queue->pcq_head_lock), NULL);
    queue->pcq_head = 0;

    pthread_mutex_init(&(queue->pcq_tail_lock), NULL);
    queue->pcq_tail = 0;

    pthread_mutex_init(&(queue->pcq_pusher_condvar_lock), NULL);
    pthread_cond_init(&(queue->pcq_pusher_condvar), NULL);

    pthread_mutex_init(&(queue->pcq_popper_condvar_lock), NULL);
    pthread_cond_init(&(queue->pcq_popper_condvar), NULL);
    return 0;
}



int pcq_destroy(pc_queue_t *queue) {
    
    for (int i=0; i<5; i++) {
        free(queue->pcq_buffer[i]);
    }
    free(*queue->pcq_buffer);

    pthread_mutex_destroy(&(queue->pcq_current_size_lock));
    pthread_mutex_destroy(&(queue->pcq_head_lock));
    pthread_mutex_destroy(&(queue->pcq_tail_lock));
    pthread_mutex_destroy(&(queue->pcq_pusher_condvar_lock));
    pthread_mutex_destroy(&(queue->pcq_popper_condvar_lock));
    pthread_cond_destroy(&(queue->pcq_pusher_condvar));
    pthread_cond_destroy(&(queue->pcq_popper_condvar));
    return 0;
}

//true=1, false=0
int pcq_isEmpty(pc_queue_t *queue) {

    return queue->pcq_head == queue->pcq_tail;
}

//true=1, false=0
int pcq_isFull(pc_queue_t *queue) {
    
    return queue->pcq_capacity == queue->pcq_current_size;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
    //pthread_mutex_lock(&queue->pcq_current_size_lock);
    //pthread_mutex_lock(&queue->pcq_tail_lock);

    

    while (pcq_isFull(queue)) {
        printf("enqueue loop\n");
        pthread_cond_wait(&(queue->pcq_pusher_condvar), &(queue->pcq_pusher_condvar_lock));
    }


    memcpy((queue->pcq_buffer[queue->pcq_tail]), (char*) elem, 1026);
    queue->pcq_tail++;
    queue->pcq_tail = queue->pcq_tail % queue->pcq_capacity;
    queue->pcq_current_size++;

    //TO ASK: interessa ordem dos unlocks?
    //TO ASK: usar trylocks?

    pthread_cond_signal(&(queue->pcq_popper_condvar)); // Signal or broadcast?
    pthread_mutex_unlock(&(queue->pcq_pusher_condvar_lock));
    //pthread_mutex_unlock(&queue->pcq_current_size_lock);
    //pthread_mutex_unlock(&queue->pcq_tail_lock);

    return 0;
}



void *pcq_dequeue(pc_queue_t *queue) {
    pthread_mutex_lock(&(queue->pcq_popper_condvar_lock));
    //pthread_mutex_lock(&queue->pcq_current_size_lock);
    //pthread_mutex_lock(&queue->pcq_head_lock);


    while (pcq_isEmpty(queue)) {
        printf("dequeue loop\n");
        pthread_cond_wait(&(queue->pcq_popper_condvar), &(queue->pcq_popper_condvar_lock));
    }


    void* elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head++;
    queue->pcq_head = queue->pcq_head % queue->pcq_capacity;
    queue->pcq_current_size--;

    pthread_cond_signal(&(queue->pcq_pusher_condvar));  // Signal or broadcast?
    pthread_mutex_unlock(&(queue->pcq_popper_condvar_lock));
    //pthread_mutex_unlock(&queue->pcq_current_size_lock);
    //pthread_mutex_unlock(&queue->pcq_head_lock);

    return elem;
}