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
#include "exec/cpu_ldst.h"


#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("bblbrx-user rawload: " fmt , ## __VA_ARGS__); } while(0)


int load_raw_binary(struct bblbrx_binprm *bprm)
{
    abi_ulong   code_start, code_size;
    ssize_t     read;

    code_start= 0x0000;
    code_size= bprm->filesize;

    read= pread(bprm->fd, g2h(code_start), code_size, code_start);
    if (read != code_size)
    {
        fprintf(stderr, "%s: %s\n", bprm->filename, strerror(errno));
        exit(-ENOEXEC);
    }

#if 1   /* WmT - TRACE */
;DPRINTF("%s(): pread() of %zd bytes successful\n", __func__, read);
#endif
    return 0;   /* "success" */
}
