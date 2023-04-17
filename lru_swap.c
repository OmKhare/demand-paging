/*

Includes implementation of the functions to handle a doubly circular list.

*/

//Clean LRU function to be implemented to clear the LRU pages after the process has exited.
//Whenever doing any kalloc() -> Make a call to lru_insert. If less than zero, swap.

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

int (*swapfunc_ptr_arr[])(struct proc*, int) = {swap_in, swap_out};

void lru_swap_init()
{
    for (int i = 0; i < MAX_FRAME_LRU; i++)
    {
        bitmap.frame_bitmap[i] = -1;
    }
    for (int i = 0; i < MAX_FRAME_SWAP; i++)
    {
        swap_bitmap.frame_bitmap[i] = -1;
    }
    initlock(&lru.lk, "lru");
    initlock(&swap.lk, "swap");
}

int get_free_frame_lru()
{
    for (int i = 0; i < MAX_FRAME_LRU; i++)
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
    cprintf("Insert LRU for PID %d and vaddr %d\n", pid, vaddr);
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
        lru_read();
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
    lru_read();
    return 0;
}

void lru_free_frame(int pid, int vaddr)
{
    cprintf("Delete LRU for PID %d and vaddr %d\n", pid, vaddr);
    if (lru.head == 0)
    {
        return;
    }
    struct frame *curr, *prev;
    int index;
    curr = lru.head;
    prev = 0;
    acquire(&lru.lk);
    while (curr->pid != pid || curr->vaddr != vaddr)
    {
        prev = curr;
        if (curr->next == lru.head)
        {
            break;
        }
        curr = curr->next;
    }
    if (curr->pid == pid && curr->vaddr == vaddr) {
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
    lru_read();
    release(&lru.lk);
}

int lru_delete()
{
    int index;
    if (lru.head == 0)
    {
        return -1;
    }
    acquire(&lru.lk);
    if (lru.head->next == lru.head)
    {
        index = lru.head - lru.lru_frame_list;
        lru.head->next = 0;
        lru.head->prev = 0;
    }
    else
    {
        struct frame *head = lru.head;
        lru.head->prev->next = lru.head->next;
        lru.head->next->prev = lru.head->prev;
        index = lru.head - lru.lru_frame_list;
        lru.head = lru.head->next;
        head->next = 0;
        head->prev = 0;
    }
    return index;
}

int lru_get_pid_frame(int index)
{
    if (!holding(&lru.lk))
        panic("Not Holding Lock\n");
    return lru.lru_frame_list[index].pid;
}

int lru_get_vaddr_frame(int index)
{
    if (!holding(&lru.lk))
        panic("Not Holding Lock\n");
    return lru.lru_frame_list[index].vaddr;
}

/*
    Mandatory function after deleting a lru entry and getting the data from the lru;
*/

void lru_delete_frame(int index)
{
    bitmap.frame_bitmap[index] = -1;
    release(&lru.lk);
}

/*
    Free the lru doubly linked list for a PID and set the index 
    for it to be zero set the bit map to -1
*/

void lru_free(int pid) //DONE
{
    if (lru.head == 0)
    {
        return;
    }
    int flag = 0;
    struct frame *curr = lru.head;
    acquire(&lru.lk);
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
            if (curr->prev != 0)
            {
                curr->prev->next = curr->next;
            }
            if (curr->next != 0)
            {
                curr->next->prev = curr->prev;
            }
            int index = curr - lru.lru_frame_list;
            bitmap.frame_bitmap[index] = -1;
        }
        curr = curr->next;
    } while (flag == 1 || curr != lru.head);
    release(&lru.lk);
}

void lru_read() 
{
    struct frame *curr = lru.head;
    if (lru.head == 0){
        cprintf("LRU Empty List!\n");
        return;
    }
    cprintf("LRU READ PID : %d | Index : %d | Vaddr : %d\n", curr->pid, curr - lru.lru_frame_list, curr->vaddr);
    curr = curr->next;
    while (curr != lru.head){
        cprintf("LRU READ PID : %d | Index : %d | Vaddr : %d\n", curr->pid, curr - lru.lru_frame_list, curr->vaddr);
        curr = curr->next;
    }
}


/*
    Functions to be written here : 
    1. swap_out(int index)

    2. swap_in(pid, vaddr)

    4. swap_free() -> For a process after exec basically.

    5. swap_check(pid, vaddr)

    Flow of Page Faults

    Page Fault -> Check Whether it is present in the Swap (swap_check) 
        Yes -> Read 8 Frames into a page. (swap_in) (remove it from the list in proc) (set swap bitmap to -1)
        No -> Replace the lru with the new Page (swap = swap_out + swap_in) (add it into the list of that proc) (set swap the bitstore to 1)
    When the process exits -> 
        call swap_free() to make all the bitmaps to zero and also the structs of swap

*/

