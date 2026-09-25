/*
 * The few dosemu2 settings the msdos plugin reads, filled from what the
 * DPMI host tells us.
 */
#ifndef EMU_H
#define EMU_H

#include "cpu.h"
#include "utilities.h"

struct pmdapi_config {
    unsigned dpmi_base;
};
extern struct pmdapi_config config;

#endif
