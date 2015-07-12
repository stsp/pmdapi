#include <dpmi.h>
#include <stdlib.h>
#include "sigcontext.h"
#include "cpu.h"
#include "vm86.h"
#include "entry.h"
#include "wrapper.h"

struct vm86_regs regs;

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
  info.size = size;
  if (__dpmi_allocate_memory(&info) == -1)
    info.size = 0;
  return info;
}

int DPMIfree(unsigned long handle)
{
  return __dpmi_free_memory(handle);
}

dpmi_pm_block DPMIrealloc(unsigned long handle, unsigned long size)
{
  __dpmi_meminfo info;
  info.handle = handle;
  info.size = size;
  if (__dpmi_resize_memory(&info) == -1)
    info.size = 0;
  return info;
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

int SegmentIs32(unsigned short selector)
{
  int lar = __dpmi_get_descriptor_access_rights(selector);
  return lar & 0x4000;
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

void copy_context(struct sigcontext *d, struct sigcontext *s)
{
  *d = *s;
}

void dpmi_set_interrupt_vector(unsigned char num, INTDESC desc)
{
  __dpmi_set_protected_mode_interrupt_vector(num, &desc);
}

INTDESC dpmi_get_interrupt_vector(unsigned char num)
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

unsigned long SEL_ADR(unsigned short sel, unsigned long reg)
{
  if (!(sel & 0x0004)) {
    /* GDT */
    return (unsigned long) reg;
  } else {
    /* LDT */
    if (SegmentIs32(sel))
      return (unsigned long) (GetSegmentBaseAddress(sel) + reg );
    else
      return (unsigned long) (GetSegmentBaseAddress(sel) + LO_WORD(reg));
  }
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

void emm_get_map_registers(char *ptr)
{
}

void emm_set_map_registers(char *ptr)
{
}

void emm_unmap_all(void)
{
}

void GetFreeMemoryInformation(unsigned long *lp)
{
}

int GetDescriptor(us selector, unsigned long *lp)
{
    return 0;
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
