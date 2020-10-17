/*
 * Minimal QEmu Z80 CPU - virtual CPU header
 * vim: ft=c sw=4 ts=4 et :
 *
 *  Copyright (c) 2007-2009 Stuart Brady <stuart.brady@gmail.com>
 *  Ported/provisional reimplementations by William Towle 2018-2022
 *  All versions under GPL
 */

#ifndef Z80_CPU_H
#define Z80_CPU_H


//#ifdef CONFIG_USER_ONLY
//    /* Temporarily bail from builds of z80-bblbrx-user while partial */
//#error "CONFIG_USER_ONLY builds for z80 have incomplete support"
//#endif
#ifdef CONFIG_SOFTMMU
    /* Temporarily bail from builds of z80-softmmu while partial */
#error "CONFIG_SOFTMMU builds for z80 have incomplete support"
#endif

/* TODO: CPUZ80State, Z80CPU, MMU modes */

#endif /* Z80_CPU_H */
