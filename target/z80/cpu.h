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


/* Array indexes for registers.
 * NB: corresponding z80-cpu.xml "gdb target" file with register sizes
 * has not been written
 */
enum {
    R_SP= 0,    /* repo.or.cz original has idx=7 (REGISTERS > 1) */
};

/* TODO: hidden flags, exception/interrupt defines */

#define CPU_NB_REGS 1

//#define NB_MMU_MODES 1


/* CPUZ80State */

typedef struct CPUZ80State {
    /* TODO: needs support variables, other registers */
    uint32_t        regs[CPU_NB_REGS];
    target_ulong    pc;

    /* TODO: full CPU reset needs imode, iff<n> */

    /* emulator internal flags handling */
    uint32_t hflags;    /* hidden flags, see HF_xxx constants */

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


void z80_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                        int flags);

void z80_cpu_list(FILE *f, fprintf_function cpu_fprintf);


int cpu_z80_signal_handler(int host_signum, void *pinfo, void *puc);


#define TARGET_PAGE_BITS 8

#define TARGET_PHYS_ADDR_SPACE_BITS 24
#define TARGET_VIRT_ADDR_SPACE_BITS 24


#define Z80_CPU_TYPE_SUFFIX "-" TYPE_Z80_CPU
#define Z80_CPU_TYPE_NAME(name) (name Z80_CPU_TYPE_SUFFIX)
#define CPU_RESOLVING_TYPE TYPE_Z80_CPU

/* TODO: TARGET_DEFAULT_CPU_TYPE */


#define cpu_signal_handler cpu_z80_signal_handler
#define cpu_list z80_cpu_list


/* MMU modes definitions:
 * Unlike x86 we have no kernel/user/SMAP distinction
 */
#define MMU_NONE_IDX    0
#define MMU_USER_IDX    MMU_NONE_IDX    /* used by tcg implementation */
static inline int cpu_mmu_index(CPUZ80State *env, bool ifetch)
{
    return MMU_NONE_IDX;
}


#include "exec/cpu-all.h"


static inline void cpu_get_tb_cpu_state(CPUZ80State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *cs_base = 0;               /* Z80: unused */
    *pc = env->pc;
    *flags = env->hflags;       /* Z80: no env->eflags */
}

#endif /* Z80_CPU_H */
