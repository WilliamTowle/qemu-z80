/* "Zaphod" Z80 machine family for QEmu */
/* Wm. Towle c. 2013-2020 */

#ifndef _ZAPHOD_H_
#define _ZAPHOD_H_

#include "qemu-common.h"

#include "hw/irq.h"

#define ZAPHOD_DEBUG	1	/* WARNING: uses fprintf() */

/* Feature configuration */
#define ZAPHOD_HAS_BIOS
#define ZAPHOD_HAS_IOPORTS
#define ZAPHOD_HAS_MACHINESPEC		/* support -M "zaphodpb" */
#define ZAPHOD_HAS_SERIALIO
#define ZAPHOD_HAS_KEYBIO

/* Support for specific configurations (Phil Brown, Grant Searle...) */

/* Feature-specific defines */
#ifdef ZAPHOD_HAS_SERIALIO
#define ZAPHOD_HAS_RXINT_IRQ		/* for Grant Searle machine */
#endif

typedef struct {
	char		cs_inkey;
#ifdef ZAPHOD_HAS_RXINT_IRQ
	qemu_irq		*rxint_irq;
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
} ZaphodConsoleState;

typedef struct {
	CPUState		*cpu;
	ram_addr_t		ram_handle;
#ifdef ZAPHOD_HAS_RXINT_IRQ
	qemu_irq		*irqs;
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
	ZaphodConsoleState	*console;
} ZaphodState;


#ifdef ZAPHOD_HAS_SERIALIO
void zaphod_serio_putchar(const unsigned char ch);
void zaphod_serial_init(ZaphodConsoleState *zcs);
#endif	/* ZAPHOD_HAS_SERIALIO */

void *zaphod_console_init(qemu_irq *irq);
#endif	/*  _ZAPHOD_H_  */
