/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

/*
 * Purpose: glue between dosemu2's msdos.c and a real DPMI host.
 *
 * dosemu2 runs its msdos helpers as coopthreads that it enters from
 * hlt instructions in its own code segment. Here the plugin code is a
 * DPMI client of its own: the client enters it through the stubs in
 * entry.S, and a helper that needs real mode just makes a DPMI call and
 * waits for it, so what dosemu2 does with threads is a plain call here.
 *
 * Every stub has a continuation stub: it is what dosemu2 has after the
 * hlt (a lret or an iret), and a handler that leaves cs:eip on it gets
 * there. Continuations and stubs reached this way are processed right
 * here, without going back to the client, see pmdapi_entry().
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dpmi.h>
#include <sys/segments.h>
#include "emudpmi.h"
#include "dpmi_api.h"
#include "cpu.h"
#include "entry.h"
#include "msdoshlp.h"
#include "pmdapi.h"

#define MAX_CBKS 3
#define MAX_EXT 32
#define MAX_HLP 16
enum {
    ID_FAULT, ID_PAGEFAULT, ID_API, ID_WINOS2, ID_LDT16, ID_LDT32,
    ID_INT31,
    ID_RMCB_CALL0, ID_RMCB_CALL1, ID_RMCB_CALL2,
    ID_RMCB_RET0, ID_RMCB_RET1, ID_RMCB_RET2,
    ID_EXT0,
    ID_HLP0 = ID_EXT0 + MAX_EXT,
    ID_MAX = ID_HLP0 + MAX_HLP,
    /* the continuation of stub n is stub CONT_BASE + n */
    CONT_BASE = 64,
};
static_assert(ID_MAX <= CONT_BASE, "too many stubs");
static_assert(ID_INT31 == ID_INT31_STUB, "int31_entry jumps to the wrong stub");
static_assert(CONT_BASE * 2 <= STUB_NUM, "too many stubs");
#define ID_RSP16 -1
#define ID_RSP32 -2

enum { C_NONE, C_RETF, C_RETF16, C_RETF32, C_IRET, C_IRET_EXT };

struct msdos_ops {
    void (*fault)(cpuctx_t *scp, void *arg);
    void *fault_arg;
    void (*pagefault)(cpuctx_t *scp, void *arg);
    void *pagefault_arg;
    void (*api_call)(cpuctx_t *scp, void *arg);
    void *api_arg;
    void (*api_winos2_call)(cpuctx_t *scp, void *arg);
    void *api_winos2_arg;
    void (*ldt_update_call16)(cpuctx_t *scp, void *arg);
    void (*ldt_update_call32)(cpuctx_t *scp, void *arg);
    void (*rsp_call16)(cpuctx_t *scp, void *arg);
    void (*rsp_call32)(cpuctx_t *scp, void *arg);
    struct pmrm_ret (*ext_call)(cpuctx_t *scp,
	struct RealModeCallStructure *rmreg, unsigned short rm_seg,
	void *(*arg)(int), int off);
    void *(*ext_arg)(int);
    struct pext_ret (*ext_ret)(cpuctx_t *scp,
	const struct RealModeCallStructure *rmreg, unsigned short rm_seg,
	int off);
    void (*rmcb_handler[MAX_CBKS])(cpuctx_t *scp,
	const struct RealModeCallStructure *rmreg, int is_32, void *arg);
    void *rmcb_arg[MAX_CBKS];
    void (*rmcb_ret_handler[MAX_CBKS])(cpuctx_t *scp,
	struct RealModeCallStructure *rmreg, int is_32);
    int (*is_32)(void);
    u_short cb_es;
    u_int cb_edi;
};
static struct msdos_ops msdos;

static const struct msdos_ldt_ops *msdos_ldt;

static struct dos_helper_s ext_helper;
struct hlp_s {
    void (*thr)(void *);
    void (*post)(cpuctx_t *);
};
static struct hlp_s hlps[MAX_HLP];
static int num_hlps;

