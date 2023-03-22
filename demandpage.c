#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "elf.h"
#include "demandpage.h"

void
pgflt_handler()
{
    struct proc* curproc = myproc();
    cprintf("Page Fault: Kernel Panic\n");
    cprintf("rcr2 value : %d\n", rcr2());
    if (rcr2() > curproc->sz)
    {
        panic("Illegal Memory Access!");
    }
    else
    {
        char* mem = kalloc();
        if (mem == 0)
        {
            panic("Kalloc out of Free Pages!");
        }
        else
        {
            cprintf("Page Fault Actually Woorking!");
            memset(mem, 0, PGSIZE);
            mappages(curproc->pgdir, (void *)rcr2(), PGSIZE, V2P(mem), PTE_U| PTE_W, 1);
            for (int i = 0 ; i < 2 ; i++)
            {
                if (rcr2() >= curproc->prog_head_info[i].vaddr && rcr2() < curproc->prog_head_info[i].vaddr + curproc->prog_head_info[i].sz)
                {
                    loaduvm(curproc->pgdir, (char *)curproc->prog_head_info[i].vaddr, curproc->prog_head_info->ip, curproc->prog_head_info->offset ,curproc->prog_head_info->sz);
                }
            }
        }
    }

}