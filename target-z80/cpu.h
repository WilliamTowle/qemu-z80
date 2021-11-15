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

#endif /* !defined (CPU_Z80_H) */
