/*
 * The part of dosemu2's cpu.h that the msdos plugin uses, for a DPMI
 * client. cpuctx_t is the same struct pm_regs dosemu2 has, and entry.S
 * builds one from the registers of whoever entered our stubs.
 */
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <dos.h>
#include "memory.h"

typedef unsigned char      u_char;
typedef unsigned short     u_short;
typedef unsigned int       u_int;
typedef unsigned long      u_long;
typedef int                Boolean;
typedef uint8_t            Bit8u;   /* type of 8 bit unsigned quantity */
typedef  int8_t            Bit8s;   /* type of 8 bit signed quantity */
typedef uint16_t           Bit16u;  /* type of 16 bit unsigned quantity */
typedef   int16_t          Bit16s;  /* type of 16 bit signed quantity */
typedef uint32_t           Bit32u;  /* type of 32 bit unsigned quantity */
typedef  int32_t           Bit32s;  /* type of 32 bit signed quantity */
typedef uint64_t           Bit64u;  /* type of 64 bit unsigned quantity */
typedef  int64_t           Bit64s;  /* type of 64 bit signed quantity */
typedef unsigned           Bitu;
typedef int                Bits;

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#define MAY_ALIAS __attribute__((may_alias))

union dword {
  Bit32u d;
  struct { Bit16u l, h; } w;
  unsigned long ul;
  struct { Bit8u l, h, b2, b3; } b;
} MAY_ALIAS;

union word {
  Bit16u w;
  struct { Bit8u l, h; } b;
} MAY_ALIAS;

#define LO_WORD_(wrd, c)	(((c union dword *)&(wrd))->w.l)
#define HI_WORD_(wrd, c)	(((c union dword *)&(wrd))->w.h)
#define LO_WORD(wrd)		LO_WORD_(wrd,)
#define HI_WORD(wrd)		HI_WORD_(wrd,)
#define LO_BYTE_(wrd, c)	(((c union word *)&(wrd))->b.l)
#define HI_BYTE_(wrd, c)	(((c union word *)&(wrd))->b.h)
#define LO_BYTE(wrd)		LO_BYTE_(wrd,)
#define HI_BYTE(wrd)		HI_BYTE_(wrd,)
#define LO_BYTE_c(wrd)		LO_BYTE_(wrd, const)
#define HI_BYTE_c(wrd)		HI_BYTE_(wrd, const)
#define LO_BYTE_d(wrd)	(((union dword *)&(wrd))->b.l)
#define HI_BYTE_d(wrd)	(((union dword *)&(wrd))->b.h)
#define LO_BYTE_dc(wrd)	(((const union dword *)&(wrd))->b.l)
#define HI_BYTE_dc(wrd)	(((const union dword *)&(wrd))->b.h)

#define _LO(reg) LO_BYTE_d(_##e##reg)
#define _HI(reg) HI_BYTE_d(_##e##reg)
#define _LO_(reg) LO_BYTE_dc(_##e##reg##_)
#define _HI_(reg) HI_BYTE_dc(_##e##reg##_)
#define _LWORD(reg)	LO_WORD(_##reg)
#define _HWORD(reg)	HI_WORD(_##reg)
#define _LWORD_(reg)	((unsigned)LO_WORD_(_##reg, const))
#define _HWORD_(reg)	((unsigned)HI_WORD_(_##reg, const))

#define SEGOFF2LINEAR(seg, off)  ((((unsigned)(seg)) << 4) + (off))
#define SEG2UNIX(seg)		LINEAR2UNIX(SEGOFF2LINEAR(seg, 0))

typedef unsigned int FAR_PTR;	/* non-normalized seg:off 32 bit DOS pointer */
typedef struct {
  Bit16u offset;
  Bit16u segment;
} far_t;
#define FAR_NULL (far_t){0,0}
#define MK_FP16(s,o)		((((unsigned int)(s)) << 16) | ((o) & 0xffff))
#define MK_FP(f)		MK_FP16(f.segment, f.offset)
#define FP_OFF16(far_ptr)	((far_ptr) & 0xffff)
#define FP_SEG16(far_ptr)	(((far_ptr) >> 16) & 0xffff)
#define MK_FP32(s,o)		LINEAR2UNIX(SEGOFF2LINEAR(s,o))
#define FP_OFF32(linear)	((linear) & 15)
#define FP_SEG32(linear)	(((linear) >> 4) & 0xffff)
#define rFAR_PTR(type,far_ptr) ((type)((FP_SEG16(far_ptr) << 4)+(FP_OFF16(far_ptr))))
#define FARt_PTR(f_t_ptr) (MK_FP32((f_t_ptr).segment, (f_t_ptr).offset))
#define MK_FARt(seg, off) ((far_t){(off), (seg)})

