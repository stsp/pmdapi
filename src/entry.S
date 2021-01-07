#include "asm-offsets.h"

#define stack_size 0x4000

#define S_REG(breg, reg) %ss:SIGCONTEXT_##reg(%breg)
#define MOV_TO_REG(breg, src, dreg) movl S_REG(breg, src), %dreg
#define LOAD_REG(breg, reg) MOV_TO_REG(breg, reg, reg)
#define MOV_FROM_REG(breg, sreg, dst) movl %sreg, S_REG(breg, dst)
#define SAVE_REG(breg, reg) MOV_FROM_REG(breg, reg, reg)

do_int:
      subl   $12,%esp			/* dummy, cr2,oldmask,fpstate */
      pushl  %cs:tmp_ss
      pushl  %cs:tmp_esp		/* esp_at_signal */
      pushl  %cs:tmp_flags
      pushl  8(%ebp)			/* cs */
      pushl  4(%ebp)			/* eip */
      pushl  $0				/* dummy, err */
      pushl  $0				/* dummy, trapno */
      pusha
      pushl  (%ebp)
      popl   8(%esp)			/* ebp */
      movl   %ebp, %ss:tmp_ebp
      pushl  %ds
      pushl  %es
      pushl  %fs
      pushl  %gs
      movl   %cs:_dseg32, %eax
      movl   %eax, %ds
      movl   %eax, %es
      pushl  %esp
      call   *-4(%ebp)
      addl   $4, %esp
      MOV_TO_REG(esp, eflags, eax)
      movl   %eax, tmp_flags
      popl   %gs
      popl   %fs
      popl   %es
      popl   %ds
      pushl  8(%esp)
      popl   (%ebp)
      popa
      movl   %cs:tmp_ebp, %ebp
      addl   $2*4, %esp
      popl   4(%ebp)
      popl   8(%ebp)
      addl   $24, %esp
      ret

.macro _scp_call cname ir
.globl _entry_\cname
_entry_\cname:
    enter $4, $0
    movl $_\cname, -4(%ebp)
    call do_int
    leave
    \ir
.endm
.macro scp_call cname
    _scp_call \cname lret
.endm
.macro scp_call_i cname
    _scp_call \cname iret
.endm

scp_call MSDOS_API_call
scp_call MSDOS_API_WINOS2_call
scp_call MSDOS_XMS_call
scp_call_i MSDOS_rmcb_call0
scp_call_i MSDOS_rmcb_call1
scp_call_i MSDOS_rmcb_call2

dos_int21:
    enter $4, $0
    movl $_int21_handler, -4(%ebp)
    call do_int
    leave
    ret

.globl _dos32_int21
_dos32_int21:
    pushl %esi
    pushl %edi
    pushl %ds
    movl %cs:_dseg32, %edi
    movl %edi, %ds
    mov  %ss, tmp_ss
    movl %esp, tmp_esp
    addl $12, tmp_esp
    pushfl
    popl tmp_flags
    popl %ds
    movl %ss, %esi
    cmpw %si, %cs:_dseg32
    je 1f
    pushl %edi
    pushl $(stack+stack_size)
    movl %esp, %edi
    lss %ss:(%edi), %esp
    pushl %esi
    pushl %edi
    pushl %ds
    movl %esi, %ds
    movl 12(%edi), %esi
    movl 8(%edi), %edi
    popl %ds
    call dos_int21
    lss (%esp), %esp
    addl $16, %esp
    jmp 2f
1:
    popl %edi
    popl %esi
    call dos_int21
2:
    pushl %cs:tmp_flags
    popfl
    lret $4

.globl _dos16_int21
_dos16_int21:
    pushl %esi
    pushl %edi
    pushl %ds
    movl %cs:_dseg32, %edi
    movl %edi, %ds
    mov  %ss, tmp_ss
    movl %esp, tmp_esp
    addl $12, tmp_esp
    pushfl
    popl tmp_flags
    popl %ds
    movl %ss, %esi
    cmpw %si, %di
    jne 1f
    popl %edi
    popl %esi
    jmp _dos32_int21
1:
    pushl %edi
    pushl $(stack+stack_size)
    movzwl %sp, %edi
    lss %ss:(%edi), %esp
    pushl %esi
    pushl %edi
    pushl %ds
    movl %esi, %ds
    movl 12(%edi), %esi
    movl 8(%edi), %edi
    popl %ds
    call dos_int21
    lss (%esp), %esp
    addl $16, %esp
    pushl %cs:tmp_flags
    popfl
    data16 lret $2

.globl _entry32
_entry32:
    movl $1, _clnt_is_32
common_entry:
    movl %esp, lss_esp
    mov  %ss, lss_ss
    movl %ds, %esi
    movl %esi, %es
    movl %esi, lss_ss1
    movl $(stack+stack_size), lss_esp1
    lss lss_esp1, %esp
    pushl %ecx
    pushl %ebx
    pushl %eax
    call _entry
    lss lss_esp, %esp
    lret

.code16
.globl _code16
_code16:
.globl _entry16
_entry16:
    pushw %ds
    popw %es
    movl $0, _clnt_is_32-_data16
    movw %ax, %dx
    movw %bx, %si
    movw $0, %ax
    movw $2, %cx
    int $0x31
    jc 1f
    movw %ax, %bx
    movw $3, %ax
    int $0x31
    movw %ax, %cx
    movw $0xc, %ax
    movw $(_ds32_desc-_data16), %di
    int $0x31
    jc 1f
    movw %bx, %ds
    addw %cx, %bx
    movw $(_cs32_desc-_data16), %di
    int $0x31
    jc 1f
    pushw $0
    pushw %bx
    pushw $0
    pushw $common_entry
    movw %dx, %ax
    movw %si, %bx
    movzwl %sp, %esi
    data32 addr32 lcall *%ss:(%esi)
    addw $8, %sp
1:
    lret
.globl _code16_end
_code16_end:
.code32

.data

.globl _data16
_data16:
.globl _cs32_desc
_cs32_desc:
.long 0,0
.globl _ds32_desc
_ds32_desc:
.long 0,0
.globl _clnt_is_32
_clnt_is_32:
.long 1
.globl _dseg32
_dseg32:
.long 0
.globl _data16_end
_data16_end:

tmp_flags:
.long 0
tmp_esp:
.long 0
tmp_ss:
.long 0
tmp_ebp:
.long 0
lss_esp:
.long 0
lss_ss:
.long 0
lss_esp1:
.long 0
lss_ss1:
.long 0
ret_adr:
.long 0
stack:
.rept stack_size
.byte 0
.endr

.text
