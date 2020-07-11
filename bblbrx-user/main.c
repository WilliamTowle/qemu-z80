/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */


#include "qemu/osdep.h"
#include "qemu.h"
#include "cpu.h"

#include "qemu/error-report.h"
#include "qemu/help_option.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user main: " fmt , ## __VA_ARGS__); } while(0)


const char *cpu_model;
unsigned long guest_base;
unsigned long reserved_va;

__thread CPUState *thread_cpu;
int singlestep;


bool qemu_cpu_is_self(CPUState *cpu)
{   /* [QEmu v2] called by generic_handle_interrupt() */
    return thread_cpu == cpu;
}

void qemu_cpu_kick(CPUState *cpu)
{
    cpu_exit(cpu);
}


static void usage(int exitcode)
{
    /* NB: platforms may pass program arguments */
    printf("Usage: qemu-" TARGET_NAME " [options] program\n");
    exit(exitcode);
}

static void handle_arg_cpu(char *arg)
{
    cpu_model= arg;

    if (cpu_model == NULL || is_help_option(cpu_model)) {
#if defined(cpu_list)
        cpu_list();
#endif
        exit(EXIT_FAILURE);
    }
}

static int parse_args(int argc, char **argv)
{
    int         optind;
    const char  *r;

    optind= 1;
    for (;;) {
        if (optind >= argc) {
            break;
        }

        r= argv[optind];
        if (r[0] != '-') {
            break;
        }
        optind++;

        if (r[1] == '-') r++;   /* have '-X' and '--X' equivalent */
        if (strcmp(r, "-help") == 0)
        {
            usage(EXIT_SUCCESS);
        }
        else if (strcmp(r, "-cpu") == 0)
        {
            handle_arg_cpu(argv[optind++]);
        }
        /* TODO: support "singlestep" option */
        else
        {
            fprintf(stderr, "Unexpected option '%s'\n", &r[1]);
            usage(EXIT_FAILURE);
        }
    }

    return optind;
}

int main(int argc, char **argv)
{
    const char *cpu_type;
    CPUArchState *env;
    CPUState *cpu;
    char *filename;
    void  *target_ram;
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

    /* PARTIAL: prior to argument parsing, v5 has:
     * 1. error_init() is called, and the TRACE subsystem set up
     * 2. qemu_init_cpu_list() is called, and QOM subsystem set up
     * 3. target environment and stack get set up, if required
     * 4. cpu_model is defaulted to NULL
     * 5. trace options and plugins are managed
     */

    //error_init(argv[0]);
    //module_call_init(MODULE_INIT_TRACE);
    qemu_init_cpu_list();
    module_call_init(MODULE_INIT_QOM);

    cpu_model= NULL;

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    if (cpu_model == NULL) {
        cpu_model= "z80";       /* TODO: respect "cpu" option here */
    }

    cpu_type= parse_cpu_option(cpu_model);

    /* init tcg before creating CPUs and to get qemu_host_page_size */
    //tcg_exec_init(0);

    cpu= cpu_create(cpu_type);
    env= cpu->env_ptr;
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: CPU created at %p [env %p]; reset() to follow\n", cpu, env);
#endif
    cpu_reset(cpu);

#if 1   /* WmT - TRACE */
;DPRINTF("%s(): INFO - CPU reset OK; initial state dump follows...\n", __func__);
;cpu_dump_state(cpu, stderr, 0);
#endif

    /* Following cpu_reset(), we:
     *  1. initialise 'thread_cpu'
     *  2. set 'do_strace', if supported
     *  3. initialisation of argc/argv and environment [if required]
     *  4. initialise any TaskState
     */
    thread_cpu= cpu;


    /* Since we have no MMU, the entirety of target RAM is
     * effectively available to programs at all times without
     * protection (ie. not just where we load any code).
     * Setting guest_base ensures that disas_insn()'s byte fetch
     * doesn't segfault
     */
    target_ram= mmap(0, 64*1024,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (target_ram == MAP_FAILED)
    {
        perror("MAP_FAILED");
        exit(-1);
    }

    guest_base= (unsigned long)target_ram;

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(EXIT_FAILURE);
    }

#if 1   /* WmT - PARTIAL */
;return fprintf(stderr, "%s(): INCOMPLETE - need cpu_loop() to execute %s\n", __func__, filename);
#else
    /* NB: cpu_loop() exits on ILLOP/KERNEL_TRAP */

  return EXIT_SUCCESS;
#endif
}
