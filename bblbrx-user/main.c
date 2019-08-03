/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */


#include "qemu/osdep.h"
#include "qemu.h"

#include "qemu/error-report.h"

//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 1
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user main: " fmt , ## __VA_ARGS__); } while(0)


unsigned long guest_base;


static void usage(int exitcode)
{
    /* NB: platforms may pass program arguments */
    printf("Usage: qemu-" TARGET_NAME " [options] program\n");
    exit(exitcode);
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
        /* TODO: support "cpu" option */
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

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    /* PARTIAL:
     * to effect CPU init, following parse_cpu_option() we:
     *  1. call tcg_exec_init()
     *  2. initialise 'cpu' from cpu_create() result
     *  3. calls cpu_reset()
     *  4. initialise 'thread_cpu'
     *  5. handle CONFIG_USE_GUEST_BASE/target_ram and TCG init
     *  6. manage passing arg{c|v} to target, if required
     *  7. allocate/initialise any TaskState
     */

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

    /* PARTIAL: next...
     * 1. manage passing arg{c|v} to target, if required
     * 2. initialise any TaskState
     */

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
