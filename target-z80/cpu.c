/*
 * Skeleton for minimal QEmu Z80 CPU
 * vim: ft=c sw=4 ts=4 et :
 *
 * WmT, 2018-2022 [...after Stuart Brady, under GPL...]
 */

#include "qemu/error-report.h"

#define DPRINTF(fmt, ...) \
    do { error_printf( "Z80 cpu.c: " fmt , ## __VA_ARGS__); } while(0)

