/*
 * Minimal QEmu Z80 CPU - instruction translation
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */


#include "qemu/osdep.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/exec-all.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 translate: " fmt , ## __VA_ARGS__); } while(0)


/* TODO: struct DisasContext, translation ops/disas_insn() */


void tcg_z80_init(void)
{
    /* CPU initialisation for i386 has:
     * - cpu_regs[] as a static TCGv[] above, initialised here
     * - "local temps" cpu_{T0|T1|A0} are set up at disas context init
     */
}

/* generate intermediate code for basic block 'tb'.  */
void gen_intermediate_code(CPUState *cs, TranslationBlock *tb)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("Z80 translate.c is incomplete (%s() PARTIAL)\n", __func__);
;exit(1);
#else
    /* TODO:
     * Prior to QEmu v2, we align to legacy target-i386 and call
     * disas_insn() from here (with attendant opcode buffer management
     * logic). QEmu v2+ gives us TranslatorOps and translator_loop()
     */

#endif
}

void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb,
                            target_ulong *data)
{
    //int cc_op = data[1];  /* unused for Z80 */
    env->pc = data[0];
}
