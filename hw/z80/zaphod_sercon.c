/* "Zaphod" Z80 machine family for QEmu
 *
 * QEmu serial console handlers
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#include "zaphod.h"         /* for config */
#include "qemu/error-report.h"

#include "sysemu/char.h"


#ifdef ZAPHOD_DEBUG
#define DPRINTF(fmt, ...) \
    do { error_printf( "zaphod: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


static
int zaphod_sercon_can_receive(void *opaque)
{
    /* Maybe implement a FIFO queue? */
    return 1;
}

static
void zaphod_sercon_receive(void *opaque, const uint8_t *buf, int len)
{
    ZaphodSerConState *zss= (ZaphodSerConState *)opaque;

    zaphod_set_inkey(zss->super, buf[0], true);
}

void zaphod_sercon_putchar(ZaphodSerConState *zss, const unsigned char ch)
{
    if (unlikely (!zss->sercon))
    {
        return;
    }

    /* TODO: standard control codes are handled by the serial
     * console - non-standard ones will need to be treated specially
     */
    zss->sercon->chr_write(zss->sercon, &ch, 1);
}

static uint32_t zaphod_sercon_read(void *opaque, uint32_t addr)
{
    ZaphodSerConState *zss= (ZaphodSerConState *)opaque;
    int		value;

	switch (addr)
	{
	case 0x00:		/* stdin */
		value= zaphod_get_inkey(zss->super, true);
		return value;
	default:
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
		return 0x00;
	}
}

static void zaphod_sercon_write(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodSerConState *zss= (ZaphodSerConState *)opaque;

    switch (addr)
    {
    case 0x01:        /* stdout */
        //zaphod_sercon_putchar(zss, value);
        zaphod_putchar((ZaphodState *)zss->super, value);
        break;
    default:
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
        break;
    }
}


static const MemoryRegionPortio zaphod_sercon_portio[] = {
    { 0x00, 1, 1, .read = zaphod_sercon_read },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_sercon_write, },  /* stdout */
    PORTIO_END_OF_LIST(),
};


ZaphodSerConState *zaphod_new_sercon(ZaphodState *super, CharDriverState* sercon)
{
    ZaphodSerConState *zss= g_new(ZaphodSerConState, 1);

    zss->super= super;

    if ((zss->sercon= sercon) != NULL)
    {
        zss->ports = g_new(PortioList, 1);

        portio_list_init(zss->ports, OBJECT(zss), zaphod_sercon_portio, zss, "zaphod.sercon");
        portio_list_add(zss->ports, get_system_io(), 0x00);

        qemu_chr_add_handlers(zss->sercon,
                zaphod_sercon_can_receive, zaphod_sercon_receive,
                NULL, zss);
    }

    return zss;
}
