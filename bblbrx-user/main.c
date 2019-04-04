/* bblbrx - barebones layer for binary execution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "qemu.h"
#include "cpu.h"

#if defined(CONFIG_USE_GUEST_BASE)
unsigned long guest_base;
#endif

static void usage(int exitcode)
{
    /* PARTIAL: not yet accepting arguments to the program */
    printf(//"usage: qemu-" TARGET_ARCH " [options] program [arguments...]\n"
            "usage: qemu-" TARGET_ARCH " program\n"
            );
    exit(exitcode);
}

static int parse_args(int argc, char **argv)
{
  int   optind;
  const char    *r;

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

        /* TODO: give help. process options properly here */
        /* '--help' should pass EXIT_SUCCESS; all other calls should:
         * 1. do `(void) fprintf(stderr, ...)`
         * 2. call usage() or exit() with EXIT_FAILURE
         */
        usage(EXIT_SUCCESS);
    }

    return optind;
}

#if defined(TARGET_Z80)
void cpu_loop(CPUZ80State *env)
{
	/* PARTIAL:
	 * Temporary indication we're doing something
	 */
#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): PARTIAL - empty infinite loop (missing cpu_exec() call) follows\n", __func__);
#endif

	for (;;) {
            /* PARTIAL: linux-user main.c has a 'trapnr' to store
             * cpu_TYPE_exec*()'s result. We then interpret its value
             * Normally:
             * - cpu.h has cpu_TYPE_exec()'s prototype
             * - cpu.h define "renames" cpu_exec() [in cpu-exec.c]
             * - cpu_exec() has cpu-specific #ifdef sections
             * - internally it fetches/executes "translation blocks"
             * For new code:
             *	exec.c logic calls internal tb_gen_code()
             *	tb_gen_code() calls cpu_gen_code() [translate-all.c]
             *	cpu_gen_code() calls tcg_gen_code() [tcg/tcg.c]
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
;fprintf(stderr, "%s(): PARTIAL - missing initialisation 1/3...\n", __func__);
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
        cpu_model = "z80";
#else
#error unsupported target CPU
#endif
    }

#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): PARTIAL - missing initialisation 2/3...\n", __func__);
#endif
	/* PARTIAL:
	 * prior to CPU init, we do:
	 *	1. tcg_exec_init(0);
	 *	2. call to cpu_exec_init_all(0);
	 */

	env = cpu_init(cpu_model);
	if (!env) {
		fprintf(stderr, "Unable to find CPU definition\n");
		exit(1);
	}

	/* cpu_reset() can contain a tlb_flush() for some platforms. Based
	 * on linux-user/main.c, side effects apply on I386/SPARC/PPC
	 */
	cpu_reset(env);
#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): INFO - CPU has reset; initial state follows...\n", __func__);
;cpu_dump_state(env, stderr, fprintf, 0);
#endif

	/* Following cpu_reset():
	 *	1. initialise 'thread_env' (externed via qemu.h)
	 *	2. set 'do_strace', if supported
	 *	3. initialise 'target_environ'
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
;fprintf(stderr, "%s(): INFO - set guest_base=%p\n", __func__, (void *)guest_base);
#endif
#endif

#if 1	/* WmT - TRACE */
;fprintf(stderr, "%s(): PARTIAL - missing initialisation 3/3...\n", __func__);
#endif
        /* PARTIAL: next...
         *	1. manage passing arg{c|v} to target, if required
         *	2. allocate/initialise any TaskState
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
;fprintf(stderr, "%s(): PARTIAL - run filename=%s via local cpu_loop() (%d bit '%s' CPU, env %p)\n", __func__, filename, TARGET_LONG_BITS, cpu_model, env);
#endif
    cpu_loop(env); /* NB: doesn't exit to our 'return' statement below */
  return 0;
}
