/*
 * QEmu Zaphod board - IOCore support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */


#include "qemu/osdep.h"
#include "zaphod.h"
#include "zaphod_uart.h"

#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "sysemu/reset.h"
#include "ui/console.h"


//#define EMIT_DEBUG ZAPHOD_DEBUG
#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("zaphod_iocore: " fmt , ## __VA_ARGS__); } while(0)


/* PARTIAL
 * Handles IO ports for 'stdio' and ACIA input, as expected by the
 * "teletype" ROM for the Phil Brown emulator and the BASIC ROM for
 * the Grant Searle board emulation respectively.
 * The IRQ for the MC6850 UART is managed here, and the 'inkey'
 * value shared by UART and corresponding screen is part of the UART
 * code.
 */

/* stdio chardev handlers */

static
int zaphod_iocore_can_receive_stdio(void *opaque)
{
    return zaphod_uart_can_receive(opaque);
}

static
void zaphod_iocore_receive_stdio(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

    zaphod_uart_set_inkey(zis->uart_stdio, buf[0], true);
}


/* ACIA chardev handlers */

static
int zaphod_iocore_can_receive_acia(void *opaque)
{
    return zaphod_uart_can_receive(opaque);
}

static
void zaphod_iocore_receive_acia(void *opaque, const uint8_t *buf, int len)
{
    ZaphodIOCoreState *zis= (ZaphodIOCoreState *)opaque;

#if QEMU_VERSION_MAJOR >= 5 /* possibly `SDL_MAJOR_VERSION >= 2` ? */
    if (buf[0] == '\n')
    {
        /* QEmu's SDL v2 support introduces sdl2_process_key(), which
         * injects '\n' in the input stream for Q_KEY_CODE_RET rather
         * than '\r' as before. Send the latter.
         */
        zaphod_uart_set_inkey(zis->uart_acia, '\r', true);
    }
    else
#endif
    {
        zaphod_uart_set_inkey(zis->uart_acia, buf[0], true);
    }
    if (zis->irq_acia)
        qemu_irq_raise(*zis->irq_acia);
}


/* stdio ioport handlers */

static uint32_t zaphod_iocore_read_stdio(void *opaque, uint32_t addr)
{
    static ZaphodUARTState  *zus= NULL;
    static bool             vc_present= false;
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int                 value;

    if (!vc_present)
    {   /* check a VC exists for input; simulate one otherwise */
        if ( (zus= zis->uart_stdio) )
        {
            vc_present= true;
        }
    }

    switch (addr)
    {
    case 0x00:      /* stdin */
        value= 0x00;
        if (vc_present)
            value= zaphod_uart_get_inkey(zus, true);
        return value;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
#endif
        return 0x00;
    }
}

static
void zaphod_iocore_putchar_stdio(ZaphodIOCoreState *zis, const unsigned char ch)
{
#ifdef CONFIG_ZAPHOD_HAS_UART
    if (zis->uart_stdio)
        zaphod_uart_putchar(zis->uart_stdio, ch);
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
    zaphod_screen_putchar(zis->screen, ch);
#endif
}

static void zaphod_iocore_write_stdio(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x01:      /* stdout */
        zaphod_iocore_putchar_stdio(zis, value & 0xff);
        break;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
#endif
        break;
    }
}

static const MemoryRegionPortio zaphod_iocore_portio_stdio[] = {
    { 0x00, 1, 1, .read = zaphod_iocore_read_stdio },     /* stdin */
    { 0x01, 1, 1, .write = zaphod_iocore_write_stdio, },  /* stdout */
    PORTIO_END_OF_LIST()
};


ZaphodScreenState *zaphod_iocore_get_screen(ZaphodIOCoreState *zis)
{
    if (zis->screen) return zis->screen;

    return zis->screen= ZAPHOD_SCREEN(object_new(TYPE_ZAPHOD_SCREEN));
}

/* ACIA ioport handlers */

static uint32_t zaphod_iocore_read_acia(void *opaque, uint32_t addr)
{
    static ZaphodUARTState  *zus= NULL;
    static bool             vc_present= false;
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(opaque);
    int                 value;

    if (!vc_present)
    {
        if ( (zus= zis->uart_acia) )
        {   /* QEmu has a VC for input */
            vc_present= true;
        }
    }

    switch (addr)
    {
    case 0x80:      /* ACIA: read UART PortStatus */
        if (vc_present)
            return zaphod_uart_portstatus(zus);
        else
            return 0x0f;

    case 0x81:      /* ACIA: read UART RxData */
        value= 0x00;
        if (vc_present)
        {
            value= zaphod_uart_get_inkey(zus, true);
            if (zis->irq_acia)
                qemu_irq_lower(*zis->irq_acia);
        }
        return value? value : 0xff;

    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected read, with port=%d\n", __func__, addr);
#endif
        return 0x00;
    }
}

