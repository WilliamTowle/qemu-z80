/*
 * QEMU BBLBRX USERMODE - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * ...
 */

#include <stdio.h>
#include <errno.h>

#include "qemu.h"

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

;fprintf(stderr, "%s(): PARTIAL - proceeding with RAM containing a minimal program\n", __func__);
    return 0;   /* "success" */
}
