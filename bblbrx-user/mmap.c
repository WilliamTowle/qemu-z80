/*
 * QEmu "bblbrx" usermode - mmap support
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2019-2022 William Towle <william_towle@yahoo.co.uk>
 *  [...under GPL...]
 */

#include "qemu.h"
#include "qemu-common.h"
//#include "bsd-mman.h"

#if defined(CONFIG_USE_NPTL)
/* Threadsafe implementation needs pthread lock/unlock calls */
#error "Needs thread-safe mmap_{lock|unlock}()"
#else
/* We aren't threadsafe to start with, so no need to worry about locking.  */
void mmap_lock(void)
{
}

void mmap_unlock(void)
{
}
#endif
