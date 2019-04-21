/*
 * QEmu Z80 CPU - helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 */

#include "cpu.h"

//void z80_cpu_dump_state(CPUState *cs, FILE *f, int flags)
void z80_cpu_dump_state(CPUState *cs, FILE *f, fprintf_function cpu_fprintf,
                        int flags)
{
    Z80CPU *cpu= Z80_CPU(cs);
    CPUZ80State *env= &cpu->env;
    int fl = env->regs[R_F];

#if 0	/* PARTIAL: Full register set not yet implemented */
    cpu_fprintf(f, "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
                   "AF'=%04x BC'=%04x DE'=%04x HL'=%04x IY=%04x\n"
                   "PC =%04x SP =%04x F=[%c%c%c%c%c%c%c%c]\n"
                   "IM=? IFF1=? IFF2=? I=?? R=??\n",
                   (env->regs[R_A] << 8) | env->regs[R_F],
                   env->regs[R_BC],
                   env->regs[R_DE],
                   env->regs[R_HL],
                   env->regs[R_IX],
                   (env->regs[R_AX] << 8) | env->regs[R_FX],
                   env->regs[R_BCX],
                   env->regs[R_DEX],
                   env->regs[R_HLX],
                   env->regs[R_IY],
		   env->pc,
                   env->regs[R_SP],
                   fl & 0x80 ? 'S' : '-',
                   fl & 0x40 ? 'Z' : '-',
                   fl & 0x20 ? 'Y' : '-',
                   fl & 0x10 ? 'H' : '-',
                   fl & 0x08 ? 'X' : '-',
                   fl & 0x04 ? 'P' : '-',
                   fl & 0x02 ? 'N' : '-',
                   fl & 0x01 ? 'C' : '-'
		    );
#else	/* final repo.or.cz implementation */
    //int fl = env->regs[R_F];
    cpu_fprintf(f, "AF =%04x BC =%04x DE =%04x HL =%04x IX=%04x\n"
                   "AF'=%04x BC'=%04x DE'=%04x HL'=%04x IY=%04x\n"
                   "PC =%04x SP =%04x F=[%c%c%c%c%c%c%c%c]\n"
                   "IM=%i IFF1=%i IFF2=%i I=%02x R=%02x\n",
                   (env->regs[R_A] << 8) | env->regs[R_F],
                   env->regs[R_BC],
                   env->regs[R_DE],
                   env->regs[R_HL],
                   env->regs[R_IX],
                   (env->regs[R_AX] << 8) | env->regs[R_FX],
                   env->regs[R_BCX],
                   env->regs[R_DEX],
                   env->regs[R_HLX],
                   env->regs[R_IY],
                   env->pc, /* pc == -1 ? env->pc : pc, */
                   env->regs[R_SP],
                   fl & 0x80 ? 'S' : '-',
                   fl & 0x40 ? 'Z' : '-',
                   fl & 0x20 ? 'Y' : '-',
                   fl & 0x10 ? 'H' : '-',
                   fl & 0x08 ? 'X' : '-',
                   fl & 0x04 ? 'P' : '-',
                   fl & 0x02 ? 'N' : '-',
                   fl & 0x01 ? 'C' : '-',
                   env->imode, env->iff1, env->iff2, env->regs[R_I], env->regs[R_R]);
#endif
}


/* return value:
   -1 = cannot handle fault
   0  = nothing more to do
   1  = generate pf fault
   2  = soft mmu activation required for this block
*/
#if 0	/* legacy prototype */
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write1, int mmu_idx, int is_softmmu)
#else
int cpu_z80_handle_mmu_fault(CPUZ80State *env, target_ulong addr,
                             int is_write1, int mmu_idx)
#endif
{
#if 1	/* temp'y */
	printf("%s() skeleton ... INCOMPLETE\n", __func__);
	exit(1);
#else
	/* TODO: see g106f733 "Add target-z80 tree, z80-dis.c" */
#endif
}