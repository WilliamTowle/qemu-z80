/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */

#ifndef CPU_Z80_H
#define CPU_Z80_H

#include "config.h"

#define TARGET_LONG_BITS 32

/* Z80 registers */

#define R_A     0
#define R_F     1

#define R_BC    2
#define R_DE    3
#define R_HL    4
#define R_IX    5
#define R_IY    6
#define R_SP    7

#define R_I     8
#define R_R     9

#define R_AX    10
#define R_FX    11
#define R_BCX   12
#define R_DEX   13
#define R_HLX   14

#define CPU_NB_REGS 15

#define CPUState struct CPUZ80State

#include "cpu-defs.h"

typedef struct CPUZ80State {
    CPU_COMMON

    /* Z80 registers */
    uint16_t pc;

    target_ulong regs[CPU_NB_REGS];

    int iff1;
    int iff2;
    int imode;
} CPUZ80State;

CPUZ80State *cpu_z80_init(const char *cpu_model);

#define TARGET_VIRT_ADDR_SPACE_BITS 32

#define cpu_init cpu_z80_init

#include "cpu-all.h"

static inline bool cpu_has_work(CPUState *env)
{
    /* repo.or.cz just triggers on CPU_INTERRUPT_HARD, but i386
     * also has NMI cause a 'true' result [? for legacy apic.o]
     */
    return env->interrupt_request & CPU_INTERRUPT_HARD;
}

#include "exec-all.h"

#endif /* CPU_Z80_H */
