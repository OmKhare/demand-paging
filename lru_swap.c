/*

Includes implementation of the functions to handle a doubly circular list.

*/

//Clean LRU function to be implemented to clear the LRU pages after the process has exited.
//Whenever doing any kalloc() -> Make a call to insert_lru. If less than zero, swap.

#include "types.h"
#include "param.h"
#include "defs.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "lru_swap.h"

/*
    Wont need any locks.
*/
void init_lru_swap()
{
    // Iniitalisation of the lock to be taken into account after lock added.
    for (int i = 0; i < MAX_FRAME_LRU_SWAP; i++)
    {
        bitmap.frame_bitmap[i] = -1;
        lru.lru_frame_list[i].index = i;
        swap_bitmap.frame_bitmap[i] = -1;
        swap.swap_frame_list[i].index = i;
    }
    initlock(&lru.lk, "lru");
    initlock(&swap.lk, "swap");
}

int get_free_frame_lru()
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

int insert_lru(int pid, int vaddr)
{
    int index;
    if (lru.head == 0)
    {
        acquire(&lru.lk);
        index = get_free_frame_lru();
        if (index == -1)
        {
            release(&lru.lk);
            return -1;
        }
        lru.lru_frame_list[index].pid = pid;
        lru.lru_frame_list[index].vaddr = vaddr;
        lru.lru_frame_list[index].next = lru.lru_frame_list[index].prev = &(lru.lru_frame_list[index]);
        lru.head = &(lru.lru_frame_list[index]);
        bitmap.frame_bitmap[index] = 1;
        release(&lru.lk);
        return 0;
    }
    struct frame *last = lru.head->prev;
    acquire(&lru.lk);
    index = get_free_frame_lru();
    if (index == -1)
    {
        release(&lru.lk);
        return -1;
    }
    lru.lru_frame_list[index].pid = pid;
    lru.lru_frame_list[index].vaddr = vaddr;
    lru.lru_frame_list[index].next = lru.head;
    lru.lru_frame_list[index].prev = last;
    lru.head->prev = &(lru.lru_frame_list[index]);
    last->next = &(lru.lru_frame_list[index]);
    bitmap.frame_bitmap[index] = 1;
    release(&lru.lk);
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
        acquire(&lru.lk);
        int index = lru.head->index;
        lru.head->next = 0;
        lru.head->prev = 0;
        bitmap.frame_bitmap[index] = 0;
        release(&lru.lk);
        return index;
    }
    else
    {
        acquire(&lru.lk);
        struct frame *head = lru.head;
        lru.head->prev->next = lru.head->next;
        lru.head->next->prev = lru.head->prev;
        int index = lru.head->index;
        lru.head = lru.head->next;
        head->next = 0;
        head->prev = 0;
        bitmap.frame_bitmap[index] = 0;
        release(&lru.lk);
        return index;
    }
}

int get_pid_lru_frame(int index)
{
    return lru.lru_frame_list[index].pid;
}

int get_vaddr_lru_frame(int index)
{
    return lru.lru_frame_list[index].vaddr;
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
void free_lru_frame(struct proc* p, int vaddr)
{
    if (lru.head == 0)
    {
        return;
    }
    struct frame *curr, *prev;
    int index;
    curr = lru.head;
    prev = 0;
    acquire(&lru.lk);
    while (curr->pid != p->pid  && curr->vaddr == vaddr && curr->next != lru.head)
    {
        prev = curr;
        curr = curr->next;
    }

    if (curr->pid == p->pid && curr->vaddr == vaddr)
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
        index = curr->index;
        bitmap.frame_bitmap[index] = -1;
    }
    release(&lru.lk);
}

void free_lru(int pid)
{
    if (lru.head == 0)
    {
        return;
    }
    struct frame *curr;
    int index;
    curr = lru.head;
    acquire(&lru.lk);
    do 
    {
        if (curr->pid == pid)
        {
            if (curr == lru.head)
            {
                lru.head = lru.head->next;
            }
            if (curr->prev != 0)
            {
                curr->prev->next = curr->next;
            }
            if (curr->next != 0)
            {   
                curr->next->prev = curr->prev;
            }
            index = curr->index;
            bitmap.frame_bitmap[index] = -1;
            curr = curr->next;
        }
        else
        {
            curr = curr->next;
        }
    } while (curr != lru.head);
    if (curr->pid == pid)
    {
        index = curr->index;
        bitmap.frame_bitmap[index] = -1;
        lru.head = 0;
    }
    release(&lru.lk);
}

