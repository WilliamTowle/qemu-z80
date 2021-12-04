/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPLv2+...]
 */

#ifndef __TARGET_Z80_EXEC_H_
#define __TARGET_Z80_EXEC_H_

/* FIXME. This header is obsolete, and exists to catch commits
 * that modify it
 */


/* op_helper.c */
void raise_interrupt(int intno, int is_int, int error_code,
                     int next_eip_addend);
void raise_exception(int exception_index);

#if !defined(CONFIG_USER_ONLY)

#include "softmmu_exec.h"

#endif /* !defined(CONFIG_USER_ONLY) */

#endif	/* __TARGET_Z80_EXEC_H_ */
