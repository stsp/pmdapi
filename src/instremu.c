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
