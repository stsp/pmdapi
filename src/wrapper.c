/*
 * dosemu2's DPMI server functions the msdos plugin calls, done with
 * the DPMI API of whatever host we run under.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/segments.h>
#include "emudpmi.h"
#include "emu.h"
#include "entry.h"
#include "handlers.h"
#include "msdos/msdos_priv.h"
#include "pmdapi.h"

unsigned char *mem_base;
struct pmdapi_config config;
int pmdapi_debug;

static unsigned short log_seg;
#define LOG_SIZE 256

struct clnt {
    uint16_t ds;
    uint16_t flat;
};
static struct clnt clients[DPMI_MAX_CLIENTS];

/* Our libc keeps selectors of the client we were loaded in, and a new
 * client has none of them: only the descriptors the host copied from our
 * RSP registration. So every client gets its own flat selector for libc's
 * linear memory accesses, and the DS copies follow our DS. */
extern unsigned short djgpp_ds_alias asm("___djgpp_ds_alias");
extern unsigned short djgpp_our_DS asm("___djgpp_our_DS");
extern unsigned short djgpp_app_DS asm("___djgpp_app_DS");
extern unsigned short djgpp_dos_sel asm("___djgpp_dos_sel");

static void set_ds(const struct clnt *c)
{
    dseg32 = c->ds;
    djgpp_ds_alias = c->ds;
    djgpp_our_DS = c->ds;
    djgpp_app_DS = c->ds;
    djgpp_dos_sel = c->flat;
    _dos_ds = c->flat;
}

static uint16_t alloc_flat(void)
{
    int sel = __dpmi_allocate_ldt_descriptors(1);
    if (sel == -1)
	return 0;
    __dpmi_set_segment_base_address(sel, 0);
    __dpmi_set_segment_limit(sel, 0xffffffff);
    return sel;
}

void pmdapi_rsp_pre(int op, int clnt, uint16_t ds)
{
    struct clnt c;

    if (clnt < 0 || clnt >= DPMI_MAX_CLIENTS)
	return;
    switch (op) {
    case 0:
	c.ds = ds;
	c.flat = alloc_flat();
	clients[clnt] = c;
	set_ds(&clients[clnt]);
	break;
    case 2:
	set_ds(&clients[clnt]);
	break;
    }
}

void pmdapi_rsp_post(int op, int clnt, int prev)
{
    if (op == 1 && prev >= 0 && prev < DPMI_MAX_CLIENTS)
	set_ds(&clients[prev]);
}

/* dosemu2's int e6h/13h takes a string in real mode, so copy it there;
 * on anything but dosemu2 there is no such vector and nothing to log to */
PRINTF(1)
int emu_printf(const char *format, ...)
{
    char msg[1024];
    va_list args;
    __dpmi_regs r = {};
    int ret;

    va_start(args, format);
    ret = vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
    if (!log_seg)
	return ret;
    msg[LOG_SIZE - 1] = '\0';
    MEMCPY_2DOS(SEGOFF2LINEAR(log_seg, 0), msg, strlen(msg) + 1);
    r.x.ax = 0x13;
    r.x.es = log_seg;
    r.x.dx = 0;
    __dpmi_simulate_real_mode_interrupt(0xe6, &r);
    return ret;
}

void wrapper_init(void)
{
    __dpmi_raddr v;
    int sel, seg;

    if (__dpmi_get_real_mode_interrupt_vector(0xe6, &v) == 0 &&
	    (v.segment || v.offset16)) {
	seg = __dpmi_allocate_dos_memory(LOG_SIZE >> 4, &sel);
	if (seg != -1) {
	    /* belongs to the system: we outlive our DOS process */
	    WRITE_WORD(SEGOFF2LINEAR(seg - 1, 1), 8);
	    log_seg = seg;
	}
    }
    if (getenv("PMDAPI_DEBUG"))
	pmdapi_debug = atoi(getenv("PMDAPI_DEBUG"));
}

