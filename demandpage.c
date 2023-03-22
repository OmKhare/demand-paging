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

//Page Fault Handler currently placed here. Verify whether this is a good position for placing it.
void
pgflt_handler(){
    panic("Page Fault: Kernel Panic");
}