#include "types.h"
#include "user.h"
int main(int argc, char *argv[])
{
    int i = 0;
    char* pages[11];
    // access the pages in a cyclic manner to generate page faults
    for (i = 0; i < 11; i++) {
        pages[i] = (char *)sbrk(4096);
        pages[i][0] = 1;
    }

    printf(1, "\nDone with Part 1\n");
    for (i = 0; i < 11; i++) {
        printf(1, "Value at %d %d\n", pages[i][0], pages[i]);
        pages[i][0] = 2;
    }

    printf(1, "\nDone with Part 2\n");
    exit();
}