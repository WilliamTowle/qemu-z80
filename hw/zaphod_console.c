/* "Zaphod" Z80 machine family for QEmu */
/* Wm. Towle c. 2013-2018 */

#include "hw/zaphod.h"

#ifdef ZAPHOD_HAS_KEYBIO
#include "console.h"	/* qemu_add_kbd_event_handler() */
#endif	/* ZAPHOD_HAS_KEYBIO */

#ifdef ZAPHOD_HAS_SERIALIO
#include "qemu-char.h"	/* qemu_chr_add_handlers() */
#include "sysemu.h"	/* serial_hds[] */
#endif

#ifdef ZAPHOD_HAS_KEYBIO
static const unsigned char keycode_to_asciilc[128]= {
	/* keymap for UK QWERTY keyboard */
	/* FIXME: this is (unintentionally) partial, and the handler
	 * (also) lacks code to sense/track/apply modifier keys
	 */
	  0,  0,'1','2','3','4','5','6',
	'7','8','9','0',  0,  0,  0,  0,
	'q','w','e','r','t','y','u','i',
	'o','p',  0,  0, 13,  0,'a','s',
	'd','f','g','h','j','k','l',  0,
	  0,  0,  0,  0,'z','x','c','v',
	'b','n','m',  0,  0,  0,  0,  0,
	  0,' ',  0,  0,  0,  0,  0,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
};

static void zaphod_put_keycode(void *opaque, int keycode)
{
  ZaphodConsoleState	*zcs= (ZaphodConsoleState *)opaque;
  int	release= keycode & 0x80;
	if (release)
	{
		/* TODO: resetting cs_inkey on key release is risky if
		 * systems without an IRQ sent on keypress don't poll
		 * the port promptly enough. Do we want to implement key
		 * repeat (in which case we start a timer elsewhere that
		 * should get stopped here)?
		 */
		zcs->cs_inkey= 0;
	}
	else
	{
	  int	ch= keycode_to_asciilc[keycode & 0x7f];
		zcs->cs_inkey= ch;
#ifdef ZAPHOD_DEBUG
		//fprintf(stderr, "DEBUG: %s() stored keycode %d to inkey as ch=%02x\n", __func__, keycode, ch);
#endif
#ifdef ZAPHOD_HAS_RXINT_IRQ
		if (zcs->rxint_irq)
		    qemu_irq_raise(*zcs->rxint_irq);
#endif
	}
}

static
void zaphod_keyboard_init(ZaphodConsoleState *zcs)
{
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	/* TODO: what happens re keycodes in non-GUI mode? */
	qemu_add_kbd_event_handler(zaphod_put_keycode, zcs);

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
}
#endif	/* ZAPHOD_HAS_KEYBIO */

#ifdef ZAPHOD_HAS_SERIALIO
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

void *zaphod_console_init(qemu_irq *irq)
{
  ZaphodConsoleState *zcs= qemu_mallocz(sizeof(ZaphodConsoleState));
#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Enter %s()\n", __func__);
#endif

	zcs->cs_inkey= 0;

#ifdef ZAPHOD_HAS_CONSOLEGUI
	zcs->cpos_c= 0;
	zcs->cpos_r= 0;
#endif
#ifdef ZAPHOD_HAS_RXINT_IRQ
	zcs->rxint_irq= irq;	/* can be NULL */
#endif	/* ZAPHOD_HAS_RXINT_IRQ */

#ifdef ZAPHOD_HAS_KEYBIO
	/* provide the inkey (ASCII) feed for zaphod_io_read() */
	zaphod_keyboard_init(zcs);
#endif	/* ZAPHOD_HAS_KEYBIO */

#ifdef ZAPHOD_HAS_SERIALIO
	zaphod_serial_init(zcs);
#endif

#ifdef ZAPHOD_HAS_CONSOLEGUI
	zaphod_consolegui_init(zcs);

	zaphod_consolegui_invalidate_display(zcs);
#endif

#ifdef ZAPHOD_DEBUG
	fprintf(stderr, "DEBUG: Exit %s()\n", __func__);
#endif
  return zcs;
}
