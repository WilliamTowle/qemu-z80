/*
 * Minimal QEmu Z80 CPU - opcode helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */


#include "qemu/osdep.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/helper-proto.h"
#include "exec.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 op_helper: " fmt , ## __VA_ARGS__); } while(0)


/* NB. set_inhibit_irq() not invoked
 * repo.or.cz doesn't make it clear what should have called this (but
 * does this explain FIXMEs against 'retn'/'reti' insns?)
 * In target-i386 for QEmu v1, calls occurred:
 * - on pop of ES/SS/DS
 * - on 'mov seg, Gv'
 * - on 'sti'
 */
//void HELPER(set_inhibit_irq)(CPUZ80State *env)
//{
//    env->hflags |= HF_INHIBIT_IRQ_MASK;
//}

void HELPER(reset_inhibit_irq)(CPUZ80State *env)
{
    env->hflags &= ~HF_INHIBIT_IRQ_MASK;
}


void helper_movl_pc_im(CPUZ80State *env, int new_pc)
{
    PC= (uint16_t)new_pc;
}