/* the real mode helpers, copied to DOS memory */
extern char rm_stubs[], rm_stubs_end[], rm_s_r[], rm_s_r_len[];
extern char rm_exec[], rm_term[];
extern char quit_stub[];
static unsigned short rm_stubs_seg;
#define RM_OFF(x) ((x) - rm_stubs)

/* state of one pass through pmdapi_entry() */
struct loop_s {
    unsigned entry_flags;
    int post_push;
    unsigned post_arg;
};

static unsigned stub_off(int id)
{
    return (uintptr_t)stubs + id * STUB_SIZE;
}

static int stub_id(unsigned eip)
{
    unsigned off = eip - (uintptr_t)stubs;
    if (eip < (uintptr_t)stubs || off >= STUB_NUM * STUB_SIZE ||
	    (off % STUB_SIZE))
	return -1;
    return off / STUB_SIZE;
}

static int cont_kind(int id)
{
    switch (id) {
    case ID_FAULT:
    case ID_PAGEFAULT:
    case ID_API:
    case ID_WINOS2:
	return C_RETF;
    case ID_LDT16:
	return C_RETF16;
    case ID_LDT32:
	return C_RETF32;
    case ID_RMCB_RET0 ... ID_RMCB_RET2:
	return C_IRET;
    case ID_EXT0 ... ID_EXT0 + MAX_EXT - 1:
    case ID_INT31:
	return C_IRET_EXT;
    }
    return C_NONE;
}

/* the size of the stack is that of ss, not of the client */
static void *stk_adr(cpuctx_t *scp)
{
    return SEL_ADR(_ss, _esp);
}

static void stk_add(cpuctx_t *scp, int delta)
{
    if (dpmi_segment_is32(_ss))
	_esp += delta;
    else
	_LWORD(esp) += delta;
}

static void do_retf_x(cpuctx_t *scp, int is_32)
{
    void *sp = stk_adr(scp);
    if (is_32) {
	unsigned int *ssp = sp;
	_eip = *ssp++;
	_cs = *ssp++;
	stk_add(scp, 8);
    } else {
	unsigned short *ssp = sp;
	_eip = *ssp++;
	_cs = *ssp++;
	stk_add(scp, 4);
    }
}

static void do_retf(cpuctx_t *scp)
{
    do_retf_x(scp, msdos.is_32());
}

static void do_dpmi_iret(cpuctx_t *scp)
{
    int is_32 = msdos.is_32();
    void *sp = stk_adr(scp);
    if (is_32) {
	unsigned int *ssp = sp;
	_eip = *ssp++;
	_cs = *ssp++;
	_eflags = dpmi_flags_from_stack_iret(scp, *ssp++);
	stk_add(scp, 12);
    } else {
	unsigned short *ssp = sp;
	_eip = *ssp++;
	_cs = *ssp++;
	_eflags = dpmi_flags_from_stack_iret(scp, *ssp++);
	stk_add(scp, 6);
    }
}

/* return from an interrupt the ext helper has served: the flags are the
 * ones the helper leaves, as the iret frame dosemu2 builds for its thread
 * carries them */
static void do_ext_iret(cpuctx_t *scp, struct loop_s *l)
{
    unsigned flags = _eflags;
    int is_32 = msdos.is_32();

    _eflags = l->entry_flags;
    do_retf_x(scp, is_32);
    stk_add(scp, is_32 ? 4 : 2);
    _eflags = dpmi_flags_from_stack_iret(scp, flags);
    if (l->post_push) {
	l->post_push = 0;
	if (is_32) {
	    stk_add(scp, -4);
	    *(uint32_t *)stk_adr(scp) = l->post_arg;
	} else {
	    stk_add(scp, -2);
	    *(uint16_t *)stk_adr(scp) = l->post_arg;
	}
    }
}

