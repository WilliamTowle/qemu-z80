/*
 * Z80 virtual CPU header
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
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

/* flags masks */
#define CC_C   	0x0001
#define CC_N    0x0002
#define CC_P 	0x0004
#define CC_X 	0x0008
#define CC_H	0x0010
#define CC_Y 	0x0020
#define CC_Z	0x0040
#define CC_S    0x0080

/* hidden flags - used internally by qemu to represent additionnal cpu
   states. Only the CPL and INHIBIT_IRQ are not redundant. We avoid
   using the IOPL_MASK, TF_MASK and VM_MASK bit position to ease oring
   with eflags. */
/* current cpl */
#define HF_CPL_SHIFT         0
/* true if soft mmu is being used */
#define HF_SOFTMMU_SHIFT     2
/* true if hardware interrupts must be disabled for next instruction */
#define HF_INHIBIT_IRQ_SHIFT 3
/* 16 or 32 segments */
#define HF_CS32_SHIFT        4
#define HF_SS32_SHIFT        5
/* zero base for DS, ES and SS : can be '0' only in 32 bit CS segment */
#define HF_ADDSEG_SHIFT      6
/* copy of CR0.PE (protected mode) */
#define HF_PE_SHIFT          7
#define HF_TF_SHIFT          8 /* must be same as eflags */
#define HF_MP_SHIFT          9 /* the order must be MP, EM, TS */
#define HF_EM_SHIFT         10
#define HF_TS_SHIFT         11
#define HF_IOPL_SHIFT       12 /* must be same as eflags */
#define HF_LMA_SHIFT        14 /* only used on x86_64: long mode active */
#define HF_CS64_SHIFT       15 /* only used on x86_64: 64 bit code segment  */
#define HF_OSFXSR_SHIFT     16 /* CR4.OSFXSR */
#define HF_VM_SHIFT         17 /* must be same as eflags */
#define HF_SMM_SHIFT        19 /* CPU in SMM mode */

#define HF_CPL_MASK          (3 << HF_CPL_SHIFT)
#define HF_SOFTMMU_MASK      (1 << HF_SOFTMMU_SHIFT)
#define HF_INHIBIT_IRQ_MASK  (1 << HF_INHIBIT_IRQ_SHIFT)
#define HF_CS32_MASK         (1 << HF_CS32_SHIFT)
#define HF_SS32_MASK         (1 << HF_SS32_SHIFT)
#define HF_ADDSEG_MASK       (1 << HF_ADDSEG_SHIFT)
#define HF_PE_MASK           (1 << HF_PE_SHIFT)
#define HF_TF_MASK           (1 << HF_TF_SHIFT)
#define HF_MP_MASK           (1 << HF_MP_SHIFT)
#define HF_EM_MASK           (1 << HF_EM_SHIFT)
#define HF_TS_MASK           (1 << HF_TS_SHIFT)
#define HF_LMA_MASK          (1 << HF_LMA_SHIFT)
#define HF_CS64_MASK         (1 << HF_CS64_SHIFT)
#define HF_OSFXSR_MASK       (1 << HF_OSFXSR_SHIFT)
#define HF_SMM_MASK          (1 << HF_SMM_SHIFT)

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

#define CPUState struct CPUZ80State

#include "cpu-defs.h"

#define NB_MMU_MODES 2

typedef struct CPUZ80State {
#if 1	/* was: TARGET_LONG_BITS > HOST_LONG_BITS
	 * but can't compile else cases
	 */
    /* temporaries if we cannot store them in host registers */
    target_ulong t0, t1;
#endif
    target_ulong a0;

    CPU_COMMON

    int model;

    /* Z80 registers */
    uint16_t pc;

    target_ulong regs[CPU_NB_REGS];

    int iff1;
    int iff2;
    int imode;

    /* emulator internal eflags handling */
    uint32_t hflags; /* hidden flags, see HF_xxx constants */
} CPUZ80State;

CPUZ80State *cpu_z80_init(const char *cpu_model);
void z80_translate_init(void);
int cpu_z80_exec(CPUZ80State *s);

#define Z80_CPU_Z80  1
#define Z80_CPU_R800 2

struct siginfo;
int cpu_z80_signal_handler(int host_signum, void *pinfo,
                           void *puc);

#define TARGET_PAGE_BITS 12

#define TARGET_PHYS_ADDR_SPACE_BITS  16	/* 32? 36 for x86 - 64GiB */
#define TARGET_VIRT_ADDR_SPACE_BITS  16	/* 32? 32 for x86 - 4GiB */

/* helper.c */
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write, int mmu_idx);
#define cpu_handle_mmu_fault cpu_z80_handle_mmu_fault

#define cpu_init cpu_z80_init
#define cpu_exec cpu_z80_exec
#define cpu_signal_handler cpu_z80_signal_handler


#define MMU_USER_IDX 1

static inline int cpu_mmu_index(CPUState *env)
{
    /* see i386? returns 1 or 0 depending on HF_CPL_MASK setting:
     *	return (env->hflags & HF_CPL_MASK) == 3 ? 1 : 0;
     */
    return 0;
}

#include "cpu-all.h"

static inline bool cpu_has_work(CPUState *env)
{
    /* repo.or.cz just triggers on CPU_INTERRUPT_HARD, but i386
     * also has NMI cause a 'true' result [? for legacy apic.o]
     */
    return env->interrupt_request & CPU_INTERRUPT_HARD;
}

#include "exec-all.h"

static inline void cpu_pc_from_tb(CPUState *env, TranslationBlock *tb)
{
    env->pc = tb->pc;
    env->hflags = tb->flags;
}

static inline void cpu_get_tb_cpu_state(CPUState *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
    *pc = env->pc;
    *cs_base = 0;           /* Z80: no code segments */
    *flags= env->hflags;    /* TARGET_I386 includes eflags too */
}

#endif /* CPU_Z80_H */
