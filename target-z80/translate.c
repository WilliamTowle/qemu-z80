/*
 * Minimal QEmu Z80 CPU - instruction translation
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 */

#include "cpu.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif

///* generate intermediate code in gen_opc_buf and gen_opparam_buf for
//   basic block 'tb'. If search_pc is TRUE, also generate PC
//   information for each intermediate instruction. */
static inline void gen_intermediate_code_internal(Z80CPU *cpu,
                                                 TranslationBlock *tb,
                                                 int search_pc)
{
;DPRINTF("BAIL %s() - INCOMPLETE\n", __func__);
;exit(1);
}

void gen_intermediate_code(CPUZ80State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(z80_env_get_cpu(env), tb, false);
}

void gen_intermediate_code_pc(CPUZ80State *env, TranslationBlock *tb)
{
    gen_intermediate_code_internal(z80_env_get_cpu(env), tb, true);
}

void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb, int pc_pos)
{
#if 1
;DPRINTF("%s() UNIMPLEMENTED\n", __func__);
;exit(1);
#else   /* Suitable for X86 */
    int cc_op;
#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_OP)) {
        int i;
        qemu_log("RESTORE:\n");
        for(i = 0;i <= pc_pos; i++) {
            if (tcg_ctx.gen_opc_instr_start[i]) {
                qemu_log("0x%04x: " TARGET_FMT_lx "\n", i,
                        tcg_ctx.gen_opc_pc[i]);
            }
        }
        qemu_log("pc_pos=0x%x eip=" TARGET_FMT_lx " cs_base=%x\n",
                pc_pos, tcg_ctx.gen_opc_pc[pc_pos] - tb->cs_base,
                (uint32_t)tb->cs_base);
    }
#endif
    env->eip = tcg_ctx.gen_opc_pc[pc_pos] - tb->cs_base;
    cc_op = gen_opc_cc_op[pc_pos];
    if (cc_op != CC_OP_DYNAMIC)
        env->cc_op = cc_op;
#endif
}
