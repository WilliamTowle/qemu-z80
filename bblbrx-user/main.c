/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void usage(int exitcode)
{
    printf(//"Usage: qemu-" TARGET_ARCH " [options] program [arguments...]\n"
            "usage: bblbrx-main <program>\n"
            );
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

    if (argc <= 1)
        usage(EXIT_SUCCESS);    /* effectively "--help" */

    optind= parse_args(argc, argv);
    if (optind >= argc)
        usage(EXIT_FAILURE);
    filename= argv[optind];

    fprintf(stderr, "%s: Filename '%s' was given...\n", argv[0], filename);
    exit(EXIT_FAILURE);
}
