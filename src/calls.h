#ifndef __CALLS_H
#define __CALLS_H

void do_pm_int_call32(struct sigcontext *scp, __dpmi_paddr *addr);
void do_pm_int_call16(struct sigcontext *scp, __dpmi_raddr *addr);
void do_rm_int(int inum, __dpmi_regs *regs);
void do_rm_call(__dpmi_regs *regs);

void do_pm_call(struct sigcontext *scp);

#endif