int swap_get_free_frame()
{
    for (int i = 0 ; i < MAX_FRAME_SWAP ; i++)
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

int swap_out(struct proc* p, int vaddr)
{
    cprintf("Swapping out for PID : %d | vaddr : %d\n", p->pid, vaddr);
    int findex;
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
        swap.swap_frame_list[findex].vaddr = vaddr;
        swap.swap_frame_list[findex].count = 1;
        release(&swap.lk);
        writeswap(p, ROOTDEV, SWAP_START+findex*8);
        demappages_swap_out(p, vaddr);
        klru_free_swap(p, (char *)vaddr);
        swap_read(p);
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
    swap.swap_frame_list[findex].vaddr = vaddr;
    swap.swap_frame_list[findex].count = 1;
    release(&swap.lk);
    writeswap(p, ROOTDEV, SWAP_START+findex*8);
    //Updated Page Directory
    //Should Not be present here.
    demappages_swap_out(p, vaddr);
    klru_free_swap(p, (char *)vaddr);
    swap_read(p);
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
    int flag = 0, findex;
    if (curr == 0)
    {
        return -1;
    }
    acquire(&swap.lk);
    while(curr->next != -1)
    {
        if(curr->vaddr == vaddr)
        {
            flag = 1;
            break;
        }
        prev = curr;
        curr = &(swap.swap_frame_list[curr->next]);
    }
    if (curr->vaddr == vaddr)
    {
        flag = 1;
    } 
    if (flag == 0){
        release(&swap.lk);
        return -1;
    }
    flag = 0;
    findex = curr - swap.swap_frame_list;
    swap.swap_frame_list[findex].count -= 1;
    if (swap.swap_frame_list[findex].count == 0){
        if (&(swap.swap_frame_list[findex]) - swap.swap_frame_list == p->swap_list - swap.swap_frame_list){
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
    }
    release(&swap.lk);
    readswap(p, ROOTDEV, SWAP_START+findex*8);
    swap_read(p);
    return mappages_swap_in(p, vaddr);   
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
void swap_free(int pid)
{
    struct disk_frame* curr;
    int index;
    struct proc* p = get_proc(pid);
    if (p)
    {
        if (p->forked == 0){
            acquire(&swap.lk);
            curr = p->swap_list;
            if (curr == 0){
                release(&swap.lk);
                return;
            }
            if (curr->next == -1){
                index = curr - swap.swap_frame_list;
                swap_bitmap.frame_bitmap[index] = -1;
                release(&swap.lk);
                return;
            }
            while(curr->next != -1)
            {
                index = curr - swap.swap_frame_list;
                swap_bitmap.frame_bitmap[index] = -1;
                curr = &(swap.swap_frame_list[curr->next]);
                if (curr->next == -1){
                    index = curr - swap.swap_frame_list;
                    swap_bitmap.frame_bitmap[index] = -1;
                }
            }
            release(&swap.lk);
            p->swap_list = 0;
            return;
        }
        p->swap_list = 0;
    }
}

void swap_read(struct proc* p)
{   
    struct disk_frame* df = p->swap_list;
    if (df == 0){
        cprintf("SWAP Empty List!\n");
        return;
    }
    while (df->next != -1){
        cprintf("SWAP READ Index : %d | Vaddr : %d | count : %d\n", df - swap.swap_frame_list, df->vaddr, df->count);
        df = &(swap.swap_frame_list[df->next]);
    }
    cprintf("SWAP READ Index : %d | Vaddr : %d | count : %d\n", df - swap.swap_frame_list, df->vaddr, df->count);
}

void swap_fork(struct proc* p)
{
    struct disk_frame* curr;
    curr = p->swap_list;
    if (curr == 0){
        return;
    }
    if (curr->next == -1){
        curr->count++;
        return;
    }
    while(curr->next != -1)
    {
        curr->count++;
        curr = &(swap.swap_frame_list[curr->next]);
        if (curr->next == -1){
            curr->count++;
        }
    }
}

/*
    void get_lru(struct proc* p, int vaddr)
    This will make a call to insert lru.
    If successful, return
    If not, delete a lru, get the pid and vaddr, delete the frame
    get the proc related to the PID.
    If not present call the swap_free and lru_free for PID.
    If present call the swap out function and swap out the page.
    make the insert lru call again.
    basically have a while loop.
*/

void get_lru(int pid, int vaddr)
{
    int index, vaddr_lru, pid_lru;
    struct proc* pr;
    if (lru_insert(pid, vaddr) < 0)
    {   
        cprintf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
        if ((index = lru_delete()) < 0)
        {
            panic("LRU full as well as empty.");
        }
        pid_lru = lru_get_pid_frame(index);
        vaddr_lru = lru_get_vaddr_frame(index);
        lru_delete_frame(index);
        if ((pr = get_proc(pid_lru)) == 0)
        {
            lru_free(pid_lru);
            swap_free(pid_lru);
            lru_insert(pid, vaddr);
        }
        else
        {
            cprintf("Swap Out vaddr %d from PID %d to free LRU\n", vaddr_lru, pid_lru);
            read_vaddr(pr, vaddr_lru);
            if(swapfunc_ptr_arr[1](pr, vaddr_lru) < 0)
            {
                panic("Swap Space is Full");
            }
            lru_insert(pid, vaddr);
        }

    }
}