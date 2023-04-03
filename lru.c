/*

Includes implementation of the functions to handle a doubly circular list.

*/
#include "lru_swap.h"
#include "proc.h"

void init_lru()
{
    // Iniitalisation of the lock to be taken into account after lock added.
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        bitmap.frame_bitmap[i] = -1;
    }
}

int get_free_frame_lru()
{
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        if (bitmap.frame_bitmap[i] = -1)
            return i;
    }
    return -1;
}

int insert_lru(struct proc *p, int vaddr)
{
    int index;
    if (lru.head == 0)
    {
        index = get_free_frame_lru();
        if (index == -1)
        {
            return -1;
        }
        lru.lru_frame_list[index].pid = p->pid;
        lru.lru_frame_list[index].vaddr = vaddr;
        lru.lru_frame_list[index].next = lru.lru_frame_list[index].prev = &(lru.lru_frame_list[index]);
        lru.lru_frame_list[index].device = -1;
        lru.lru_frame_list[index].block = -1;
        lru.head = &(lru.lru_frame_list[index]);
        bitmap.frame_bitmap[index] = 1;
        return 0;
    }
    struct frame *last = lru.head->prev;
    index = get_free_frame_lru();
    if (index == -1)
    {
        return -1;
    }
    lru.lru_frame_list[index].pid = p->pid;
    lru.lru_frame_list[index].vaddr = vaddr;
    lru.lru_frame_list[index].next = lru.head;
    lru.lru_frame_list[index].device = -1;
    lru.lru_frame_list[index].block = -1;
    lru.head->prev = &(lru.lru_frame_list[index]);
    lru.lru_frame_list[index].prev = last;
    last->next = &(lru.lru_frame_list[index]);
    bitmap.frame_bitmap[index] = 1;
    return 0;
}

int delete_lru()
{
    if (lru.head == 0)
    {
        return -1;
    }
    else if (lru.head->next == lru.head)
    {
        int index = (lru.head - &(lru.lru_frame_list[0]))/sizeof(struct frame);
        lru.head->next = 0;
        lru.head->prev = 0;
        bitmap.frame_bitmap[index] = 0;
        return index;
    }
    else
    {
        struct frame *head = lru.head;
        lru.head->prev->next = lru.head->next;
        lru.head->next->prev = lru.head->prev;
        int index = (lru.head - &(lru.lru_frame_list[0]))/sizeof(struct frame);
        lru.head = lru.head->next;
        head->next = 0;
        head->prev = 0;
        bitmap.frame_bitmap[index] = 0;
        return index;
    }
}