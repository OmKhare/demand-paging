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

void pgflt_handler()
{

    struct proc *curproc = myproc();
    struct elfhdr elf;
    struct inode *ip;
    struct proghdr ph;
    uint i, off;
    char* mem;
    uint pgflt_addr = PGROUNDDOWN(rcr2());
    // cprintf("Page Fault Occoured for Vaddr : %d\n", pgflt_addr);
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
        get_lru(curproc->pid, pgflt_addr);
        if (swap_check(curproc, pgflt_addr) == 0)
        {
            //Handle the reading from the swap and returning here.
            if (swap_in(curproc, pgflt_addr) < 0)
            {
                panic("Swap In not working!");
            }
            return;
        }

        if ((mem = kalloc(curproc->pid, 1)) == 0)
        {
            panic("No Free Page");
        }
        if (mappages(curproc->pgdir, (char*)pgflt_addr, PGSIZE, V2P(mem), PTE_W | PTE_U | PTE_P, 1) < 0)
            panic("Error in Mappages");
        if ((ip = namei(curproc->path)) == 0)
            panic("namei error");
        ilock(ip);
        if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf))
            panic("readi error");
        if (elf.magic != ELF_MAGIC)
            panic("elf magic error");
        for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
        {
            if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
                panic("readi error");
            if (ph.vaddr <= pgflt_addr && pgflt_addr <= ph.vaddr + ph.memsz)
            {
                //Entire section of the code only present
                if (ph.vaddr + ph.filesz >= pgflt_addr + PGSIZE)
                    loaduvm(curproc->pgdir, (char *)(ph.vaddr + pgflt_addr), ip, ph.off + pgflt_addr, PGSIZE);
                else
                {
                    //Two cases possible.
                    //If the section is for bss section completly.
                    if (pgflt_addr >= ph.filesz)
                    {
                        if (pgflt_addr + PGSIZE <= ph.memsz)
                            stosb(mem, 0, PGSIZE);
                        else
                            stosb(mem, 0, ph.memsz - pgflt_addr);
                    }
                    else
                    //If the section overlaps bss as well as code+data
                    {   
                        loaduvm(curproc->pgdir, (char *)(ph.vaddr + pgflt_addr), ip, ph.off + pgflt_addr, ph.filesz - pgflt_addr);
                        stosb(mem + (ph.filesz - pgflt_addr), 0, PGSIZE- ( ph.filesz - pgflt_addr));
                        ip = namei(curproc->path);
                    }
                }
            }
        }
        iunlockput(ip);
    }
}