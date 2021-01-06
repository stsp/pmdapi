#ifndef __ENTRY_H
#define __ENTRY_H

#ifndef __ASSEMBLER__

#include "sigcontext.h"

extern void entry32(void);
extern void dos32_int21(void);
extern void dos16_int21(void);
extern void code16(void);
extern void entry16(void);
extern void code16_end(void);
extern void data16(void);
extern unsigned long cs32_desc[2];
extern unsigned long ds32_desc[2];
extern unsigned long clnt_is_32;
extern unsigned long dseg32;
extern void data16_end(void);

extern void entry_MSDOS_API_call(void);
extern void MSDOS_API_call(struct sigcontext *scp);
extern void entry_MSDOS_API_WINOS2_call(void);
extern void MSDOS_API_WINOS2_call(struct sigcontext *scp);
extern void entry_MSDOS_XMS_call(void);
extern void MSDOS_XMS_call(struct sigcontext *scp);
extern void MSDOS_rmcb_call0(struct sigcontext *scp);
extern void MSDOS_rmcb_call1(struct sigcontext *scp);
extern void MSDOS_rmcb_call2(struct sigcontext *scp);
extern void entry_MSDOS_rmcb_call0(void);
extern void entry_MSDOS_rmcb_call1(void);
extern void entry_MSDOS_rmcb_call2(void);

#endif
#endif
