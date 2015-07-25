#include <dpmi.h>
#include <stdlib.h>
#include <stdint.h>
#include "sigcontext.h"
#include "cpu.h"
#include "entry.h"
#include "wrapper.h"

typedef struct segment_descriptor_s
{
    unsigned int	base_addr;	/* Pointer to segment in flat memory */
    unsigned int	limit;		/* Limit of Segment */
    unsigned int	type:2;
    unsigned int	is_32:1;	/* one for is 32-bit Segment */
    unsigned int	readonly:1;	/* one for read only Segments */
    unsigned int	is_big:1;	/* Granularity */
    unsigned int	not_present:1;
    unsigned int	useable:1;
    unsigned int	used;		/* Segment in use by client # */
					/* or Linux/GLibc (0xfe) */
					/* or DOSEMU (0xff) */
} SEGDESC;
#define LDT_ENTRIES     8192
#define MAX_SELECTORS   LDT_ENTRIES
static SEGDESC Segments[MAX_SELECTORS];

unsigned char *mem_base;

int ValidAndUsedSelector(unsigned short selector)
{
  return 1;
}

int ConvertSegmentToDescriptor(unsigned short segment)
{
  return __dpmi_segment_to_descriptor(segment);
}

int ConvertSegmentToCodeDescriptor(unsigned short segment)
{
  return 0;
}

int SetSegmentBaseAddress(unsigned short selector, unsigned long baseaddr)
{
  return __dpmi_set_segment_base_address(selector, baseaddr);
}

dpmi_pm_block DPMImalloc(unsigned long size)
{
  __dpmi_meminfo info;
  dpmi_pm_block block;
  info.size = size;
  if (__dpmi_allocate_memory(&info) == -1)
    info.size = 0;
  block.base = info.address;
  block.size = info.size;
  block.handle = info.handle;
  block.linear = 0;
  return block;
}

int DPMIfree(unsigned long handle)
{
  return __dpmi_free_memory(handle);
}

dpmi_pm_block DPMIrealloc(unsigned long handle, unsigned long size)
{
  __dpmi_meminfo info;
  dpmi_pm_block block;
  info.handle = handle;
  info.size = size;
  if (__dpmi_resize_memory(&info) == -1)
    info.size = 0;
  block.base = info.address;
  block.size = info.size;
  block.handle = info.handle;
  block.linear = 0;
  return block;
}

unsigned long GetSegmentBaseAddress(unsigned short selector)
{
  unsigned long addr;
  __dpmi_get_segment_base_address(selector, &addr);
  return addr;
}

unsigned long GetSegmentLimit(unsigned short selector)
{
  unsigned long lim;
  __dpmi_get_segment_base_address(selector, &lim);
  return lim;
}

int dpmi_mhp_get_selector_size(int selector)
{
  int lar = __dpmi_get_descriptor_access_rights(selector);
  return !!(lar & 0x4000);
}

int SetSegmentLimit(unsigned short selector, unsigned int limit)
{
  return __dpmi_set_segment_limit(selector, limit);
}

unsigned short AllocateDescriptors(int num)
{
  return __dpmi_allocate_ldt_descriptors(num);
}

int FreeDescriptor(unsigned short selector)
{
  return __dpmi_free_ldt_descriptor(selector);
}

void FreeSegRegs(struct sigcontext *scp, unsigned short selector)
{
    if ((_ds | 7) == (selector | 7)) _ds = 0;
    if ((_es | 7) == (selector | 7)) _es = 0;
    if ((_fs | 7) == (selector | 7)) _fs = 0;
    if ((_gs | 7) == (selector | 7)) _gs = 0;
}

void copy_context(struct sigcontext_struct *d,
    struct sigcontext_struct *s, int copy_fpu)
{
  *d = *s;
}

void dpmi_set_interrupt_vector(unsigned char num, DPMI_INTDESC desc)
{
  __dpmi_set_protected_mode_interrupt_vector(num, &desc);
}

