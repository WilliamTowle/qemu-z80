/*
 * QEmu "bblbrx" usermode - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */

#ifndef QEMU_H
#define QEMU_H

#include "config.h"     /* for arch config - normally via cpu.h */

int bblbrx_exec(const char *filename);

#endif /* QEMU_H */