void read_lru()
{
    struct frame* curr = lru.head;
    if (lru.head == 0){
        cprintf("Empty Lsit!\n");
        return;
    }
    cprintf("PID : %d | Index : %d | Vaddr : %d", curr->pid ,curr->index, curr->pid);
    curr = curr->next;
    while (curr != lru.head){
        cprintf("PID : %d | Index : %d | Vaddr : %d", curr->pid ,curr->index, curr->pid);
        curr = curr->next;
        if (curr != lru.head){
            cprintf(" -> ");
        }
    }
    cprintf("\n");
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
        if (swap_bitmap.frame_bitmap[i] == -1)
        {
            swap_bitmap.frame_bitmap[i] = 1;
            return i;
        }
    }
    return -1;
}

/*
    This function is responsible for writing the page to the swap space.
    The data to be transfered in the swap space is stored in the buffer of the struct proc and then allocated.
    No changes related to lru pages done here done here.
*/

int swap_out(struct proc* p, int vaddr, int kfree_flag)
{
    cprintf("Swapping out for PID : %d | vaddr : %d\n", p->pid, vaddr);
    int findex, i;
    struct buf* buffer;
    pte_t *pte;
    uint pa;
    struct disk_frame* curr = p->swap_list;
    if (p->swap_list == 0)
    {
        acquire(&swap.lk);
        if ((findex = swap_get_free_frame()) < 0)
        {
            return -1;
        }
        p->swap_list = &(swap.swap_frame_list[findex]);
        swap.swap_frame_list[findex].next = -1;
        swap.swap_frame_list[findex].pid = p->pid;
        swap.swap_frame_list[findex].vaddr = vaddr;
        release(&swap.lk);
        for (i = 0 ; i < 8 ; i++){
            buffer = bget(ROOTDEV, SWAP_START+findex*8 + i);
            memmove(buffer->data, p->buffer + BSIZE*i, BSIZE);
            bwrite(buffer);
            brelse(buffer);
        }
        if (kfree_flag)
            kfree_lru_swap(p, (char *)vaddr);
        else
            free_lru_frame(p, vaddr);
        return 1;
    }
    while (curr->next != -1)
    {
        curr = &(swap.swap_frame_list[curr->next]);
    }
    acquire(&swap.lk);
    if ((findex = swap_get_free_frame()) < 0)
    {
        return -1;
    }
    curr->next = findex;
    swap.swap_frame_list[findex].next = -1;
    swap.swap_frame_list[findex].pid = p->pid;
    swap.swap_frame_list[findex].vaddr = vaddr;
    release(&swap.lk);
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
    if (kfree_flag)
        kfree_lru_swap(p, (char *)vaddr);
    else
        free_lru_frame(p, vaddr);
    return 1;
}

