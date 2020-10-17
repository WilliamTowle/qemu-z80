/*
 * Minimal QEmu Z80 CPU - virtual CPU header
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */

#ifndef Z80_CPU_H
#define Z80_CPU_H


//#ifdef CONFIG_USER_ONLY
//    /* Temporarily bail from builds of z80-bblbrx-user while partial */
//#error "CONFIG_USER_ONLY builds for z80 have incomplete support"
//#endif
#ifdef CONFIG_SOFTMMU
    /* Temporarily bail from builds of z80-softmmu while partial */
#error "CONFIG_SOFTMMU builds for z80 have incomplete support"
#endif


#include "cpu-qom.h"

/* Optimal host size of target-ulong. '32' leads to TARGET_FMT_lx
 * of "%08x" from cpu-defs.h
 */
#define TARGET_LONG_BITS 32

#include "exec/cpu-defs.h"

/* TODO: TARGET_MAX_INSN_SIZE, TARGET_HAS_PRECISE_SMC */

#define CPUArchState struct CPUZ80State

/* TODO: hidden flags, exception/interrupt defines, register count */


/* TODO: Register-related declarations and count */

//#define NB_MMU_MODES 1


/* CPUZ80State */

typedef struct CPUZ80State {
    /* TODO: needs support variables, other registers */
    target_ulong    pc;

    /* TODO: CPU_COMMON adds fields used by z80-softmmu */
    //CPU_COMMON
} CPUZ80State;


/* Z80CPU - a Z80 CPU */

struct Z80CPU {
    /*< private >*/
    CPUState parent_obj;

    /*< public >*/
    CPUZ80State env;
};


static inline Z80CPU *z80_env_get_cpu(CPUZ80State *env)
{
    return container_of(env, Z80CPU, env);
}

#define ENV_GET_CPU(e) CPU(z80_env_get_cpu(e))

#define ENV_OFFSET offsetof(Z80CPU, env)


#define TARGET_PAGE_BITS 8

#define TARGET_PHYS_ADDR_SPACE_BITS 24
#define TARGET_VIRT_ADDR_SPACE_BITS 24


#define Z80_CPU_TYPE_SUFFIX "-" TYPE_Z80_CPU
#define Z80_CPU_TYPE_NAME(name) (name Z80_CPU_TYPE_SUFFIX)
#define CPU_RESOLVING_TYPE TYPE_Z80_CPU

/* TODO: TARGET_DEFAULT_CPU_TYPE */


/* TODO: MMU modes list */

#include "exec/cpu-all.h"

#endif /* Z80_CPU_H */
