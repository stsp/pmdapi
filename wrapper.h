/* 
 * (C) Copyright 1992, ..., 2004 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING in the DOSEMU distribution
 */

/* this is for the DPMI support */
#ifndef DPMI_H
#define DPMI_H

typedef struct {
  unsigned short offset;
  unsigned short segment;
} far_t;


#define D_16_32(reg)		(DPMI_CLIENT_is_32() ? reg : reg & 0xffff)

#define PAGE_MASK	(~(PAGE_SIZE-1))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

enum { es_INDEX, cs_INDEX, ss_INDEX, ds_INDEX, fs_INDEX, gs_INDEX,
  eax_INDEX, ebx_INDEX, ecx_INDEX, edx_INDEX, esi_INDEX, edi_INDEX,
  ebp_INDEX, esp_INDEX };

typedef struct pmaddr_s
{
    unsigned long	offset;
    unsigned short	selector, __selectorh;
} INTDESC;

int ValidAndUsedSelector(unsigned short selector);

extern int ConvertSegmentToDescriptor(unsigned short segment);
extern int ConvertSegmentToCodeDescriptor(unsigned short segment);

void *DPMImalloc(unsigned long size);
int DPMIfree(void *addr);
void *DPMIrealloc(void *addr, unsigned long size);

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

struct sigcontext *GetRegs(void);
unsigned short GetCurrentPSPSeg(void);
void SetCurrentPSPSeg(unsigned short seg);
void SetUserPSPSel(unsigned short sel);
unsigned short GetUserPSPSel(void);
void UnsetUserPSP(void);
unsigned short GetCurrentEnvSel(void);
void SetCurrentEnvSel(unsigned short sel);
unsigned short GetUserDTASel(void);
unsigned short GetDTASeg(void);
unsigned long GetUserDTAOff(void);
void SetUserDTA(struct pmaddr_s *addr);
struct pmaddr_s *GetUserDTA(void);
void UnsetUserDTA(void);
int DPMI_CLIENT_is_32(void);
void SetMouseCallBack(unsigned short sel, unsigned long off);
void SetPS2mouseCallBack(unsigned short sel, unsigned long off);
void SetInterruptVector(unsigned char num, unsigned short sel, unsigned long off);
struct pmaddr_s GetInterruptVector(unsigned char num);
unsigned long GetFreeMemory(void);
void prepare_ems_frame(void);
void restore_ems_frame(void);
void DPMIpush(unsigned long val);

#define SEL_ADR(seg, reg) sel_adr(seg, reg)
extern unsigned long sel_adr(unsigned short seg, unsigned long reg);

void fake_int_to(int cs, int ip);
void set_io_buffer(char *ptr, unsigned int size);
void unset_io_buffer(void);

void emm_get_map_registers(char *ptr);
void emm_set_map_registers(char *ptr);
void emm_unmap_all(void);

#define DPMI_SEG 0
#define DPMI_OFF 0
#define HLT_OFF(x) 0
#define EMM_SEGMENT 0
#define DTA_Para_ADD 0
#define DOS_LONG_READ_SEG 0
#define DOS_LONG_READ_OFF 0
#define DOS_LONG_WRITE_SEG 0
#define DOS_LONG_WRITE_OFF 0

#define D_printf(...)
#define error printf

#endif /* DPMI_H */
