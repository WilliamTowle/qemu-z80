/*
 * QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Porting by William Towle 2018-2023
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
#include "qemu/qemu-print.h"
#ifndef CONFIG_USER_ONLY
#include "sysemu/reset.h"
#endif


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 cpu: " fmt , ## __VA_ARGS__); } while(0)


/* list registered CPU models */

static void z80_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    const char *typename= object_class_get_name(oc);
    char *suffix= strstr(typename, "-" TYPE_Z80_CPU);
    char *name;

    assert(suffix != NULL);
    name = g_strndup(typename, suffix - typename);
    qemu_printf("  %s\n", name);
    g_free(name);
}

void z80_cpu_list(void)
{
    GSList *list;
    qemu_printf("Available CPUs:\n");

    list = object_class_get_list(TYPE_Z80_CPU, false);
    //list = object_class_get_list_sorted(TYPE_Z80_CPU, false);
    g_slist_foreach(list, z80_cpu_list_entry, NULL);
    g_slist_free(list);
}

static void zilog_z80_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    Z80CPU *cpu = Z80_CPU(obj);
    CPUZ80State *env = &cpu->env;

    cs->env_ptr = env;

    env->model = Z80_CPU_Z80;
}

static void mitsui_r800_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    Z80CPU *cpu = Z80_CPU(obj);
    CPUZ80State *env = &cpu->env;

    cs->env_ptr = env;

    env->model = Z80_CPU_R800;
}


/* CPU class instance init.
 * See also separate {zilog_z80|mitsui_r800}_cpu_initfn
 */
static void z80_cpu_initfn(Object *obj)
{
    Z80CPU *cpu = Z80_CPU(obj);

    cpu_set_cpustate_pointers(cpu);
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

static void z80_cpu_unrealizefn(DeviceState *dev)
{
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(dev);

#ifndef CONFIG_USER_ONLY
    cpu_remove_sync(CPU(dev));
    qemu_unregister_reset(z80_cpu_machine_reset_cb, dev);
#endif

    zcc->parent_unrealize(dev);
}

static void z80_cpu_reset(DeviceState *dev)
{
    Z80CPU *cpu = Z80_CPU(CPU(dev));
    Z80CPUClass *zcc = Z80_CPU_GET_CLASS(cpu);
    CPUZ80State *env = &cpu->env;

    zcc->parent_reset(dev);

    memset(env, 0, offsetof(CPUZ80State, end_reset_fields));

    /* init to reset state */
    env->pc= 0x0000;
    env->iff1= 0;
    env->iff2= 0;
    env->imode= 0;
    env->regs[R_A]= 0xff;
    env->regs[R_F]= 0xff;
    env->regs[R_SP]= 0xffff;

    /* QEmu v2+: no initial hidden flags required */
    env->hflags= 0;
}


/* Return type name for a given CPU model name
 * Caller is responsible for freeing the returned string.
 */
static char *z80_cpu_type_name(const char *model_name)
{
    /* This name must correspond to a non-abstract TypeInfo struct
     * entry (below). Direct reference to TYPE_Z80_CPU suits an
     * implementation that doesn't distinguish between Zilog's Z80
     * and ASCII-Mitsui's R800
     */
    //return g_strdup_printf(TYPE_Z80_CPU);
    return g_strdup_printf(Z80_CPU_TYPE_NAME("%s"), model_name);
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
    /* For i386, INTERRUPT_HARD is only flagged if eflags has
     * IF_MASK unset or interrupts are not inhibited if it is.
     * Zilog's Z80 also has NMI type interrupts [not implemented]
     */
#if QEMU_VERSION_MAJOR < 5
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
#else
    return z80_cpu_pending_interrupt(cs, cs->interrupt_request) != 0;
#endif
}


static void z80_cpu_set_pc(CPUState *cs, vaddr value)
{
    Z80CPU *cpu = Z80_CPU(cs);

    cpu->env.pc = value;
}

static void z80_disas_set_info(CPUState *cs, disassemble_info *info)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    info->mach = (env->model == Z80_CPU_Z80)? bfd_mach_z80_z80 : bfd_mach_z80_r800;
    info->print_insn = print_insn_z80;
}

