#include <dpmi.h>
#include <sys/segments.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "sigcontext.h"
#include "cpu.h"
#include "entry.h"
#include "calls.h"
#include "dpmi.h"
#include "msdoshlp.h"
#include "wrapper.h"

unsigned char *mem_base;
static __dpmi_meminfo ldt_alias;
static unsigned short dpmi_ldt_alias;

#define LDT_ENTRIES     8192
#define LDT_ENTRY_SIZE  8

void wrapper_init(void)
{
  int err;
  ldt_alias.size = PAGE_ALIGN(LDT_ENTRIES * LDT_ENTRY_SIZE);
  err = __dpmi_allocate_linear_memory(&ldt_alias, 0);
  if (err) {
    printf("Warning: linear mem alloc failed\n");
    return;
  }
  dpmi_ldt_alias = __dpmi_allocate_ldt_descriptors(1);
  __dpmi_set_segment_base_address(dpmi_ldt_alias, ldt_alias.address);
  __dpmi_set_segment_limit(dpmi_ldt_alias, ldt_alias.size - 1);
}

int ValidAndUsedSelector(unsigned short selector)
{
  int lar;
  if (!(selector & 4))
    return 0;
  lar = __dpmi_get_descriptor_access_rights(selector);
  if (!lar)
    return 0;
  if ((lar & 0x90) != 0x90)
    return 0;
  return 1;
}

int ConvertSegmentToDescriptor(unsigned short segment)
{
  return __dpmi_segment_to_descriptor(segment);
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

unsigned long GetSegmentLimit(unsigned short selector)
{
  return __dpmi_get_segment_limit(selector);
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

void copy_context(struct sigcontext *d, struct sigcontext *s,
    int copy_fpu)
{
  struct _fpstate *fptr = d->fpstate;
  *d = *s;
  switch (copy_fpu) {
    case 1:   // copy FPU context
      if (fptr == s->fpstate)
        dosemu_error("Copy FPU context between the same locations?\n");
      *fptr = *s->fpstate;
      /* fallthrough */
    case -1:  // don't copy
      d->fpstate = fptr;
      break;
  }
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

far_t DPMI_get_real_mode_interrupt_vector(int vec)
{
    far_t addr;
    __dpmi_raddr _address;
    __dpmi_get_real_mode_interrupt_vector(vec, &_address);
    addr.segment = _address.segment;
    addr.offset = _address.offset16;
    return addr;
}

unsigned long GetFreeMemory(void)
{
  __dpmi_free_mem_info info;
  __dpmi_get_free_memory_information(&info);
  return info.largest_available_free_block_in_bytes;
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
  return SEL_ADR_LDT(sel, reg, clnt_is_32);
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

void GetFreeMemoryInformation(unsigned int *lp)
{
  __dpmi_get_free_memory_information((__dpmi_free_mem_info *)lp);
}

int GetDescriptor(us selector, unsigned int *lp)
{
    return __dpmi_get_descriptor(selector, lp);
}

unsigned int GetSegmentBase(unsigned short selector)
{
  unsigned long addr;
  __dpmi_get_segment_base_address(selector, &addr);
  return addr;
}

int DPMI_allocate_specific_ldt_descriptor(unsigned short selector)
{
    return __dpmi_allocate_specific_ldt_descriptor(selector);
}

int SetDescriptor(unsigned short selector, unsigned int *lp)
{
    return __dpmi_set_descriptor(selector, lp);
}

int SetDescriptorAccessRights(unsigned short selector, unsigned short acc_rights)
{
    return __dpmi_set_descriptor_access_rights(selector, acc_rights);
}

unsigned short CreateAliasDescriptor(unsigned short selector)
{
    return __dpmi_create_alias_descriptor(selector);
}

static void *get_handler(u_short sel, int offs)
{
    if (sel != _my_cs())
	return NULL;
#define RH(n) \
    if (offs == (uintptr_t)entry_MSDOS_rmcb_call##n) \
	return entry_MSDOS_rmcb_call##n
    RH(0);
    RH(1);
    RH(2);
    return NULL;
}

far_t DPMI_allocate_realmode_callback(u_short sel, int offs, u_short rm_sel,
       int rm_offs)
{
    __dpmi_raddr ret;
    void *hndl = get_handler(sel, offs);
    assert(hndl);
    __dpmi_allocate_real_mode_callback(hndl,
	    SEL_ADR_LDT(rm_sel, rm_offs, clnt_is_32), &ret);
    return (far_t){ .segment = ret.segment, .offset = ret.offset16 };
}

int DPMI_free_realmode_callback(u_short seg, u_short off)
{
    __dpmi_raddr addr = (__dpmi_raddr){ .segment = seg, .offset16 = off };
    return __dpmi_free_real_mode_callback(&addr);
}

int DPMI_get_save_restore_address(far_t *raddr, struct pmaddr_s *paddr)
{
    __dpmi_raddr rm;
    __dpmi_paddr pm;
    int err = __dpmi_get_state_save_restore_addr(&rm, &pm);
    if (err)
	return err;
    raddr->segment = rm.segment;
    raddr->offset = rm.offset16;
    paddr->selector = pm.selector;
    paddr->offset = pm.offset32;
    return 0;
}

u_short dos_get_psp(void)
{
  __dpmi_regs regs = {0};
  regs.h.ah = 0x51;
  do_rm_int(0x21, &regs);
  return regs.x.bx;
}
