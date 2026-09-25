#ifndef MSDOS_LDT_H
#define MSDOS_LDT_H

unsigned short msdos_ldt_init(int page_size);
void msdos_ldt_done(void);
int msdos_ldt_fault(cpuctx_t *scp, uint16_t sel);
int _msdos_ldt_access(dosaddr_t cr2);
void _msdos_ldt_write(cpuctx_t *scp, uint32_t op, int len,
    dosaddr_t cr2);
int _msdos_ldt_pagefault(cpuctx_t *scp);
int msdos_ldt_is32(unsigned short selector);

#endif
