#include <dpmi.h>
#include <sys/segments.h>
#include "sigcontext.h"
#include "entry.h"
#include "calls.h"
#include "startup.h"

int current_client;
__dpmi_paddr old_int21;

void int21_handler(struct sigcontext *scp)
{
  __dpmi_raddr addr16;
  if (is_32) {
    do_pm_int_call32(scp, &old_int21);
  } else {
    addr16.offset16 = old_int21.offset32;
    addr16.segment = old_int21.selector;
    do_pm_int_call16(scp, &addr16);
  }
}

void entry(unsigned short term, unsigned short handle)
{
  __dpmi_paddr addr;
  short fs, gs;
  if (have_fs) {
    if ((fs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(fs, fs_desc) == -1) {
      return;
    }
    asm volatile ("movl %0, %%fs\n" :: "a"((unsigned long)fs));
  }
  if (have_gs) {
    if ((gs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(gs, gs_desc) == -1) {
      return;
    }
    asm volatile ("movl %0, %%gs\n":: "a"((unsigned long)gs));
  }
  dseg32 = _my_ds();
  current_client = handle;
  addr.selector = _my_cs();
  if (is_32)
    addr.offset32 = (unsigned long)dos32_int21;
  else
    addr.offset32 = (unsigned long)dos16_int21;
//printf("tst\n");
  if (__dpmi_get_protected_mode_interrupt_vector(0x21, &old_int21) == -1) {
    return;
  }
  if (__dpmi_set_protected_mode_interrupt_vector(0x21, &addr) == -1) {
    return;
  }
}
