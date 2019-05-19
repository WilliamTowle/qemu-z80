/*
 * QEmu Z80 virtual CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...Stuart Brady/William Towle, under GPL...]
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

#endif	/* __TARGET_Z80_EXEC_H_ */
