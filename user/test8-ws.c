#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#include <stdarg.h>

int
main(int argc, char *argv[])
{
    /* Allocate as many pages as allowed. */
    int npages = 101;
    void* heappages = sbrk(4096*npages);
    if (!heappages) {
        printf("[X] Heap memory allocation FAILED.\n");
        return -1;
    }

    /* Write random numbers to the allocated heap regions. */

    int* a;
    a = ((int*) (heappages));
    *a = 0;
    printf("Wrote data to first page and sleeping for 5 seconds...\n");
    sleep(50);

    int count = 0;
    for (int i = 1; i < (npages-1); i++) {
        a = ((int*) (heappages + i*PGSIZE));
        for (int j = 0; j < PGSIZE/sizeof(int); j++) {
            *a = count;
            a++;
        }
        count++;
    }
    printf("Wrote data to pages 2 to 100\n");
    printf("Attempting to write to page 101\n");
    a = ((int*) (heappages + 100*PGSIZE));
    *a = count;
    printf("Although page 1 was the last accessed, \n");
    printf("since it is not part of the working set (WS_THRESHOLD = 3 seconds), \n");
    printf("Page 100 (VA 67000) should be the evicted page. \n");
    printf("If prev. line reads 'EVICT: Page (67000) --> PSA (0 - 3)', TEST PASSED.\n");

    return 0;
}