int tempname(char *tmpl, size_t x_suffix_len)
{
    static const char c[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static unsigned v;
    size_t len = strlen(tmpl);
    size_t i;

    if (len < x_suffix_len)
	return -1;
    v += (unsigned)time(NULL) ^ ((uintptr_t)&v >> 4);
    for (i = len - x_suffix_len; i < len; i++) {
	v = v * 1103515245 + 12345;
	tmpl[i] = c[(v >> 16) % (sizeof(c) - 1)];
    }
    return 0;
}

unsigned short dpmi_sel(void)
{
    /* the host gives each client its own copy of our code segment,
     * and we always run on the one of the current client */
    return _my_cs();
}

unsigned short dpmi_sel16(void)
{
    return dpmi_sel();
}

unsigned short dpmi_sel32(void)
{
    return dpmi_sel();
}

int dpmi_segment_is32(int sel)
{
    unsigned lar;
    int ok;

    asm("larl %2, %0\n"
	"setz %b1\n"
	: "=r"(lar), "=q"(ok) : "rm"(sel) : "cc");
    return (ok & 1) && (lar & 0x400000);
}

const char *DPMI_show_state(cpuctx_t *scp)
{
    static char buf[256];
    snprintf(buf, sizeof(buf),
	"eip: 0x%08x  esp: 0x%08x  eflags: 0x%08x\n"
	"\tcs: 0x%04x  ds: 0x%04x  es: 0x%04x  ss: 0x%04x\n",
	_eip, _esp, _eflags, _cs, _ds, _es, _ss);
    return buf;
}

unsigned dpmi_mem_size(void)
{
    __dpmi_free_mem_info info;
    if (__dpmi_get_free_memory_information(&info))
	return 0;
    return info.total_number_of_physical_pages * DPMI_page_size;
}

unsigned short dpmi_get_private_pool(unsigned short *r_paras)
{
    /* dosemu2's own extension, the host has no such thing to tell us */
    *r_paras = 0;
    return 0;
}

int ValidAndUsedSelector(unsigned int selector)
{
    int lar;
    if (!(selector & 4))
	return 0;
    lar = __dpmi_get_descriptor_access_rights(selector);
    if ((lar & 0x90) != 0x90)
	return 0;
    return 1;
}

int ConvertSegmentToDescriptor(unsigned short segment)
{
    return __dpmi_segment_to_descriptor(segment);
}

int SetSegmentBaseAddress(unsigned short selector, dosaddr_t baseaddr)
{
    return __dpmi_set_segment_base_address(selector, baseaddr);
}

unsigned int GetSegmentBase(unsigned short selector)
{
    ULONG addr;
    if (__dpmi_get_segment_base_address(selector, &addr))
	return 0;
    return addr;
}

unsigned int GetSegmentLimit(unsigned short selector)
{
    return __dpmi_get_segment_limit(selector);
}

int SetSegmentLimit(unsigned short selector, unsigned int limit)
{
    return __dpmi_set_segment_limit(selector, limit);
}

unsigned short AllocateDescriptors(int num)
{
    int sel = __dpmi_allocate_ldt_descriptors(num);
    if (sel == -1)
	return 0;
    return sel;
}

int FreeDescriptor(unsigned short selector)
{
    return __dpmi_free_ldt_descriptor(selector);
}

void FreeSegRegs(cpuctx_t *scp, unsigned short selector)
{
    if ((_ds | 7) == (selector | 7)) _ds = 0;
    if ((_es | 7) == (selector | 7)) _es = 0;
    if ((_fs | 7) == (selector | 7)) _fs = 0;
    if ((_gs | 7) == (selector | 7)) _gs = 0;
}

int GetDescriptor(u_short selector, unsigned int *lp)
{
    return __dpmi_get_descriptor(selector, (void *)lp);
}

int SetDescriptor(unsigned short selector, unsigned int *lp)
{
    return __dpmi_set_descriptor(selector, (void *)lp);
}

int SetDescriptorAccessRights(unsigned short selector, unsigned short acc_rights)
{
    return __dpmi_set_descriptor_access_rights(selector, acc_rights);
}

unsigned short CreateAliasDescriptor(unsigned short selector)
{
    int sel = __dpmi_create_alias_descriptor(selector);
    if (sel == -1)
	return 0;
    return sel;
}

int DPMI_allocate_specific_ldt_descriptor(unsigned short selector)
{
    return __dpmi_allocate_specific_ldt_descriptor(selector);
}

static void *SEL_ADR_LDT(unsigned short sel, unsigned int reg, int is_32)
{
    dosaddr_t p;
    if (is_32)
	p = GetSegmentBase(sel) + reg;
    else
	p = GetSegmentBase(sel) + LO_WORD(reg);
    return LINEAR2UNIX(p);
}

void *SEL_ADR(unsigned short sel, unsigned int reg)
{
    if (!(sel & 0x0004)) {
	/* GDT */
	return (void *)(uintptr_t)reg;
    }
    return SEL_ADR_LDT(sel, reg, dpmi_segment_is32(sel));
}

void *SEL_ADR_CLNT(unsigned short sel, unsigned int reg, int is_32)
{
    if (!(sel & 0x0004)) {
	/* GDT */
	dosemu_error("GDT not allowed\n");
	return (void *)(uintptr_t)reg;
    }
    return SEL_ADR_LDT(sel, reg, is_32);
}

static dpmi_pm_block mk_block(const __dpmi_meminfo *info)
{
    dpmi_pm_block block = {};
    block.base = info->address;
    block.size = info->size;
    block.handle = info->handle;
    return block;
}

dpmi_pm_block DPMImalloc(unsigned size)
{
    __dpmi_meminfo info = {};
    info.size = size;
    if (__dpmi_allocate_memory(&info) == -1)
	info.size = 0;
    return mk_block(&info);
}

int DPMIfree(unsigned handle)
{
    return __dpmi_free_memory(handle);
}

dpmi_pm_block DPMIrealloc(unsigned handle, unsigned size)
{
    __dpmi_meminfo info = {};
    info.handle = handle;
    info.size = size;
    if (__dpmi_resize_memory(&info) == -1)
	info.size = 0;
    return mk_block(&info);
}

int DPMISetPageAttributes(unsigned handle, int offs, u_short attrs[], int count)
{
    __dpmi_meminfo info = { .handle = handle, .size = count, .address = offs };
    return __dpmi_set_page_attributes(&info, (short *)attrs);
}

int DPMIGetPageAttributes(unsigned handle, int offs, u_short attrs[], int count)
{
    __dpmi_meminfo info = { .handle = handle, .size = count, .address = offs };
    return __dpmi_get_page_attributes(&info, (short *)attrs);
}

void GetFreeMemoryInformation(unsigned int *lp)
{
    __dpmi_get_free_memory_information((__dpmi_free_mem_info *)lp);
}

/* djgpp's __dpmi_shminfo has no room for dosemu2's flags */
int DPMIAllocateShared(struct SHM_desc *shm)
{
    int err;
    asm volatile("int $0x31\n"
	"sbbl %0, %0\n"
	: "=a"(err)
	: "a"(0x0d00), "D"(shm)
	: "memory", "cc");
    return err;
}

int DPMIFreeShared(uint32_t handle)
{
    int err;
    asm volatile("int $0x31\n"
	"sbbl %0, %0\n"
	: "=a"(err)
	: "a"(0x0d01), "S"(handle >> 16), "D"(handle & 0xffff)
	: "cc");
    return err;
}

dosaddr_t DPMIMapHWRam(unsigned addr, unsigned size)
{
    __dpmi_meminfo info = { .size = size, .address = addr };
    if (__dpmi_physical_address_mapping(&info))
	return -1;
    return info.address;
}

int DPMIUnmapHWRam(dosaddr_t vbase)
{
    __dpmi_meminfo info = { .address = vbase };
    return __dpmi_free_physical_address_mapping(&info);
}

void dpmi_set_interrupt_vector(unsigned char num, DPMI_INTDESC desc)
{
    __dpmi_paddr addr = { .offset32 = desc.offset32,
	    .selector = desc.selector };
    __dpmi_set_protected_mode_interrupt_vector(num, &addr);
}

DPMI_INTDESC dpmi_get_interrupt_vector(unsigned char num)
{
    __dpmi_paddr addr = {};
    __dpmi_get_protected_mode_interrupt_vector(num, &addr);
    return (DPMI_INTDESC){ .offset32 = addr.offset32,
	    .selector = addr.selector };
}

DPMI_INTDESC dpmi_get_pm_exc_addr(int num)
{
    __dpmi_paddr addr = {};
    __dpmi_get_processor_exception_handler_vector(num, &addr);
    return (DPMI_INTDESC){ .offset32 = addr.offset32,
	    .selector = addr.selector };
}

void dpmi_set_pm_exc_addr(int num, DPMI_INTDESC desc)
{
    __dpmi_paddr addr = { .offset32 = desc.offset32,
	    .selector = desc.selector };
    __dpmi_set_processor_exception_handler_vector(num, &addr);
}

far_t DPMI_get_real_mode_interrupt_vector(int vec)
{
    __dpmi_raddr addr = {};
    __dpmi_get_real_mode_interrupt_vector(vec, &addr);
    return MK_FARt(addr.segment, addr.offset16);
}

far_t DPMI_allocate_realmode_callback(u_short sel, int offs, u_short rm_sel,
	int rm_offs)
{
    __dpmi_raddr ret;
    int err;

    /* djgpp's __dpmi_allocate_real_mode_callback() takes our own cs,
     * so do it by hand */
    asm volatile(
	"pushl %%ds\n"
	"pushl %%es\n"
	"movw %w3, %%ds\n"
	"movw %w5, %%es\n"
	"int $0x31\n"
	"popl %%es\n"
	"popl %%ds\n"
	"sbbl %0, %0\n"
	: "=a"(err), "=c"(ret.segment), "=d"(ret.offset16)
	: "r"((unsigned)sel), "S"(offs), "r"((unsigned)rm_sel), "D"(rm_offs),
	  "a"(0x0303)
	: "memory", "cc");
    if (err)
	return (far_t){ 0, 0 };
    return MK_FARt(ret.segment, ret.offset16);
}

int DPMI_free_realmode_callback(u_short seg, u_short off)
{
    __dpmi_raddr addr = { .segment = seg, .offset16 = off };
    return __dpmi_free_real_mode_callback(&addr);
}

int DPMI_get_save_restore_address(far_t *raddr, struct pmaddr_s *paddr)
{
    __dpmi_raddr rm = {};
    __dpmi_paddr pm = {};
    int len = __dpmi_get_state_save_restore_addr(&rm, &pm);
    if (len == -1)
	return 0;
    raddr->segment = rm.segment;
    raddr->offset = rm.offset16;
    paddr->selector = pm.selector;
    paddr->offset = pm.offset32;
    return len;
}

/* the RSP call gives us our data segment in ds, so the host needs its
 * descriptor, which dosemu2's msdos.c leaves to us */
int dpmi_install_rsp(struct RSPcall_s *callback)
{
    if (__dpmi_get_descriptor(_my_ds(), callback->data16) == -1)
	return -1;
    memcpy(callback->data32, callback->data16, 8);
    return __dpmi_install_resident_service_provider_callback(
	    (__dpmi_callback_info *)callback);
}

/* dosemu2's LDT monitor lives behind its DPMI_API_extension, which
 * returns with a retf of the bitness of the current client */
static int api_ext(unsigned eax, unsigned ebx, unsigned ecx, unsigned edx)
{
    __dpmi_paddr api;
    static const char name[] = "LDT_MONITOR";

    if (__dpmi_get_vendor_specific_api_entry_point((char *)name, &api)) {
	error("MSDOS: the DPMI host has no LDT monitor\n");
	return -1;
    }
    if (msdos_is_32())
	return ldtmon_call32(eax, ebx, ecx, edx, &api);
    return ldtmon_call16(eax, ebx, ecx, edx, &api);
}

void dpmi_ext_set_ldt_monitor16(DPMI_INTDESC call, uint16_t ds)
{
    api_ext(0x100, ds, call.selector, call.offset32);
}

void dpmi_ext_set_ldt_monitor32(DPMI_INTDESC call, uint16_t ds)
{
    api_ext(0x200, ds, call.selector, call.offset32);
}

void dpmi_ext_ldt_monitor_enable(int on)
{
    api_ext(0x300 | (on & 0xff), 0, 0, 0);
}
