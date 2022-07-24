/*
 * QEmu Zaphod board - UART support
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...William Towle c. 2013-2022, under GPL...]
 */

#ifndef HW_Z80_ZAPHOD_UART_H
#define HW_Z80_ZAPHOD_UART_H

//#include "zaphod.h"

#include "chardev/char-fe.h"


typedef DeviceClass ZaphodUARTClass;

typedef struct {
    DeviceState     parent;

    CharBackend     chr;
    uint8_t         inkey;
} ZaphodUARTState;


#define TYPE_ZAPHOD_UART "zaphod-uart"

#define ZAPHOD_UART_GET_CLASS(obj) \
    OBJECT_GET_CLASS(ZaphodUARTClass, obj, TYPE_ZAPHOD_UART)
#define ZAPHOD_UART_CLASS(klass) \
    OBJECT_CLASS_CHECK(ZaphodUARTClass, klass, TYPE_ZAPHOD_UART)
#define ZAPHOD_UART(obj) \
    OBJECT_CHECK(ZaphodUARTState, obj, TYPE_ZAPHOD_UART)


#if defined(CONFIG_ZAPHOD_HAS_IOCORE)
int zaphod_uart_can_receive(void *opaque);
uint8_t zaphod_uart_get_inkey(void *opaque, bool read_and_clear);
void zaphod_uart_set_inkey(void *opaque, uint8_t val, bool is_data);
#endif

void zaphod_uart_putchar(ZaphodUARTState *zus, const unsigned char ch);

//ZaphodUARTState?
DeviceState *zaphod_uart_new(Chardev *chr_fallback);


#endif  /* HW_Z80_ZAPHOD_UART_H */
