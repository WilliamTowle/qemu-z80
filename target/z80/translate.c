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
#include "exec/translator.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 translate: " fmt , ## __VA_ARGS__); } while(0)


typedef struct DisasContext {
    DisasContextBase base;
    /* [WmT] repo.or.cz omits or does not use:
     *  override
     *  prefix
     *  cc_{op|op_dirty}
     *  tf (x86 "TF cpu flag") [but HF_INHIBIT_IRQ is present]
     */
} DisasContext;


void tcg_z80_init(void)
{
    /* CPU initialisation for i386 has:
     * - cpu_regs[] as a static TCGv[] above, initialised here
     * - "TCG local temps" A0, T0, T1 are in struct DisasContext
     */
}

static void z80_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cpu)
{
#if 1   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else   /* unported */
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    CPUX86State *env = cpu->env_ptr;
    uint32_t flags = dc->base.tb->flags;
    target_ulong cs_base = dc->base.tb->cs_base;

    dc->pe = (flags >> HF_PE_SHIFT) & 1;
    dc->code32 = (flags >> HF_CS32_SHIFT) & 1;
    dc->ss32 = (flags >> HF_SS32_SHIFT) & 1;
    dc->addseg = (flags >> HF_ADDSEG_SHIFT) & 1;
    dc->f_st = 0;
    dc->vm86 = (flags >> VM_SHIFT) & 1;
    dc->cpl = (flags >> HF_CPL_SHIFT) & 3;
    dc->iopl = (flags >> IOPL_SHIFT) & 3;
    dc->tf = (flags >> TF_SHIFT) & 1;
    dc->cc_op = CC_OP_DYNAMIC;
    dc->cc_op_dirty = false;
    dc->cs_base = cs_base;
    dc->popl_esp_hack = 0;
    /* select memory access functions */
    dc->mem_index = 0;
#ifdef CONFIG_SOFTMMU
    dc->mem_index = cpu_mmu_index(env, false);
#endif
    dc->cpuid_features = env->features[FEAT_1_EDX];
    dc->cpuid_ext_features = env->features[FEAT_1_ECX];
    dc->cpuid_ext2_features = env->features[FEAT_8000_0001_EDX];
    dc->cpuid_ext3_features = env->features[FEAT_8000_0001_ECX];
    dc->cpuid_7_0_ebx_features = env->features[FEAT_7_0_EBX];
    dc->cpuid_xsave_features = env->features[FEAT_XSAVE];
#ifdef TARGET_X86_64
    dc->lma = (flags >> HF_LMA_SHIFT) & 1;
    dc->code64 = (flags >> HF_CS64_SHIFT) & 1;
#endif
    dc->flags = flags;
    dc->jmp_opt = !(dc->tf || dc->base.singlestep_enabled ||
                    (flags & HF_INHIBIT_IRQ_MASK));
    /* Do not optimize repz jumps at all in icount mode, because
       rep movsS instructions are execured with different paths
       in !repz_opt and repz_opt modes. The first one was used
       always except single step mode. And this setting
       disables jumps optimization and control paths become
       equivalent in run and single step modes.
       Now there will be no jump optimization for repz in
       record/replay modes and there will always be an
       additional step for ecx=0 when icount is enabled.
     */
    dc->repz_opt = !dc->jmp_opt && !(tb_cflags(dc->base.tb) & CF_USE_ICOUNT);
#if 0
    /* check addseg logic */
    if (!dc->addseg && (dc->vm86 || !dc->pe || !dc->code32))
        printf("ERROR addseg\n");
#endif

    dc->T0 = tcg_temp_new();
    dc->T1 = tcg_temp_new();
    dc->A0 = tcg_temp_new();

    dc->tmp0 = tcg_temp_new();
    dc->tmp1_i64 = tcg_temp_new_i64();
    dc->tmp2_i32 = tcg_temp_new_i32();
    dc->tmp3_i32 = tcg_temp_new_i32();
    dc->tmp4 = tcg_temp_new();
    dc->ptr0 = tcg_temp_new_ptr();
    dc->ptr1 = tcg_temp_new_ptr();
    dc->cc_srcT = tcg_temp_local_new();
#endif
}