static
void zaphod_iocore_putchar_acia(ZaphodIOCoreState *zis, const unsigned char ch)
{
#ifdef CONFIG_ZAPHOD_HAS_UART
    if (zis->uart_acia)
        zaphod_uart_putchar(zis->uart_acia, ch);
#endif
#ifdef CONFIG_ZAPHOD_HAS_SCREEN
    zaphod_screen_putchar(zis->screen, ch);
#endif
}

static void zaphod_iocore_write_acia(void *opaque, uint32_t addr, uint32_t value)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    switch (addr)
    {
    case 0x80:      /* ACIA: write UART PortControl */
        /* Received byte controls baud rate, encoding, and which
         * status events trigger the interrupt. The Zaphod machines do
         * not need these to be emulated
         */
//;DPRINTF("DEBUG: %s() write ACIA PortControl (port 0x%02x) is a NOP <- value %d\n", __func__, addr, value);
        break;
    case 0x81:      /* ACIA: write UART TxData */
//;DPRINTF("DEBUG: %s() write ACIA UART TxData (port 0x%02x) -> ch-value=%d\n", __func__, addr, value);
        zaphod_iocore_putchar_acia(zis, value & 0xff);
        break;
    default:
#if 1   /* WmT - TRACE */
;DPRINTF("DEBUG: %s() Unexpected write, port 0x%02x, value %d\n", __func__, addr, value);
#endif
        break;
    }
}

static const MemoryRegionPortio zaphod_iocore_portio_acia[] = {
    /* TODO: 0x80-0x81 apply to Grant Searle BASIC ROM but hardware
     * decodes all of 0x80-0xbf [with even/odd ports equivalent]
     */
    { 0x80, 2, 1,
                .read = zaphod_iocore_read_acia,
                .write = zaphod_iocore_write_acia
                },
    PORTIO_END_OF_LIST()
};


#if 1   /* keyboard I/O */
#if 1
/* Tables for mapping qcode of keypress to corresponding ascii
 * TODO: must account for modifier keys
 */
#define NO_KEY                  0xff

