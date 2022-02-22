/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */

#ifndef CPU_Z80_H
#define CPU_Z80_H

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


#define CPUArchState struct CPUZ80State

#include "exec/cpu-defs.h"

/* cpu-defs.h needs NB_MMU_MODES */
#define NB_MMU_MODES 1	/* more in target-x86 */

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
Z80CPU *cpu_z80_create(const char *cpu_model, DeviceState *icc_bridge,
                       Error **errp);
int cpu_z80_exec(CPUZ80State *s);


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


// MMU modes definitions?

#define cpu_exec cpu_z80_exec

static inline int cpu_mmu_index (CPUZ80State *env)
{
    return 0;	/* verbatim from target-lm32 */
		/* maybe MMU_USER_IDX? */
}

#include "exec/cpu-all.h"

static inline bool cpu_has_work(CPUState *cs)
{
#if 1   /* As implemented at repo.or.cz target-z80/exec.h
         * Fixme? Zilog's Z80 has NMI type interrupts, and not
         * all Zaphod platforms have I/O using IRQs
         */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
#else   /* as implemented for target-i386
         * POLL/SMI/NMI/MCE/VIRQ/INIT/SIPI/TPR are described as
         * i386-specific
         */
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;
    return ((cs->interrupt_request & (CPU_INTERRUPT_HARD |
                                      CPU_INTERRUPT_POLL)) &&
            (env->eflags & IF_MASK)) ||
           (cs->interrupt_request & (CPU_INTERRUPT_NMI |
                                     CPU_INTERRUPT_INIT |
                                     CPU_INTERRUPT_SIPI |
                                     CPU_INTERRUPT_MCE));
#endif
}

#include "exec/exec-all.h"

static inline void cpu_get_tb_cpu_state(CPUZ80State *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
#if 1   /* As implemented at repo.or.cz target-z80/cpu.h */
    *pc= env->pc;
    *cs_base= 0;
    *flags= env->hflags;
#else   /* as implemented for target-i386 */
    *cs_base = env->segs[R_CS].base;
    *pc = *cs_base + env->eip;
    *flags = env->hflags |
        (env->eflags & (IOPL_MASK | TF_MASK | RF_MASK | VM_MASK | AC_MASK));
#endif
}

#endif /* !defined (CPU_Z80_H) */
