#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "trap-and-emulate-registers.h"

// Struct to keep VM registers (Sample; feel free to change.)
struct vm_reg {
    int code;
    enum mode_t {
        MACHINE_MODE,
        SUPERVISOR_MODE,
        USER_MODE
    } mode;
    uint64 val;
};

// Keep the virtual state of the VM's privileged registers
struct vm_virtual_state {
    // Machine information registers
    uint64 mvendorid;
    uint64 marchid;
    uint64 mimpid;
    uint64 mhartid;
    // Machine trap setup registers
    uint64 mstatus;
    uint64 medeleg;
    uint64 mideleg;
    uint64 mie;
    uint64 mtvec;
    // Machine trap handling registers
    uint64 mscratch;
    uint64 mepc;
    uint64 mcause;
    uint64 mtval;
    uint64 mip;

    // Supervisor trap setup
    uint64 sstatus;
    uint64 sie;
    uint64 stvec;
    uint64 sscratch;
    uint64 sepc;
    uint64 scause;
    uint64 stval;
    // Supervisor page table register
    uint64 satp;

    // User trap setup
    uint64 ustatus;
    uint64 uie;
    uint64 utvec;
    // User trap handling
    uint64 uscratch;
    uint64 uepc;
    uint64 ucause;
    uint64 utval;
    uint64 uip;

    // PMP
    uint64 pmpcfg0;
    uint64 pmpcfg2;
    uint64 pmpcfg4;
    uint64 pmpcfg6;
    uint64 pmpcfg8;
    uint64 pmpcfg10;
    uint64 pmpcfg12;
    uint64 pmpcfg14;
    uint64 pmpaddr0;
    uint64 pmpaddr1;
    uint64 pmpaddr2;
    uint64 pmpaddr3;
    uint64 pmpaddr4;
    uint64 pmpaddr5;
    uint64 pmpaddr6;
    uint64 pmpaddr7;
    
    struct vm_reg registers;
};

static struct vm_virtual_state vm;

static uint64 *csr_mappings[] = {
    [CSR_ustatus]       &vm.ustatus,
    [CSR_uie]           &vm.uie,
    [CSR_utvec]         &vm.utvec,
    [CSR_uscratch]      &vm.uscratch,
    [CSR_uepc]          &vm.uepc,
    [CSR_ucause]        &vm.ucause,
    [CSR_utval]         &vm.utval,
    [CSR_uip]           &vm.uip,
    [CSR_sstatus]       &vm.sstatus,
    // [CSR_sedeleg]       &vm.sedeleg,
    // [CSR_sideleg]       &vm.sideleg,
    [CSR_sie]           &vm.sie,
    [CSR_stvec]         &vm.stvec,
    // [CSR_scounteren]    &vm.scounteren,
    [CSR_sscratch]      &vm.sscratch,
    [CSR_sepc]          &vm.sepc,
    [CSR_scause]        &vm.scause,
    [CSR_stval]         &vm.stval,
    // [CSR_sip]           &vm.sip,
    [CSR_satp]          &vm.satp,
    [CSR_mvendorid]     &vm.mvendorid,
    [CSR_marchid]       &vm.marchid,
    [CSR_mimpid]        &vm.mimpid,
    [CSR_mhartid]       &vm.mhartid,
    [CSR_mstatus]       &vm.mstatus,
    // [CSR_misa]          &vm.misa,
    [CSR_medeleg]       &vm.medeleg,
    [CSR_mideleg]       &vm.mideleg,
    [CSR_mie]           &vm.mie,
    [CSR_mtvec]         &vm.mtvec,
    // [CSR_mcounteren]    &vm.mcounteren,
    // [CSR_mstatush]      &vm.mstatush,
    [CSR_mscratch]      &vm.mscratch,
    [CSR_mepc]          &vm.mepc,
    [CSR_mcause]        &vm.mcause,
    [CSR_mtval]         &vm.mtval,
    [CSR_mip]           &vm.mip,
    // [CSR_mtinst]        &vm.mtinst,
    // [CSR_mtval2]        &vm.mtval2,
    [CSR_pmpcfg0]       &vm.pmpcfg0,
    [CSR_pmpcfg2]       &vm.pmpcfg2,
    [CSR_pmpcfg4]       &vm.pmpcfg4,
    [CSR_pmpcfg6]       &vm.pmpcfg6,
    [CSR_pmpcfg8]       &vm.pmpcfg8,
    [CSR_pmpcfg10]      &vm.pmpcfg10,
    [CSR_pmpcfg12]      &vm.pmpcfg12,
    [CSR_pmpcfg14]      &vm.pmpcfg14,
    [CSR_pmpaddr0]      &vm.pmpaddr0,
    [CSR_pmpaddr1]      &vm.pmpaddr1,
    [CSR_pmpaddr2]      &vm.pmpaddr2,
    [CSR_pmpaddr3]      &vm.pmpaddr3,
    [CSR_pmpaddr4]      &vm.pmpaddr4,
    [CSR_pmpaddr5]      &vm.pmpaddr5,
    [CSR_pmpaddr6]      &vm.pmpaddr6,
    [CSR_pmpaddr7]      &vm.pmpaddr7,
};

