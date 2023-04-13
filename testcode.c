#include "types.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_FRAME_LRU_SWAP 2560
// 2560 Number can be managed as per the actual pages present in the system.

struct frame
{
    int pid;
    int vaddr;
    struct frame *next;
    struct frame *prev;
};

struct disk_frame
{
    int vaddr;
    int index;
    int next;
    int count;
};

struct
{
    int frame_bitmap[MAX_FRAME_LRU_SWAP];
} bitmap, swap_bitmap;

// The pointer here will store a least recently used frame and will be used to swap out of the disk.
struct
{
    struct frame *head;
    struct frame lru_frame_list[MAX_FRAME_LRU_SWAP];
    struct spinlock lk;
} lru;

struct
{
    struct disk_frame swap_frame_list[MAX_FRAME_LRU_SWAP];
    struct spinlock lk;
} swap;

void lru_swap_init() // DONE
{
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        bitmap.frame_bitmap[i] = -1;
        swap_bitmap.frame_bitmap[i] = -1;
    }
}

int get_free_frame_lru() // DONE
{
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        if (bitmap.frame_bitmap[i] == -1)
        {
            return i;
        }
    }
    return -1;
}

int lru_insert(int pid, int vaddr) // DONE
{
    int index;
    if (lru.head == 0)
    {
        index = get_free_frame_lru();
        if (index == -1)
        {
            return -1;
        }
        lru.lru_frame_list[index].pid = pid;
        lru.lru_frame_list[index].vaddr = vaddr;
        lru.lru_frame_list[index].next = lru.lru_frame_list[index].prev = &(lru.lru_frame_list[index]);
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
    lru.lru_frame_list[index].pid = pid;
    lru.lru_frame_list[index].vaddr = vaddr;
    lru.lru_frame_list[index].next = lru.head;
    lru.lru_frame_list[index].prev = last;
    lru.head->prev = &(lru.lru_frame_list[index]);
    last->next = &(lru.lru_frame_list[index]);
    bitmap.frame_bitmap[index] = 1;
    return 0;
}

int lru_delete()
{
    if (lru.head == 0)
    {
        return -1;
    }
    else if (lru.head->next == lru.head)
    {
        int index = lru.head - lru.lru_frame_list;
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
        int index = lru.head - lru.lru_frame_list;
        lru.head = lru.head->next;
        head->next = 0;
        head->prev = 0;
        bitmap.frame_bitmap[index] = 0;
        return index;
    }
}

int lru_get_pid_frame(int index)
{
    return lru.lru_frame_list[index].pid;
}

int lru_get_vaddr_frame(int index)
{
    return lru.lru_frame_list[index].vaddr;
}

/*
    Mandatory function after deleting a lru entry and getting the data from the lru;
*/

void lru_delete_frame(int index) //DONE
{
    bitmap.frame_bitmap[index] = -1;
}

/*
    Free the lru doubly linked list for a PID and set the index
    for it to be zero set the bit map to -1
*/
void lru_free_frame(int pid, int vaddr) //DONE
{
    if (lru.head == 0)
    {
        return;
    }
    struct frame *curr, *prev;
    int index;
    curr = lru.head;
    prev = 0;
    printf("%d %d\n", pid, vaddr);
    while (curr->pid != pid || curr->vaddr != vaddr)
    {
        printf("%d %d\n", curr->pid, curr->vaddr);
        prev = curr;
        if (curr->next == lru.head)
        {
            break;
        }
        curr = curr->next;
    }
    if (curr->pid == pid && curr->vaddr == vaddr)
    {
        if (curr == lru.head)
        {
            lru.head = lru.head->next;
        }

        if (curr == lru.head && prev != 0)
        {
            prev->next = lru.head;
            lru.head->prev = prev;
        }
        else
        {
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
        }
        index = curr - lru.lru_frame_list;
        bitmap.frame_bitmap[index] = -1;
    }
}

void lru_free(int pid) //DONE
{
    if (lru.head == NULL)
    {
        return;
    }
    int flag = 0;
    struct frame *curr = lru.head;
    do
    {
        flag = 0;
        if (curr->pid == pid)
        {
            if (curr == lru.head)
            {
                lru.head = lru.head->next;
                flag = 1;
            }
            if (curr->prev != NULL)
            {
                curr->prev->next = curr->next;
            }
            if (curr->next != NULL)
            {
                curr->next->prev = curr->prev;
            }
            int index = curr - lru.lru_frame_list;
            bitmap.frame_bitmap[index] = -1;
        }
        curr = curr->next;
    } while (flag == 1 || curr != lru.head);
}




void lru_read() //DONE
{
    struct frame *curr = lru.head;
    if (lru.head == 0)
    {
        printf("Empty Lsit!\n");
        return;
    }
    printf("PID : %d | Index : %ld | Vaddr : %d\n", curr->pid, curr - lru.lru_frame_list, curr->vaddr);
    curr = curr->next;
    while (curr != lru.head)
    {
        printf("PID : %d | Index : %ld | Vaddr : %d\n", curr->pid, curr - lru.lru_frame_list, curr->vaddr);
        curr = curr->next;
    }
}

int main()
{
    lru_swap_init();
    lru_insert(1, 5);
    lru_insert(1, 6);
    lru_insert(1, 7);
    lru_insert(1, 8);
    lru_insert(2, 7);
    lru_delete();
    lru_insert(2, 8);
    lru_insert(1, 9);
    lru_insert(1, 10);
    lru_read();
    printf("\n");
    // lru_free_frame(1, 7);
    // lru_read();
    // printf("\n");
    // lru_insert(1, 7);
    // lru_read();
    // printf("\n");
    // lru_free_frame(1, 10);
    // lru_insert(2, 7);
    // lru_insert(2, 8);
    // lru_read();
    // printf("\n");
    // lru_insert(1, 10);
    // lru_read();
    // printf("\n");
    lru_free(1);
    lru_read();
    printf("\n");
    // lru_insert(2, 7);
    // lru_insert(2, 8);
    // lru_read();
}