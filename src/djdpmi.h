/* dosemu2 carries its own copy of djgpp's dpmi.h; here we have the real one */
#ifndef DJDPMI_H
#define DJDPMI_H
#include <dpmi.h>
#define LONG int32_t
#define ULONG ULONG32
#endif