static void do_callf(cpuctx_t *scp, struct pmaddr_s pma)
{
    int is_32 = msdos.is_32();
    if (is_32) {
	unsigned int *ssp;
	stk_add(scp, -8);
	ssp = stk_adr(scp);
	ssp[0] = _eip;
	ssp[1] = _cs;
    } else {
	unsigned short *ssp;
	stk_add(scp, -4);
	ssp = stk_adr(scp);
	ssp[0] = _LWORD(eip);
	ssp[1] = _cs;
    }
    _cs = pma.selector;
    _eip = pma.offset;
}

struct pmaddr_s doshlp_get_entry(unsigned entry)
{
    struct pmaddr_s ret = {
	    .offset = entry,
	    .selector = dpmi_sel(),
	};
    return ret;
}

struct pmaddr_s doshlp_get_entry16(unsigned entry)
{
    return doshlp_get_entry(entry);
}

struct pmaddr_s doshlp_get_entry32(unsigned entry)
{
    return doshlp_get_entry(entry);
}

void doshlp_setup(struct dos_helper_s *h, const char *name,
	void (*thr)(void *), void (*post)(cpuctx_t *))
{
    assert(num_hlps < MAX_HLP);
    hlps[num_hlps].thr = thr;
    hlps[num_hlps].post = post;
    h->tid = ID_HLP0 + num_hlps;
    h->entry = stub_off(h->tid);
    num_hlps++;
}

void doshlp_setup_retf(struct dos_helper_s *h, const char *name,
	void (*thr)(void *),
	unsigned short (*rm_seg)(cpuctx_t *, int, void *),
	void *rm_arg)
{
    doshlp_setup(h, name, thr, do_retf);
    h->rm_seg = rm_seg;
    h->rm_arg = rm_arg;
}

struct pmaddr_s get_pmcb_handler(void (*handler)(cpuctx_t *,
	const struct RealModeCallStructure *, int, void *),
	void *arg,
	void (*ret_handler)(cpuctx_t *,
	struct RealModeCallStructure *, int),
	int num)
{
    struct pmaddr_s ret;
    assert(num < MAX_CBKS);
    msdos.rmcb_handler[num] = handler;
    msdos.rmcb_arg[num] = arg;
    msdos.rmcb_ret_handler[num] = ret_handler;
    ret.selector = dpmi_sel();
    ret.offset = stub_off(ID_RMCB_CALL0 + num);
    return ret;
}

