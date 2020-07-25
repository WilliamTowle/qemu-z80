/* "Zaphod" Z80 machine family for QEmu */
/* Wm. Towle c. 2013-2020 */

#include "hw/zaphod.h"

#ifdef ZAPHOD_HAS_SERIALIO
#include "qemu-char.h"	/* qemu_chr_add_handlers() */
#include "sysemu.h"	/* serial_hds[] */

static
int zaphod_serio_can_receive(void *opaque)
{
	/* We can always store in zcs->inkey :) */
	/* Maybe implement a FIFO queue? */
	return 1;
}

static
void zaphod_serio_receive(void *opaque, const uint8_t *buf, int len)
{
  ZaphodConsoleState	*zcs= (ZaphodConsoleState *)opaque;
	zcs->cs_inkey= buf[0];

#ifdef ZAPHOD_HAS_RXINT_IRQ
	if (zcs->rxint_irq)
		qemu_irq_raise(*zcs->rxint_irq);
#endif	/* ZAPHOD_HAS_RXINT_IRQ */
}

void zaphod_serio_putchar(const unsigned char ch)
{
	qemu_chr_write(serial_hds[0], &ch, 1);
}

void zaphod_serial_init(ZaphodConsoleState *zcs)
{
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif
	/* support read and write via "serial0" console */
	qemu_chr_add_handlers(serial_hds[0],
			zaphod_serio_can_receive, zaphod_serio_receive,
			NULL, zcs);
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}
#endif	/* ZAPHOD_HAS_SERIALIO */
