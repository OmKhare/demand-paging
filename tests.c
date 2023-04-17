#include "types.h"
#include "user.h"
int main(int argc, char *argv[])
{
    int i = 0;
    char* pages[13];
    // access the pages in a cyclic manner to generate page faults
    for (i = 0; i < 11; i++) {
        pages[i] = (char *)sbrk(4096);
    }
    exit();
}