struct pmaddr_s get_pm_handler(enum MsdOpIds id,
	void (*handler)(cpuctx_t *, void *), void *arg)
{
    struct pmaddr_s ret = { .selector = dpmi_sel() };
    switch (id) {
    case MSDOS_FAULT:
	msdos.fault = handler;
	msdos.fault_arg = arg;
	ret.offset = stub_off(ID_FAULT);
	break;
    case MSDOS_PAGEFAULT:
	msdos.pagefault = handler;
	msdos.pagefault_arg = arg;
	ret.offset = stub_off(ID_PAGEFAULT);
	break;
    case API_CALL:
	msdos.api_call = handler;
	msdos.api_arg = arg;
	ret.offset = stub_off(ID_API);
	break;
    case API_WINOS2_CALL:
	msdos.api_winos2_call = handler;
	msdos.api_winos2_arg = arg;
	ret.offset = stub_off(ID_WINOS2);
	break;
    case MSDOS_LDT_CALL16:
	msdos.ldt_update_call16 = handler;
	ret.offset = stub_off(ID_LDT16);
	break;
    case MSDOS_LDT_CALL32:
	msdos.ldt_update_call32 = handler;
	ret.offset = stub_off(ID_LDT32);
	break;
    case MSDOS_RSP_CALL16:
	msdos.rsp_call16 = handler;
	ret.offset = (uintptr_t)rsp_stub16;
	break;
    case MSDOS_RSP_CALL32:
	msdos.rsp_call32 = handler;
	ret.offset = (uintptr_t)rsp_stub32;
	break;
    default:
	dosemu_error("unknown pm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

struct pmaddr_s get_pmrm_handler_m(enum MsdOpIds id,
	struct pmrm_ret (*handler)(
	cpuctx_t *, struct RealModeCallStructure *,
	unsigned short, void *(*)(int), int),
	void *(*arg)(int),
	struct pext_ret (*ret_handler)(
	cpuctx_t *, const struct RealModeCallStructure *,
	unsigned short, int),
	unsigned short (*rm_seg)(cpuctx_t *, int, void *),
	void *rm_arg, int len, int r_offs[])
{
    struct dos_helper_s *h;
    struct pmaddr_s ret;
    int i;

    switch (id) {
    case MSDOS_EXT_CALL:
	assert(len <= MAX_EXT);
	msdos.ext_call = handler;
	msdos.ext_arg = arg;
	msdos.ext_ret = ret_handler;
	h = &ext_helper;
	h->rm_seg = rm_seg;
	h->rm_arg = rm_arg;
	for (i = 0; i < len; i++)
	    r_offs[i] = i * STUB_SIZE;
	ret = doshlp_get_entry(stub_off(ID_EXT0));
	break;
    default:
	dosemu_error("unknown pmrm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

far_t get_exec_helper(void)
{
    struct pmaddr_s pma;
    far_t s_r;
    int len = DPMI_get_save_restore_address(&s_r, &pma);
    unsigned base = SEGOFF2LINEAR(rm_stubs_seg, 0);

    WRITE_WORD(base + RM_OFF(rm_s_r), s_r.offset);
    WRITE_WORD(base + RM_OFF(rm_s_r) + 2, s_r.segment);
    WRITE_WORD(base + RM_OFF(rm_s_r_len), len);
    return MK_FARt(rm_stubs_seg, RM_OFF(rm_exec));
}

far_t get_term_helper(void)
{
    return MK_FARt(rm_stubs_seg, RM_OFF(rm_term));
}

static void run_call_handler(int idx, cpuctx_t *scp)
{
    int is_32 = msdos.is_32();
    struct RealModeCallStructure *rmreg =
	    SEL_ADR_CLNT(_es, _edi, is_32);
    msdos.cb_es = _es;
    msdos.cb_edi = _edi;
    msdos.rmcb_handler[idx](scp, rmreg, is_32, msdos.rmcb_arg[idx]);
}

static void run_ret_handler(int idx, cpuctx_t *scp)
{
    int is_32 = msdos.is_32();
    struct RealModeCallStructure *rmreg =
	    SEL_ADR_CLNT(msdos.cb_es, msdos.cb_edi, is_32);
    msdos.rmcb_ret_handler[idx](scp, rmreg, is_32);
    _es = msdos.cb_es;
    _edi = msdos.cb_edi;
}

static void do_int_call(cpuctx_t *scp, int is_32, int num,
	struct RealModeCallStructure *rmreg)
{
    RMREG(ss) = 0;
    RMREG(sp) = 0;
    _dpmi_simulate_real_mode_interrupt(scp, is_32, num, (__dpmi_regs *)rmreg);
}

static void do_int_to(cpuctx_t *scp, int is_32, far_t dst,
		struct RealModeCallStructure *rmreg)
{
    RMREG(ss) = 0;
    RMREG(sp) = 0;
    RMREG(cs) = dst.segment;
    RMREG(ip) = dst.offset;
    _dpmi_simulate_real_mode_procedure_iret(scp, is_32, (__dpmi_regs *)rmreg);
}

static void copy_rest(cpuctx_t *scp, cpuctx_t *src)
{
#define CP_R(r) _##r = get_##r(src)
    CP_R(eax);
    CP_R(ebx);
    CP_R(ecx);
    CP_R(edx);
    CP_R(esi);
    CP_R(edi);
    CP_R(es);
}

static void do_restore(cpuctx_t *scp, cpuctx_t *sa)
{
    /* make sure most things did not change */
#define _CHK(r) assert(_##r == get_##r(sa))
    _CHK(ds);
    _CHK(fs);
    _CHK(gs);
    _CHK(cs);
    _CHK(eip);
    _CHK(ss);
    _CHK(esp);
    _CHK(ebp);

    copy_rest(scp, sa);
}

void doshlp_quit_dpmi(cpuctx_t *scp)
{
    struct pmaddr_s pma = {
	.offset = (uintptr_t)quit_stub,
	.selector = dpmi_sel(),
    };
    _eax = 0x4c01;
    do_callf(scp, pma);
}

struct pmaddr_s doshlp_get_abort_helper(void)
{
    return (struct pmaddr_s){
	.offset = (uintptr_t)quit_stub,
	.selector = dpmi_sel(),
    };
}

void doshlp_call_reinit(cpuctx_t *scp)
{
    /* dosemu2 re-enters its own DPMI server here; a DPMI host has
     * nothing like that, so the client stays as it was */
    error("MSDOS: reinit is not supported under pmdapi\n");
}

static void ext_call(cpuctx_t *scp, int off, struct loop_s *l)
{
    cpuctx_t sa = *scp;
    struct dos_helper_s *hlp = &ext_helper;
    struct RealModeCallStructure rmreg = {};
    unsigned short rm_seg = hlp->rm_seg(scp, off, hlp->rm_arg);
    int is_32 = msdos.is_32();
    struct pmrm_ret ret;
    struct pext_ret pret;

    if (rm_seg == (unsigned short)-1) {
	error("RM seg not set\n");
	doshlp_quit_dpmi(scp);
	return;
    }
    ret = msdos.ext_call(scp, &rmreg, rm_seg, msdos.ext_arg, off);
    switch (ret.ret) {
    case MSDOS_NONE:
    case MSDOS_PM:
	/* chain: the previous handler gets the client's iret frame and
	 * the flags we were entered with */
	_eflags = l->entry_flags;
	_cs = ret.prev.selector;
	_eip = ret.prev.offset32;
	return;
    case MSDOS_RMINT:
	do_int_call(scp, is_32, ret.inum, &rmreg);
	break;
    case MSDOS_RM:
	do_int_to(scp, is_32, ret.faddr, &rmreg);
	break;
    case MSDOS_DONE:
	return;
    }
    do_restore(scp, &sa);
    pret = msdos.ext_ret(scp, &rmreg, rm_seg, off);
    switch (pret.ret) {
    case POSTEXT_NONE:
	break;
    case POSTEXT_PUSH:
	l->post_push = 1;
	l->post_arg = pret.arg;
	break;
    }
}

/*
 * dosemu2 sees a write to the LDT alias before the client does. Here it is
 * a page fault that the host gives to the client's exception handler, so
 * ours has to stay the one the host calls: a client that sets its own #GP
 * or #PF handler gets it recorded as the one ours goes on to, which is
 * what dosemu2 does with the handler it had before.
 */
static DPMI_INTDESC prev_int31[DPMI_MAX_CLIENTS];
static int cur_clnt;

static void int31_call(cpuctx_t *scp, struct loop_s *l)
{
    int is_32 = msdos.is_32();
    int num = _LO(bx);
    DPMI_INTDESC *p;

    if ((_LWORD(eax) == 0x202 || _LWORD(eax) == 0x203) &&
	    (num == 0xd || num == 0xe)) {
	void *(*get)(void) = num == 0xd ? msdos.fault_arg :
		msdos.pagefault_arg;
	p = get();
	if (_LWORD(eax) == 0x202) {
	    _LWORD(ecx) = p->selector;
	    if (is_32)
		_edx = p->offset32;
	    else
		_LWORD(edx) = p->offset32;
	} else {
	    p->selector = _LWORD(ecx);
	    p->offset32 = is_32 ? _edx : _LWORD(edx);
	}
	_eflags &= ~CF;
	return;
    }
    /* the host's own: it gets the client's iret frame as it is */
    p = &prev_int31[cur_clnt];
    _eflags = l->entry_flags;
    _cs = p->selector;
    _eip = p->offset32;
}

static void set_clnt(int clnt)
{
    /* a client from before we were loaded has not got our int 31h, and
     * its own is the host's, as the one we already have */
    if (!prev_int31[clnt].selector)
	return;
    cur_clnt = clnt;
    int31_prev[0] = prev_int31[clnt].offset32;
    int31_prev[1] = prev_int31[clnt].selector;
}

static void hook_int31(int clnt)
{
    DPMI_INTDESC desc = {
	.selector = dpmi_sel(),
	.offset32 = (uintptr_t)int31_entry,
    };
    prev_int31[clnt] = dpmi_get_interrupt_vector(0x31);
    set_clnt(clnt);
    dpmi_set_interrupt_vector(0x31, desc);
}

static void run_stub(cpuctx_t *scp, int id, struct loop_s *l)
{
    if (id >= CONT_BASE) {
	switch (cont_kind(id - CONT_BASE)) {
	case C_RETF:
	    do_retf(scp);
	    break;
	case C_RETF16:
	    do_retf_x(scp, 0);
	    break;
	case C_RETF32:
	    do_retf_x(scp, 1);
	    break;
	case C_IRET:
	    do_dpmi_iret(scp);
	    break;
	case C_IRET_EXT:
	    do_ext_iret(scp, l);
	    break;
	default:
	    error("MSDOS: stray continuation %i\n", id - CONT_BASE);
	    doshlp_quit_dpmi(scp);
	    break;
	}
	return;
    }

    _eip = stub_off(CONT_BASE + id);
    switch (id) {
    case ID_FAULT:
	msdos.fault(scp, msdos.fault_arg);
	break;
    case ID_PAGEFAULT:
	msdos.pagefault(scp, msdos.pagefault_arg);
	break;
    case ID_API:
	msdos.api_call(scp, msdos.api_arg);
	break;
    case ID_WINOS2:
	msdos.api_winos2_call(scp, msdos.api_winos2_arg);
	break;
    case ID_LDT16:
	msdos.ldt_update_call16(scp, NULL);
	break;
    case ID_LDT32:
	msdos.ldt_update_call32(scp, NULL);
	break;
    case ID_INT31:
	int31_call(scp, l);
	break;
    case ID_RMCB_CALL0 ... ID_RMCB_CALL2:
	/* dosemu2 has the ret hlt right after the call one */
	_eip = stub_off(id - ID_RMCB_CALL0 + ID_RMCB_RET0);
	run_call_handler(id - ID_RMCB_CALL0, scp);
	break;
    case ID_RMCB_RET0 ... ID_RMCB_RET2:
	run_ret_handler(id - ID_RMCB_RET0, scp);
	break;
    case ID_EXT0 ... ID_EXT0 + MAX_EXT - 1:
	ext_call(scp, id - ID_EXT0, l);
	break;
    case ID_HLP0 ... ID_HLP0 + MAX_HLP - 1: {
	struct hlp_s *h = &hlps[id - ID_HLP0];
	if (id - ID_HLP0 >= num_hlps)
	    goto bad;
	h->thr(scp);
	if (h->post)
	    h->post(scp);
	break;
    }
    default:
    bad:
	error("MSDOS: unknown pm call %#x\n", _eip);
	doshlp_quit_dpmi(scp);
	break;
    }
}

static void rsp_call(cpuctx_t *scp, int is_32)
{
    unsigned short ds = _ds;
    int prev = (short)_LWORD(ecx);
    int op = _LWORD(eax);

    int clnt = _LWORD(ebx);

    pmdapi_rsp_pre(op, clnt, ds);
    if (is_32)
	msdos.rsp_call32(scp, NULL);
    else
	msdos.rsp_call16(scp, NULL);
    pmdapi_rsp_post(op, clnt, prev);
    switch (clnt >= 0 && clnt < DPMI_MAX_CLIENTS ? op : -1) {
    case 0:
	hook_int31(clnt);
	break;
    case 1:
	if (prev >= 0 && prev < DPMI_MAX_CLIENTS)
	    set_clnt(prev);
	break;
    case 2:
	set_clnt(clnt);
	break;
    }
    do_retf_x(scp, is_32);
}

/* called from entry.S on our own stack; on return the client goes on
 * from f->regs */
void pmdapi_entry(struct entry_frame *f)
{
    cpuctx_t *scp = &f->regs;
    unsigned short my_cs = _my_cs();
    struct loop_s l = { .entry_flags = _eflags };
    unsigned *ssp;
    int id = f->id;

    /* where the client's stack was before the stub */
    _ss = f->ss;
    _esp = f->esp;
    stk_add(scp, 24);
    _cs = my_cs;
    if ((unsigned char *)cur_sp < pmdapi_stack)
	error("MSDOS: pmdapi stack overflow, depth %i\n", STK_DEPTH);

    if (id == ID_RSP16 || id == ID_RSP32) {
	rsp_call(scp, id == ID_RSP32);
    } else {
	_eip = stub_off(id);
	while (_cs == my_cs && (id = stub_id(_eip)) != -1)
	    run_stub(scp, id, &l);
    }

    /* never let NT through: iret would take it for a task return */
    _eflags &= ~(NT_MASK | RF | VM_MASK);
    /* what entry.S pops before iret, on the client's stack */
    stk_add(scp, -20);
    ssp = stk_adr(scp);
    ssp[0] = _ebx;
    ssp[1] = _ds;
    ssp[2] = _eip;
    ssp[3] = _cs;
    ssp[4] = _eflags;
}

void msdoshlp_init(int (*is_32)(void), int len)
{
    int size = rm_stubs_end - rm_stubs;
    int sel;
    int seg;

    msdos.is_32 = is_32;
    assert(len <= MAX_EXT);
    /* DOS memory for the real mode helpers. It belongs to the system,
     * not to us: we stay resident after our DOS process is gone. */
    seg = __dpmi_allocate_dos_memory((size + 15) >> 4, &sel);
    if (seg == -1) {
	error("MSDOS: no DOS memory for the helpers\n");
	return;
    }
    WRITE_WORD(SEGOFF2LINEAR(seg - 1, 1), 8);
    MEMCPY_2DOS(SEGOFF2LINEAR(seg, 0), rm_stubs, size);
    rm_stubs_seg = seg;
}

void msdoshlp_setup(void)
{
}

int doshlp_idle(void)
{
    __dpmi_regs r = {};

    r.x.ax = 0x1680;
    __dpmi_simulate_real_mode_interrupt(0x2f, &r);
    return r.h.al == 0;
}

Bit16u hlt_register_handler_pm(emu_hlt_t handler)
{
    error("MSDOS: no hlt handlers under pmdapi\n");
    return 0;
}

void msdos_register_ops(const struct msdos_ldt_ops *ops)
{
    assert(!msdos_ldt);
    msdos_ldt = ops;
}

void msdos_reset(void)
{
    if (!msdos_ldt)
        return;
    msdos_ldt->reset();
}

int msdos_ldt_access(dosaddr_t cr2)
{
    if (!msdos_ldt)
        return 0;
    return msdos_ldt->access(cr2);
}

void msdos_ldt_write(cpuctx_t *scp, uint32_t op, int len, dosaddr_t cr2)
{
    if (!msdos_ldt)
        return;
    msdos_ldt->write(scp, op, len, cr2);
}

int msdos_ldt_pagefault(cpuctx_t *scp)
{
    if (!msdos_ldt)
        return 0;
    return msdos_ldt->pagefault(scp);
}

const char *msdos_describe_selector(unsigned short sel)
{
    if (!msdos_ldt)
        return NULL;
    return msdos_ldt->describe_selector(sel);
}
