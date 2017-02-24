#include <dpmi.h>
#include <sys/segments.h>
#include "sigcontext.h"
#include "entry.h"
#include "calls.h"
#include "desc.h"
#include "startup.h"

int current_client;
__dpmi_paddr old_int21;
static short fs, gs;

static void load_fs_gs()
{
  if (have_fs)
    asm volatile ("movl %0, %%fs\n" :: "a"((unsigned long)fs));
  if (have_gs)
    asm volatile ("movl %0, %%gs\n":: "a"((unsigned long)gs));
}

void int21_handler(struct sigcontext *scp)
{
  load_fs_gs();
  if (clnt_is_32) {
    do_pm_int_call32(scp, &old_int21);
  } else {
    __dpmi_raddr addr16;
    addr16.offset16 = old_int21.offset32;
    addr16.segment = old_int21.selector;
    do_pm_int_call16(scp, &addr16);
  }
}

static void done(unsigned short handle)
{
  load_fs_gs();
  __dpmi_set_protected_mode_interrupt_vector(0x21, &old_int21);
  if (have_fs)
    __dpmi_free_ldt_descriptor(fs);
  if (have_gs)
    __dpmi_free_ldt_descriptor(gs);
}

void entry(unsigned short term, unsigned short handle)
{
  __dpmi_paddr addr;

  if (term)
    return done(handle);

  if (have_fs) {
    if ((fs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(fs, fs_desc) == -1) {
      return;
    }
  }
  if (have_gs) {
    if ((gs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(gs, gs_desc) == -1) {
      return;
    }
  }
  load_fs_gs();
//  dseg32 = _my_ds();
  current_client = handle;
  addr.selector = _my_cs();
  if (clnt_is_32)
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
