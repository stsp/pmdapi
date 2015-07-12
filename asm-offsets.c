/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed
 * to extract and format the required data.
 */

#include "sigcontext.h"
#include "offsets.h"

DO_OFFSETS(1,
	OFFSET(SIGCONTEXT_ds, sigcontext, ds);
	OFFSET(SIGCONTEXT_es, sigcontext, es);
	OFFSET(SIGCONTEXT_fs, sigcontext, fs);
	OFFSET(SIGCONTEXT_gs, sigcontext, gs);
	OFFSET(SIGCONTEXT_ss, sigcontext, ss);
	OFFSET(SIGCONTEXT_eflags, sigcontext, eflags);
	OFFSET(SIGCONTEXT_eax, sigcontext, eax);
	OFFSET(SIGCONTEXT_ebx, sigcontext, ebx);
	OFFSET(SIGCONTEXT_ecx, sigcontext, ecx);
	OFFSET(SIGCONTEXT_edx, sigcontext, edx);
	OFFSET(SIGCONTEXT_esi, sigcontext, esi);
	OFFSET(SIGCONTEXT_edi, sigcontext, edi);
	OFFSET(SIGCONTEXT_ebp, sigcontext, ebp);
	OFFSET(SIGCONTEXT_esp, sigcontext, esp_at_signal);
	OFFSET(SIGCONTEXT_eip, sigcontext, eip);
)
