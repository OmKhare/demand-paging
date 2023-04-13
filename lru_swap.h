/*

This includes implementation of the LRU Page Replacement Model for
all the pages being swapped out of the system.

*/
#ifndef LRU_SWAP_H_
#define LRU_SWAP_H_

// #define MAX_FRAME_LRU_SWAP 2560 //So total 20480 blocks in memory and 2560 pages can be present in LRU
#define MAX_FRAME_LRU_SWAP 2560
//2560 Number can be managed as per the actual pages present in the system.

struct frame{
    int pid;
    int vaddr;
    struct frame* next;
    struct frame* prev;
};

struct disk_frame{
    int vaddr;
    int next;
    int count;
};

struct{
    int frame_bitmap[MAX_FRAME_LRU_SWAP];
}bitmap, swap_bitmap;

//The pointer here will store a least recently used frame and will be used to swap out of the disk.
struct{
    struct frame* head;
    struct frame lru_frame_list[MAX_FRAME_LRU_SWAP];
    struct spinlock lk;
}lru;

struct{
    struct disk_frame swap_frame_list[MAX_FRAME_LRU_SWAP];
    struct spinlock lk;
}swap;

#endif
//If in swap, It will be present in the list of frames in the process itself to reduce the cost of interating over MAX_FRAME_LRU_SWAP
