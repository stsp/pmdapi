#include <dpmi.h>
#include <stdlib.h>
#include "sigcontext.h"
#include "cpu.h"
#include "vm86.h"
#include "entry.h"
#include "wrapper.h"

struct vm86_regs regs;
#define MAX_MEM_ALLOCS 1024
static __dpmi_meminfo mem_map[MAX_MEM_ALLOCS] = {{0,},};

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

void *DPMImalloc(unsigned long size)
{
  __dpmi_meminfo info;
  int i;
  info.size = size;
  if (__dpmi_allocate_memory(&info) == -1)
    return NULL;
  for (i = 0; i < MAX_MEM_ALLOCS; i++) {
    if (mem_map[i].size == 0) {
      mem_map[i] = info;
      break;
    }
  }
  return (void*)info.address;
}

int DPMIfree(void *addr)
{
  int i;
  for (i = 0; i < MAX_MEM_ALLOCS; i++) {
    if (mem_map[i].address == (unsigned long)addr) {
      __dpmi_free_memory(mem_map[i].handle);
      return 0;
    }
  }
  return -1;
}

void *DPMIrealloc(void *addr, unsigned long size)
{
  __dpmi_meminfo *info = NULL;
  int i;
  for (i = 0; i < MAX_MEM_ALLOCS; i++) {
    if (mem_map[i].address == (unsigned long)addr) {
      info = &mem_map[i];
      break;
    }
  }
  if (!info)
    return NULL;
  info->size = size;
  __dpmi_resize_memory(info);
  return (void*)info->address;
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

unsigned short GetCurrentPSPSeg(void)
{
  return 0;
}

void SetCurrentPSPSeg(unsigned short seg)
{
}

void SetUserPSPSel(unsigned short sel)
{
}

unsigned short GetUserPSPSel(void)
{
  return 0;
}

void UnsetUserPSP(void)
{
}

unsigned short GetCurrentEnvSel(void)
{
  return 0;
}

void SetCurrentEnvSel(unsigned short sel)
{
}

unsigned short GetUserDTASel(void)
{
  return 0;
}

unsigned short GetDTASeg(void)
{
  return 0;
}

unsigned long GetUserDTAOff(void)
{
  return 0;
}

void SetUserDTA(struct pmaddr_s *addr)
{
}

struct pmaddr_s *GetUserDTA(void)
{
  return 0;
}

void UnsetUserDTA(void)
{
}

int DPMI_CLIENT_is_32(void)
{
  return is_32;
}

void SetMouseCallBack(unsigned short sel, unsigned long off)
{
}

void SetPS2mouseCallBack(unsigned short sel, unsigned long off)
{
}

void SetInterruptVector(unsigned char num, unsigned short sel, unsigned long off)
{
  __dpmi_paddr addr;
  addr.selector = sel;
  addr.offset32 = off;
  __dpmi_set_protected_mode_interrupt_vector(num, &addr);
}

struct pmaddr_s GetInterruptVector(unsigned char num)
{
  struct pmaddr_s ret_addr;
  __dpmi_paddr addr;
  __dpmi_get_protected_mode_interrupt_vector(num, &addr);
  ret_addr.selector = addr.selector;
  ret_addr.offset = addr.offset32;
  return ret_addr;
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

void DPMIpush(unsigned long val)
{
}

unsigned long sel_adr(unsigned short seg, unsigned long reg)
{
  unsigned long __res;
  if (!((seg) & 0x0004)) {
    /* GDT */
    __res = (unsigned long) reg;
  } else {
    /* LDT */
    if (SegmentIs32(seg))
      __res = (unsigned long) (GetSegmentBaseAddress(seg) + reg );
    else
      __res = (unsigned long) (GetSegmentBaseAddress(seg) + *((unsigned short *)&(reg)) );
  }
  return __res;
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
