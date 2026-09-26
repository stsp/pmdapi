/*
 * The part of dosemu2's emudpmi.h that the msdos plugin uses. dosemu2
 * implements these on its own DPMI server; pmdapi is a client of some
 * other one, so wrapper.c implements them with DPMI calls.
 */
#ifndef EMUDPMI_H
#define EMUDPMI_H

#include "cpu.h"
#include "djdpmi.h"
#include "utilities.h"
#include "dosemu_debug.h"

#define DPMI_VERSION		1
#define DPMI_MINOR_VERSION	0
#define DPMI_MAX_CLIENTS	32
#define DPMI_page_size		4096
#define LDT_ENTRIES		8192
#define LDT_ENTRY_SIZE		8

enum { es_INDEX, cs_INDEX, ss_INDEX, ds_INDEX, fs_INDEX, gs_INDEX,
  eax_INDEX, ebx_INDEX, ecx_INDEX, edx_INDEX, esi_INDEX, edi_INDEX,
  ebp_INDEX, esp_INDEX, eip_INDEX, eflags_INDEX };

typedef struct pmaddr_s
{
    unsigned int	offset;
    unsigned short	selector;
} __attribute__((packed)) INTDESC;
typedef struct
{
    unsigned int	offset32;
    unsigned short	selector;
} __attribute__((packed)) DPMI_INTDESC;

typedef struct dpmi_pm_block_stuct {
  struct   dpmi_pm_block_stuct *next;
  unsigned int handle;
  unsigned int size;
  dosaddr_t base;
  u_short  *attrs;
  int linear;
} dpmi_pm_block;

/* dosemu2 has its own layout here, the msdos plugin only needs it
 * to be the one of DPMI function 0300h */
struct RealModeCallStructure {
  __dpmi_regs;
};
#define RMREG(r) (rmreg->x.r)
#define RMLWORD(r) (rmreg->x.r)
#define E_RMREG(r) (rmreg->d.r)
#define X_RMREG(r) (rmreg->d.r)

struct SHM_desc {
  uint32_t req_len;
  uint32_t ret_len;
  uint32_t handle;
  uint32_t addr;
  uint32_t name_offset32;
  uint16_t name_selector;
#define SHM_NOEXEC 1
#define SHM_EXCL 2
#define SHM_NEW_NS 4
#define SHM_NS 8
  uint16_t flags;
  uint32_t opaque;
};

struct RSPcall_s {
  unsigned char data16[8];
  unsigned char code16[8];
  unsigned short ip;
#define RSP_F_SW 1	// switch_client extension
#define RSP_F_LOWMEM 2	// extra lowmem extension
  unsigned char flags;	// extension
  unsigned char para;	// extension
  unsigned char data32[8];
  unsigned char code32[8];
  unsigned int eip;
};

#define DPMI_EXT_COOKIE 0xd05e
#define DPMI_EXT_GET_POOL 1

/* eflags are stored with IF reflecting the actual interrupt state */
#define dpmi_flags_to_stack(flags) (flags)
static inline unsigned _dpmi_flags_from_stack_iret(const cpuctx_t *scp,
    int r0, unsigned flags)
{
  int iopl = ((_eflags_ & IOPL_MASK) >> IOPL_SHIFT);
  unsigned new_flags = (flags & 0xdd5) | 2;

  if (r0)
    new_flags |= (flags & IOPL_MASK);
  else
    new_flags |= (_eflags_ & IOPL_MASK);

  if (r0 || iopl == 3)
    new_flags |= (flags & IF);
  else
    new_flags |= (_eflags_ & IF);
  return new_flags;
}
static inline unsigned dpmi_flags_from_stack_iret(const cpuctx_t *scp,
    unsigned flags)
{
  return _dpmi_flags_from_stack_iret(scp, 0, flags);
}
#define flags_to_pm(flags) ((flags & (0xdd5|IF)) | 2 | (_eflags_ & IOPL_MASK))
#define flags_to_rm(flags) ((flags) | 2 | IOPL_MASK)

void *SEL_ADR(unsigned short sel, unsigned int reg);
void *SEL_ADR_CLNT(unsigned short sel, unsigned int reg, int is_32);
int dpmi_segment_is32(int sel);
unsigned short dpmi_sel(void);
unsigned short dpmi_sel16(void);
unsigned short dpmi_sel32(void);
unsigned dpmi_mem_size(void);
const char *DPMI_show_state(cpuctx_t *scp);

dpmi_pm_block DPMImalloc(unsigned size);
int DPMIfree(unsigned handle);
dpmi_pm_block DPMIrealloc(unsigned handle, unsigned size);
int DPMISetPageAttributes(unsigned handle, int offs, u_short attrs[], int count);
int DPMIGetPageAttributes(unsigned handle, int offs, u_short attrs[], int count);
void GetFreeMemoryInformation(unsigned int *lp);
int GetDescriptor(u_short selector, unsigned int *lp);
unsigned int GetSegmentBase(unsigned short sel);
unsigned int GetSegmentLimit(unsigned short sel);
int ValidAndUsedSelector(unsigned int selector);
int ConvertSegmentToDescriptor(unsigned short segment);
int SetSegmentBaseAddress(unsigned short selector, dosaddr_t baseaddr);
int SetSegmentLimit(unsigned short, unsigned int);
DPMI_INTDESC dpmi_get_interrupt_vector(unsigned char num);
void dpmi_set_interrupt_vector(unsigned char num, DPMI_INTDESC desc);
far_t DPMI_get_real_mode_interrupt_vector(int vec);
DPMI_INTDESC dpmi_get_pm_exc_addr(int num);
void dpmi_set_pm_exc_addr(int num, DPMI_INTDESC addr);
int DPMI_allocate_specific_ldt_descriptor(unsigned short selector);
unsigned short AllocateDescriptors(int);
unsigned short CreateAliasDescriptor(unsigned short selector);
int SetDescriptorAccessRights(unsigned short selector, unsigned short acc_rights);
int SetDescriptor(unsigned short selector, unsigned int *lp);
int FreeDescriptor(unsigned short selector);
void FreeSegRegs(cpuctx_t *scp, unsigned short selector);
far_t DPMI_allocate_realmode_callback(u_short sel, int offs, u_short rm_sel,
	int rm_offs);
int DPMI_free_realmode_callback(u_short seg, u_short off);
int DPMI_get_save_restore_address(far_t *raddr, struct pmaddr_s *paddr);
int DPMIAllocateShared(struct SHM_desc *shm);
int DPMIFreeShared(uint32_t handle);
dosaddr_t DPMIMapHWRam(unsigned addr, unsigned size);
int DPMIUnmapHWRam(dosaddr_t vbase);

void dpmi_ext_set_ldt_monitor16(DPMI_INTDESC call, uint16_t ds);
void dpmi_ext_set_ldt_monitor32(DPMI_INTDESC call, uint16_t ds);
void dpmi_ext_ldt_monitor_enable(int on);

int dpmi_install_rsp(struct RSPcall_s *callback);
unsigned short dpmi_get_private_pool(unsigned short *r_paras);

#endif
