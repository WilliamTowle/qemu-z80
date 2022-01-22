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


#define cpu_exec cpu_z80_exec

/* TODO: cpu_mmu_index() */

///* MMU modes definitions */
//// No modes to define - unlike x86 there is no kernel/usr/SMAP distinction
//static inline int cpu_mmu_index (CPUZ80State *env)
//{
//    return 0;	/* verbatim from target-lm32 */
//}

#include "exec/cpu-all.h"

#include "exec/exec-all.h"

#endif /* !defined (CPU_Z80_H) */
