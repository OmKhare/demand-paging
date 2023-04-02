/*

This includes implementation of the LRU Page Replacement Model for
all the pages being swapped out of the system.

*/
#define MAX_FRAME_LRU_SWAP 124 //So total 996 blocks in memory and 124 pages can be present in LRU

#define FRAME_VALID       1     //Is working. Wither in lru or in swap.
#define FRAME_INVALID    -1     //Is not working wither side.
#define FRAME_PHASED      0     //Is in transition phase from one to another.

#define SWAP_DISK 1
#define SWAP_ELF 0

struct frame{
    int pid;
    int vaddr;
    struct frame* next;
    struct frame* prev;
};

struct disk_frame{
    int vaddr;
    int block;
    int device;
    struct frame* next;
};


struct{
    int frame_bitmap[MAX_FRAME_LRU_SWAP];
}bitmap, disk_bitmap;

//The pointer here will store a least recently used frame and will be used to swap out of the disk.
struct{
    struct frame* head;
    struct frame lru_frame_list[MAX_FRAME_LRU_SWAP];
}lru;

struct{
    struct frame swap_frame_list[MAX_FRAME_LRU_SWAP];
}swap;


//If in swap, It will be present in the list of frames in the process itself to reduce the cost of interating over MAX_FRAME_LRU_SWAP
