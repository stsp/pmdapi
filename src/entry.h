/*
 * What entry.S and the C code share.
 */
#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>
#include "asm.h"
#include "cpu.h"

/* what the stubs build on our stack for pmdapi_entry() */
struct entry_frame {
    cpuctx_t regs;
    unsigned id;
    unsigned ss;
    unsigned esp;
};
static_assert(offsetof(cpuctx_t, eip) == PM_eip, "PM_eip");
static_assert(offsetof(cpuctx_t, eflags) == PM_eflags, "PM_eflags");
static_assert(offsetof(cpuctx_t, esp) == PM_esp, "PM_esp");
static_assert(offsetof(cpuctx_t, ss) == PM_ss, "PM_ss");
static_assert(offsetof(cpuctx_t, gs) == PM_gs, "PM_gs");
static_assert(offsetof(struct entry_frame, id) == EF_id, "EF_id");
static_assert(offsetof(struct entry_frame, esp) == EF_esp, "EF_esp");
static_assert(sizeof(struct entry_frame) == EF_size, "EF_size");

extern char stubs[], rsp_stub16[], rsp_stub32[], quit_stub[], int31_entry[];
extern uint32_t int31_prev[2];
extern unsigned char pmdapi_stack[];
extern unsigned dseg32, cur_sp;

void pmdapi_entry(struct entry_frame *f);

#endif
