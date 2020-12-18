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
#include "exec/cpu_ldst.h"
#include "exec/translator.h"
#include "tcg/tcg-op.h"

#ifdef CONFIG_USER_ONLY
#include "qemu.h"       /* bblbrx wants TaskState struct */
#endif


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 translate: " fmt , ## __VA_ARGS__); } while(0)


#define EMIT_INSNS 1
#define zprintf(fmt, ...) \
    do { if (EMIT_INSNS) printf(fmt , ## __VA_ARGS__); } while(0)


/* Prefixes and parsing mode defines */

#define PREFIX_CB   0x01
#define PREFIX_DD   0x02
#define PREFIX_ED   0x04
#define PREFIX_FD   0x08

#define MODE_NORMAL 0
#define MODE_DD     1
#define MODE_FD     2


/* global register indexes */
//static TCGv cpu_A0;
#if 0   /* overkill? feature unused for z80 */
static TCGv_i32 cpu_cc_op;
#endif
/* local temps */
static TCGv cpu_T[3];           /* n=2, n=3 unused? */


#define MEM_INDEX 0     /* MMU_USER_IDX? */

typedef struct DisasContext {
    DisasContextBase base;
    /* [WmT] repo.or.cz does not implement:
     *  dc->cc_{op|op_dirty}
     *  dc->tf ("TF cpu flag") [but HF_INHIBIT_IRQ is present]
     */

    /* current insn context */
    int                 prefix;
    target_ulong        pc;
    //int model;

    /* current block context */
#if 0   /* overkill? feature unused for z80 */
    CCOp cc_op;  /* current CC operation */
    bool cc_op_dirty;
#endif
    uint32_t        flags; /* all execution flags */
#ifdef CONFIG_USER_ONLY
    target_ulong    magic_ramloc;
#endif
} DisasContext;


enum {
    /* 8-bit registers */
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_HLmem,
    OR_A,

    OR_IXh,
    OR_IXl,

    OR_IYh,
    OR_IYl,

    OR_IXmem,
    OR_IYmem
};

static const char *regnames[]= {
    [OR_B]     = "b",
    [OR_C]     = "c",
    [OR_D]     = "d",
    [OR_E]     = "e",
    [OR_H]     = "h",
    [OR_L]     = "l",
    [OR_HLmem] = "(hl)",
    [OR_A]     = "a",

    [OR_IXh]   = "ixh",
    [OR_IXl]   = "ixl",

    [OR_IYh]   = "iyh",
    [OR_IYl]   = "iyl",

    [OR_IXmem] = "(ix+d)",
    [OR_IYmem] = "(iy+d)",
};

static const char *const idxnames[]= {
    [OR_IXmem] = "ix",
    [OR_IYmem] = "iy",
};


/* Register accessor functions */

#if defined(HOST_WORDS_BIGENDIAN)
#define UNIT_OFFSET(type, field, units, num) (sizeof_field(type, field) - ((num + 1) * units))
#else
#define UNIT_OFFSET(type, field, units, num) (num * units)
#endif

#define BYTE_OFFSET(type, field, num) UNIT_OFFSET(type, field, 1, num)
#define WORD_OFFSET(type, field, num) UNIT_OFFSET(type, field, 2, num)


#define REGPAIR SP
#include "genreg_template.h"
#undef REGPAIR


#if 0   /* overkill: feature unused for z80 */
static void gen_update_cc_op(DisasContext *s)
{
    if (s->cc_op_dirty) {
        tcg_gen_movi_i32(cpu_cc_op, s->cc_op);
        s->cc_op_dirty = false;
    }
}
#endif


static inline void gen_popw(TCGv v)
{
    TCGv addr = tcg_temp_new();
    gen_movw_v_SP(addr);
    tcg_gen_qemu_ld16u(v, addr, MEM_INDEX);
    tcg_gen_addi_i32(addr, addr, 2);
    tcg_gen_ext16u_i32(addr, addr);
    gen_movw_SP_v(addr);
    tcg_temp_free(addr);
}


static inline void gen_jmp_im(target_ulong pc)
{
    gen_helper_movl_pc_im(cpu_env, tcg_const_i32(pc));
}


static const int reg[8] = {
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_HLmem,
    OR_A
};


static void gen_eob(DisasContext *s)
{
    if (s->base.tb->flags & HF_INHIBIT_IRQ_MASK) {
        gen_helper_reset_inhibit_irq(cpu_env);
    }
    if (s->base.singlestep_enabled) {
        gen_helper_debug(cpu_env);
    } else {
        tcg_gen_exit_tb(0);
    }
    s->base.is_jmp = DISAS_NORETURN;
}


static void gen_exception(DisasContext *s, int trapno, target_ulong cur_pc)
{
#if 0   /* overkill: feature unused for z80 */
    gen_update_cc_op(s);
#endif
    gen_jmp_im(cur_pc);
    gen_helper_raise_exception(cpu_env, tcg_const_i32(trapno));
    s->base.is_jmp = DISAS_NORETURN;
}

/* Called by gen_unknown_opcode() only. For Z80, we don't have the
 * concept of modes in which instructions are disallowed
 */
static void gen_illegal_opcode(DisasContext *s)
{
#if 1   /* WmT - TRACE */
//;DPRINTF("DEBUG: %s() deferring to gen_exception(); passing pc_start 0x%04x less cs_base 0x%04x\n", __func__, s->pc_start, s->cs_base);
;DPRINTF("DEBUG: %s() deferring to gen_exception(); passing s->base.pc_next 0x%04x\n", __func__, s->base.pc_next);
#endif
    //gen_exception(s, EXCP_ILLOP, s->pc_start - s->cs_base);
    gen_exception(s, EXCP_ILLOP, s->base.pc_next);
}

/* Signal a missing opcode or an unimplemented feature. For I386
 * there is also a concept of "bogus instruction stream", whereas
 * Z80 simulates a 'nop'.
 */
static void gen_unknown_opcode(CPUZ80State *env, DisasContext *s)
{
    gen_illegal_opcode(s);

    if (qemu_loglevel_mask(LOG_UNIMP)) {
        target_ulong pc = s->base.pc_next, end = s->pc;
        qemu_log_lock();
        qemu_log("ILLOPC: " TARGET_FMT_lx ":", pc);
        for (; pc < end; ++pc) {
            qemu_log(" %02x", cpu_ldub_code(env, pc));
        }
        qemu_log("\n");
        qemu_log_unlock();
    }
}


static target_ulong advance_pc(CPUZ80State *env, DisasContext *s, int num_bytes)
{
    target_ulong pc = s->pc;

    /* Z80 program counter adjustment needs to be manually wrapped at
     * RAMTOP=64KiB due to cpu-defs.h needing TARGET_LONG_BITS >= 32
     */
    s->pc += num_bytes;
    s->pc&= 0xffff;

    return pc;
}

/* Helpers for physical memory read (TODO: ld{sb|uw} also needed) */

static inline uint8_t z80_ldub_code(CPUZ80State *env, DisasContext *s)
{
    return cpu_ldub_code(env, advance_pc(env, s, 1));
}


/* Convert one instruction and return the next PC value */
static target_ulong disas_insn(DisasContext *s, CPUState *cpu)
{
    /* PARTIAL. For testing purposes, we implement:
     * 1. Reading of a byte (at least one ensures TB has non-zero size)
     * 2. Triggering of ILLOP exception for missing translation cases
     * 3. Unprefixed and prefixed instruction parse/CPU differentiation
     */
    CPUZ80State *env = cpu->env_ptr;
    unsigned int b;     /* instruction byte */
    int             prefixes, m;
    target_ulong    pc_start = s->base.pc_next;

    //s->pc_start = s->pc = pc_start;
    s->pc = pc_start;
    prefixes= 0;

    zprintf("PC = %04x: ", s->pc);
    /* TODO: later prefix handling will jump here to keep track
     * of prefixes seen in state
//next_byte:
     */
    s->prefix= prefixes;
    if (prefixes & PREFIX_DD) {
        m = MODE_DD;
    } else if (prefixes & PREFIX_FD) {
        m = MODE_FD;
    } else {
        m = MODE_NORMAL;
    }

    if ((prefixes & (PREFIX_CB | PREFIX_ED)) == 0) {
        /* DD/FD/plain insns without CB/ED */
        unsigned int x, y, z, p, q;
        int n, d;           /* immediate 'n', displacement 'd' */
        int r1;             /* register number */

        b= z80_ldub_code(env, s);
        //s->pc++;

        x = (b >> 6) & 0x03;    /* isolate bits 7, 6 */
        y = (b >> 3) & 0x07;    /* isolate bits 5, 4, 3 */
        z = b & 0x07;           /* isolate bits 2, 1, 0 */
        p = y >> 1;
        q = y & 0x01;

#if 1   /* WmT - TRACE */
;DPRINTF("INFO: Byte read at pc_start 0x%04x got value 0x%02x - has x %o, y %o [p=%o/q=%o], z %o\n", pc_start, b, x, y,p,q, z);
#endif

        switch (x)
        {
        case 0:      /* insn pattern 00yyyzzz */
            switch (z) {
            /* TODO: case(s) for z=0 to z=5 */

            case 6: /* 8-bit load immediate */
                r1 = regmap(reg[y], m);
                if (is_indexed(r1)) {
                    d = ldsb_code(s->pc);
                    s->pc++;
                }
                n = ldub_code(s->pc);
                s->pc++;
                tcg_gen_movi_tl(cpu_T[0], n);
                if (is_indexed(r1)) {
                    gen_movb_idx_v(r1, cpu_T[0], d);
                } else {
                    gen_movb_reg_v(r1, cpu_T[0]);
                }
                if (is_indexed(r1)) {
                    zprintf("ld (%s%c$%02x),$%02x\n", idxnames[r1], shexb(d), n);
                } else {
                    zprintf("ld %s,$%02x\n", regnames[r1], n);
                }
                break;

            /* TODO: case for z=7 */

            default:    /* PARTIAL: switch(z) incomplete */
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - MODE_%s op 0x%02x (x %o, y %o [p=%o/q=%o], z %o) unhandled z case\n", __FILE__, __LINE__, (m == MODE_NORMAL)?"NORMAL":"xD", b, x, y,p,q, z);
                goto unknown_op;
#endif
            }
            break;

        /* PARTIAL: missing cases for
         * - x=1 - insn pattern 01yyyzzz case
         * - x=2 - insn pattern 10yyyzzz case
         */

        case 3: /* insn pattern 11yyyzzz */
            switch (z) {
            /* TODO: z=0 case covers conditional return */

            case 1: /* POP and various ops */
                switch (q)
                {
                /* TODO: case for q=0 */

                case 1:
                    switch (p)
                    {
                    case 0: /* 0xc9 */
                        gen_popw(cpu_T[0]);
                        gen_helper_jmp_T0(cpu_env);
                        zprintf("ret\n");
                        gen_eob(s);
                        s->base.is_jmp = DISAS_NORETURN;
//                      s->is_ei = 1;
                        break;

                    /* TODO: case(s) for p=1 to p=3 */

                    default:    /* PARTIAL: switch(p) incomplete */
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - MODE_%s op 0x%02x (x %o, y %o [p=%o/q=%o], z %o) read - unhandled p case\n", __FILE__, __LINE__, (m == MODE_NORMAL)?"NORMAL":"xD", b, x, y,p,q, z);
#endif
                        goto unknown_op;
                    }
                    break;

                default:    /* PARTIAL: switch(q) incomplete */
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - MODE_%s op 0x%02x (x %o, y %o [p=%o/q=%o], z %o) read - unhandled q case\n", __FILE__, __LINE__, (m == MODE_NORMAL)?"NORMAL":"xD", b, x, y,p,q, z);
#endif
                    goto unknown_op;
                }
                break;

            /* TODO: case(s) for z=2 to z=7 */

            default:    /* PARTIAL: switch(z) incomplete */
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - MODE_%s op 0x%02x (x %o, y %o [p=%o/q=%o], z %o) read - unhandled z case\n", __FILE__, __LINE__, (m == MODE_NORMAL)?"NORMAL":"xD", b, x, y,p,q, z);
#endif
                goto unknown_op;
            }
            break;

        default:    /* PARTIAL: switch(x) incomplete */
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - MODE_%s op 0x%02x (x %o, y %o [p=%o/q=%o], z %o) read - unhandled x case\n", __FILE__, __LINE__, (m == MODE_NORMAL)?"NORMAL":"xD", b, x, y,p,q, z);
#endif
            goto unknown_op;
        }   /* switch(x) ends */
    }
    else /* TODO: differentiate "cb mode" and "ed mode" cases */
    {
#if 1   /* WmT - TRACE */
;DPRINTF("[%s:%d] FALLTHROUGH - CB- or ED-prefixed opcode unhandled [prefixes=0x%x, mode=%x]\n", __FILE__, __LINE__, prefixes, m);
#endif
        goto unknown_op;
    }

    /* For Z80, there are no "illegal" instructions to signal here.
     * After parsing either we have acted on a fetch; we saw a 'nop'
     * and should do nothing; or we should otherwise simulate a 'nop'
     * [by not acting further]. In the last case, the fetch is allowed
     * to have side effects on internal state/interrupt configuration
     */
    prefixes= 0;

#if 1   /* WmT - TRACE */
;DPRINTF("** EXIT %s() - OK - retval from s->pc 0x%04x **\n", __func__, s->pc);
#endif
    return s->pc;

 unknown_op:    /* "bad insn" case (Z80: normally unreachable) */
    gen_unknown_opcode(env, s);
#if 1   /* WmT - TRACE */
;DPRINTF("** EXIT %s() - unknown opcode 0x%02x seen - next s->pc 0x%04x **\n", __func__, b, s->pc);
#endif
    return s->pc;
}


void tcg_z80_init(void)
{
    /* CPU initialisation for i386 has:
     * - cpu_regs[] as a static TCGv[] above, initialised here
     * - "local temps" cpu_{T0|T1|A0} are set up at disas context init
     */

#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() tcg_global_mem_new_i32() for cpu_T[0]...\n", __func__);
#endif
    cpu_T[0] = tcg_global_mem_new_i32(cpu_env, offsetof(CPUZ80State, t0), "T0");
    /* PARTIAL: also create cpu_T[1], cpu_A0 */
}


static int z80_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cpu,
                                      int max_insns)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
//    CPUX86State *env = cpu->env_ptr;
#ifdef CONFIG_USER_ONLY
    TaskState       *ts = cpu->opaque;
    target_ulong    magic= ts->bprm->magic_ramloc;
#endif
#if 1   /* WmT - PARTIAL */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
#endif
/* TODO: v2 sets:
    ...dc->mem_index to 0, or cpu_mmu_index() result [CONFIG_SOFTMMU]
    ...dc->flags
    ...dc->jmp_opt, based on singlestepping configuration
    ...initialisation of relevant 'static TCGv's
 */
    uint32_t flags = dc->base.tb->flags;
//    target_ulong cs_base = dc->base.tb->cs_base;
//
//    dc->pe = (flags >> HF_PE_SHIFT) & 1;
//    dc->code32 = (flags >> HF_CS32_SHIFT) & 1;
//    dc->ss32 = (flags >> HF_SS32_SHIFT) & 1;
//    dc->addseg = (flags >> HF_ADDSEG_SHIFT) & 1;
//    dc->f_st = 0;
//    dc->vm86 = (flags >> VM_SHIFT) & 1;
//    dc->cpl = (flags >> HF_CPL_SHIFT) & 3;
//    dc->iopl = (flags >> IOPL_SHIFT) & 3;
//    dc->tf = (flags >> TF_SHIFT) & 1;
//    dc->cc_op = CC_OP_DYNAMIC;
//    dc->cc_op_dirty = false;
//    dc->cs_base = cs_base;
//    dc->popl_esp_hack = 0;
//    /* select memory access functions */
//    dc->mem_index = 0;
//#ifdef CONFIG_SOFTMMU
//    dc->mem_index = cpu_mmu_index(env, false);
//#endif
//    dc->cpuid_features = env->features[FEAT_1_EDX];
//    dc->cpuid_ext_features = env->features[FEAT_1_ECX];
//    dc->cpuid_ext2_features = env->features[FEAT_8000_0001_EDX];
//    dc->cpuid_ext3_features = env->features[FEAT_8000_0001_ECX];
//    dc->cpuid_7_0_ebx_features = env->features[FEAT_7_0_EBX];
//    dc->cpuid_xsave_features = env->features[FEAT_XSAVE];
//#ifdef TARGET_X86_64
//    dc->lma = (flags >> HF_LMA_SHIFT) & 1;
//    dc->code64 = (flags >> HF_CS64_SHIFT) & 1;
//#endif

    /* [QEmu v2] z80-softmmu initialises with non-zero flags (due
     * to obsolete HF_SOFTMMU_MASK use?)
     */
    dc->flags = flags;

//    dc->jmp_opt = !(dc->tf || dc->base.singlestep_enabled ||
//                    (flags & HF_INHIBIT_IRQ_MASK));
//    /* Do not optimize repz jumps at all in icount mode, because
//       rep movsS instructions are execured with different paths
//       in !repz_opt and repz_opt modes. The first one was used
//       always except single step mode. And this setting
//       disables jumps optimization and control paths become
//       equivalent in run and single step modes.
//       Now there will be no jump optimization for repz in
//       record/replay modes and there will always be an
//       additional step for ecx=0 when icount is enabled.
//     */
//    dc->repz_opt = !dc->jmp_opt && !(tb_cflags(dc->base.tb) & CF_USE_ICOUNT);
//#if 0
//    /* check addseg logic */
//    if (!dc->addseg && (dc->vm86 || !dc->pe || !dc->code32))
//        printf("ERROR addseg\n");
//#endif
//
//    cpu_T0 = tcg_temp_new();
//    cpu_T1 = tcg_temp_new();
//    cpu_A0 = tcg_temp_new();
//
//    cpu_tmp0 = tcg_temp_new();
//    cpu_tmp1_i64 = tcg_temp_new_i64();
//    cpu_tmp2_i32 = tcg_temp_new_i32();
//    cpu_tmp3_i32 = tcg_temp_new_i32();
//    cpu_tmp4 = tcg_temp_new();
//    cpu_ptr0 = tcg_temp_new_ptr();
//    cpu_ptr1 = tcg_temp_new_ptr();
//    cpu_cc_srcT = tcg_temp_local_new();

#ifdef CONFIG_USER_ONLY
    dc->magic_ramloc= magic;
#endif

    return max_insns;
}

