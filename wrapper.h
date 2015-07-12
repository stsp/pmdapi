/* 
 * (C) Copyright 1992, ..., 2004 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING in the DOSEMU distribution
 */

/* this is for the DPMI support */
#ifndef DPMI_H
#define DPMI_H

#include <dpmi.h>
#include "sigcontext.h"
#include "cpu.h"
#include "vm86.h"

typedef struct {
  unsigned short offset;
  unsigned short segment;
} far_t;

typedef unsigned short us;

#define sigcontext_struct sigcontext

#define DPMI_MAX_CLIENTS	32	/* maximal number of clients */
#define PAGE_MASK	(~(PAGE_SIZE-1))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

enum { es_INDEX, cs_INDEX, ss_INDEX, ds_INDEX, fs_INDEX, gs_INDEX,
  eax_INDEX, ebx_INDEX, ecx_INDEX, edx_INDEX, esi_INDEX, edi_INDEX,
  ebp_INDEX, esp_INDEX };

typedef __dpmi_paddr INTDESC;
#define pmaddr_s __dpmi_paddr
#if 0
typedef struct dpmi_pm_block_stuct {
  struct   dpmi_pm_block_stuct *next;
  unsigned long handle;
  unsigned long size;
  char     *base;
  u_short  *attrs;
  int linear;
} dpmi_pm_block;
#else
#define dpmi_pm_block __dpmi_meminfo
#endif
int ValidAndUsedSelector(unsigned short selector);

extern int ConvertSegmentToDescriptor(unsigned short segment);
extern int ConvertSegmentToCodeDescriptor(unsigned short segment);

dpmi_pm_block DPMImalloc(unsigned long size);
int DPMIfree(unsigned long handle);
dpmi_pm_block DPMIrealloc(unsigned long handle, unsigned long size);

extern INTDESC dpmi_get_interrupt_vector(unsigned char num);
extern void dpmi_set_interrupt_vector(unsigned char num, INTDESC desc);
void GetFreeMemoryInformation(unsigned long *lp);
int GetDescriptor(us selector, unsigned long *lp);

extern int SetSegmentBaseAddress(unsigned short selector,
					unsigned long baseaddr);
unsigned long GetSegmentBaseAddress(unsigned short);
unsigned long GetSegmentLimit(unsigned short);
int SegmentIs32(unsigned short);
extern int SetSegmentLimit(unsigned short, unsigned int);
extern unsigned short AllocateDescriptors(int);
extern int FreeDescriptor(unsigned short selector);
extern void FreeSegRegs(struct sigcontext *scp, unsigned short selector);
extern void copy_context(struct sigcontext *d, struct sigcontext *s);

extern unsigned long SEL_ADR(unsigned short sel, unsigned long reg);

void fake_int_to(int cs, int ip);
void set_io_buffer(char *ptr, unsigned int size);
void unset_io_buffer(void);

void emm_get_map_registers(char *ptr);
void emm_set_map_registers(char *ptr);
void emm_unmap_all(void);

extern void pm_to_rm_regs(struct sigcontext_struct *scp, unsigned int mask);
extern void rm_to_pm_regs(struct sigcontext_struct *scp, unsigned int mask);

extern unsigned short dpmi_sel(void);
void fake_call_to(int cs, int ip);

#define DPMI_SEG 0
#define DPMI_OFF 0
#define DPMI_ADD 0
#define HLT_OFF(x) 0
#define EMM_SEGMENT 0
#define DTA_Para_ADD 0
#define DOS_LONG_READ_SEG 0
#define DOS_LONG_READ_OFF 0
#define DOS_LONG_WRITE_SEG 0
#define DOS_LONG_WRITE_OFF 0

#define D_printf(...)
#define error(...)
#define snprintf(a,b,c,d) sprintf(a,c,d)
#define ConvertSegmentToDescriptor_lim(a,b) ConvertSegmentToDescriptor(a)

#endif /* DPMI_H */
