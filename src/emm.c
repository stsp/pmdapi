#include <string.h>
#include <assert.h>
#include <dpmi.h>
#include "sigcontext.h"
#include "calls.h"
#include "wrapper.h"
#include "emmwrp.h"
#include "emm.h"

#define ALLOCATE_PAGES          0x43
#define SAVE_PAGE_MAP           0x47
#define RESTORE_PAGE_MAP        0x48
#define MAP_UNMAP_MULTIPLE      0x50

#define EMM_INT                 0x67

static int rmseg, rmsel;
static unsigned long rmaddr;

int emm_init(void)
{
  /* TODO: check EMM presence */

  /* allocate 1 para for page map array */
  rmseg = __dpmi_allocate_dos_memory(1, &rmsel);
  if (rmseg == -1)
    return -1;
  __dpmi_get_segment_base_address(rmsel, &rmaddr);
  return 0;
}

int emm_allocate_handle(int pages_needed)
{
  __dpmi_regs regs = {0};
  regs.h.ah = ALLOCATE_PAGES;
  regs.x.bx = pages_needed;
  __dpmi_simulate_real_mode_interrupt(EMM_INT, &regs);
  if (regs.h.ah)
    return -1;
  return regs.x.dx;
}

int emm_save_handle_state(int handle)
{
  __dpmi_regs regs = {0};
  regs.h.ah = SAVE_PAGE_MAP;
  regs.x.dx = handle;
  __dpmi_simulate_real_mode_interrupt(EMM_INT, &regs);
  if (regs.h.ah)
    return -1;
  return 0;
}

int emm_restore_handle_state(int handle)
{
  __dpmi_regs regs = {0};
  regs.h.ah = RESTORE_PAGE_MAP;
  regs.x.dx = handle;
  __dpmi_simulate_real_mode_interrupt(EMM_INT, &regs);
  if (regs.h.ah)
    return -1;
  return 0;
}

int emm_map_unmap_multi(const u_short *array, int handle, int map_len)
{
  __dpmi_regs regs = {0};
  int arr_len = sizeof(u_short) * 2 * map_len;

  assert(arr_len <= 16);
  regs.h.ah = MAP_UNMAP_MULTIPLE;
  regs.h.al = 0;
  regs.x.dx = handle;
  regs.x.cx = map_len;
  memcpy((void *)rmaddr, array, arr_len);
  regs.x.ds = rmseg;
  regs.x.si = 0;
  __dpmi_simulate_real_mode_interrupt(EMM_INT, &regs);
  if (regs.h.ah)
    return -1;
  return 0;
}
