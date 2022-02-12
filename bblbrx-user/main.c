/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "qemu.h"
#include "cpu.h"
#include "tcg.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


#if defined(CONFIG_USE_GUEST_BASE)
unsigned long guest_base;
#endif

const char *cpu_model= NULL;
int singlestep;

static void usage(int exitcode)
{
    printf("Usage: qemu-" TARGET_ARCH " [options] program\n");
    exit(exitcode);
}

static void handle_arg_cpu(char *arg)
{
    cpu_model= arg;

    if (cpu_model == NULL || strcmp(cpu_model, "?") == 0) {
#if defined(cpu_list_id)
	cpu_list_id(stdout, &fprintf, "");
#elif defined(cpu_list)
	cpu_list(stdout, &fprintf); /* deprecated */
#endif
        exit(1);
    }
}

static void handle_arg_singlestep(void)
{
    singlestep = 1;
}

static int parse_args(int argc, char **argv)
{
    int		optind;
    const char	*r;

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
        else if (strcmp(r, "-singlestep") == 0)
        {
            handle_arg_singlestep();
        }
        else
        {
            fprintf(stderr, "Unexpected option '%s'\n", &r[1]);
            usage(EXIT_FAILURE);
        }
    }

    return optind;
}

void cpu_list_lock(void)
{
/* Nothing to lock without multiple CPUs? */
}

void cpu_list_unlock(void)
{
/* Nothing to lock without multiple CPUs? */
}

#if defined(TARGET_Z80)
void cpu_loop(CPUZ80State *env)
{
  int	trapnr;
    /* PARTIAL:
     * Temporary indication we're doing something
     */

    for (;;)
    {
        /* PARTIAL:
         * Key exception cases are for ILLOP (bad/unsupported
         * instruction) and KERNEL_TRAP ("magic ramtop") cases.
         * The EXCP_KERNEL_TRAP in particular works like a ROM
         * system call, resulting in program exit in the
         * simplest case.
         */
        printf("%s(): Calling cpu_exec() here...\n", __func__);
        //trapnr= cpu_z80_exec(env);
        trapnr= cpu_exec(env);
        switch(trapnr)
        {
        case EXCP_KERNEL_TRAP:
            /* Our "magic ramtop" logic was triggered. In the
             * simple code interpretation code case, we just exit.
             * For a system with ROMs we might need to emulate or
             * call relevant routines.
             */
            printf("Successful program exit. Register dump follows:\n");
            cpu_dump_state(env, stderr, fprintf, 0);
            break;	/* to exit() beyond switch */
        case EXCP06_ILLOP:
            printf("cpu_exec() encountered EXCP06_ILLOP (trapnr=%d) - aborting emulation\n", trapnr);
            break;	/* to exit() beyond switch */
        default:
            printf("BAILING - abnormal return %d from cpu_exec()\n", trapnr);
        }

        /* PARTIAL:
         * For a system that can generate interrupts and signals
         * them here, process_pending_signals() is required here,
         * and we should restart the loop. Abormal exit drops here
         */
	    exit(1);
    }
}
#else	/* non-z80 CPUs */
void cpu_loop(CPUState *env)
{
	printf("cpu_loop() for unimplemented CPU type\n");
	cpu_dump_state(env, stderr, fprintf, 0);
	exit(1);
}
#endif

