#ifndef _QUEUE_LOCK_H
#define _QUEUE_LOCK_H

#include <pthread.h>

typedef struct QNode QNode;
struct QNode {
    int data;
    QNode *next;
};

typedef struct {
    QNode *head;
    QNode *tail;
    pthread_mutex_t lock;
} Q;

static Q* queue_init()
{
    Q* q = malloc(sizeof(Q));

    q->head = (QNode*) malloc(sizeof(QNode));
    q->head->data = 0;
    q->head->next = NULL;

    q->tail = q->head;
    pthread_mutex_init(&q->lock, NULL);
    return q;
}

static void enqueue(Q *q, int data)
{
    pthread_mutex_lock(&q->lock);

    QNode *node = malloc(sizeof(QNode));
    node->data = data;
    node->next = NULL;

    q->tail->next = node;
    q->tail = node;

    pthread_mutex_unlock(&q->lock);
}

static int dequeue(Q *q, int *data)
{
    pthread_mutex_lock(&q->lock);

    if (q->tail == q->head) 
    {
        pthread_mutex_unlock(&q->lock);
        return 1;
    }

    QNode *front = q->head->next;
    *data = front->data;
    q->head->next = front->next;

    free(front);
    if (q->head->next == NULL) q->tail = q->head;

    pthread_mutex_unlock(&q->lock);
    return 0;
}

static void queue_free(Q *q)
{
    QNode *curr = q->head;

    while (curr != NULL)
    {
        QNode *next = curr->next;
        free(curr);
        curr = next;
    }

    pthread_mutex_destroy(&q->lock);
    free(q);
}

#endif /* _QUEUE_LOCK_H */