#define peek(seg, off)	(READ_WORD(SEGOFF2LINEAR(seg, off)))

#define pushw(base, ptr, val) \
	do { \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, (val) >> 8); \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, val); \
	} while(0)

#define popb(base, ptr) \
	({ \
		Bit8u __res = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res; \
	})

#define popw(base, ptr) \
	({ \
		Bit8u __res0, __res1; \
		__res0 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res1 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		(__res1 << 8) | __res0; \
	})

#define TF_MASK		0x00000100
#define IF_MASK		0x00000200
#define IOPL_MASK	0x00003000
#define IOPL_SHIFT	12
#define NT_MASK		0x00004000
#define VM_MASK		0x00020000
#define AC_MASK		0x00040000
#define VIF_MASK	0x00080000	/* virtual interrupt flag */
#define VIP_MASK	0x00100000	/* virtual interrupt pending */
#define ID_MASK		0x00200000

/* flags */
#define CF  (1 <<  0)
#define PF  (1 <<  2)
#define AF  (1 <<  4)
#define ZF  (1 <<  6)
#define SF  (1 <<  7)
#define TF  TF_MASK	/* (1 <<  8) */
#define IF  IF_MASK	/* (1 <<  9) */
#define DF  (1 << 10)
#define OF  (1 << 11)
#define NT  NT_MASK	/* (1 << 14) */
#define RF  (1 << 16)
#define VM  VM_MASK	/* (1 << 17) */
#define AC  AC_MASK	/* (1 << 18) */
#define VIF VIF_MASK
#define VIP VIP_MASK
#define ID  ID_MASK

#define OP_IRET			0xcf

struct pm_regs {
	unsigned ebx;
	unsigned ecx;
	unsigned edx;
	unsigned esi;
	unsigned edi;
	unsigned ebp;
	unsigned eax;
	unsigned eip;
	unsigned short cs;
	unsigned eflags;
	unsigned esp;
	unsigned short ss;
	unsigned short es;
	unsigned short ds;
	unsigned short fs;
	unsigned short gs;

	unsigned trapno;
	unsigned err;
	dosaddr_t cr2;
};
typedef struct pm_regs cpuctx_t;
#define REGS_SIZE offsetof(struct pm_regs, trapno)

#define _es     (scp->es)
#define _ds     (scp->ds)
#define _es_    (scp->es)
#define _ds_    (scp->ds)
#define get_edi(s)    ((s)->edi)
#define get_esi(s)    ((s)->esi)
#define get_ebp(s)    ((s)->ebp)
#define get_esp(s)    ((s)->esp)
#define get_ebx(s)    ((s)->ebx)
#define get_edx(s)    ((s)->edx)
#define get_ecx(s)    ((s)->ecx)
#define get_eax(s)    ((s)->eax)
#define get_eip(s)    ((s)->eip)
#define get_eflags(s) ((s)->eflags)
#define get_es(s)     ((s)->es)
#define get_ds(s)     ((s)->ds)
#define get_ss(s)     ((s)->ss)
#define get_fs(s)     ((s)->fs)
#define get_gs(s)     ((s)->gs)
#define get_cs(s)     ((s)->cs)
#define get_trapno(s) ((s)->trapno)
#define get_err(s)    ((s)->err)
#define get_cr2(s)    ((s)->cr2)
#define _edi    get_edi(scp)
#define _esi    get_esi(scp)
#define _ebp    get_ebp(scp)
#define _esp    get_esp(scp)
#define _ebx    get_ebx(scp)
#define _edx    get_edx(scp)
#define _ecx    get_ecx(scp)
#define _eax    get_eax(scp)
#define _eip    get_eip(scp)
#define _edi_   (scp->edi)
#define _esi_   (scp->esi)
#define _ebp_   (scp->ebp)
#define _esp_   (scp->esp)
#define _ebx_   (scp->ebx)
#define _edx_   (scp->edx)
#define _ecx_   (scp->ecx)
#define _eax_   (scp->eax)
#define _eip_   (scp->eip)
#define _cs     (scp->cs)
#define _cs_    (scp->cs)
#define _gs     (scp->gs)
#define _fs     (scp->fs)
#define _ss     (scp->ss)
#define _err    (scp->err)
#define _eflags (scp->eflags)
#define _eflags_ (scp->eflags)
#define _cr2    (scp->cr2)
#define _trapno (scp->trapno)

#endif
