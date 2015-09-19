#ifndef __ENTRY_H
#define __ENTRY_H

#define _dseg32 ___djgpp_ds_alias

#ifndef __ASSEMBLER__

#include <sys/exceptn.h>

#define dseg32 __djgpp_ds_alias
extern void entry32(void);
extern void dos32_int21(void);
extern void dos16_int21(void);
extern void code16(void);
extern void entry16(void);
extern void code16_end(void);
extern void data16(void);
extern unsigned long cs32_desc[2];
extern unsigned long ds32_desc[2];
extern unsigned long is_32;
extern void data16_end(void);

extern void api_call_ent(void);

#endif
#endif
