/* This file contains code for a generic page fault handler for processes. */
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

int loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz);
int flags2perm(int flags);

int find_heap_page(struct proc* p, uint64 addr) {
    // printf("finding heap page");
    for (int i=0; i<MAXHEAP; i++) {
        uint64 start = p->heap_tracker[i].addr;
        // printf("Start %d\n", start);
        if (addr == start) {
            return i;
        }
        if (start == -1) {
            return -1;
        }
    }
    return -1;
}

/* CSE 536: (2.4) read current time. */
uint64 read_current_timestamp() {
  uint64 curticks = 0;
  acquire(&tickslock);
  curticks = ticks;
  wakeup(&ticks);
  release(&tickslock);
  return curticks;
}

bool psa_tracker[PSASIZE];

/* All blocks are free during initialization. */
void init_psa_regions(void)
{
    for (int i = 0; i < PSASIZE; i++) 
        psa_tracker[i] = false;
}

int fifo_page_eviction(struct proc* p) {
    int least_idx = 0;
    uint64 least_time = read_current_timestamp();
    for (int i=0; i<MAXHEAP; i++) {
        if (p->heap_tracker[i].addr != 0xFFFFFFFFFFFFFFFF) {
            continue;
        }
        if (p->heap_tracker[i].last_load_time < least_time) {
            least_idx = i;
            least_time = p->heap_tracker[i].last_load_time;
        }
    }
    return least_idx;
}

/* Evict heap page to disk when resident pages exceed limit */
void evict_page_to_disk(struct proc* p) {
    /* Find free block */
    int blockno = 0;
    /* Find victim page using FIFO. */
    int evict_idx = fifo_page_eviction(p);
    /* Print statement. */
    print_evict_page(p->heap_tracker[evict_idx].addr, p->heap_tracker[evict_idx].startblock);
    /* Read memory from the user to kernel memory first. */
    
    /* Write to the disk blocks. Below is a template as to how this works. There is
     * definitely a better way but this works for now. :p */
    struct buf* b;
    b = bread(1, PSASTART+(blockno));
        // Copy page contents to b.data using memmove.
    bwrite(b);
    brelse(b);

    /* Unmap swapped out page */
    /* Update the resident heap tracker. */
}

/* Retrieve faulted page from disk. */
void retrieve_page_from_disk(struct proc* p, uint64 uvaddr) {
    /* Find where the page is located in disk */

    /* Print statement. */
    print_retrieve_page(0, 0);

    /* Create a kernel page to read memory temporarily into first. */
    
    /* Read the disk block into temp kernel page. */

    /* Copy from temp kernel page to uvaddr (use copyout) */

}

void page_fault_handler(void) 
{
    /* Current process struct */
    struct proc *p = myproc();
    struct inode *ip;
    struct elfhdr elf;
    struct proghdr ph;

    /* Track whether the heap page should be brought back from disk or not. */
    bool load_from_disk = false;

    /* Find faulting address. */
    uint64 stval = r_stval(); // Read the stval register to get faulting address
    uint64 faulting_addr = PGROUNDDOWN(stval); // Address of the Page Fault

    print_page_fault(p->name, faulting_addr);

    /* Check if the fault address is a heap page. Use p->heap_tracker */
    int heap_idx = find_heap_page(p, faulting_addr);
    if (heap_idx >= 0) {
        load_from_disk = p->heap_tracker[heap_idx].loaded;
        goto heap_handle;
    }

    begin_op();
    if((ip = namei(p->name)) == 0){
        printf("Error");
        return;
    }
    ilock(ip);
    if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf)) {
        printf("Read Error");
        return;
    }  
    if(elf.magic != ELF_MAGIC) {
        printf("Not so magical");
        return;
    }

    /* Iterate through program sections to find the program binary page */
    for (int i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)) {
        if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
            return;
        if(ph.type != ELF_PROG_LOAD)
            continue;
        if(ph.memsz < ph.filesz)
            return;
        if(ph.vaddr + ph.memsz < ph.vaddr)
            return;
        if(ph.vaddr % PGSIZE != 0)
            return;
            
        /* Check if faulting address belongs to this section */
        uint64 section_end = ph.vaddr + ph.memsz;
        if (faulting_addr >= ph.vaddr && faulting_addr < section_end) {
            // printf("Found the section of the faulting addr\n");
            /* Allocate a free physical page for the program binary page */
            uint64 pg = uvmalloc(p->pagetable, faulting_addr, faulting_addr + PGSIZE, flags2perm(ph.flags));
            if (pg == 0) {
                printf("Page Fault Handler: uvmalloc failed\n");
                break;
            }
            /* Load the required page from the program binary */
            if (loadseg(p->pagetable, faulting_addr, ip, ph.off, PGSIZE) < 0) {
                printf("Page Fault Handler: loadseg failed\n");
                break;
            }
            print_load_seg(faulting_addr, faulting_addr + PGSIZE, PGSIZE);
            break;
        }
    }

    iunlockput(ip);
    end_op();
    /* Go to out, since the remainder of this code is for the heap. */
    goto out;

heap_handle:
    /* 2.4: Check if resident pages are more than heap pages. If yes, evict. */
    if (p->resident_heap_pages == MAXRESHEAP) {
        evict_page_to_disk(p);
    }

    /* 2.3: Map a heap page into the process' address space. (Hint: check growproc) */
    uint64 sz = p->sz;
    if((sz = uvmalloc(p->pagetable, sz, sz + PGSIZE, PTE_W)) == 0) {
        return;
    }
    p->sz = sz;

    /* 2.4: Update the last load time for the loaded heap page in p->heap_tracker. */
    p->heap_tracker[heap_idx].last_load_time = read_current_timestamp();

    /* 2.4: Heap page was swapped to disk previously. We must load it from disk. */
    if (load_from_disk) {
        retrieve_page_from_disk(p, faulting_addr);
        p->heap_tracker[heap_idx].loaded = false;
    }

    /* Track that another heap page has been brought into memory. */
    p->resident_heap_pages++;

out:
    /* Flush stale page table entries. This is important to always do. */
    sfence_vma();
    return;
}