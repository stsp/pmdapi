#include "cpu.h"
#include "instremu.h"

#define OPandFLAG0(eflags, insn, op1, istype) __asm__ __volatile__("\n\
	"#insn"	%0\n\
	pushf; pop	%1\n \
	" : #istype (op1), "=g" (eflags) : "0" (op1));

#define OPandFLAG1(eflags, insn, op1, istype) __asm__ __volatile__("\n\
	"#insn"	%0, %0\n\
	pushf; pop	%1\n \
	" : #istype (op1), "=g" (eflags) : "0" (op1));

#define OPandFLAG(eflags, insn, op1, op2, istype, type) __asm__ __volatile__("\n\
	"#insn"	%3, %0\n\
	pushf; pop	%1\n \
	" : #istype (op1), "=g" (eflags) : "0" (op1), #type (op2));

#define OPandFLAGC(eflags, insn, op1, op2, istype, type) __asm__ __volatile__("\n\
       shr     $1, %0\n\
       "#insn" %4, %1\n\
       pushf; pop     %0\n \
       " : "=r" (eflags), #istype (op1)  : "0" (eflags), "1" (op1), #type (op2));

/* 6 logical and arithmetic "RISC" core functions
   follow
*/
unsigned char instr_binary_byte(unsigned char op, unsigned char op1, unsigned char op2, unsigned long *eflags)
{
  unsigned long flags;

  switch (op&0x7){
  case 1: /* or */
    OPandFLAG(flags, orb, op1, op2, =q, q);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 4: /* and */
    OPandFLAG(flags, andb, op1, op2, =q, q);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 6: /* xor */
    OPandFLAG(flags, xorb, op1, op2, =q, q);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 0: /* add */
    *eflags &= ~CF; /* Fall through */
  case 2: /* adc */
    flags = *eflags;
    OPandFLAGC(flags, adcb, op1, op2, =q, q);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return op1;
  case 5: /* sub */
  case 7: /* cmp */
    *eflags &= ~CF; /* Fall through */
  case 3: /* sbb */
    flags = *eflags;
    OPandFLAGC(flags, sbbb, op1, op2, =q, q);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return op1;
  }
  return 0;
}

