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

int singlestep;

static void usage(int exitcode)
{
    printf("Usage: qemu-" TARGET_ARCH " [options] program\n");
    exit(exitcode);
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
#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - trap in cpu_exec() will exit() below...\n", __func__);
#endif

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
        printf("BAILING - abnormal return %d from cpu_z80_exec() - dump of env at %p follows\n", trapnr, env);
        cpu_dump_state(env, stderr, fprintf, 0);
        exit(1);

        /* PARTIAL:
         * For a system that can generate interrupts and signals
         * them here, process_pending_signals() is required here,
         * and we should restart the loop. Abormal exit drops here
         */
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
    int optind;
  int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 1/3...\n", __func__);
#endif
    /* PARTIAL: at this point:
     * 1. may need to call qemu_cache_utils_init(envp);
     * 2. log filename is set to default
     * 3. may need to initialise environment list
     * 4. consider support for cpu model change in arguments
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

#if 1	/* No tcg_exec_init() in v0.15.0+ */
	/* cpu_exec_init_all() ensures we can perform tb_alloc() later */
	cpu_exec_init_all(0);
#else	/* v1.0 */
	tcg_exec_init(0);
	cpu_exec_init_all();
#endif

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

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 3/3...\n", __func__);
#endif
    /* PARTIAL: next...
     *  1. manage passing arg{c|v} to target, if required
     *  2. allocate/initialise any TaskState
     */

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
        printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
        printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(1);
    }

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
