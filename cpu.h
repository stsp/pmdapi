#ifndef CPU_H
#define CPU_H

extern struct vm86_regs regs;
#define REG(reg) (regs.reg)

/* these are used like:  LO(ax) = 2 (sets al to 2) */
#define LO(reg)  (*(unsigned char *)&REG(e##reg))
#define HI(reg)  (*((unsigned char *)&REG(e##reg) + 1))

#define _LO(reg) (*(unsigned char *)&(scp->e##reg))
#define _HI(reg) (*((unsigned char *)&(scp->e##reg) + 1))

/* these are used like: LWORD(eax) = 65535 (sets ax to 65535) */
#define LWORD(reg)	(*((unsigned short *)&REG(reg)))
#define HWORD(reg)	(*((unsigned short *)&REG(reg) + 1))

#define _LWORD(reg)	(*((unsigned short *)&(scp->reg)))
#define _HWORD(reg)	(*((unsigned short *)&(scp->reg) + 1))

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

/* this is used like: SEG_ADR((char *), es, bx) */
#define SEG_ADR(type, seg, reg)  type((LWORD(seg) << 4) + LWORD(e##reg))
#define MK_FARt(seg, off) ((far_t){(off), (seg)})
#define SEG2LINEAR(seg)	((void *)  ( ((unsigned int)(seg)) << 4)  )
#define SEGOFF2LINEAR(seg, off)  ((((Bit32u)(seg)) << 4) + (off))

#define MEMCPY_DOS2DOS(dos_addr, unix_addr, n) \
	memcpy((void *)(dos_addr), (void *)(unix_addr), (n))
#define READ_BYTE(addr)                 (*(Bit8u *) (addr))
#define WRITE_BYTE(addr, val)           (*(Bit8u *) (addr) = (val) )
#define READ_WORD(addr)                 (*(Bit16u *) (addr))
#define WRITE_WORD(addr, val)           (*(Bit16u *) (addr) = (val) )
#define READ_DWORD(addr)                (*(Bit32u *) (addr))
#define WRITE_DWORD(addr, val)          (*(Bit32u *) (addr) = (val) )

#define u_short unsigned short
#define Bit16u unsigned short
#define Bit32u unsigned long

#endif