DPMI_INTDESC dpmi_get_interrupt_vector(unsigned char num)
{
  __dpmi_paddr addr;
  __dpmi_get_protected_mode_interrupt_vector(num, &addr);
  return addr;
}

unsigned long GetFreeMemory(void)
{
  __dpmi_free_mem_info info;
  __dpmi_get_free_memory_information(&info);
  return info.largest_available_free_block_in_bytes;
}

void prepare_ems_frame(void)
{
}

void restore_ems_frame(void)
{
}

static void *SEL_ADR_LDT(unsigned short sel, unsigned int reg, int is_32)
{
  unsigned long p;
  if (is_32)
    p = GetSegmentBase(sel) + reg;
  else
    p = GetSegmentBase(sel) + LO_WORD(reg);
  /* The address needs to wrap, also in 64-bit! */
  return MEM_BASE32(p);
}

void *SEL_ADR(unsigned short sel, unsigned int reg)
{
  if (!(sel & 0x0004)) {
    /* GDT */
    return (void *)(uintptr_t)reg;
  }
  /* LDT */
  return SEL_ADR_LDT(sel, reg, Segments[sel>>3].is_32);
}

void *SEL_ADR_CLNT(unsigned short sel, unsigned int reg, int is_32)
{
  if (!(sel & 0x0004)) {
    /* GDT */
    dosemu_error("GDT not allowed\n");
    return (void *)(uintptr_t)reg;
  }
  return SEL_ADR_LDT(sel, reg, is_32);
}

void fake_int_to(int cs, int ip)
{
}

void set_io_buffer(char *ptr, unsigned int size)
{
}

void unset_io_buffer(void)
{
}

void GetFreeMemoryInformation(unsigned int *lp)
{
}

int GetDescriptor(us selector, unsigned long *lp)
{
    return 0;
}

unsigned int GetSegmentBase(unsigned short selector)
{
  if (!ValidAndUsedSelector(selector))
    return 0;
  return Segments[selector >> 3].base_addr;
}

void pm_to_rm_regs(struct sigcontext_struct *scp, unsigned int mask)
{
}

void rm_to_pm_regs(struct sigcontext_struct *scp, unsigned int mask)
{
}

unsigned short dpmi_sel(void)
{
  return 0;
}

void fake_call_to(int cs, int ip)
{
}

#define DPMI_max_rec_pm_func 16
static struct sigcontext_struct DPMI_pm_stack[DPMI_max_rec_pm_func];
static int DPMI_pm_procedure_running = 0;

void save_pm_regs(struct sigcontext_struct *scp)
{
  if (DPMI_pm_procedure_running >= DPMI_max_rec_pm_func) {
    error("DPMI: DPMI_pm_procedure_running = 0x%x\n",DPMI_pm_procedure_running);
//    leavedos(25);
    return;
  }
//  _eflags = eflags_VIF(_eflags);
  copy_context(&DPMI_pm_stack[DPMI_pm_procedure_running++], scp, 0);
}

void restore_pm_regs(struct sigcontext_struct *scp)
{
  if (DPMI_pm_procedure_running > DPMI_max_rec_pm_func ||
    DPMI_pm_procedure_running < 1) {
    error("DPMI: DPMI_pm_procedure_running = 0x%x\n",DPMI_pm_procedure_running);
//    leavedos(25);
    return;
  }
  copy_context(scp, &DPMI_pm_stack[--DPMI_pm_procedure_running], -1);
#if 0
  if (_eflags & VIF) {
    if (!isset_IF())
      D_printf("DPMI: set IF on restore_pm_regs\n");
    set_IF();
  } else {
    if (isset_IF())
      D_printf("DPMI: clear IF on restore_pm_regs\n");
    clear_IF();
  }
#endif
}

void lrhlp_setup(void)
{
}

u_short DPMI_ldt_alias(void)
{
  return 0;
}

u_short dos_get_psp(void)
{
  return 0;
}