static uint64* reg_mappings(struct trapframe *tf, uint32 reg) {
    switch(reg) {
        case REG_ra: return &tf->ra;
        case REG_sp: return &tf->sp;
        case REG_gp: return &tf->gp;
        case REG_tp: return &tf->tp;
        case REG_t0: return &tf->t0;
        case REG_t1: return &tf->t1;
        case REG_t2: return &tf->t2;
        case REG_s0: return &tf->s0;
        case REG_s1: return &tf->s1;
        case REG_a0: return &tf->a0;
        case REG_a1: return &tf->a1;
        case REG_a2: return &tf->a2;
        case REG_a3: return &tf->a3;
        case REG_a4: return &tf->a4;
        case REG_a5: return &tf->a5;
        default: return &tf->a5;
    }
}

void trap_and_emulate_init(void) {
    /* Create and initialize all state for the VM */
    vm.mvendorid = 0x637365353336; // cse536
    vm.marchid = 0;
    vm.mimpid = 0;
    vm.mhartid = 0;
    vm.mstatus = 0;
    vm.medeleg = 0;
    vm.mideleg = 0;
    vm.mie = 0;
    vm.mtvec = 0;
    vm.mscratch = 0;
    vm.mepc = 0;
    vm.mcause = 0;
    vm.mtval = 0;
    vm.mip = 0;

    vm.sstatus = 0;
    vm.sie = 0;
    vm.stvec = 0;
    vm.sscratch = 0;
    vm.sepc = 0;
    vm.scause = 0;
    vm.stval = 0;
    vm.satp = 0;

    vm.ustatus = 0;
    vm.uie = 0;
    vm.utvec = 0;
    vm.uscratch = 0;
    vm.uepc = 0;
    vm.ucause = 0;
    vm.utval = 0;
    vm.uip = 0;
    
    vm.pmpcfg0 = 0;
    vm.pmpcfg2 = 0;
    vm.pmpcfg4 = 0;
    vm.pmpcfg6 = 0;
    vm.pmpcfg8 = 0;
    vm.pmpcfg10 = 0;
    vm.pmpcfg12 = 0;
    vm.pmpcfg14 = 0;
    vm.pmpaddr0 = 0;
    vm.pmpaddr1 = 0;
    vm.pmpaddr2 = 0;
    vm.pmpaddr3 = 0;
    vm.pmpaddr4 = 0;
    vm.pmpaddr5 = 0;
    vm.pmpaddr6 = 0;
    vm.pmpaddr7 = 0;
    
    // Current execution mode
    vm.registers.mode = MACHINE_MODE;
}

int check_access(uint32 uimm) {
    if (vm.registers.mode == USER_MODE && uimm > 0x50)
        return 0;
    if (vm.registers.mode == SUPERVISOR_MODE && uimm > 0x200) 
        return 0;
    return 1;
}

void trap_and_emulate_ecall() {
    struct proc *p = myproc();
    printf("(EC at %p)\n", p->trapframe->epc);
    vm.sepc = p->trapframe->epc;
    p->trapframe->epc = vm.stvec;
    vm.registers.mode = SUPERVISOR_MODE;
}

void trap_and_emulate(void) {
    /* Comes here when a VM tries to execute a supervisor instruction. */

    struct proc *p = myproc();
    struct trapframe *tf = p->trapframe;
    uint32 instr;
    if (copyin(p->pagetable, (char *)&instr, tf->epc, sizeof(instr))) {
        printf("prob\n");
    }
    // printf("INSTR: %p\n", instr);

    // printf("Checking 1 %x\n", vm.stvec);
    // printf("Checking 2 %x\n", vm.sepc);

    uint64 addr     = tf->epc;
    uint32 op       = instr & 0x7F;
    uint32 rd       = (instr >> 7) & 0x1F;
    uint32 funct3   = (instr >> 12) & 0x7;
    uint32 rs1      = (instr >> 15) & 0x1F;
    uint32 uimm     = (instr >> 20);
    // uint32 rs2 = (instr >> 20) & 0x1F;
    // uint32 funct7 = (instr >> 25) & 0x7F;

    /* Print the statement */
    printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", 
                addr, op, rd, funct3, rs1, uimm);

    if (!check_access(uimm)) {
        printf("Illegal register access! 😈\n");
        printf("Killing Guest OS 🤺\n");
        setkilled(p);
        return;
    }

    if (addr == 0x424) {
        panic("Stop\n");
    }
    
    if (op == 0x73) {
        if (funct3 == 2) { // csrr
            *reg_mappings(tf, rd) = *csr_mappings[uimm];
            p->trapframe->epc += 4;
        } else if (funct3 == 1) { // csrw
            *csr_mappings[uimm] = *reg_mappings(tf, rs1);
            p->trapframe->epc += 4;
        } else { // mret, sret, ecall
            if (uimm == 0x302) { // mret
                vm.registers.mode = SUPERVISOR_MODE;
                p->trapframe->epc = vm.mepc;
            } else if (uimm == 0x102) { // sret
                vm.registers.mode = USER_MODE;
                p->trapframe->epc = vm.sepc;
            } else { // ecall
                p->trapframe->epc = vm.stvec;
            }
        }
    }
}