#ifndef EMUDPMI_H
#define EMUDPMI_H

#include "cpu.h"
#include "entry.h"

#define DPMI_MAX_CLIENTS	32	/* maximal number of clients */
#define PAGE_SIZE 4096
#define PAGE_MASK	(~(PAGE_SIZE-1))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)
#define DPMI_page_size		4096	/* 4096 bytes per page */

#define dpmi_sel() _my_cs()
#define dpmi_sel32() _my_cs()
#define dpmi_sel16() _my_cs()  /* FIXME */
#define DPMI_SEL_OFF(x) (uintptr_t)entry_##x

enum { es_INDEX, cs_INDEX, ss_INDEX, ds_INDEX, fs_INDEX, gs_INDEX,
  eax_INDEX, ebx_INDEX, ecx_INDEX, edx_INDEX, esi_INDEX, edi_INDEX,
  ebp_INDEX, esp_INDEX, eip_INDEX, eflags_INDEX };

typedef __dpmi_paddr DPMI_INTDESC;
struct pmaddr_s
{
    unsigned int	offset;
    unsigned short	selector;
};
typedef struct dpmi_pm_block_stuct {
  struct   dpmi_pm_block_stuct *next;
  unsigned int handle;
  unsigned int size;
  dosaddr_t base;
  u_short  *attrs;
  int linear;
} dpmi_pm_block;

struct RealModeCallStructure {
  __dpmi_regs;
};

#define RMREG(r) (rmreg->x.r)
#define RMLWORD(r) (rmreg->x.r)
#define E_RMREG(r) (rmreg->d.r)
#define X_RMREG(r) (rmreg->d.r)

int ValidAndUsedSelector(unsigned short selector);

extern int ConvertSegmentToDescriptor(unsigned short segment);

dpmi_pm_block DPMImalloc(unsigned long size);
int DPMIfree(unsigned long handle);
dpmi_pm_block DPMIrealloc(unsigned long handle, unsigned long size);

extern DPMI_INTDESC dpmi_get_interrupt_vector(unsigned char num);
extern void dpmi_set_interrupt_vector(unsigned char num, DPMI_INTDESC desc);
extern far_t DPMI_get_real_mode_interrupt_vector(int vec);
void GetFreeMemoryInformation(unsigned int *lp);
int GetDescriptor(us selector, unsigned int *lp);
extern int DPMI_allocate_specific_ldt_descriptor(unsigned short selector);
extern int SetDescriptor(unsigned short selector, unsigned int *lp);
extern int SetSegmentBaseAddress(unsigned short selector,
					unsigned long baseaddr);
unsigned long GetSegmentLimit(unsigned short);
extern unsigned int GetSegmentBase(unsigned short);
extern unsigned short CreateAliasDescriptor(unsigned short selector);
extern int SetDescriptorAccessRights(unsigned short selector,
	unsigned short acc_rights);
int dpmi_mhp_get_selector_size(int sel);
extern int SetSegmentLimit(unsigned short, unsigned int);
extern unsigned short AllocateDescriptors(int);
extern int FreeDescriptor(unsigned short selector);
extern void FreeSegRegs(struct sigcontext *scp, unsigned short selector);
extern far_t allocate_realmode_callback(void (*handler)(
	struct RealModeCallStructure *));
extern int DPMI_free_realmode_callback(u_short seg, u_short off);
extern int DPMI_get_save_restore_address(far_t *raddr, struct pmaddr_s *paddr);
extern far_t DPMI_allocate_realmode_callback(u_short sel, int offs, u_short rm_sel,
	int rm_offs);

extern void copy_context(struct sigcontext *d,
    struct sigcontext *s, int copy_fpu);

void *SEL_ADR(unsigned short sel, unsigned int reg);
void *SEL_ADR_CLNT(unsigned short sel, unsigned int reg, int is_32);
DPMI_INTDESC dpmi_get_pm_exc_addr(int num);
void dpmi_set_pm_exc_addr(int num, DPMI_INTDESC addr);

struct SHM_desc {
  uint32_t req_len;
  uint32_t ret_len;
  uint32_t handle;
  uint32_t addr;
  uint32_t name_offset32;
  uint16_t name_selector;
  uint16_t padding;
#define SHM_NOEXEC 1
  uint32_t flags;
};

int DPMIAllocateShared(struct SHM_desc *shm);
int DPMIFreeShared(uint32_t handle);

int DPMISetPageAttributes(unsigned long handle, int offs, u_short attrs[], int count);
int DPMIGetPageAttributes(unsigned long handle, int offs, u_short attrs[], int count);

void dpmi_ext_set_ldt_monitor16(DPMI_INTDESC call, uint16_t ds);
void dpmi_ext_set_ldt_monitor32(DPMI_INTDESC call, uint16_t ds);
void dpmi_ext_ldt_monitor_enable(int on);

int dpmi_segment_is32(int sel);

#endif
