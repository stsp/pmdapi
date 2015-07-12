#include <dpmi.h>
#include "sigcontext.h"
#include "calls.h"

void do_pm_int_call32(struct sigcontext *scp, __dpmi_paddr *addr)
{
  static __dpmi_paddr saddr;
  static unsigned long saved_eax;
  static struct sigcontext saved_context;
  saddr = *addr;
  asm volatile (
    "movl %1, %%eax\n"
    "call save_context\n"
    "movl %2, %%eax\n"
    "call load_context\n"
    "pushfl\n"
    "lcall *%%cs:%3\n"
    "movl %%eax, %%ss:%0\n"
    "movl %2, %%eax\n"
    "call save_context\n"
    "movl %1, %%eax\n"
    "call load_context\n"
    : "=m"(saved_eax)
    : "i"(&saved_context), "m"(scp), "m"(saddr)
  );
  scp->eax = saved_eax;
}

void do_pm_int_call16(struct sigcontext *scp, __dpmi_raddr *addr)
{
  static __dpmi_raddr saddr;
  static unsigned long saved_eax;
  static struct sigcontext saved_context;
  saddr = *addr;
  asm volatile (
    "movl %1, %%eax\n"
    "call save_context\n"
    "movl %2, %%eax\n"
    "call load_context\n"
    "pushfw\n"
    "data16 lcall *%%cs:%3\n"
    "movl %%eax, %%ss:%0\n"
    "movl %2, %%eax\n"
    "call save_context\n"
    "movl %1, %%eax\n"
    "call load_context\n"
    : "=m"(saved_eax)
    : "i"(&saved_context), "m"(scp), "m"(saddr)
  );
  scp->eax = saved_eax;
}