static void z80_cpu_synchronize_from_tb(CPUState *cs, TranslationBlock *tb)
{
    Z80CPU *cpu = Z80_CPU(cs);
    CPUZ80State *env = &cpu->env;

    env->pc= tb->pc;
}

int z80_cpu_pending_interrupt(CPUState *cs, int interrupt_request)
{
    /* [QEmu v5] return the interrupt designator (or zero if none
     * pending) so that it can be queried.
     * TODO: test/return CPU_INTERRUPT_NMI?
     */
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
}

static void z80_cpu_class_init(ObjectClass *oc, void *data)
{
    Z80CPUClass *zcc = Z80_CPU_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);

    /* target-i386 common class init */
    device_class_set_parent_realize(dc, z80_cpu_realizefn,
                                    &zcc->parent_realize);
    device_class_set_parent_unrealize(dc, z80_cpu_unrealizefn,
                                    &zcc->parent_unrealize);

    device_class_set_parent_reset(dc, z80_cpu_reset, &zcc->parent_reset);
    cc->reset_dump_flags = 0;   /* i386: CPU_DUMP_FPU | CPU_DUMP_CCOP */

    cc->class_by_name = z80_cpu_class_by_name;

    cc->has_work = z80_cpu_has_work;
#ifdef CONFIG_TCG
    cc->do_interrupt = z80_cpu_do_interrupt;
    cc->cpu_exec_interrupt = z80_cpu_exec_interrupt;
#endif
    cc->dump_state = z80_cpu_dump_state;
    cc->set_pc = z80_cpu_set_pc;
    cc->synchronize_from_tb = z80_cpu_synchronize_from_tb;
//    cc->gdb_read_register = z80_cpu_gdb_read_register;
//    cc->gdb_write_register = z80_cpu_gdb_write_register;
//    cc->gdb_num_core_regs = 35;       /* NUMBER_OF_CPU_REGISTERS? */
//    cc->gdb_core_xml_file = "z80-cpu.xml";
#if !defined(CONFIG_USER_ONLY)
    cc->get_phys_page_debug = z80_cpu_get_phys_page_debug;
//    cc->get_crash_info = z80_cpu_get_crash_info;
#endif
//    cc->vmsd = &vmstate_z80_cpu;
    /* TODO? i386 adjusts CPU eflags with:
     * cc->cpu_exec_enter
     * cc->cpu_exec_exit
     */
#ifdef CONFIG_TCG
    cc->tcg_initialize = tcg_z80_init;
    cc->tlb_fill = z80_cpu_tlb_fill;
#endif
    cc->disas_set_info = z80_disas_set_info;

    /* NB. user_creatable relates to instantiation in the monitor */
    //dc->user_creatable = true;
}


/* CPU support: Z80_CPU_TYPE permits us to declare types for
 * - Zilog Z80 CPU
 * - Mitsui R800 CPU
 * - disas_set_info() must account for each CPU type
 */
#define DEFINE_Z80_CPU_TYPE(cpu_model, initfn) \
    { \
        .parent = TYPE_Z80_CPU, \
        .name = Z80_CPU_TYPE_NAME(cpu_model), \
        .instance_init = initfn, \
    }

static const TypeInfo z80_cpus_type_info[] = {
    {
        /* NB: default to regular Zilog Z80 for now [TODO: use 'name' of
         * TYPE_Z80_CPU for the base class and Z80_CPU_TYPE_NAME(x) for
         * the Z80/R800 subclasses and in z80_cpu_type_name() likewise]
         */
        .name = TYPE_Z80_CPU,
        .parent = TYPE_CPU,
        .instance_size = sizeof(Z80CPU),
        .instance_init = z80_cpu_initfn,
        .abstract = true,
        .class_size = sizeof(Z80CPUClass),
        .class_init = z80_cpu_class_init,
    },
    DEFINE_Z80_CPU_TYPE("z80", zilog_z80_cpu_initfn), \
    DEFINE_Z80_CPU_TYPE("r800", mitsui_r800_cpu_initfn) \
};

DEFINE_TYPES(z80_cpus_type_info)