static void z80_tr_tb_start(DisasContextBase *db, CPUState *cpu)
{
#if 0   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    /* nothing to do */
#endif
}

static void z80_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
#if 0   /* WmT - TRACE */
;DPRINTF("INFO: Reached %s() ** PARTIAL **\n", __func__);
;exit(1);
#else
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    //tcg_gen_insn_start(dc->base.pc_next, dc->cc_op);
    tcg_gen_insn_start(dc->base.pc_next);
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

static bool z80_pre_translate_insn(DisasContext *dc)
{
#ifdef CONFIG_USER_ONLY
    if (dc->base.pc_next == dc->magic_ramloc)
    {
        //gen_exception(dc, EXCP_KERNEL_TRAP, dc->pc_start - dc->cs_base);
        gen_exception(dc, EXCP_KERNEL_TRAP, dc->base.pc_next);
        dc->base.is_jmp = DISAS_NORETURN;
        return true; /* "handled" */
    }
#endif

    return false;
}

static void z80_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    target_ulong pc_next;

    if (z80_pre_translate_insn(dc)) {
        return;
    }

    pc_next = disas_insn(dc, cpu);

#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() PARTIAL - proceeding with pc_next 0x%04x from disas_insn()\n", __func__, pc_next);
#endif
    if (/* dc->tf || */ (dc->base.tb->flags & HF_INHIBIT_IRQ_MASK)) {
        /* if single step mode, we generate only one instruction and
           generate an exception */
        /* if irq were inhibited with HF_INHIBIT_IRQ_MASK, we clear
           the flag and abort the translation to give the irqs a
           chance to happen */
        dc->base.is_jmp = DISAS_TOO_MANY;
    } else if ((tb_cflags(dc->base.tb) & CF_USE_ICOUNT)
               && ((dc->base.pc_next & TARGET_PAGE_MASK)
                   != ((dc->base.pc_next + TARGET_MAX_INSN_SIZE - 1)
                       & TARGET_PAGE_MASK)
                   || (dc->base.pc_next & ~TARGET_PAGE_MASK) == 0)) {
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
}

static void z80_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    if (dc->base.is_jmp == DISAS_TOO_MANY) {
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() handling DISAS_TOO_MANY - using pc_next 0x%04x for gen_jmp_im()\n", __func__, dc->base.pc_next);
#endif
        //gen_jmp_im(dc->base.pc_next - dc->cs_base);
        gen_jmp_im(dc->base.pc_next);
        gen_eob(dc);
    }
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
void gen_intermediate_code(CPUState *cpu, TranslationBlock *tb)
{
    DisasContext dc;

    translator_loop(&z80_tr_ops, &dc.base, cpu, tb);
}


void restore_state_to_opc(CPUZ80State *env, TranslationBlock *tb,
                            target_ulong *data)
{
    //int cc_op = data[1];  /* unused for Z80 */
    env->pc = data[0];
}
