#ifndef _ASMi386_SIGCONTEXT_H
#define _ASMi386_SIGCONTEXT_H

#define ULONG unsigned

/*
 * As documented in the iBCS2 standard..
 *
 * The first part of "struct _fpstate" is just the normal i387
 * hardware setup, the extra "status" word is used to save the
 * coprocessor status word before entering the handler.
 *
 * Pentium III FXSR, SSE support
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 *
 * The FPU state data structure has had to grow to accomodate the
 * extended FPU state required by the Streaming SIMD Extensions.
 * There is no documented standard to accomplish this at the moment.
 */
struct _fpreg {
	unsigned short significand[4];
	unsigned short exponent;
};

struct _fpxreg {
	unsigned short significand[4];
	unsigned short exponent;
	unsigned short padding[3];
};

struct _xmmreg {
	ULONG element[4];
};

struct _fpstate {
	/* Regular FPU environment */
	ULONG 	cw;
	ULONG	sw;
	ULONG	tag;
	ULONG	ipoff;
	ULONG	cssel;
	ULONG	dataoff;
	ULONG	datasel;
	struct _fpreg	_st[8];
	unsigned short	status;
	unsigned short	magic;		/* 0xffff = regular FPU data only */

	/* FXSR FPU environment */
	ULONG	_fxsr_env[6];	/* FXSR FPU env is ignored */
	ULONG	mxcsr;
	ULONG	reserved;
	struct _fpxreg	_fxsr_st[8];	/* FXSR FPU reg data is ignored */
	struct _xmmreg	_xmm[8];
	ULONG	padding[56];
};

#define X86_FXSR_MAGIC		0x0000

struct sigcontext {
	unsigned short gs, __gsh;
	unsigned short fs, __fsh;
	unsigned short es, __esh;
	unsigned short ds, __dsh;
	ULONG edi;
	ULONG esi;
	ULONG ebp;
	ULONG esp;
	ULONG ebx;
	ULONG edx;
	ULONG ecx;
	ULONG eax;
	ULONG trapno;
	ULONG err;
	ULONG eip;
	unsigned short cs, __csh;
	ULONG eflags;
	ULONG esp_at_signal;
	unsigned short ss, __ssh;
	struct _fpstate * fpstate;
	ULONG oldmask;
	ULONG cr2;
};

typedef struct sigcontext sigcontext_t;

#endif
