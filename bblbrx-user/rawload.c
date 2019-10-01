/* bblbrx - barebones layer for binary execution */

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

    return 0;	/* "success" */
}