static const uint8_t qcode_to_ascii[]= {
    [0 ... 0xff]                = NO_KEY,

    /* TODO: '`' */
    [Q_KEY_CODE_1]              = '1',
    [Q_KEY_CODE_2]              = '2',
    [Q_KEY_CODE_3]              = '3',
    [Q_KEY_CODE_4]              = '4',
    [Q_KEY_CODE_5]              = '5',
    [Q_KEY_CODE_6]              = '6',
    [Q_KEY_CODE_7]              = '7',
    [Q_KEY_CODE_8]              = '8',
    [Q_KEY_CODE_9]              = '9',
    [Q_KEY_CODE_0]              = '0',
    /* TODO: '-', '=' */
    [Q_KEY_CODE_BACKSPACE]      = 127,

    [Q_KEY_CODE_Q]              = 'q',
    [Q_KEY_CODE_W]              = 'w',
    [Q_KEY_CODE_E]              = 'e',
    [Q_KEY_CODE_R]              = 'r',
    [Q_KEY_CODE_T]              = 't',
    [Q_KEY_CODE_Y]              = 'y',
    [Q_KEY_CODE_U]              = 'u',
    [Q_KEY_CODE_I]              = 'i',
    [Q_KEY_CODE_O]              = 'o',
    [Q_KEY_CODE_P]              = 'p',
    /* TODO: '[', ']' */

    [Q_KEY_CODE_A]              = 'a',
    [Q_KEY_CODE_S]              = 's',
    [Q_KEY_CODE_D]              = 'd',
    [Q_KEY_CODE_F]              = 'f',
    [Q_KEY_CODE_G]              = 'g',
    [Q_KEY_CODE_H]              = 'h',
    [Q_KEY_CODE_J]              = 'j',
    [Q_KEY_CODE_K]              = 'k',
    [Q_KEY_CODE_L]              = 'l',
    /* TODO: ';', '\'', '#' */
    [Q_KEY_CODE_RET]            = '\r',

    [Q_KEY_CODE_Z]              = 'z',
    [Q_KEY_CODE_X]              = 'x',
    [Q_KEY_CODE_C]              = 'c',
    [Q_KEY_CODE_V]              = 'v',
    [Q_KEY_CODE_B]              = 'b',
    [Q_KEY_CODE_N]              = 'n',
    [Q_KEY_CODE_M]              = 'm',
    /* TODO: ',', '.', '/' */

    [Q_KEY_CODE_SPC]            = ' '
};
static const uint8_t shift_qcode_to_ascii[]= {
    [0 ... 0xff]                = NO_KEY,

    /* TODO: '`' */
    [Q_KEY_CODE_1]              = '!',
    [Q_KEY_CODE_2]              = '"',
    [Q_KEY_CODE_3]              = '\xa3',   /* UK pound */
    [Q_KEY_CODE_4]              = '$',
    [Q_KEY_CODE_5]              = '%',
    [Q_KEY_CODE_6]              = '^',
    [Q_KEY_CODE_7]              = '&',
    [Q_KEY_CODE_8]              = '*',
    [Q_KEY_CODE_9]              = '(',
    [Q_KEY_CODE_0]              = ')',
    /* TODO: '-', '=' */
    [Q_KEY_CODE_BACKSPACE]      = 127,

    [Q_KEY_CODE_Q]              = 'Q',
    [Q_KEY_CODE_W]              = 'W',
    [Q_KEY_CODE_E]              = 'E',
    [Q_KEY_CODE_R]              = 'R',
    [Q_KEY_CODE_T]              = 'T',
    [Q_KEY_CODE_Y]              = 'Y',
    [Q_KEY_CODE_U]              = 'U',
    [Q_KEY_CODE_I]              = 'I',
    [Q_KEY_CODE_O]              = 'O',
    [Q_KEY_CODE_P]              = 'P',
    /* TODO: '[', ']' */

    [Q_KEY_CODE_A]              = 'A',
    [Q_KEY_CODE_S]              = 'S',
    [Q_KEY_CODE_D]              = 'D',
    [Q_KEY_CODE_F]              = 'F',
    [Q_KEY_CODE_G]              = 'G',
    [Q_KEY_CODE_H]              = 'H',
    [Q_KEY_CODE_J]              = 'J',
    [Q_KEY_CODE_K]              = 'K',
    [Q_KEY_CODE_L]              = 'L',
    /* TODO: ';', '\'', '#' */
    [Q_KEY_CODE_RET]            = '\r',

    [Q_KEY_CODE_Z]              = 'Z',
    [Q_KEY_CODE_X]              = 'X',
    [Q_KEY_CODE_C]              = 'C',
    [Q_KEY_CODE_V]              = 'V',
    [Q_KEY_CODE_B]              = 'B',
    [Q_KEY_CODE_N]              = 'N',
    [Q_KEY_CODE_M]              = 'M',
    /* TODO: ',', '.', '/' */

    [Q_KEY_CODE_SPC]            = ' '
};
#else   /* previous keyboard I/O */
static const uint8_t keycode_to_asciilc[128]= {
    /* keymap for UK QWERTY keyboard - NB. repo.or.cz uses its
     * callback to translate keycode to row and column [as per the
     * Spectrum's keyboard electronics]. When feeding input to our
     * sercon or MC6850 input streams, we want ASCII.
     * (FIXME: this is (unintentionally) partial, and the handler
     * (also) lacks code to sense/track/apply modifier keys)
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
#endif

static void zaphod_kbd_event(DeviceState *dev, QemuConsole *src,
                             InputEvent *evt)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);
    InputKeyEvent *key;
    int qcode, modifier_bit, scancodes[3], count;

    assert(evt->type == INPUT_EVENT_KIND_KEY);
    key= evt->u.key.data;
    qcode = qemu_input_key_value_to_qcode(key->key);

    modifier_bit= 0;
    switch (qcode)
    {
    case Q_KEY_CODE_SHIFT:
        modifier_bit= 1;
        break;
    case Q_KEY_CODE_SHIFT_R:
        modifier_bit= 2;
        break;

    /* TODO: other modifiers? */

    default:
        break;  /* fall through to inject ASCII for key */
    }

    if (modifier_bit)
    {
        zis->modifiers&= ~modifier_bit;
        if (key->down)
            zis->modifiers|= modifier_bit;
    }
    else if (zis->screen)
    {
        /* Put character - convert to ASCII and inject into ACIA or
         * stdio stream via the relevant chardev's receive functions.
         * TODO: rewrite as 'else' case of modifier adjust test
         */

        count = qemu_input_key_value_to_scancode(key->key,
                                                 key->down,
                                                 scancodes);
        if (count == 1) /* single-byte XT scancode */
        {
            ZaphodMachineState  *zms= zis->board;

            if (!key->down)
            {
                if (zis->uart_acia)
                    zaphod_uart_set_inkey(zis->uart_acia, 0, false);
                else
                    zaphod_uart_set_inkey(zis->uart_stdio, 0, false);
            }
            else
            {
              const uint8_t     *conv_table= qcode_to_ascii;
              int               conv_table_size;
              uint8_t           ch= NO_KEY;
              ZaphodUARTState   *uart_mux;

                if (zis->modifiers & 3)
                {
                    conv_table= shift_qcode_to_ascii;
                    conv_table_size= sizeof shift_qcode_to_ascii
                                    / sizeof shift_qcode_to_ascii[0];
                }
                else
                {
                    conv_table_size= sizeof qcode_to_ascii
                                    / sizeof qcode_to_ascii[0];
                }

                if (qcode < conv_table_size)
                {
                    ch= conv_table[qcode];
#if 1   /* WmT - TRACE */
;DPRINTF("*** INFO: key-down event (scancode %d, val %d) from source device %p ***\n", scancodes[0], ch, dev);
#endif
                }

                if ( (ch != NO_KEY) && (uart_mux= zis->uart_acia) != NULL )
                {
                    if (zaphod_iocore_can_receive_acia(zms->iocore))
                        zaphod_iocore_receive_acia(zms->iocore, &ch, 1);
                }
                else if ( (ch != NO_KEY) && (uart_mux= zis->uart_stdio) != NULL )
                {
                    if (zaphod_iocore_can_receive_stdio(zms->iocore))
                        zaphod_iocore_receive_stdio(zms->iocore, &ch, 1);
                }
            }
        }
    }
}

