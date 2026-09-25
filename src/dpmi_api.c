/*
 * dosemu2's coopthreaded DPMI API: in a DPMI client it is the plain
 * djgpp one, and the call simply waits for real mode.
 */
#include "cpu.h"
#include "dpmi_api.h"

int _dpmi_get_real_mode_interrupt_vector(cpuctx_t *scp, int is_32,
	int _vector, __dpmi_raddr *_address)
{
    return __dpmi_get_real_mode_interrupt_vector(_vector, _address);
}

int _dpmi_simulate_real_mode_interrupt(cpuctx_t *scp, int is_32,
	int _vector, __dpmi_regs *__regs)
{
    return __dpmi_simulate_real_mode_interrupt(_vector, __regs);
}

int _dpmi_simulate_real_mode_procedure_retf(cpuctx_t *scp, int is_32,
	__dpmi_regs *__regs)
{
    return __dpmi_simulate_real_mode_procedure_retf(__regs);
}

int _dpmi_simulate_real_mode_procedure_iret(cpuctx_t *scp, int is_32,
	__dpmi_regs *__regs)
{
    return __dpmi_simulate_real_mode_procedure_iret(__regs);
}
