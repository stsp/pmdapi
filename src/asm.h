/*
 * Declarations shared between entry.S and the C code.
 * Plain defines only: entry.S includes it too.
 */
#ifndef PMDAPI_ASM_H
#define PMDAPI_ASM_H

/* Every way into pmdapi from a client goes through one of the stubs:
 * it pushes its number and jumps to common code that saves the client's
 * registers, moves to our own stack and calls pmdapi_entry(). */
#define STUB_SIZE 16
#define STUB_NUM 128
/* the stub for int 31h, see int31_entry */
#define ID_INT31_STUB 6
/* each entry takes this much of our stack, so an entry that comes in
 * while another one waits in a DPMI call finds a fresh piece */
#define STK_CHUNK 0x4000
#define STK_DEPTH 8

/* struct pm_regs, see cpu.h */
#define PM_ebx 0
#define PM_ecx 4
#define PM_edx 8
#define PM_esi 12
#define PM_edi 16
#define PM_ebp 20
#define PM_eax 24
#define PM_eip 28
#define PM_cs 32
#define PM_eflags 36
#define PM_esp 40
#define PM_ss 44
#define PM_es 46
#define PM_ds 48
#define PM_fs 50
#define PM_gs 52
/* struct entry_frame */
#define EF_id 68
#define EF_ss 72
#define EF_esp 76
#define EF_size 80

#endif
