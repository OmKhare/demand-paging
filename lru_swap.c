/*

Includes implementation of the functions to handle a doubly circular list.

*/

//Clean LRU function to be implemented to clear the LRU pages after the process has exited.
//Whenever doing any kalloc() -> Make a call to insert_lru. If less than zero, swap.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "memlayout.h"
#include "lru_swap.h"

void init_lru_swap()
{
    // Iniitalisation of the lock to be taken into account after lock added.
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        bitmap.frame_bitmap[i] = -1;
        swap_bitmap.frame_bitmap[i] = -1;
    }
}

int get_free_frame_lru()
{
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        if (bitmap.frame_bitmap[i] == -1)
            return i;
    }
    return -1;
}

int insert_lru(int pid, int vaddr)
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


/*
    Mandatory function after deleting a lru entry and getting the data from the lru;
*/
void delete_lru_frame(int index)
{
    bitmap.frame_bitmap[index] = -1;
}

/*
    Free the lru doubly linked list for a PID and set the index 
    for it to be zero set the bit map to -1
*/
void free_lru(int pid)
{
    if (lru.head == 0)
    {
        return;
    }
    struct frame *curr, *next;
    int index;
    curr = lru.head;
    do 
    {
        next = curr->next;
        if (curr->pid == pid)
        {
            if (curr->next == curr)
            {
                lru.head = 0;
            }
            else if (lru.head == curr)
            {
                lru.head = curr->next;
            }
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
            index = (curr - &(lru.lru_frame_list[0]))/(sizeof(struct frame));
            bitmap.frame_bitmap[index] = -1;
        }
        curr = next;
    }while (curr != lru.head);
}


/*
    Functions to be written here : 
    1. swap_out(int index)

    2. swap_in(pid, vaddr)

    4. free_swap() -> For a process after exec basically.

    5. swap_check(pid, vaddr)

    Flow of Page Faults

    Page Fault -> Check Whether it is present in the Swap (swap_check) 
        Yes -> Read 8 Frames into a page. (swap_in) (remove it from the list in proc) (set swap bitmap to -1)
        No -> Replace the lru with the new Page (swap = swap_out + swap_in) (add it into the list of that proc) (set swap the bitstore to 1)
    When the process exits -> 
        call free_swap() to make all the bitmaps to zero and also the structs of swap

*/

int swap_get_free_frame()
{
    for (int i = 0 ; i < MAX_FRAME_LRU_SWAP ; i++)
    {
        if (swap_bitmap.frame_bitmap[i] != -1)
        {
            swap_bitmap.frame_bitmap[i] = 1;
            return i;
        }
    }
    return -1;
}

/*
    This function is responsible for writing the function to the swap space.
    The data to be transfered in the swap space is stored in the buffer of the struct proc and then allocated.
    No changes related to lru pages done here done here.
*/
int swap_out(struct proc* p, int vaddr)
{
    int findex, i;
    struct buf* buffer;
    pte_t *pte;
    uint pa;
    struct disk_frame* curr = p->swap_list;
    if (curr == 0)
    {
        if ((findex = swap_get_free_frame()) < 0)
        {
            return -1;
        }
        curr = &(swap.swap_frame_list[findex]);
        curr->vaddr = vaddr;
        curr->pid = p->pid;
        curr->next = 0;
        for (i = 0 ; i < 8 ; i++){
            buffer = bget(ROOTDEV, SWAP_START+findex*8 + i);
            memmove(buffer->data, p->buffer + BSIZE*i, BSIZE);
            bwrite(buffer);
            brelse(buffer);
        }
        return 1;
    }
    while (curr->next != 0)
    {
        curr = curr->next;
    }
    if ((findex = swap_get_free_frame()) < 0)
    {
        return -1;
    }
    curr->next = &(swap.swap_frame_list[findex]);
    curr = curr->next;
    curr->vaddr = vaddr;
    curr->pid = p->pid;
    curr->next = 0;
    for (i = 0 ; i < 8 ; i++){
        buffer = bget(ROOTDEV, SWAP_START+findex*8 + i);
        memmove(buffer->data, p->buffer + BSIZE*i, BSIZE);
        bwrite(buffer);
        brelse(buffer);
    }
    //Updated Page Directory
    //Should Not be present here.
    if((pte = walkpgdir(p->pgdir, (char *)vaddr, 0)) == 0)
        panic("address should exist");
    pa = PTE_ADDR(*pte);
    if(mappages(p->pgdir, (char *)vaddr, PGSIZE, pa, PTE_W | PTE_U, 0) < 0) 
    {
        panic("Map pages failed");
    }
    kfree((char *)vaddr);
    return 1;

}

/*
    The page read will be present in the buffer of the struct proc and then allocated.
    No changes related to lru pages done here.
*/
int swap_in(struct proc* p, int vaddr)
{
    struct disk_frame* curr = p->swap_list;
    char* mem;
    int flag, findex, i;
    struct buf* buffer;
    if (curr == 0)
    {
        return -1;
    }
    while(curr->next != 0)
    {
        if(curr->vaddr == vaddr)
        {
            flag = 1;
            break;
        }
    }
    if (flag == 1){
        return -1;
    }
    flag = 0;
    findex = (curr - &(swap.swap_frame_list[0]))/sizeof(struct disk_frame);
    for (i = 0 ; i < 8 ; i++)
    {   
        buffer = bread(ROOTDEV, SWAP_START+findex*8 + i);
        memmove(p->buffer + BSIZE*i, buffer->data, BSIZE);
        brelse(buffer);
    }
    swap_bitmap.frame_bitmap[findex] = -1;
    //Updated Page Directory
    //Should Not be present here.
    mem = kalloc();
    memmove(mem, p->buffer, PGSIZE);
    if(mappages(p->pgdir, (char*)vaddr, PGSIZE, V2P(mem), PTE_W|PTE_U, 1) < 0)
      return -1;
    return 0;
}

/*
    Check whether the page to be loaded is present in the swap.
*/

int swap_check(struct proc* p, int vaddr)
{
    struct disk_frame* df;
    df = p->swap_list;
    while (df->next != 0)
    {
        if (df->vaddr == vaddr)
            return 0;
    }
    return -1;
}

/*
    Set all the bits from the swap_bitmap to -1 for a particular PID.
*/
void free_swap(int pid)
{
    int i;
    for (i = 0 ; i < MAX_FRAME_LRU_SWAP ; i++)
    {
        if (swap.swap_frame_list[i].pid == pid)
            swap_bitmap.frame_bitmap[i] = -1;
    }
}
