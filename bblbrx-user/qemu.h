/*
 * QEMU BBLBRX USERMODE - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * ...
 */

#ifndef QEMU_H
#define QEMU_H

#include "cpu.h"

#include "qemu-types.h"

struct bblbrx_binprm {
    const char		*filename;
    int			fd;
    long		filesize;
};

int bblbrx_exec(const char *filename);

int load_raw_binary(struct bblbrx_binprm *bprm);

void cpu_loop(CPUState *env);

#endif /* QEMU_H */
