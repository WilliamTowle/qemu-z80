/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
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
