#ifndef _QUEUE_LOCKFREE_CK_H
#define _QUEUE_LOCKFREE_CK_H

#include <ck_fifo.h>
#include "free-list.h"

typedef ck_fifo_mpmc_t Q;

static Q* queue_init()
{
    Q *q = malloc(sizeof(Q));
    ck_fifo_mpmc_init(q, malloc(sizeof(ck_fifo_mpmc_entry_t)));

    return q;
}

static void enqueue(Q *q, int data)
{
    ck_fifo_mpmc_entry_t *fifo_entry = malloc(sizeof(ck_fifo_mpmc_entry_t));
    int *entry = malloc(sizeof(int));
    *entry = data;

    ck_fifo_mpmc_enqueue(q, fifo_entry, entry);
}

static int dequeue(Q *q, int *data)
{
    ck_fifo_mpmc_entry_t *garbage;
    int *entry;

    if (!ck_fifo_mpmc_dequeue(q, &entry, &garbage))
    {
        return 1;
    }

    *data = *entry;
    push_fl(garbage);
    push_fl(entry);
    return 0;
}

static void queue_free(Q *q)
{
    ck_fifo_mpmc_entry_t *garbage;
    int *entry;

    while (ck_fifo_mpmc_dequeue(q, &entry, &garbage))
    {
        free(garbage);
        free(entry);
    }

    ck_fifo_mpmc_deinit(q, &garbage);
    free(garbage);
    free(q);
}

#endif /* _QUEUE_LOCKFREE_CK_H */