unsigned instr_binary_word(unsigned op, unsigned op1, unsigned op2, unsigned long *eflags)
{
  unsigned long flags;
  unsigned short opw1 = op1;
  unsigned short opw2 = op2;

  switch (op&0x7){
  case 1: /* or */
    OPandFLAG(flags, orw, opw1, opw2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return opw1;
  case 4: /* and */
    OPandFLAG(flags, andw, opw1, opw2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return opw1;
  case 6: /* xor */
    OPandFLAG(flags, xorw, opw1, opw2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return opw1;
  case 0: /* add */
    *eflags &= ~CF; /* Fall through */
  case 2: /* adc */
    flags = *eflags;
    OPandFLAGC(flags, adcw, opw1, opw2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return opw1;
  case 5: /* sub */
  case 7: /* cmp */
    *eflags &= ~CF; /* Fall through */
  case 3: /* sbb */
    flags = *eflags;
    OPandFLAGC(flags, sbbw, opw1, opw2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return opw1;
  }
  return 0;
}

unsigned instr_binary_dword(unsigned op, unsigned op1, unsigned op2, unsigned long *eflags)
{
  unsigned long flags;

  switch (op&0x7){
  case 1: /* or */
    OPandFLAG(flags, orl, op1, op2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 4: /* and */
    OPandFLAG(flags, andl, op1, op2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 6: /* xor */
    OPandFLAG(flags, xorl, op1, op2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|PF|CF)) | (flags & (OF|ZF|SF|PF|CF));
    return op1;
  case 0: /* add */
    *eflags &= ~CF; /* Fall through */
  case 2: /* adc */
    flags = *eflags;
    OPandFLAGC(flags, adcl, op1, op2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return op1;
  case 5: /* sub */
  case 7: /* cmp */
    *eflags &= ~CF; /* Fall through */
  case 3: /* sbb */
    flags = *eflags;
    OPandFLAGC(flags, sbbl, op1, op2, =r, r);
    *eflags = (*eflags & ~(OF|ZF|SF|AF|PF|CF)) | (flags & (OF|ZF|AF|SF|PF|CF));
    return op1;
  }
  return 0;
}

#if DEBUG_INSTR >= 2
#define instr_deb2(x...) v_printf("VGAEmu: " x)
#else
#define instr_deb2(x...)
#endif

static unsigned char it[0x100] = {
  7, 7, 7, 7, 2, 3, 1, 1,    7, 7, 7, 7, 2, 3, 1, 0,
  7, 7, 7, 7, 2, 3, 1, 1,    7, 7, 7, 7, 2, 3, 1, 1,
  7, 7, 7, 7, 2, 3, 0, 1,    7, 7, 7, 7, 2, 3, 0, 1,
  7, 7, 7, 7, 2, 3, 0, 1,    7, 7, 7, 7, 2, 3, 0, 1,

  1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 7, 7, 0, 0, 0, 0,    3, 9, 2, 8, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2,    2, 2, 2, 2, 2, 2, 2, 2,

  8, 9, 8, 8, 7, 7, 7, 7,    7, 7, 7, 7, 7, 7, 7, 7,
  1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 6, 1, 1, 1, 1, 1,
  4, 4, 4, 4, 1, 1, 1, 1,    2, 3, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2,    3, 3, 3, 3, 3, 3, 3, 3,

  8, 8, 3, 1, 7, 7, 8, 9,    5, 1, 3, 1, 1, 2, 1, 1,
  7, 7, 7, 7, 2, 2, 1, 1,    0, 0, 0, 0, 0, 0, 0, 0,
  2, 2, 2, 2, 2, 2, 2, 2,    4, 4, 6, 2, 1, 1, 1, 1,
  0, 1, 0, 0, 1, 1, 7, 7,    1, 1, 1, 1, 1, 1, 7, 7
};

static unsigned arg_len(unsigned char *p, int asp)
{
  unsigned u = 0, m, s = 0;

  m = *p & 0xc7;
  if(asp) {
    if(m == 5) {
      u = 5;
    }
    else {
      if((m >> 6) < 3 && (m & 7) == 4) s = 1;
      switch(m >> 6) {
        case 1:
          u = 2; break;
        case 2:
          u = 5; break;
        default:
          u = 1;
      }
      u += s;
    }
  }
  else {
    if(m == 6)
      u = 3;
    else
      switch(m >> 6) {
        case 1:
          u = 2; break;
        case 2:
          u = 3; break;
        default:
          u = 1;
      }
  }

  instr_deb2("arg_len: %02x %02x %02x %02x: %u bytes\n", p[0], p[1], p[2], p[3], u);

  return u;
}

int instr_len(unsigned char *p, int is_32)
{
  unsigned u, osp, asp;
  unsigned char *p0 = p;
#if DEBUG_INSTR >= 1
  int seg, lock, rep;
  unsigned char *p1 = p;
#endif

#if DEBUG_INSTR >= 1
  seg = lock = rep = 0;
#endif
  osp = asp = is_32;

  for(u = 1; u && p - p0 < 17;) switch(*p++) {		/* get prefixes */
    case 0x26:	/* es: */
#if DEBUG_INSTR >= 1
      seg = 1;
#endif
      break;
    case 0x2e:	/* cs: */
#if DEBUG_INSTR >= 1
      seg = 2;
#endif
      break;
    case 0x36:	/* ss: */
#if DEBUG_INSTR >= 1
      seg = 3;
#endif
      break;
    case 0x3e:	/* ds: */
#if DEBUG_INSTR >= 1
      seg = 4;
#endif
      break;
    case 0x64:	/* fs: */
#if DEBUG_INSTR >= 1
      seg = 5;
#endif
      break;
    case 0x65:	/* gs: */
#if DEBUG_INSTR >= 1
      seg = 6;
#endif
      break;
    case 0x66:	/* operand size */
      osp ^= 1;
      break;
    case 0x67:	/* address size */
      asp ^= 1;
      break;
    case 0xf0:	/* lock */
#if DEBUG_INSTR >= 1
      lock = 1;
#endif
      break;
    case 0xf2:	/* repnz */
#if DEBUG_INSTR >= 1
      rep = 2;
#endif
      break;
    case 0xf3:	/* rep(z) */
#if DEBUG_INSTR >= 1
      rep = 1;
#endif
      break;
    default:	/* no prefix */
      u = 0;
  }
  p--;

#if DEBUG_INSTR >= 1
  p1 = p;
#endif

  if(p - p0 >= 16) return 0;

  if(*p == 0x0f) {
    /* not yet */
    error("msdos: unsupported instr_len %x %x\n", p[0], p[1]);
    return 0;
  }

  switch(it[*p]) {
    case 1:	/* op-code */
      p += 1; break;

    case 2:	/* op-code + byte */
      p += 2; break;

    case 3:	/* op-code + word/dword */
      p += osp ? 5 : 3; break;

    case 4:	/* op-code + [word/dword] */
      p += asp ? 5 : 3; break;

    case 5:	/* op-code + word/dword + byte */
      p += osp ? 6 : 4; break;

    case 6:	/* op-code + [word/dword] + word */
      p += asp ? 7 : 5; break;

    case 7:	/* op-code + mod + ... */
      p++;
      p += (u = arg_len(p, asp));
      if(!u) p = p0;
      break;

    case 8:	/* op-code + mod + ... + byte */
      p++;
      p += (u = arg_len(p, asp)) + 1;
      if(!u) p = p0;
      break;

    case 9:	/* op-code + mod + ... + word/dword */
      p++;
      p += (u = arg_len(p, asp)) + (osp ? 4 : 2);
      if(!u) p = p0;
      break;

    default:
      p = p0;
  }

#if DEBUG_INSTR >= 1
  if(p >= p0) {
    instr_deb("instr_len: instr = ");
    v_printf("%s%s%s%s%s",
      osp ? "osp " : "", asp ? "asp " : "",
      lock_txt[lock], rep_txt[rep], seg_txt[seg]
    );
    if(p > p1) for(u = 0; u < p - p1; u++) {
      v_printf("%02x ", p1[u]);
    }
    v_printf("\n");
  }
#endif

  return p - p0;
}