static void z80_tr_tb_start(DisasContextBase *db, CPUState *cpu)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    /* nothing to do */
#endif
}

static void z80_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(dc->base.pc_next, dc->cc_op);
#endif
}

static bool z80_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                     const CPUBreakpoint *bp)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    /* If RF is set, suppress an internally generated breakpoint.  */
    int flags = dc->base.tb->flags & HF_RF_MASK ? BP_GDB : BP_ANY;
    if (bp->flags & flags) {
        gen_debug(dc, dc->base.pc_next - dc->cs_base);
        dc->base.is_jmp = DISAS_NORETURN;
        /* The address covered by the breakpoint must be included in
           [tb->pc, tb->pc + tb->size) in order to for it to be
           properly cleared -- thus we increment the PC here so that
           the generic logic setting tb->size later does the right thing.  */
        dc->base.pc_next += 1;
        return true;
    } else {
        return false;
    }
#endif
}

static void z80_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else   /* unported */
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    target_ulong pc_next;

    pc_next = disas_insn(dc, cpu);

    if (dc->tf || (dc->base.tb->flags & HF_INHIBIT_IRQ_MASK)) {
        /* if single step mode, we generate only one instruction and
           generate an exception */
        /* if irq were inhibited with HF_INHIBIT_IRQ_MASK, we clear
           the flag and abort the translation to give the irqs a
           chance to happen */
        dc->base.is_jmp = DISAS_TOO_MANY;
    } else if ((tb_cflags(dc->base.tb) & CF_USE_ICOUNT)
               && ((pc_next & TARGET_PAGE_MASK)
                   != ((pc_next + TARGET_MAX_INSN_SIZE - 1)
                       & TARGET_PAGE_MASK)
                   || (pc_next & ~TARGET_PAGE_MASK) == 0)) {
        /* Do not cross the boundary of the pages in icount mode,
           it can cause an exception. Do it only when boundary is
           crossed by the first instruction in the block.
           If current instruction already crossed the bound - it's ok,
           because an exception hasn't stopped this code.
         */
        dc->base.is_jmp = DISAS_TOO_MANY;
    } else if ((pc_next - dc->base.pc_first) >= (TARGET_PAGE_SIZE - 32)) {
        dc->base.is_jmp = DISAS_TOO_MANY;
    }

    dc->base.pc_next = pc_next;
#endif
}

static void z80_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    if (dc->base.is_jmp == DISAS_TOO_MANY) {
        gen_jmp_im(dc->base.pc_next - dc->cs_base);
        gen_eob(dc);
    }
#endif
}

static void z80_tr_disas_log(const DisasContextBase *dcbase,
                              CPUState *cpu)
{
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    qemu_log("IN: %s\n", lookup_symbol(dc->base.pc_first));
    log_target_disas(cpu, dc->base.pc_first, dc->base.tb->size);
#endif
}

static const TranslatorOps z80_tr_ops = {
    .init_disas_context = z80_tr_init_disas_context,
    .tb_start           = z80_tr_tb_start,
    .insn_start         = z80_tr_insn_start,
    .breakpoint_check   = z80_tr_breakpoint_check,
    .translate_insn     = z80_tr_translate_insn,
    .tb_stop            = z80_tr_tb_stop,
    .disas_log          = z80_tr_disas_log,
};

/* generate intermediate code for basic block 'tb'.  */
void gen_intermediate_code(CPUState *cpu, TranslationBlock *tb, int max_insns)
{
    DisasContext dc;

    translator_loop(&z80_tr_ops, &dc.base, cpu, tb, max_insns);
}


void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb,
                            target_ulong *data)
{
    //int cc_op = data[1];  /* unused for Z80 */
    env->pc = data[0];
}
