/*
 * QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2022
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */


#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "exec/exec-all.h"
#include "sysemu/reset.h"


#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 cpu: " fmt , ## __VA_ARGS__); } while(0)


/* list registered CPU models */

static void z80_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    CPUListState *s = user_data;
    const char *typename= object_class_get_name(oc);
    char *suffix= strstr(typename, "-cpu");
    char *name;

    assert(suffix != NULL);
    name = g_strndup(typename, suffix - typename);
    (*s->cpu_fprintf)(s->file, "  %s\n", name);
    g_free(name);
}

void z80_cpu_list(FILE *f, fprintf_function cpu_fprintf)
{
    CPUListState s = {
        .file = f,
        .cpu_fprintf = cpu_fprintf,
    };
    GSList *list;

    (*cpu_fprintf)(f, "Available CPUs:\n");
    list = object_class_get_list_sorted(TYPE_Z80_CPU, false);
    g_slist_foreach(list, z80_cpu_list_entry, &s);
    g_slist_free(list);
}


static void z80_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    Z80CPU *cpu = Z80_CPU(obj);

    cs->env_ptr = &cpu->env;
}

#ifndef CONFIG_USER_ONLY
/* TODO: remove me, when reset over QOM tree is implemented */
static void z80_cpu_machine_reset_cb(void *opaque)
{
    Z80CPU *cpu = opaque;
    cpu_reset(CPU(cpu));
}
#endif

static void z80_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
#ifndef CONFIG_USER_ONLY
    Z80CPU *cpu = Z80_CPU(dev);
#endif
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

#ifndef CONFIG_USER_ONLY
    qemu_register_reset(z80_cpu_machine_reset_cb, cpu);
#endif

    qemu_init_vcpu(cs);

    cpu_reset(cs);

    zcc->parent_realize(dev, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }
}

static void z80_cpu_unrealizefn(DeviceState *dev, Error **errp)
{
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

#ifndef CONFIG_USER_ONLY
    cpu_remove_sync(CPU(dev));
    qemu_unregister_reset(z80_cpu_machine_reset_cb, dev);
#endif

    zcc->parent_unrealize(dev, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }
}

/* CPUClass::reset() */
static void z80_cpu_reset(CPUState *s)
{
    Z80CPU *cpu = Z80_CPU(s);
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(cpu);
    CPUZ80State *env = &cpu->env;
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif

    zcc->parent_reset(s);

    /* init to reset state */
    memset(env->regs, 0, sizeof(env->regs));
    env->pc= 0x0000;
    env->iff1= 0;
    env->iff2= 0;
    env->imode= 0;
    env->regs[R_A]= 0xff;
    env->regs[R_F]= 0xff;
    env->regs[R_SP]= 0xffff;

    env->hflags= 0;

#ifdef CONFIG_SOFTMMU
    /* TODO: not required? i386 now always just sets HF2_GIF_MASK */
    env->hflags |= HF_SOFTMMU_MASK;
#endif

#if 0   /* untested */
    cpu_breakpoint_remove_all(s, BP_CPU);
    cpu_watchpoint_remove_all(s, BP_CPU);
    //also have class init set cc->debug_excp_handler
#endif
}


/* Return type name for a given CPU model name
 * Caller is responsible for freeing the returned string.
 */
static char *z80_cpu_type_name(const char *model_name)
{
    /* NB: must generate a name corresponding to a non-abstract
     * TypeInfo struct below. Direct reference to TYPE_Z80_CPU
     * suits an implementation that doesn't distinguish between
     * Zilog's Z80 and ASCII Corp's R800
     */
    return g_strdup_printf(TYPE_Z80_CPU);
    //return g_strdup_printf(Z80_CPU_TYPE_NAME("%s"), model_name);
}

static ObjectClass *z80_cpu_class_by_name(const char *cpu_model)
{
#if 1   /* redo like i386 */
    g_autofree char *typename= z80_cpu_type_name(cpu_model);
    return object_class_by_name(typename);
#else
    ObjectClass *oc;

    oc = object_class_by_name(cpu_model);
;DPRINTF("DEBUG: %s() got initial oc=%p for cpu_model '%s'\n", __func__, oc);
    if (object_class_dynamic_cast(oc, TYPE_Z80_CPU) == NULL ||
        object_class_is_abstract(oc)) {
;DPRINTF("DEBUG: Failed cast - reason %s\n",
        (object_class_dynamic_cast(oc, TYPE_Z80_CPU) == NULL)?"cast was NULL":"object_class_is_abstract() problem");
        oc = NULL;
    }
;DPRINTF("DEBUG: %s() returning oc %p\n", __func__, oc);
    return oc;
#endif
}


