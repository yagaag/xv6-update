#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// Struct to keep VM registers (Sample; feel free to change.)
struct vm_reg {
    int code;
    enum mode_t {
        MACHINE_MODE,
        SUPERVISOR_MODE,
        USER_MODE
    } mode;
    uint64  val;
};

// Keep the virtual state of the VM's privileged registers
struct vm_virtual_state {
    // User trap setup
    uint64 uscratch;
    // User trap handling
    uint64 ustatus;
    uint64 utvec;
    uint64 uepc;
    uint64 ucause;
    uint64 utval;
    uint64 uip;
    // Supervisor trap setup
    uint64 stvec;
    uint64 sepc;
    uint64 sscratch;
    // Supervisor page table register
    uint64 satp;
    // Machine information registers
    uint64 mvendorid;
    uint64 sstatus;
    uint64 scause;
    // Machine trap setup registers
    uint64 medeleg;
    uint64 mideleg;
    uint64 mie;
    uint64 mip;
    // Machine trap handling registers
    uint64 mstatus;
    uint64 mtvec;
    uint64 mepc;
    uint64 mcause;
    uint64 mtval;

    struct vm_reg registers;
};

void trap_and_emulate(void) {
    /* Comes here when a VM tries to execute a supervisor instruction. */

    printf("holabol\n");

    uint32 op       = 0;
    uint32 rd       = 0;
    uint32 rs1      = 0;
    uint32 upper    = 0;

    printf("[PI] op = %x, rd = %x, rs1 = %x, upper = %x\n", op, rd, rs1, upper);
}

void trap_and_emulate_init(void) {
    /* Create and initialize all state for the VM */
    // struct vm_virtual_state vm = {
    //     // Machine trap handling registers
    //     .mstatus = 0,
    //     .mtvec = 0,
    //     .mepc = 0,
    //     .mcause = 0,
    //     .mtval = 0,
        
    //     // Machine setup trap registers
    //     .medeleg = 0,
    //     .mideleg = 0,
    //     .mie = 0,
    //     .mip = 0,
        
    //     // Machine information state registers
    //     .satp = 0,
    //     .sstatus = 0,
    //     .scause = 0,
        
    //     // Machine physical memory protection registers
    //     // .pmpcfg0 = 0,
    //     // .pmpcfg1 = 0,
    //     // .pmpcfg2 = 0,
    //     // .pmpcfg3 = 0,
    //     // .pmpaddr0 = 0,
    //     // .pmpaddr1 = 0,
    //     // .pmpaddr2 = 0,
    //     // .pmpaddr3 = 0,
        
    //     // Supervisor trap setup registers
    //     .stvec = 0,
    //     .sepc = 0,
    //     .sscratch = 0,
        
    //     // User trap handling registers
    //     .ustatus = 0,
    //     .utvec = 0,
    //     .uepc = 0,
    //     .ucause = 0,
    //     .utval = 0,
    //     .uip = 0,
        
    //     // User trap setup registers
    //     .uscratch = 0,
        
    //     // Current execution mode
    //     .registers.mode = MACHINE_MODE,
        
    //     // Vendor ID register
    //     .mvendorid = 0x637365353336
    // };

}