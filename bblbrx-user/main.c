/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qemu.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


static void usage(int exitcode)
{
    printf("Usage: qemu-" TARGET_ARCH " [options] program\n");

    exit(exitcode);
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
    char *filename;
    int optind;
  int ret;

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 1/2...\n", __func__);
#endif
    /* PARTIAL: at this point:
     * 1. may need to call qemu_cache_utils_init(envp);
     * 2. log filename is set to default
     * 3. may need to initialise environment list
     * 4. consider support for cpu model change in arguments
     * 5. want to manage cpu_model default value if not in args
     */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

#if 1	/* WmT - TRACE */
;DPRINTF("%s(): PARTIAL - missing initialisation 2/2...\n", __func__);
#endif
    /* PARTIAL:
     * to effect CPU init, we do:
     *    1. call to cpu_exec_init_all(0);
     *    2. call env = cpu_init(cpu_model);
     *    3. maybe a cpu_reset(env) call (TARGET_{I386|SPARC|PPC})
     *    4. initialise 'thread_env' (externed via qemu.h)
     *    5. set 'do_strace', if supported
     *    6. initialise 'target_environ'
     *    7. handle CONFIG_USE_GUEST_BASE
     *    8. manage passing arg{c|v} to target, if required
     *    9. allocate/initialise any TaskState
     */

    ret= bblbrx_exec(filename);
    if (ret != 0) {
        if (ret > 0)
        printf("%s(): BAILING - unexpected bblbrx_exec() retval %d while loading %s\n", __func__, ret, filename);
        else
        printf("Error while loading %s: %s\n", filename, strerror(-ret));
        exit(1);
    }

#if 1	/* WmT - TODO */
;return fprintf(stderr, "%s(): INCOMPLETE - call cpu_loop() to run %s\n", __func__, filename);
#else
    /* PARTIAL: needs to pass 'env' to cpu_loop() here.
     * NB: cpu_loop() doesn't exit to our 'return' statement below
     */

  return 0;
#endif
}
