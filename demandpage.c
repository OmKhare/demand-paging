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

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

void pgflt_handler()
{
    struct proc *curproc = myproc();
    uint pgflt_addr = PGROUNDDOWN(rcr2());
    cprintf("rcr2 value : %d\n", rcr2());
    begin_op();
    if (pgflt_addr > curproc->sz)
    {
        cprintf("Illegal Memory Access\n");
        goto bad;
    }
    else
    {
        char *mem = kalloc();
        if (mem == 0)
        {
            cprintf("Cannot Allocate Pages!\n");
            goto bad;
        }
        else
        {
            memset(mem, 0, PGSIZE);
            if (mappages(curproc->pgdir, (char *)pgflt_addr, PGSIZE, V2P(mem), PTE_U | PTE_W, 1) < 0)
            {
                cprintf("Cannot Map Pages!\n");
                goto bad;
            }
            for (int i = 0; i < 2; i++)
            {
                if (pgflt_addr >= curproc->prog_head_info[i].vaddr && pgflt_addr < curproc->prog_head_info[i].vaddr + curproc->prog_head_info[i].memsz)
                {
                    while (pgflt_addr > curproc->prog_head_info[i].vaddr)
                    {
                        curproc->prog_head_info[i].vaddr += PGSIZE;
                        curproc->prog_head_info[i].offset += PGSIZE;
                        curproc->prog_head_info[i].filesz -= PGSIZE;
                    }
                    if (loaduvm(curproc->pgdir, (void *)pgflt_addr, curproc->prog_head_info->ip, curproc->prog_head_info->offset, min(PGSIZE, curproc->prog_head_info->filesz)) < 0)
                    {
                        cprintf("loaduvm failed!\n");
                        goto bad;
                    }
                    break;
                }
            }
        }
    }
bad:
    end_op();
    return;
}