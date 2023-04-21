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
    if (pgflt_addr > KERNBASE)
    {
        panic("Over the KERNBASE!");
    }
    else
    {
        if ((swapno = swap_check(curproc, pgflt_addr)) >= 0){
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
        if (mappages(curproc->pgdir, (char*)pgflt_addr, PGSIZE, V2P(mem), PTE_W | PTE_U | PTE_P, 1) < 0)
            panic("Error in Mappages");
        if((ip = iget(curproc->ip.dev, curproc->ip.inum)) == 0)
            panic("iget error");
        for (i = 0; i < 2; i++){
            if (curproc->ph->vaddr <= pgflt_addr && pgflt_addr <= curproc->ph->vaddr + curproc->ph->memsz)
            {
                if (curproc->ph->vaddr + curproc->ph->filesz >= pgflt_addr + PGSIZE)
                    loaduvm(curproc->pgdir, (char *)(curproc->ph->vaddr + pgflt_addr), ip, curproc->ph->off + pgflt_addr, PGSIZE);
                else
                {
                    if (pgflt_addr >= curproc->ph->filesz)
                    {
                        if (pgflt_addr + PGSIZE <= curproc->ph->memsz)
                            stosb(mem, 0, PGSIZE);
                        else
                            stosb(mem, 0, curproc->ph->memsz - pgflt_addr);
                    }
                    else
                    {   
                        loaduvm(curproc->pgdir, (char *)(curproc->ph->vaddr + pgflt_addr), ip, curproc->ph->off + pgflt_addr, curproc->ph->filesz - pgflt_addr);
                        stosb(mem + (curproc->ph->filesz - pgflt_addr), 0, PGSIZE- ( curproc->ph->filesz - pgflt_addr));
                    }
                }
            }
        }
    }
}