#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dpmi.h>
#include <sys/segments.h>
#include "sigcontext.h"
#include "entry.h"
#include "calls.h"
#include "desc.h"
#include "startup.h"

static int current_client;
#define MAX_CLIENTS 32
struct clnt {
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
    __dpmi_paddr old_int21;
};
static struct clnt sr[MAX_CLIENTS];

#define PRINTF(n) __attribute__((format(printf, n, n + 1)))

#if 0
static void dos_crlf(char *msg, int len)
{
  char *p;
  p = msg;
  while ((p = strchr(p, '\n'))) {
    int l = strlen(p) + 1;
    if (p - msg + l + 1 > len)
      return;
    memmove(p + 1, p, l);
    *p = '\r';
    p += 2;
  }
}

PRINTF(1)
static int dos_printf(const char *format, ...)
{
  char msg[1024];
  va_list args;
  int ret;
  va_start(args, format);
  ret = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  dos_crlf(msg, sizeof(msg));
  strcat(msg, "$");
  asm volatile("int $0x21\n" :: "a"(0x900), "d"(msg) : "cc");
  return ret;
}
#endif

PRINTF(1)
static int emu_printf(const char *format, ...)
{
  char msg[1024];
  va_list args;
  int ret;
  va_start(args, format);
  ret = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  if (ret && msg[ret - 1] == '\n')
    msg[ret - 1] = '\0';
  asm volatile("int $0xe6\n" :: "a"(0x13), "d"(msg) : "cc");
  return ret;
}

static void load_fs_gs(unsigned short handle)
{
  if (have_fs)
    asm volatile ("movl %0, %%fs\n" :: "a"((unsigned long)sr[handle].fs));
  if (have_gs)
    asm volatile ("movl %0, %%gs\n":: "a"((unsigned long)sr[handle].gs));
}

void int21_handler(struct sigcontext *scp)
{
  load_fs_gs(current_client);
  if (clnt_is_32) {
    __dpmi_paddr *old_int21 = &sr[current_client].old_int21;
    emu_printf("call %x:%lx\n", old_int21->selector, old_int21->offset32);
    do_pm_int_call32(scp, old_int21);
  } else {
    __dpmi_raddr addr16;
    addr16.offset16 = sr[current_client].old_int21.offset32;
    addr16.segment = sr[current_client].old_int21.selector;
    do_pm_int_call16(scp, &addr16);
  }
}

static void done(unsigned short handle, short prev)
{
  load_fs_gs(handle);
  __dpmi_set_protected_mode_interrupt_vector(0x21,
      &sr[current_client].old_int21);
  if (have_fs)
    __dpmi_free_ldt_descriptor(sr[handle].fs);
  if (have_gs)
    __dpmi_free_ldt_descriptor(sr[handle].gs);

  if (prev != -1) {
    load_fs_gs(prev);
    dseg32 = sr[prev].ds;
    current_client = prev;
  }
}

void entry(unsigned short term, unsigned short handle, short prev)
{
  __dpmi_paddr addr;

  emu_printf("entry %i %i %i\n", term, handle, prev);
  if (term)
    return done(handle, prev);

  if (have_fs) {
    if ((sr[handle].fs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(sr[handle].fs, fs_desc) == -1) {
      return;
    }
  }
  if (have_gs) {
    if ((sr[handle].gs = __dpmi_allocate_ldt_descriptors(1)) == -1) {
      return;
    }
    if (__dpmi_set_descriptor(sr[handle].gs, gs_desc) == -1) {
      return;
    }
  }
  load_fs_gs(handle);
  sr[handle].ds = _my_ds();
  dseg32 = sr[handle].ds;
  current_client = handle;

  addr.selector = _my_cs();
  if (clnt_is_32)
    addr.offset32 = (unsigned long)dos32_int21;
  else
    addr.offset32 = (unsigned long)dos16_int21;
  if (__dpmi_get_protected_mode_interrupt_vector(0x21,
      &sr[current_client].old_int21) == -1) {
    return;
  }
  emu_printf("old %x:%lx new %x:%lx\n",
      sr[current_client].old_int21.selector,
      sr[current_client].old_int21.offset32,
      addr.selector, addr.offset32);
  emu_printf("my_ds: 0x%x my_cs: 0x%x\n", _my_ds(), _my_cs());
  if (__dpmi_set_protected_mode_interrupt_vector(0x21, &addr) == -1) {
    return;
  }
}