static bool z80_cpu_has_work(CPUState *cs)
{
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__)
;if (cs->interrupt_request & CPU_INTERRUPT_HARD)
{
;DPRINTF("INFO: CPU_INTERRUPT_HARD is flagged -> BAIL\n");
;exit(1);
}
;return 0;
#else   /* TODO: implement */
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK set; Zilog's Z80 also has NMI type interrupts [not
     * implemented]
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
#endif
}


static void z80_cpu_set_pc(CPUState *cs, vaddr value)
{
    Z80CPU *cpu = Z80_CPU(cs);

    cpu->env.pc = value;
}

static void z80_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    /* TODO: switch to bfd_mach_z80_r800 where applicable */
    info->mach = bfd_mach_z80_z80;
    info->print_insn = print_insn_z80;
}
    /* TODO: also set info.endian to BFD_ENDIAN_BIG (default _LITTLE)? */

static void z80_cpu_synchronize_from_tb(CPUState *cs, TranslationBlock *tb)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    env->pc= tb->pc;
}

static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    Z80CPUClass *zcc = Z80_CPU_CLASS(oc);

#if 1   /* WmT - PARTIAL */
;DPRINTF("DEBUG: Reached %s() ** PARTIAL **\n", __func__);
#endif

    /* v2 common class init */
    device_class_set_parent_realize(dc, z80_cpu_realizefn,
                                    &zcc->parent_realize);
    device_class_set_parent_unrealize(dc, z80_cpu_unrealizefn,
                                    &zcc->parent_unrealize);

    zcc->parent_reset= cc->reset;
    cc->reset= z80_cpu_reset;
    cc->reset_dump_flags = 0;   /* i386: CPU_DUMP_FPU | CPU_DUMP_CCOP */

    cc->class_by_name = z80_cpu_class_by_name;

    cc->has_work = z80_cpu_has_work;
#ifdef CONFIG_TCG
    cc->do_interrupt = z80_cpu_do_interrupt;
    cc->cpu_exec_interrupt = z80_cpu_exec_interrupt;
#endif
    cc->dump_state = z80_cpu_dump_state;
    /* TODO: cc->get_crash_info() useful? or x86-specific? */
    cc->set_pc = z80_cpu_set_pc;
    cc->synchronize_from_tb = z80_cpu_synchronize_from_tb;
//    cc->gdb_read_register = z80_cpu_gdb_read_register;
//    cc->gdb_write_register = z80_cpu_gdb_write_register;
//    cc->gdb_num_core_regs = 35;	/* NUMBER_OF_CPU_REGISTERS? */
//    cc->gdb_core_xml_file = "z80-cpu.xml";
#ifdef CONFIG_USER_ONLY
    /* Without an MMU fault handler, QEmu v2's accel/tcg/user-exec.c
     * asserts. Like lm32, ours just wraps tlb_set_page()
     */
    cc->handle_mmu_fault = z80_cpu_handle_mmu_fault;
#else	/* !defined(CONFIG_USER_ONLY) */
    cc->get_phys_page_debug = z80_cpu_get_phys_page_debug;
#endif
//    cc->vmsd = &vmstate_z80_cpu;
    /* TODO? i386 adjusts CPU eflags with:
     * cc->cpu_exec_enter
     * cc->cpu_exec_exit
     */
#ifdef CONFIG_TCG
    cc->tcg_initialize = tcg_z80_init;
#endif
    cc->disas_set_info = z80_disas_set_info;
}

static const TypeInfo z80_cpu_type_info = {
    /* NB: default to .abstract=false and regular Zilog Z80 for
     * now (TODO: base class definition can use .name=TYPE_Z80_CPU
     * directly)
     */
    .name = TYPE_Z80_CPU,
    //.name = Z80_CPU_TYPE_NAME(TYPE_Z80_CPU),
    .parent = TYPE_CPU,
    .instance_size = sizeof(Z80CPU),
    .instance_init = z80_cpu_initfn,
    //.abstract = true;
    .class_size = sizeof(Z80CPUClass),
    .class_init = z80_cpu_class_init,
};

static void z80_cpu_register_types(void)
{
    type_register_static(&z80_cpu_type_info);
}

type_init(z80_cpu_register_types)
