#ifndef _FREE_LIST_H
#define _FREE_LIST_H

#include <stdlib.h>
#include <threads.h>

typedef struct FLNode FLNode;
struct FLNode {
    void *ptr;
    FLNode *next;
};

thread_local FLNode *free_list_head = NULL;

static inline void push_fl(void *ptr)
{
    FLNode *flnode = malloc(sizeof(FLNode));
    flnode->ptr = ptr;
    flnode->next = free_list_head;
    free_list_head = flnode;
}

static void fl_free()
{
    FLNode *curr = free_list_head;

    while (curr != NULL)
    {
        free(curr->ptr);
        FLNode *next = curr->next;
        free(curr);
        curr = next;
    }

    free_list_head = NULL;
}

#endif /* _FREE_LIST_H */
