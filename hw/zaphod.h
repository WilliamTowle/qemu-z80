/* Skeleton Z80 machine for QEmu */
/* Wm. Towle c. 2013-2020 */

#ifndef _ZAPHOD_H_
#define _ZAPHOD_H_

#define ZAPHOD_DEBUG	1	/* WARNING: uses fprintf() */

#include "hw/boards.h"		/* emulator datatypes */


/* "Zaphod" Z80 machine family configuration */

/* ZAPHOD_MAX_RAMTOP:
 * Address space for a Z80 ends at 64K (some emulations might want less)
 */
#define	ZAPHOD_MAX_RAMTOP	(64 * 1024)

/* zaphod.c content */

void pic_info(Monitor *mon);
void irq_info(Monitor *mon);


#endif	/*  _ZAPHOD_H_  */
