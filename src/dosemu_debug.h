/*
 * dosemu2's logging, for a DPMI client: everything goes to the host's
 * log through emu_printf(), which dosemu2 takes as int E6h AX=13h and
 * any other host ignores.
 */
#ifndef DOSEMU_DEBUG_H
#define DOSEMU_DEBUG_H

#include "handlers.h"

extern int pmdapi_debug;

#define log_printf emu_printf
#define ifprintf(flg,fmt,a...)	do{ if (flg) emu_printf(fmt,##a); }while(0)
#define debug_level(c) pmdapi_debug
#define D_printf(f,a...)	ifprintf(debug_level('M'),f,##a)
#define g_printf(f,a...)	ifprintf(debug_level('g'),f,##a)
#define x_printf(f,a...)	ifprintf(debug_level('x'),f,##a)
#define dbug_printf(f,a...)	ifprintf(1,f,##a)
#define error(f,a...)		emu_printf("ERROR: " f,##a)
#define dosemu_error(f,a...)	emu_printf("ERROR: " f,##a)
#define error_once(f,a...) do { \
    static int __warned; \
    if (!__warned) { __warned = 1; error(f,##a); } \
} while (0)
#define error_once0(s) error_once(s)

#endif
