/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */


#include <stdlib.h>
#include <string.h>

#include "cpu.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


static int cpu_z80_find_by_name(const char *name);

CPUZ80State *cpu_z80_init(const char *model)
{
    CPUZ80State *env;
    int id;

    id = cpu_z80_find_by_name(model);
    if (id == 0) {
        return NULL;
    }

    env= calloc(1, sizeof *env);

#if defined(TARGET_Z80)
;printf("%s(): PARTIAL - model/flags init missing...\n", __func__);
#endif
    /* PARTIAL: cpu_z80_init() continues (requiring enhanced
     * CPUZ80State?) with:
     * 1. cpu_exec_init() for the 'env'
     * 2. z80_translate_init() call, if not already done
     * 3. store id in env->model
     */

    cpu_reset(env);		/* target-i386: in wrapper functions */

    /* PARTIAL:
     * continue with qemu_init_vcpu() call here
     * (...which is a NOP if CONFIG_USER_ONLY defined)
     */

    return env;
}

/* Legacy cpu_*_find_by_name()
 * This function returns 0 to signify "not found"; it uses the
 * Z80_CPU_* constants in cpu.h for success values. In target-i386
 * for QEmu v1+ an index into an array is returned, with -1 on error
 */
static int cpu_z80_find_by_name(const char *name)
{
    /* PARTIAL: qemu-z80 iterates around z80_cpu_names[] list,
     * containing "z80", "r800", an end-of-list sentinel, and
     * the Z80_CPU_* constants used as return values
     */
    if (strcmp(name, "z80") == 0)
    {
        return 1;	/* value of Z80_CPU_Z80 */
    }

  return 0;
}

void cpu_reset(CPUZ80State *env)
{
#if defined(TARGET_Z80)
;printf("%s(): PARTIAL - missing breakpoint init, TLB reset...\n", __func__);
#endif

    /* PARTIAL: target-z80 version starts with:
     * 1. record of function call in logs
     * 2. memset() for breakpoints
     * 3. a tlb_flush() call:
     *	tlb_flush(env, 1);
     */

    /* init to reset state */

#ifdef CONFIG_SOFTMMU
    env->hflags |= HF_SOFTMMU_MASK;
#endif

    env->pc = 0x0000;
    env->iff1 = 0;
    env->iff2 = 0;
    env->imode = 0;
    env->regs[R_A] = 0xff;
    env->regs[R_F] = 0xff;
    env->regs[R_SP] = 0xffff;
}

void cpu_dump_state(CPUState *env, FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...),
                    int flags)
{
    int fl = env->regs[R_F];

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
}
