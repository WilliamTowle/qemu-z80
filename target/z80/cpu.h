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


/* Array indexes for registers.
 * NB: corresponding z80-cpu.xml "gdb target" file with register sizes
 * has not been written
 */
enum {
    R_SP= 0,    /* repo.or.cz original has idx=7 (REGISTERS > 1) */
};

/* TODO: hidden flags */

/* Exception defines */

#define EXCP_ILLOP          0       /* i386: EXCP06_ILLOP (n=6) */
#define EXCP_KERNEL_TRAP    1
/* TODO: need conventional exception for handling NMI? */


/* TODO: interrupt defines */

#define CPU_NB_REGS 1


/* CPUZ80State */

typedef struct CPUZ80State {
    /* TODO: needs support variables, other registers */
    target_ulong    regs[CPU_NB_REGS];
    target_ulong    pc;

    /* TODO: full CPU reset needs imode, iff<n> */

    /* emulator internal flags handling */
    uint32_t hflags;    /* hidden flags, see HF_xxx constants */

    struct {} end_reset_fields;
    /* Fields after this point are preserved across CPU reset. */

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


/* helper.c */
void z80_cpu_dump_state(CPUState *cs, FILE *f, int flags);

void z80_cpu_list(void);

int cpu_z80_signal_handler(int host_signum, void *pinfo, void *puc);


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


/* translate.c */
void tcg_z80_init(void);

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
