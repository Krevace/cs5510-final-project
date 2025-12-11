#ifndef _QUEUE_LOCKFREE_H
#define _QUEUE_LOCKFREE_H

#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include "free-list.h"

typedef struct QNode QNode;

typedef struct {
    QNode *ptr;
    unsigned short count;
} StampedPtr;
typedef uint64_t PackedPtr;
typedef _Atomic(PackedPtr) AtomicPackedPtr;

static inline PackedPtr pack(AtomicPackedPtr *aptr, StampedPtr sptr)
{
    PackedPtr pptr = ((uint64_t) sptr.count << 48) | ((uint64_t) sptr.ptr & 0x0000FFFFFFFFFFFF);
    if (aptr != NULL) atomic_store(aptr, pptr);
    return pptr;
}

static inline StampedPtr unpack(AtomicPackedPtr *aptr)
{
    PackedPtr pptr = atomic_load(aptr);
    return ((StampedPtr) {(QNode*) ((int64_t) (pptr << 16) >> 16), pptr >> 48});
}

static inline int sptr_eq(StampedPtr a, StampedPtr b)
{
    return a.ptr == b.ptr && a.count == b.count;
}

struct QNode {
    int data;
    AtomicPackedPtr next;
};

static inline QNode* new_node(int data)
{
    QNode *node = (QNode*) malloc(sizeof(QNode));
    node->data = data;
    pack(&node->next, ((StampedPtr) {NULL, 0}));
    return node;
}

typedef struct {
    AtomicPackedPtr head;
    AtomicPackedPtr tail;
} Q;

static Q* queue_init()
{
    Q *q = malloc(sizeof(Q));
    QNode *node = new_node(0);

    pack(&q->head, ((StampedPtr) {node, 0}));
    pack(&q->tail, ((StampedPtr) {node, 0}));
    return q;
}

static void enqueue(Q *q, int data)
{
    QNode *node = new_node(data);
    StampedPtr tail;

    while (1)
    {
        tail = unpack(&q->tail);
        StampedPtr next = unpack(&tail.ptr->next);
        if (sptr_eq(tail, unpack(&q->tail)))
        {
            if (next.ptr == NULL)
            {
                PackedPtr expected = pack(NULL, next);
                if (atomic_compare_exchange_strong(&tail.ptr->next, &expected, pack(NULL, (StampedPtr) {node, next.count + 1}))) break;
            }
            else
            {
                PackedPtr expected = pack(NULL, tail);
                atomic_compare_exchange_strong(&q->tail, &expected, pack(NULL, (StampedPtr) {next.ptr, tail.count + 1}));
            }
        }
    }

    PackedPtr expected = pack(NULL, tail);
    atomic_compare_exchange_strong(&q->tail, &expected, pack(NULL, (StampedPtr) {node, tail.count + 1}));
}

static int dequeue(Q *q, int *data)
{
    StampedPtr head;
    while (1)
    {
        head = unpack(&q->head);
        StampedPtr tail = unpack(&q->tail);
        StampedPtr next = unpack(&head.ptr->next);
        if (sptr_eq(head, unpack(&q->head)))
        {
            if (head.ptr == tail.ptr)
            {
                if (next.ptr == NULL) return 1;
                PackedPtr expected = pack(NULL, tail);
                atomic_compare_exchange_strong(&q->tail, &expected, pack(NULL, (StampedPtr) {next.ptr, tail.count + 1}));
            }
            else
            {
                *data = next.ptr->data;
                PackedPtr expected = pack(NULL, head);
                if (atomic_compare_exchange_strong(&q->head, &expected, pack(NULL, (StampedPtr) {next.ptr, head.count + 1}))) break;
            }
        }
    }

    push_fl(head.ptr);
    return 0;
}

static void queue_free(Q *q)
{
    StampedPtr curr = unpack(&q->head);

    while (curr.ptr != NULL)
    {
        QNode *node = curr.ptr;
        StampedPtr next = unpack(&node->next);
        free(node);
        curr = next;
    }

    free(q);
}

#endif /* _QUEUE_LOCKFREE_H */
