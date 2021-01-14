#ifndef HANDLERS_H
#define HANDLERS_H

#define PRINTF(n) __attribute__((format(printf, n, n + 1)))

PRINTF(1)
int emu_printf(const char *format, ...);

#endif