/*
    The page read will be present in the buffer of the struct proc and then allocated.
    No changes related to lru pages done here.
*/
int swap_in(struct proc* p, int vaddr)
{
    cprintf("Swapping in for PID : %d | vaddr : %d\n", p->pid, vaddr);
    struct disk_frame* curr = p->swap_list, *prev = 0;
    char* mem;
    int flag = 0, findex, i;
    struct buf* buffer;
    if (curr == 0)
    {
        return -1;
    }
    acquire(&swap.lk);
    if (curr->next == -1){
        if (curr->vaddr == vaddr)
        {
            flag = 1;
        } 
    }
    while(curr->next != -1)
    {
        if(curr->vaddr == vaddr)
        {
            flag = 1;
            break;
        }
        prev = curr;
        curr = &(swap.swap_frame_list[curr->next]);
        if (curr->next == -1){
            if (curr->vaddr == vaddr)
            {
                flag = 1;
                break;
            } 
        }
    }
    if (flag == 0){
        release(&swap.lk);
        return -1;
    }
    flag = 0;
    findex = curr->index;
    if (&(swap.swap_frame_list[findex]) == p->swap_list){
        if (swap.swap_frame_list[findex].next == -1)
        {
            p->swap_list = 0;
        }else{
            p->swap_list = &(swap.swap_frame_list[swap.swap_frame_list[findex].next]);
        }
    }else{
        prev->next = curr->next;
    }
    swap_bitmap.frame_bitmap[findex] = -1;
    release(&swap.lk);
    for (i = 0 ; i < 8 ; i++)
    {   
        buffer = bread(ROOTDEV, SWAP_START+findex*8 + i);
        memmove(p->buffer + BSIZE*i, buffer->data, BSIZE);
        brelse(buffer);
    }
    //Updated Page Directory
    //Should Not be present here.
    if ((mem = kalloc_lru_swap(p)) == 0)
    {
        panic("No Free Page");
    }
    memmove(mem, p->buffer, PGSIZE);
    if(mappages(p->pgdir, (char*)vaddr, PGSIZE, V2P(mem), PTE_W | PTE_U | PTE_P, 1) < 0)
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
    if (df == 0)
    {
        return -1;
    }
    acquire(&swap.lk);
    if (df->next == -1){
        if (df->vaddr == vaddr)
        {
            release(&swap.lk);
            return 0;
        } 
    }
    while (df->next != -1)
    {
        if (df->vaddr == vaddr)
        {
            release(&swap.lk);
            return 0;
        }
        df = &(swap.swap_frame_list[df->next]);
        if (df->next == -1){
            if (df->vaddr == vaddr)
            {
                release(&swap.lk);
                return 0;
            } 
        }
    }
    release(&swap.lk);
    return -1;
}

/*
    Set all the bits from the swap_bitmap to -1 for a particular PID.
*/
void free_swap(int pid)
{
    int i;
    struct proc* p = get_proc(pid);
    if (p)
    {
        p->swap_list = 0;
        if (p->forked == 1){
            return;
        }
    }
    acquire(&swap.lk);
    for (i = 0 ; i < MAX_FRAME_LRU_SWAP ; i++)
    {
        if (swap.swap_frame_list[i].pid == pid)
            swap_bitmap.frame_bitmap[i] = -1;
    }
    release(&swap.lk);
}

void read_swap(struct proc* p)
{   
    struct disk_frame* df = p->swap_list;
    if (df != 0){
        cprintf("Displaying the Swap array for the process with PID : %d\n", p->pid);
        cprintf("Index : %d | Vaddr : %d", df->index, df->vaddr);
         cprintf(" -> ");
    }
    while (df != 0 && df->next != -1){
        df = &(swap.swap_frame_list[df->next]);
        cprintf("Index : %d | Vaddr : %d", df->index, df->vaddr);
        if (df->next != -1){
            cprintf(" -> ");
        }
    }
    cprintf("\n");
}

/*
    void get_lru(struct proc* p, int vaddr)
    This will make a call to insert lru.
    If successful, return
    If not, delete a lru, get the pid and vaddr, delete the frame
    get the proc related to the PID.
    If not present call the free_swap and free_lru for PID.
    If present call the swap out function and swap out the page.
    make the insert lru call again.
    basically have a while loop.
*/

void get_lru(int pid, int vaddr)
{
    int index, vaddr_lru, pid_lru;
    struct proc* pr;
    while (insert_lru(pid, vaddr) < 0)
    {   
        if ((index = delete_lru()) < 0)
        {
            panic("LRU full as well as empty.");
        }
        pid_lru = get_pid_lru_frame(index);
        vaddr_lru = get_vaddr_lru_frame(index);
        delete_lru_frame(index);
        if ((pr = get_proc(pid_lru)) == 0)
        {
            free_lru(pid_lru);
            free_swap(pid_lru);
        }
        else
        {
            if(swap_out(pr, vaddr_lru, 1) < 0)
            {
                panic("Swap Space is Full");
            }
        }

    }
}