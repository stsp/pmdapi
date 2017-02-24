#include <dpmi.h>
#include "sigcontext.h"
#include "entry.h"
#include "calls.h"

#define LOAD_REG(b, src, dreg) \
  asm volatile("mov %%cs:%0, "dreg"\n" \
    :: "m"((b).src))

#define LOAD_FLAGS(f) \
  asm volatile(" \
    pushl %%cs:%0\n \
    popfl\n \
    ":: "m"(f))

#define SAVE_STACK(b) \
  SAVE_REG(b, ss, "%%ss"); \
  SAVE_REG(b, esp_at_signal, "%%esp")

#define SWITCH_STACK(s) \
  asm volatile(" \
    lss %%cs:%0, %%esp\n \
    ":: "m"(s))

#define LOAD_CONTEXT(b) \
  LOAD_REG(b, ds, "%%ds"); \
  LOAD_REG(b, es, "%%es"); \
  LOAD_REG(b, fs, "%%fs"); \
  LOAD_REG(b, gs, "%%gs"); \
  LOAD_FLAGS(b.eflags); \
  LOAD_REG(b, ebp, "%%ebp"); \
  LOAD_REG(b, ecx, "%%ecx"); \
  LOAD_REG(b, edx, "%%edx"); \
  LOAD_REG(b, ebx, "%%ebx"); \
  LOAD_REG(b, esi, "%%esi"); \
  LOAD_REG(b, edi, "%%edi"); \
  LOAD_REG(b, eax, "%%eax")

#define SAVE_REG(b, dst, reg) \
  asm volatile("mov "reg", %%ss:%0\n" \
    :: "m"((b).dst))

#define SAVE_FLAGS(f) \
  asm volatile(" \
    pushfl\n \
    popl %%ss:%0\n \
    ":: "m"(f))

#define SAVE_CONTEXT(b) \
  SAVE_REG(b, ds, "%%ds"); \
  SAVE_REG(b, es, "%%es"); \
  SAVE_REG(b, fs, "%%fs"); \
  SAVE_REG(b, gs, "%%gs"); \
  SAVE_FLAGS(b.eflags); \
  SAVE_REG(b, ebp, "%%ebp"); \
  SAVE_REG(b, ecx, "%%ecx"); \
  SAVE_REG(b, edx, "%%edx"); \
  SAVE_REG(b, ebx, "%%ebx"); \
  SAVE_REG(b, esi, "%%esi"); \
  SAVE_REG(b, edi, "%%edi"); \
  SAVE_REG(b, eax, "%%eax")

static struct sigcontext saved_context, dos_context;

void do_pm_int_call32(struct sigcontext *scp, __dpmi_paddr *addr)
{
  static __dpmi_paddr saddr;
  dos_context = *scp;
  saddr = *addr;
  SAVE_CONTEXT(saved_context);
  SAVE_STACK(saved_context);
  LOAD_CONTEXT(dos_context);
  SWITCH_STACK(dos_context.esp_at_signal);
  asm volatile (
    "pushfl\n"
    "lcall *%%cs:%0\n"
    :: "m"(saddr)
  );
  SWITCH_STACK(saved_context.esp_at_signal);
  SAVE_CONTEXT(dos_context);
  LOAD_CONTEXT(saved_context);
  *scp = dos_context;
}

void do_pm_int_call16(struct sigcontext *scp, __dpmi_raddr *addr)
{
  static __dpmi_raddr saddr;
  dos_context = *scp;
  saddr = *addr;
  SAVE_CONTEXT(saved_context);
  SAVE_STACK(saved_context);
  LOAD_CONTEXT(dos_context);
  SWITCH_STACK(dos_context.esp_at_signal);
  asm volatile (
    "pushfw\n"
    "data16 lcall *%%cs:%0\n"
    :: "m"(saddr)
  );
  SWITCH_STACK(saved_context.esp_at_signal);
  SAVE_CONTEXT(dos_context);
  LOAD_CONTEXT(saved_context);
  *scp = dos_context;
}

void do_pm_call(struct sigcontext *scp)
{
  if (clnt_is_32) {
    __dpmi_paddr addr = { .selector = scp->cs, .offset32 = scp->eip };
    do_pm_int_call32(scp, &addr);
  } else {
    __dpmi_raddr addr16 = { .segment = scp->cs, .offset16 = scp->eip };
    do_pm_int_call16(scp, &addr16);
  }
}

void do_rm_int(int inum, __dpmi_regs *regs)
{
  asm (
    "movw $0x300, %%ax\n"
    "movw $0, %%cx\n"
    "pushw %%es\n"
    "movw %1, %%es\n"
    "int $0x31\n"
    "popw %%es\n"
  :: "b"(inum), "r"(dseg32), "D"(regs)
  : "ax", "cx");
}
