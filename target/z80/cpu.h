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
#include "exec/cpu-defs.h"

/* TODO: TARGET_MAX_INSN_SIZE, TARGET_HAS_PRECISE_SMC */

/* TODO: hidden flags, exception/interrupt defines */


/* CPUZ80State */

typedef struct CPUZ80State {
    /* TODO: needs support variables, other registers */
    target_ulong    pc;

    /* TODO: full CPU reset needs imode, iff<n> */

    /* emulator internal flags handling */
    uint32_t hflags;    /* hidden flags, see HF_xxx constants */

    /* TODO: identifier for CPU model */
} CPUZ80State;


/* Z80CPU - a Z80 CPU */

struct Z80CPU {
    /*< private >*/
    CPUState parent_obj;

    /*< public >*/
    CPUNegativeOffsetState neg;
    CPUZ80State env;
};


#define Z80_CPU_TYPE_SUFFIX "-" TYPE_Z80_CPU
#define Z80_CPU_TYPE_NAME(name) (name Z80_CPU_TYPE_SUFFIX)
#define CPU_RESOLVING_TYPE TYPE_Z80_CPU

/* TODO: TARGET_DEFAULT_CPU_TYPE */



typedef CPUZ80State CPUArchState;
typedef Z80CPU      ArchCPU;

#include "exec/cpu-all.h"


static inline void cpu_get_tb_cpu_state(CPUZ80State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *cs_base = 0;               /* Z80: unused */
    *pc = env->pc;
    *flags = env->hflags;       /* Z80: no env->eflags */
}

#endif /* Z80_CPU_H */
