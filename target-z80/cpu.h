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
    /* TODO: Z80 registers */

    CPU_COMMON

    /* TODO: 'int model' CPU identifier */
} CPUZ80State;

#include "cpu-qom.h"

Z80CPU *cpu_z80_init(const char *cpu_model);


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

static inline int cpu_mmu_index (CPUZ80State *env)
{
    return 0;	/* verbatim from target-lm32 */
		/* maybe MMU_USER_IDX? */
}

#include "exec/cpu-all.h"

#include "exec/exec-all.h"

#endif /* !defined (CPU_Z80_H) */
