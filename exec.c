#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

extern int (*swapfunc_ptr_arr[NO_SWAP][2])(struct proc*, int);

int
exec(char *path, char **argv)
{
  uint argc, sz, sp, ustack[3+MAXARG+1], i, off;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  char *s, *last;

  begin_op();
  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  getinode(curproc, ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  //Currently generalised that the sz will be from the last second Program Header.
  //To verify this for all the programs.

  sz = 0;
  
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    curproc->ph->filesz = ph.filesz;
    curproc->ph->memsz = ph.memsz;
    curproc->ph->off = ph.off;
    curproc->ph->vaddr = ph.vaddr;
    if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz, 0)) == 0)
      goto bad;
  }

  curproc->cdb_size = sz;

  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE, 0)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    safestrcpy(&(curproc->stack_buffer[sp]), argv[argc], strlen(argv[argc]) + 1);
    ustack[3+argc] = PGROUNDUP(curproc->cdb_size) + PGSIZE + sp; 
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] =  PGROUNDUP(curproc->cdb_size) + PGSIZE + (sp - (argc+1)*4);  // argv pointer

  sp -= (3+argc+1) * 4;
  memmove(curproc->stack_buffer + sp, ustack, (3+argc+1)*4);

  //Adding Path as well as Name.
  safestrcpy(curproc->path, path, strlen(path)+1);
  for (last=s=path; *s; s++)
    if (*s == '/')
      last=s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->path));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = PGROUNDUP(curproc->cdb_size) + PGSIZE + sp;
  switchuvm(curproc);
  cprintf("Swap Out Stack PID : %d and vaddr : %d\n", curproc->pid, sz-PGSIZE);
  if(swapfunc_ptr_arr[1][0](curproc, sz-PGSIZE) < 0)
  {
    panic("Swap Space is Full");
  }
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
  {
    lru_free(curproc->pid);
    swap_free(curproc->pid);
    freevm(pgdir);
  }
  if(ip)
  {
    iunlockput(ip);
    end_op();
  }
  return -1;
}