ZaphodUARTState *zaphod_iocore_get_stdio_uart(ZaphodIOCoreState *zis)
{
    if (!zis->has_stdio) return NULL;
    if (zis->uart_stdio) return zis->uart_stdio;

    return zis->uart_stdio= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
}

ZaphodUARTState *zaphod_iocore_get_acia_uart(ZaphodIOCoreState *zis)
{
    if (!zis->has_acia) return NULL;
    if (zis->uart_acia) return zis->uart_acia;

    return zis->uart_acia= ZAPHOD_UART(object_new(TYPE_ZAPHOD_UART));
}

static QemuInputHandler zaphod_kbd_handler = {
    .name  = "zaphod-kbd",
    .mask  = INPUT_EVENT_MASK_KEY,
    .event = zaphod_kbd_event,
};
#endif

static void zaphod_iocore_reset(void *opaque)
{
    ZaphodIOCoreState   *zis= (ZaphodIOCoreState *)opaque;

    zis->modifiers= 0;
}

static void zaphod_iocore_realizefn(DeviceState *dev, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(dev);

    if (!zis->board)
    {
        error_setg(errp, "initialisation error - zis->board NULL");
        return;
    }

    /* stdio setup */
    zis->ioports_stdio = g_new(PortioList, 1);
    portio_list_init(zis->ioports_stdio, OBJECT(zis), zaphod_iocore_portio_stdio,
                    zis, "zaphod.stdio");
    portio_list_add(zis->ioports_stdio, get_system_io(), 0x00);

    /* TODO: configuration of stdin and ACIA inputs from command line */

    if (zis->has_stdio)
    {
#if QEMU_VERSION_MAJOR < 5
        qdev_init_nofail(DEVICE(zis->uart_stdio));
#else
        qdev_realize(DEVICE(zis->uart_stdio), NULL, NULL);
#endif

        qemu_chr_fe_set_handlers(&zis->uart_stdio->chr,
                    zaphod_iocore_can_receive_stdio, zaphod_iocore_receive_stdio,
                    NULL,
                    NULL, zis, NULL, true);
    }

    /* ACIA setup */

    zis->ioports_acia = g_new(PortioList, 1);
    portio_list_init(zis->ioports_acia, OBJECT(zis), zaphod_iocore_portio_acia,
                    zis, "zaphod.acia");
    portio_list_add(zis->ioports_acia, get_system_io(), 0x00);

    if (zis->has_acia)
    {
#if QEMU_VERSION_MAJOR < 5
        qdev_init_nofail(DEVICE(zis->uart_acia));
#else
        qdev_realize(DEVICE(zis->uart_acia), NULL, NULL);
#endif

        qemu_chr_fe_set_handlers(&zis->uart_acia->chr,
                    zaphod_iocore_can_receive_acia, zaphod_iocore_receive_acia,
                    NULL,
                    NULL, zis, NULL, true);

        zis->irq_acia= qemu_allocate_irqs(zaphod_interrupt_request, zis->board, 1);
    }

#if 1   /* keyboard I/O */
    zis->ihs= qemu_input_handler_register(dev, &zaphod_kbd_handler);
    qemu_input_handler_activate(zis->ihs);
#endif

    /* TODO: correlate screen(s) to stdio/acia input; enable (and
     * configure) screen to suit command line arguments
     */
#if 1   /* WmT - TRACE */
;DPRINTF("INFO: %s() about to do screen init/add...\n", __func__);
#endif
    if (zis->screen)
#if QEMU_VERSION_MAJOR < 5
        qdev_init_nofail(DEVICE(zis->screen));
#else
        qdev_realize(DEVICE(zis->screen), NULL, NULL);
#endif

    qemu_register_reset(zaphod_iocore_reset, zis);
}

