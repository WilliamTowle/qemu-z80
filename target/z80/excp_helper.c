/*
 * Minimal QEmu Z80 CPU - exception helpers
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2018-2022 William Towle <william_towle@yahoo.co.uk>
 *  All versions under GPL
 */


#include "qemu/error-report.h"

#define EMIT_DEBUG 0
#define DPRINTF(fmt, ...) \
    do { if (EMIT_DEBUG) error_printf("Z80 excp_helper: " fmt , ## __VA_ARGS__); } while(0)
