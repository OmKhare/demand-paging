#include "types.h"
#include "stat.h"
#include "user.h"

#define PAGE_SIZE 4096
#define NUM_PAGES 1001
#define NUM_ACCESSES 100

void foo(void)
{
  char c = 'a';
  char *addr = &c;
  int i;
  for (i = 0; i < 10; i++)
  {
    addr += PAGE_SIZE;
    *addr = c + i + 1;
  }
}

void forkTest()
{
  printf(1, "fork test\n");
  int pid = fork();
  if (pid == 0)
  {
    // child process
    foo();
    exit();
  }
  else if (pid > 0)
  {
    // parent process
    wait();
    printf(1, "fork test ok\n");
  }
  else
  {
    printf(1, "fork error\n");
  }
}

void sbrkTest()
{
  printf(1, "sbrk test\n");
  char *pages[NUM_PAGES];
  int i, j;

  // Allocate and initialize pages with zeroes
  for (i = 0; i < NUM_PAGES; i++)
  {
    pages[i] = sbrk(PAGE_SIZE);
    if (pages[i] == (char *)-1)
    {
      printf(2, "Error: sbrk test error %d.\n", i);
      exit();
    }
    memset(pages[i], 0, PAGE_SIZE);
  }

  // Access pages randomly to simulate demand paging
  for (i = 0; i < NUM_ACCESSES; i++)
  {
    j = i % NUM_PAGES;
    pages[j][0] = 'a'; // Access first byte of page to trigger page fault
  }

  sbrk(-(NUM_PAGES * PAGE_SIZE));
  printf(1, "sbrk test ok\n");
}

int main(int argc, char *argv[])
{
  lrur();
  swapr();
  sbrkTest();
  lrur();
  swapr();
  forkTest();
  exit();
}