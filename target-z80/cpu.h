/*
 * QEmu Z80 CPU
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

/* target supports implicit self modifying code */
#define TARGET_HAS_SMC
/* support for self modifying code even if the modified instruction is
   close to the modifying instruction */
#define TARGET_HAS_PRECISE_SMC

//#define TARGET_HAS_ICE 1	/* breakpoint handling */


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


#define CPUArchState struct CPUZ80State

#include "exec/cpu-defs.h"


/* hidden flags - used internally by qemu to represent additional cpu
   states. Only the CPL and INHIBIT_IRQ are not redundant. We avoid
   using the IOPL_MASK, TF_MASK and VM_MASK bit position to ease oring
   with eflags. */
/* true if soft mmu is being used */
#define HF_SOFTMMU_SHIFT     2
/* true if hardware interrupts must be disabled for next instruction */
#define HF_INHIBIT_IRQ_SHIFT 3

#define HF_SOFTMMU_MASK      (1 << HF_SOFTMMU_SHIFT)
#define HF_INHIBIT_IRQ_MASK  (1 << HF_INHIBIT_IRQ_SHIFT)


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


/* TODO: NB_MMU_MODES define */

//#define NB_MMU_MODES 1	/* no kernel/userland distinction */


typedef struct CPUZ80State {
#if 1	/* was: TARGET_LONG_BITS > HOST_LONG_BITS
	 * but can't compile else cases
	 */
    /* temporaries if we cannot store them in host registers */
    target_ulong t0, t1;
#endif
    target_ulong a0;

    /* Z80 registers */
    uint16_t pc;    /* Program Counter (referencing 64KiB main RAM) */
    target_ulong regs[CPU_NB_REGS];

    int iff1;
    int iff2;
    int imode;

    /* emulator internal eflags handling */
    uint32_t hflags; /* hidden flags, see HF_xxx constants */

    CPU_COMMON

    /* CPU model identifier */
    int model;
} CPUZ80State;


#include "cpu-qom.h"

Z80CPU *cpu_z80_init(const char *cpu_model);
void z80_translate_init(void);
void z80_cpu_list(FILE *f, fprintf_function cpu_fprintf);
Z80CPU *cpu_z80_create(const char *cpu_model, DeviceState *icc_bridge,
                       Error **errp);
int cpu_z80_exec(CPUZ80State *s);


#define Z80_CPU_Z80  1
#define Z80_CPU_R800 2


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

#define cpu_list z80_cpu_list
/* TODO: cpu_mmu_index() */

///* MMU modes definitions */
//// No modes to define - unlike x86 there is no kernel/usr/SMAP distinction
//static inline int cpu_mmu_index (CPUZ80State *env)
//{
//    return 0;	/* verbatim from target-lm32 */
//}

#include "exec/cpu-all.h"

static inline bool cpu_has_work(CPUState *cs)
{
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK set; Zilog's Z80 also has NMI type interrupts [not
     * implemented]
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
}

#include "exec/exec-all.h"

static inline void cpu_get_tb_cpu_state(CPUZ80State *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
#if 1   /* As implemented at repo.or.cz target-z80/cpu.h */
    *pc= env->pc;
    *cs_base= 0;
    *flags= env->hflags;     /* Z80: no env->eflags */
#else   /* as implemented for target-i386 */
    *cs_base = env->segs[R_CS].base;
    *pc = *cs_base + env->eip;
    *flags = env->hflags |
        (env->eflags & (IOPL_MASK | TF_MASK | RF_MASK | VM_MASK | AC_MASK));
#endif
}

/* op_helper.c */
void QEMU_NORETURN raise_exception(CPUZ80State *env, int exception_index);
void QEMU_NORETURN raise_exception_err(CPUZ80State *env, int exception_index,
                                       int error_code);
void QEMU_NORETURN raise_interrupt(CPUZ80State *env, int intno, int is_int, int error_code,
                     int next_eip_addend);

#endif /* !defined (CPU_Z80_H) */
