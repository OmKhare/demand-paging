// #include "types.h"
// #include "user.h"
// int main(int argc, char *argv[])
// {
//     int i = 0;
//     char* pages[11];
//     // access the pages in a cyclic manner to generate page faults
//     for (i = 0; i < 11; i++) {
//         pages[i] = (char *)sbrk(4096);
//         pages[i][0] = 1;
//     }

//     printf(1, "\nDone with Part 1\n");
//     for (i = 0; i < 11; i++) {
//         printf(1, "Value at %d %d\n", pages[i][0], pages[i]);
//         pages[i][0] = 2;
//     }

//     printf(1, "\nDone with Part 2\n");
//     exit();
// }

#include "types.h"
#include "user.h"

#define PAGE_SIZE 4096
#define NUM_PAGES 11
#define NUM_ACCESSES 100

int main(int argc, char *argv[]) {
  char *pages[NUM_PAGES];
  int i, j;

  // Allocate and initialize pages with zeroes
  for (i = 0; i < NUM_PAGES; i++) {
    pages[i] = sbrk(PAGE_SIZE);
    if (pages[i] == (char*)-1) {
      printf(2, "Error: Failed to allocate page %d.\n", i);
      exit();
    }
    memset(pages[i], 0, PAGE_SIZE);
  }

  // Access pages randomly to simulate demand paging
  for (i = 0; i < NUM_ACCESSES; i++) {
    j = i % NUM_PAGES;
    pages[j][0] = 'a'; // Access first byte of page to trigger page fault
  }

  printf(1, "All pages accessed without errors.\n");

  exit();
}