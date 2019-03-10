/*
 * QEMU BBLBRX USERMODE - barebones layer for binary execution
 * vim: ft=c sw=4 ts=4 et :
 *
 * ...
 */

#ifndef QEMU_H
#define QEMU_H

//#include "config.h"     /* for arch config - normally via cpu.h */
#include "cpu.h"

#include "qemu-types.h"

struct bblbrx_binprm {
    const char		*filename;
    int			fd;
    long		filesize;
};

#define VERIFY_READ 0
#define VERIFY_WRITE 1 /* implies read access */

/* XXX: todo protect every memory access */
#define lock_user(x,a,b,c)	(void*)(x)
#define unlock_user(x,y,z)

int bblbrx_exec(const char *filename);

int load_raw_binary(struct bblbrx_binprm *bprm);

void cpu_loop(CPUState *env);

void mmap_lock(void);
void mmap_unlock(void);

void cpu_list_lock(void);
void cpu_list_unlock(void);

#endif /* QEMU_H */
