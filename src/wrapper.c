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
  return SEL_ADR_LDT(sel, reg, is_32);
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

unsigned short AllocateDescriptorsAt(unsigned short selector,
    int number_of_descriptors)
{
    /* FIXME */
    return 0;
}

int SetDescriptor(unsigned short selector, unsigned int *lp)
{
    /* FIXME */
    return 0;
}

int SetDescriptorAccessRights(unsigned short selector, unsigned short type_byte)
{
    /* FIXME */
    return 0;
}

unsigned short CreateAliasDescriptor(unsigned short selector)
{
    /* FIXME */
    return 0;
}

struct msdos_ops {
    void (*api_call)(struct sigcontext *scp);
    void (*xms_call)(struct RealModeCallStructure *rmreg);
    void (**rmcb_handler)(struct sigcontext *scp,
	const struct RealModeCallStructure *rmreg);
    void (**rmcb_ret_handler)(const struct sigcontext *scp,
	struct RealModeCallStructure *rmreg);
    u_short cb_es;
    u_int cb_edi;
};
static struct msdos_ops msdos;

int allocate_realmode_callbacks(void (*handler[])(struct sigcontext *,
	const struct RealModeCallStructure *),
	void (*ret_handler[])(const struct sigcontext *,
	struct RealModeCallStructure *),
	int num, far_t *r_cbks)
{
//    int i;
    assert(num <= 3);
    msdos.rmcb_handler = handler;
    msdos.rmcb_ret_handler = ret_handler;
#if 0
    for (i = 0; i < num; i++)
	r_cbks[i] = DPMI_allocate_realmode_callback(dpmi_sel(),
	    get_cb(i), dpmi_data_sel(), DPMI_DATA_OFF(MSDOS_rmcb_data));
#endif
    return num;
}

void free_realmode_callbacks(far_t *cbks, int num)
{
    int i;
    for (i = 0; i < num; i++) {
	__dpmi_raddr addr = {
		.segment = cbks[i].segment, .offset16 = cbks[i].offset };
	__dpmi_free_real_mode_callback(&addr);
    }
}

void do_api_call(struct sigcontext *scp)
{
    msdos.api_call(scp);
}

struct pmaddr_s get_pm_handler(enum MsdOpIds id,
	void (*handler)(struct sigcontext *))
{
    struct pmaddr_s ret;
    switch (id) {
    case API_CALL:
	msdos.api_call = handler;
	ret.selector = _my_cs();
#if 0
	ret.offset = (u_int)api_call_ent;
#endif
	break;
    default:
	dosemu_error("unknown pm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

struct pmaddr_s get_pmrm_handler(enum MsdOpIds id, void (*handler)(
	struct RealModeCallStructure *))
{
    struct pmaddr_s ret;
    switch (id) {
    case XMS_CALL:
	msdos.xms_call = handler;
	ret.selector = _my_cs();
#if 0
	ret.offset = DPMI_SEL_OFF(MSDOS_XMS_call);
#endif
	break;
    default:
	dosemu_error("unknown pmrm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

far_t get_lr_helper(far_t rmcb)
{
  far_t ret = {};
  return ret;
}

far_t get_lw_helper(far_t rmcb)
{
  far_t ret = {};
  return ret;
}

far_t get_exec_helper(void)
{
  far_t ret = {};
  return ret;
}

u_short dos_get_psp(void)
{
  __dpmi_regs regs = {0};
  regs.h.ah = 0x51;
  do_rm_int(0x21, &regs);
  return regs.x.bx;
}