int main(int argc, char **argv)
{
    const char *cpu_model= NULL;
    char *filename;
    CPUState *env;
  void  *target_ram;
  struct bblbrx_binprm bprm;
    TaskState ts1, *ts= &ts1;
    int optind;
    int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 1/3...\n", __func__);
#endif
    /* PARTIAL - prior to argument parse v1.0.1 has:
     * 1. Setup for QEmu and target's environment/stack
     * 2. cpu_model is set to NULL
     * 3. Call to cpudef_setup() happens, if defined
     * 4. Logging is initialised
     */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    if (cpu_model == NULL)
    {
#if defined(TARGET_Z80)
        cpu_model = "z80";	/* TODO: support specifying "r800"? */
#else
#error unsupported target CPU
#endif
    }

#if 0	/* No tcg_exec_init() in v0.15.0+ */
	/* cpu_exec_init_all() ensures we can perform tb_alloc() later */
	cpu_exec_init_all(0);
#else	/* v1.0+ */
	tcg_exec_init(0);
	cpu_exec_init_all(); 	/* ensures we can perform tb_alloc() later */
#endif
    /* PARTIAL - in v1.0.1 here:
     * 1. CPU register/image info/paths are prepared
     * 2. cpu_model default value is set based on platform
     * 3. Call to cpu_init();
     * 4. Optional call to cpu_reset(env) for I386/SPARC/PPC targets
     * 5. Initialise 'thread_env' (externed via qemu.h)
     * 6. Set 'do_strace', if supported
     * 7. Initialisation of 'target_environ'
     */

    /* initialising the CPU at this stage allows us to get
       qemu_host_page_size */
    env = cpu_init(cpu_model);
    if (!env) {
        fprintf(stderr, "Unable to find definition for cpu_model '%s'\n", cpu_model);
        exit(1);
    }

    /* cpu_reset() can contain a tlb_flush() for some platforms. Based
     * on linux-user/main.c, side effects apply on I386/SPARC/PPC
     */
    cpu_reset(env);
#if 1    /* WmT - TRACE */
;DPRINTF("%s(): INFO - CPU has reset; initial state follows...\n", __func__);
;cpu_dump_state(env, stderr, fprintf, 0);
#endif

    /* Following cpu_reset():
     *	1. initialise 'thread_env' (externed via qemu.h)
     *	2. set 'do_strace', if supported
     *	3. initialise 'target_environ'
     */

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 2/3...\n", __func__);
#endif
    /* PARTIAL: next...
     *  1. initialise 'thread_env' (externed via qemu.h)
     *  2. set 'do_strace', if supported
     *  3. initialise 'target_environ'
     */

#if !defined(CONFIG_USE_GUEST_BASE)
    /* cpu-all.h requires us to define CONFIG_USE_GUEST_BASE if parts of
     * the guest address space are reserved on the host.
     */
#error "CONFIG_USE_GUEST_BASE not defined"
#else
    /* TODO:
     * We have no MMU, and therefore no paging/memory protection.
     * Nevertheless, we *may* want to to use the facilities of the host
     * to ensure any ROM regions can be protected from writes. In
     * the meantime we get to have self-modifying code anywhere.
     */
    target_ram= mmap(0, 64*1024,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (target_ram == MAP_FAILED)
    {
        perror("MAP_FAILED");
        exit(-1);
    }

    /* Giving guest_base a value causes ldub_code() to retrieve bytes
     * from addresses that won't segfault
     */

    guest_base= (unsigned long)target_ram;
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): INFO - set guest_base=%p\n", __func__, (void *)guest_base);
#endif
#endif

    /* PARTIAL. Depending on target system, manage passing arg{c|v} to program */

	ts->used = 1;
	ts->bprm = &bprm;
	env->opaque = ts;

    ret= bblbrx_exec(filename, &bprm);
    if (ret != 0) {
        if (ret > 0)
            printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
            printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(1);
    }

#if defined(CONFIG_USE_GUEST_BASE)
	/* loading the executable above allows us to assume GUEST_BASE
	 * has a fixed value at this point...
	 */
	/* tcg_ctx is an extern in tcg/tcg.h, from translate-all.c.
	 * cpu_gen_init() calls an init function for it
	 */
    tcg_prologue_init(&tcg_ctx);
#endif

#ifdef TARGET_Z80
	/* PARTIAL.
	 * In order to execute raw binaries with no ROMs present, we
	 * designate an address as a "magic ramtop" location and write
	 * a relevant sentinel-type value on the stack.
	 * Regular programs ending in 'ret' will pop this value and
	 * jump there; similarly, Z80 CP/M programs can use this address
	 * for the zero page's "exit program" jump.
	 */
	if (bprm.magic_ramloc)
	{
#if 1  /* WmT - HACK */
;fprintf(stderr, "[%s:%d] magic_ramloc enabled, setting regs and memory...\n", __FILE__, __LINE__);
#endif
            env->regs[R_SP]= bprm.magic_ramloc;
            stw_raw(env->regs[R_SP], bprm.magic_ramloc);
	}
#else
#error unsupported target CPU
#endif

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 3/3...\n", __func__);
#endif
    /* PARTIAL - in v1.0.1 here:
     * 1. before loader_exec(),
     * - guest_base reservation is logged
     * - command line for target is managed
     * - TaskState is initialised
     * 2. prior to cpu_loop() call:
     * - calls to target_set_brk(), {syscall|signal}_init()
     * - call to tcg_prologue_init() is made if using GUEST_BASE
     * - registers/flags/interrupts are set up based on loaded file
     * - stack and heap are set up in the TaskState
     * - GDB stub initialisation happens if port configured
     */

    /* PARTIAL. May now want/need to:
     * free memory for any redundant data structures
     * log information about the program
     * initialise brk and syscall/signal handlers
     * further initialise 'env' (esp. registers)
     * further initialise TaskState
     * manage any GDB stub
     */
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - run filename=%s via local cpu_loop() (%d bit '%s' CPU, env %p)\n", __func__, filename, TARGET_LONG_BITS, cpu_model, env);
#endif
    cpu_loop(env); /* NB: doesn't exit to our 'return' statement below */
  return 0;
}
