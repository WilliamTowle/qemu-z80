/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */

#ifndef QEMU_H
#define QEMU_H


struct bblbrx_binprm {
    const char      *filename;
    int             fd;
    long            filesize;
};


/* bblbrx-specific routines */

int load_raw_binary(struct bblbrx_binprm *bprm);
int bblbrx_exec(const char *filename);

#endif /* QEMU_H */
