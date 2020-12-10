/*
 * QEMU Z80 CPU
 * Register templates for Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 */

/* 16-bit loads */

static inline void glue(gen_movw_v_, REGPAIR)(TCGv v)
{
    tcg_gen_ld16u_tl(v, cpu_env,
                     offsetof(CPUZ80State, regs[glue(R_,REGPAIR)]) +
                            WORD_OFFSET(CPUZ80State, regs[0], 0));
}

/* 16-bit stores */

static inline void glue(glue(gen_movw_,REGPAIR),_v)(TCGv v)
{
    tcg_gen_st16_tl(v, cpu_env,
                    offsetof(CPUZ80State, regs[glue(R_,REGPAIR)]) +
                            WORD_OFFSET(CPUZ80State, regs[0], 0));
}
