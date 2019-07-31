/*
 * QEMU BBLBRX USERMODE - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * ...
 */

#ifndef QEMU_H
#define QEMU_H

#include "config.h"     /* for arch config - normally via cpu.h */

int bblbrx_exec(const char *filename);

#endif /* QEMU_H */
