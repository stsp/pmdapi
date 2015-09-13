#include <dpmi.h>
#include <stdlib.h>
#include <stdint.h>
#include "sigcontext.h"
#include "cpu.h"
#include "entry.h"
#include "dpmi.h"
#include "msdoshlp.h"
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

u_short DPMI_ldt_alias(void)
{
  return 0;
}

struct msdos_ops {
    void (*api_call)(struct sigcontext *scp);
    void (*xms_call)(struct RealModeCallStructure *rmreg);
    int (*mouse_callback)(struct sigcontext *scp,
	const struct RealModeCallStructure *rmreg);
    int (*ps2_mouse_callback)(struct sigcontext *scp,
	const struct RealModeCallStructure *rmreg);
    void (*rmcb_handler)(struct RealModeCallStructure *rmreg);
};
static struct msdos_ops msdos;

far_t allocate_realmode_callback(void (*handler)(
	struct RealModeCallStructure *))
{
  far_t ret = {};
  return ret;
}

int DPMI_free_realmode_callback(u_short seg, u_short off)
{
  return 0;
}

int free_realmode_callback(u_short seg, u_short off)
{
    return DPMI_free_realmode_callback(seg, off);
}

struct pmaddr_s get_pm_handler(enum MsdOpIds id,
	void (*handler)(struct sigcontext *))
{
    struct pmaddr_s ret;
    switch (id) {
    case API_CALL:
	msdos.api_call = handler;
#if 0
	ret.selector = dpmi_sel();
	ret.offset = DPMI_SEL_OFF(MSDOS_API_call);
#else
	ret = (struct pmaddr_s){ 0, 0 };
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
#if 0
	ret.selector = dpmi_sel();
	ret.offset = DPMI_SEL_OFF(MSDOS_XMS_call);
#else
	ret = (struct pmaddr_s){ 0, 0 };
#endif
	break;
    default:
	dosemu_error("unknown pmrm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

far_t get_rm_handler(enum MsdOpIds id, int (*handler)(struct sigcontext *,
	const struct RealModeCallStructure *))
{
    far_t ret;
    switch (id) {
    case MOUSE_CB:
	msdos.mouse_callback = handler;
#if 0
	ret.segment = DPMI_SEG;
	ret.offset = DPMI_OFF + HLT_OFF(MSDOS_mouse_callback);
#else
	ret = (far_t){ 0, 0 };
#endif
	break;
    case PS2MOUSE_CB:
	msdos.ps2_mouse_callback = handler;
#if 0
	ret.segment = DPMI_SEG;
	ret.offset = DPMI_OFF + HLT_OFF(MSDOS_PS2_mouse_callback);
#else
	ret = (far_t){ 0, 0 };
#endif
	break;
    default:
	dosemu_error("unknown rm handler\n");
	ret = (far_t){ 0, 0 };
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
  return 0;
}
