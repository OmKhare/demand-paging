#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "elf.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

extern int (*swapfunc_ptr_arr[NO_SWAP][2])(struct proc*, int);

void pgflt_handler()
{

    struct proc *curproc = myproc();
    struct inode *ip;
    uint i;
    int swapno;
    char* mem;
    uint pgflt_addr = PGROUNDDOWN(rcr2());
    cprintf("Page Fault Occoured for Vaddr : %d for PID : %d\n", pgflt_addr, curproc->pid);
    if (pgflt_addr > KERNBASE)
    {
        panic("Over the KERNBASE!");
    }
    else
    {
        /*
            Changes to be done : Handle all the panics made in an elegent way to make the system pass all the user tests.
        */ 

        /*
            Get a LRU struct.
            If not swap out till you have an empty struct. A function calling swap_out to be implemented.
            If got, then page present for sure.

            Check if present in the swap.
            if yes, get it from the swap. A function calling swap_in. Think about the cases to be checked.
            If not, ue the for loop below to get the page from the ELF.

            Stack and Heap to be thought off afterwards.
        */
        if ((swapno = swap_check(curproc, pgflt_addr)) >= 0){
            //Handle the reading from the swap and returning here.
            cprintf("Page Present in the Swap of the Proces!\n");
            swap_read(curproc);
            get_lru(curproc->pid, pgflt_addr);
            if (swapfunc_ptr_arr[swapno][1](curproc, pgflt_addr) < 0)
            {
                panic("Swap In not working!");
            }
            return;
        }

        if ((mem = kalloc(-1, -1)) == 0){
            panic("No Free Page");
        }
        cprintf("Handling the Page Fault Using the ELF\n");
        if (mappages(curproc->pgdir, (char*)pgflt_addr, PGSIZE, V2P(mem), PTE_W | PTE_U | PTE_P, 1) < 0)
            panic("Error in Mappages");
        if((ip = iget(curproc->ip.dev, curproc->ip.inum)) == 0)
            panic("iget error");
        for (i = 0; i < 2; i++){
            if (curproc->ph->vaddr <= pgflt_addr && pgflt_addr <= curproc->ph->vaddr + curproc->ph->memsz)
            {
                //Entire section of the code only present
                if (curproc->ph->vaddr + curproc->ph->filesz >= pgflt_addr + PGSIZE)
                    loaduvm(curproc->pgdir, (char *)(curproc->ph->vaddr + pgflt_addr), ip, curproc->ph->off + pgflt_addr, PGSIZE);
                else
                {
                    //Two cases possible.
                    //If the section is for bss section completly.
                    if (pgflt_addr >= curproc->ph->filesz)
                    {
                        if (pgflt_addr + PGSIZE <= curproc->ph->memsz)
                            stosb(mem, 0, PGSIZE);
                        else
                            stosb(mem, 0, curproc->ph->memsz - pgflt_addr);
                    }
                    else
                    //If the section overlaps bss as well as code+data
                    {   
                        loaduvm(curproc->pgdir, (char *)(curproc->ph->vaddr + pgflt_addr), ip, curproc->ph->off + pgflt_addr, curproc->ph->filesz - pgflt_addr);
                        stosb(mem + (curproc->ph->filesz - pgflt_addr), 0, PGSIZE- ( curproc->ph->filesz - pgflt_addr));
                    }
                }
            }
        }
    }
}