#if 0
static Property zaphod_iocore_properties[]= {
    /* "has-stdio" set in zaphod_iocore_init() */
    /* "has-acia" set in zaphod_iocore_init() */
    DEFINE_PROP_END_OF_LIST()
};
#endif

static bool zaphod_iocore_get_has_stdio(Object *obj, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    return zis->has_stdio;
}

static void zaphod_iocore_set_has_stdio(Object *obj, bool value, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off */
    zis->has_stdio= value;
}

static bool zaphod_iocore_get_has_acia(Object *obj, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off? */
    return zis->has_acia;
}

static void zaphod_iocore_set_has_acia(Object *obj, bool value, Error **errp)
{
    ZaphodIOCoreState   *zis= ZAPHOD_IOCORE(obj);

    /* TODO: bail if inited and new setting is off? */
    zis->has_acia= value;
}

static void zaphod_iocore_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc= DEVICE_CLASS(oc);

    dc->desc= "Zaphod IOCore subsystem";
    dc->realize= zaphod_iocore_realizefn;
#if 0
    /* TODO: initialisation in dc->reset? */
    dc->props= zaphod_iocore_properties;
#endif

#if QEMU_VERSION_MAJOR < 5
    object_class_property_add_bool(oc, "has-stdio",
        zaphod_iocore_get_has_stdio, zaphod_iocore_set_has_stdio, NULL);
    object_class_property_set_description(oc, "has-stdio",
        "Configure IOCore with stdio devices", NULL);
    object_class_property_add_bool(oc, "has-acia",
        zaphod_iocore_get_has_acia, zaphod_iocore_set_has_acia, NULL);
    object_class_property_set_description(oc, "has-acia",
        "Configure IOCore with ACIA devices", NULL);
#else
    object_class_property_add_bool(oc, "has-stdio",
        zaphod_iocore_get_has_stdio, zaphod_iocore_set_has_stdio);
    object_class_property_set_description(oc, "has-stdio",
        "Configure IOCore with stdio devices");
    object_class_property_add_bool(oc, "has-acia",
        zaphod_iocore_get_has_acia, zaphod_iocore_set_has_acia);
    object_class_property_set_description(oc, "has-acia",
        "Configure IOCore with ACIA devices");
#endif
}

static void zaphod_iocore_instance_init(Object *obj)
{
    /* Nothing to do here - handlers are set in realize function */
}


static const TypeInfo zaphod_iocore_info= {
    .name= TYPE_ZAPHOD_IOCORE,
    .parent= TYPE_DEVICE,
    /* For ZaphodIOCoreClass with virtual functions:
    .class_size= sizeof(ZaphodIOCoreClass),
     */
    .class_init= zaphod_iocore_class_init,
    .instance_size= sizeof(ZaphodIOCoreState),
    .instance_init= zaphod_iocore_instance_init
};

static void zaphod_iocore_register_types(void)
{
    type_register_static(&zaphod_iocore_info);
}

type_init(zaphod_iocore_register_types)
