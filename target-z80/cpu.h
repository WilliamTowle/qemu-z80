/*
 * Minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 */

#ifndef CPU_Z80_H
#define CPU_Z80_H

//#ifdef CONFIG_USER_ONLY
//    /* Temporarily bail from builds of z80-bblbrx-user while partial */
//#error "CONFIG_USER_ONLY for z80 has incomplete support"
//#endif
#ifdef CONFIG_SOFTMMU
    /* Temporarily bail from builds of z80-softmmu while partial */
#error "z80-softmmu support incomplete"
#endif


//#include "config.h"
//#include "qemu-common.h"


#define TARGET_LONG_BITS 32

//#define EXCP00_DIVZ	0
//#define EXCP01_SSTP	1
//#define EXCP02_NMI	2
//#define EXCP03_INT3	3
//#define EXCP04_INTO	4
//#define EXCP05_BOUND	5
#define EXCP06_ILLOP	6
//#define EXCP07_PREX	7
//#define EXCP08_DBLE	8
//#define EXCP09_XERR	9
//#define EXCP0A_TSS	10
//#define EXCP0B_NOSEG	11
//#define EXCP0C_STACK	12
//#define EXCP0D_GPF	13
//#define EXCP0E_PAGE	14
//#define EXCP10_COPR	16
//#define EXCP11_ALGN	17
//#define EXCP12_MCHK	18
#define EXCP_KERNEL_TRAP	19


#define CPUArchState struct CPUZ80State

#include "exec/cpu-defs.h"

/* TODO: NB_MMU_MODES define, CPUZ80State, TARGET_PAGE_BITS */

//#define NB_MMU_MODES 1	/* no kernel/userland distinction */

typedef struct CPUZ80State {
    /* Z80 registers */
    uint16_t pc;    /* Program Counter (referencing 64KiB main RAM) */

    /* FIXME: need data registers, index registers, interrupt registers */

    /* emulator internal eflags handling */
    uint32_t hflags; /* hidden flags, see HF_xxx constants */

    CPU_COMMON

    /* TODO: 'int model' CPU identifier */
} CPUZ80State;


#include "cpu-qom.h"

Z80CPU *cpu_z80_init(const char *cpu_model);
void z80_cpu_list(FILE *f, fprintf_function cpu_fprintf);
Z80CPU *cpu_z80_create(const char *cpu_model, DeviceState *icc_bridge,
                       Error **errp);
int cpu_z80_exec(CPUZ80State *s);


#define Z80_CPU_Z80  1
//#define Z80_CPU_R800 2


/* TARGET_PAGE_BITS required by exec-all.h cache */
#define TARGET_PAGE_BITS 12	/* from target-x86 */

#define TARGET_PHYS_ADDR_SPACE_BITS 32 /* min TCG register size? */
#define TARGET_VIRT_ADDR_SPACE_BITS 32 /* min TCG register size? */

static inline CPUZ80State *cpu_init(const char *cpu_model)
{
    Z80CPU *cpu = cpu_z80_init(cpu_model);
    if (cpu == NULL) {
        return NULL;
    }
    return &cpu->env;
}


#define cpu_exec cpu_z80_exec

#define cpu_list z80_cpu_list
/* TODO: cpu_mmu_index() */

///* MMU modes definitions */
//// No modes to define - unlike x86 there is no kernel/usr/SMAP distinction
//static inline int cpu_mmu_index (CPUZ80State *env)
//{
//    return 0;	/* verbatim from target-lm32 */
//}

#include "exec/cpu-all.h"

static inline bool cpu_has_work(CPUState *cs)
{
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK set; Zilog's Z80 also has NMI type interrupts [not
     * implemented]
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
}

#include "exec/exec-all.h"

static inline void cpu_get_tb_cpu_state(CPUZ80State *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
#if 1   /* As implemented at repo.or.cz target-z80/cpu.h */
    *pc= env->pc;
    *cs_base= 0;
    *flags= env->hflags;     /* Z80: no env->eflags */
#else   /* as implemented for target-i386 */
    *cs_base = env->segs[R_CS].base;
    *pc = *cs_base + env->eip;
    *flags = env->hflags |
        (env->eflags & (IOPL_MASK | TF_MASK | RF_MASK | VM_MASK | AC_MASK));
#endif
}

/* op_helper.c */
void QEMU_NORETURN raise_exception(CPUZ80State *env, int exception_index);
void QEMU_NORETURN raise_exception_err(CPUZ80State *env, int exception_index,
                                       int error_code);
void QEMU_NORETURN raise_interrupt(CPUZ80State *env, int intno, int is_int, int error_code,
                     int next_eip_addend);

#endif /* !defined (CPU_Z80_H) */