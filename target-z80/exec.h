/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */

#ifndef __TARGET_Z80_EXEC_H_
#define __TARGET_Z80_EXEC_H_

/* FIXME. This header is obsolete, and exists to catch commits
 * that modify it
 */

//#define A0 (env->a0)

#define A   (env->regs[R_A])
#define F   (env->regs[R_F])
#define BC  (env->regs[R_BC])
#define DE  (env->regs[R_DE])
#define HL  (env->regs[R_HL])
#define IX  (env->regs[R_IX])
#define IY  (env->regs[R_IY])
#define SP  (env->regs[R_SP])
#define I   (env->regs[R_I])
#define R   (env->regs[R_R])
#define AX  (env->regs[R_AX])
#define FX  (env->regs[R_FX])
#define BCX (env->regs[R_BCX])
#define DEX (env->regs[R_DEX])
#define HLX (env->regs[R_HLX])

//#define PC  (env->pc)

/* op_helper.c */
void raise_interrupt(int intno, int is_int, int error_code,
                     int next_eip_addend);
void raise_exception(int exception_index);

#endif	/* __TARGET_Z80_EXEC_H_ */
