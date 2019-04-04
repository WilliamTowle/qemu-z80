/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle, under GPL...]
 */

#include <stdio.h>
#include <errno.h>

#include "qemu.h"


#if 1	/* debug */
	/* TODO: version with error_printf() needs CPU headers */
#define DPRINTF(fmt, ...) \
	do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
	do { } while (0)
#endif


int load_raw_binary(struct bblbrx_binprm *bprm)
{
    abi_ulong	host_start, host_size;
    ssize_t	read;

    host_start= 0x0000;
    host_size= bprm->filesize;

    read= pread(bprm->fd, g2h(host_start), host_size, host_start);
    if (read != host_size)
    {
        fprintf(stderr, "%s: %s\n", bprm->filename, strerror(errno));
        exit(-1);
    }

#if 1	/* TRACE */
;DPRINTF("%s(): PARTIAL - proceeding with naive cpu_loop() implementation\n", __func__);
#endif
    return 0;   /* "success" */
}
