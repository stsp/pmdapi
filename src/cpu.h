#ifndef CPU_H
#define CPU_H

#include <stdint.h>

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

#define MAY_ALIAS __attribute__((may_alias))

union dword {
  unsigned long l;
  Bit32u d;
#ifdef __x86_64__
  struct { Bit16u l, h, w2, w3; } w;
#else
  struct { Bit16u l, h; } w;
#endif
  struct { Bit8u l, h, b2, b3; } b;
} MAY_ALIAS ;

/* these are used like:  LO(ax) = 2 (sets al to 2) */
#define LO(reg)  (*(unsigned char *)&REG(e##reg))
#define HI(reg)  (*((unsigned char *)&REG(e##reg) + 1))

#define _LO(reg) (*(unsigned char *)&(scp->e##reg))
#define _HI(reg) (*((unsigned char *)&(scp->e##reg) + 1))

#define _LWORD(reg)	(*((unsigned short *)&(scp->reg)))
#define _HWORD(reg)	(*((unsigned short *)&(scp->reg) + 1))

#define LO_WORD(wrd)	(((union dword *)&(wrd))->w.l)
#define HI_WORD(wrd)    (((union dword *)&(wrd))->w.h)

#define LO_BYTE(wrd)	(((union dword *)&(wrd))->b.l)
#define HI_BYTE(wrd)    (((union dword *)&(wrd))->b.h)

#define _gs     (scp->gs)
#define _fs     (scp->fs)
#define _es     (scp->es)
#define _ds     (scp->ds)
#define _edi    (scp->edi)
#define _esi    (scp->esi)
#define _ebp    (scp->ebp)
#define _esp    (scp->esp)
#define _ebx    (scp->ebx)
#define _edx    (scp->edx)
#define _ecx    (scp->ecx)
#define _eax    (scp->eax)
#define _err	(scp->err)
#define _trapno (scp->trapno)
#define _eip    (scp->eip)
#define _cs     (scp->cs)
#define _eflags (scp->eflags)
#define _ss     (scp->ss)
#define _cr2	(scp->cr2)

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

#define get_FLAGS(flags) ({ \
  int __flgs = flags; \
  (((__flgs & VIF) ? __flgs | IF : __flgs & ~IF)); \
})

#define MK_FARt(seg, off) ((far_t){(off), (seg)})
#define SEG2LINEAR(seg)	((void *)  ( ((unsigned int)(seg)) << 4)  )
#define SEGOFF2LINEAR(seg, off)  ((((Bit32u)(seg)) << 4) + (off))

#define MEMCPY_DOS2DOS(dos_addr1, dos_addr2, n) \
	memcpy(LINEAR2UNIX(dos_addr1), LINEAR2UNIX(dos_addr2), (n))
#define UNIX_READ_BYTE(addr)		(*(Bit8u *) (addr))
#define UNIX_WRITE_BYTE(addr, val)	(*(Bit8u *) (addr) = (val) )
#define UNIX_READ_WORD(addr)		(*(Bit16u *) (addr))
#define UNIX_WRITE_WORD(addr, val)	(*(Bit16u *) (addr) = (val) )
#define UNIX_READ_DWORD(addr)		(*(Bit32u *) (addr))
#define UNIX_WRITE_DWORD(addr, val)	(*(Bit32u *) (addr) = (val) )
#define READ_BYTE(addr)		UNIX_READ_BYTE(LINEAR2UNIX(addr))
#define WRITE_BYTE(addr, val)	UNIX_WRITE_BYTE(LINEAR2UNIX(addr), val)
#define READ_WORD(addr)		UNIX_READ_WORD(LINEAR2UNIX(addr))
#define WRITE_WORD(addr, val)	UNIX_WRITE_WORD(LINEAR2UNIX(addr), val)
#define READ_DWORD(addr)	UNIX_READ_DWORD(LINEAR2UNIX(addr))
#define WRITE_DWORD(addr, val)	UNIX_WRITE_DWORD(LINEAR2UNIX(addr), val)
#define READ_BYTEP(addr)	READ_BYTE(DOSADDR_REL(addr))
#define WRITE_BYTEP(addr, val)	WRITE_BYTE(DOSADDR_REL(addr), val)
#define READ_WORDP(addr)	READ_WORD(DOSADDR_REL(addr))
#define WRITE_WORDP(addr, val)	WRITE_WORD(DOSADDR_REL(addr), val)
#define READ_DWORDP(addr)	READ_DWORD(DOSADDR_REL(addr))
#define WRITE_DWORDP(addr, val)	WRITE_DWORD(DOSADDR_REL(addr), val)

#define IOFF(i) READ_WORD(i * 4)
#define ISEG(i) READ_WORD(i * 4 + 2)

#define u_short unsigned short

typedef struct {
  u_short offset;
  u_short segment;
} far_t;

#include "wrapper.h"

#endif
