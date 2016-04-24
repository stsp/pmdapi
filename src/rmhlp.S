/* reference implementation, not yet ported */

/* ======================= Addr = F000:F540 (FF540) */
	.org	DOS_EXEC_OFF
	.globl MSDOS_exec_start
MSDOS_exec_start:
	pushw %bp
	movw %sp, %bp
	pushw %ax
	pushw $0	# -4(%bp) - placeholder for buffer ptr
1:
	subw %cs:MSDOS_exec_buf_sz-bios_f000, %sp
	movw %sp, -4(%bp)
	pushw %di
	push %es
	push %ss
	pop %es
	movw -4(%bp), %di
	movb $0, %al
	lcall *%cs:s_r_addr-bios_f000
	pop %es
	popw %di
	movw -2(%bp), %ax
	int $0x21
	pushfw
	pushw %ax
	pushw %di
	push %es
	push %ss
	pop %es
	movw -4(%bp), %di
	movb $1, %al
	lcall *%cs:s_r_addr-bios_f000
	pop %es
	popw %di
	popw %ax
	popfw
	addw %cs:MSDOS_exec_buf_sz-bios_f000, %sp
	addw $4, %sp
	popw %bp
	iret

        .align 4,0
s_r_addr:
	.globl MSDOS_exec_entry_ip
MSDOS_exec_entry_ip:
	.word 0
	.globl MSDOS_exec_entry_cs
MSDOS_exec_entry_cs:
	.word 0
        .globl MSDOS_exec_buf_sz
MSDOS_exec_buf_sz:
	.word 0

/* ======================= Addr = F000:F400 (FF400) */
	.org	((DOS_LONG_READ_SEG - BIOSSEG) << 4) + DOS_LONG_READ_OFF
	.globl MSDOS_lr_start
MSDOS_lr_start:
	pushl	%esi
	pushl	%edi
	pushl	%ecx
	xorl	%edi, %edi
start_read:
	movl	%ecx, %esi
	cmpl	$0x10000, %esi
	jb	do_read
	xorl	%ecx, %ecx
	decw	%cx
do_read:
	movb	$0x3f, %ah
	int	$0x21
	jc	read_set_cf
	movzwl	%ax, %eax
	pushl	%ecx
	pushl	%eax
	movl	%eax, %ecx
	movb	$0, %al		/* read */
	lcall   *%cs:MSDOS_lr_entry-bios_f000
	popl	%eax
	popl	%ecx
	addl	%eax, %edi
	cmpw	%ax, %cx
	jnz	done_read
	movl	%esi, %ecx
	subl	%eax, %ecx
	jnz	start_read
	movl	%edi, %eax
	jmp done_read
read_set_cf:
	orl	%edi, %edi
	jz 1f
	movl	%edi, %eax
	jmp	done_read
1:
	movl	%eax, %ecx
	movb	$2, %al		/* set CF */
	lcall   *%cs:MSDOS_lr_entry-bios_f000
#	jmp	done_read
done_read:
	popl	%ecx
	popl	%edi
	popl	%esi
	iret

        .align 4,0
MSDOS_lr_entry:
        .globl  MSDOS_lr_entry_ip
MSDOS_lr_entry_ip:
        .word 0
        .globl  MSDOS_lr_entry_cs
MSDOS_lr_entry_cs:
        .word 0

/* ======================= Addr = F000:F4A0 (FF4A0) */
	.org	((DOS_LONG_WRITE_SEG - BIOSSEG) << 4) + DOS_LONG_WRITE_OFF
	.globl MSDOS_lw_start
MSDOS_lw_start:
	pushl	%esi
	pushl	%edi
	pushl	%ecx
	xorl	%edi, %edi
start_write:
	movl	%ecx, %esi
	cmpl	$0x10000, %esi
	jb	do_write
	xorl	%ecx, %ecx
	decw	%cx
do_write:
	movb	$1, %al		/* write */
	lcall   *%cs:MSDOS_lw_entry-bios_f000
	movb	$0x40, %ah
	int	$0x21
	jc	write_set_cf
	movzwl	%ax, %eax
	addl	%eax, %edi
	cmpw	%ax, %cx
	jnz	done_write
	movl	%esi, %ecx
	subl	%eax, %ecx
	jnz	start_write
	movl	%edi, %eax
	jmp	done_write
write_set_cf:
	orl	%edi, %edi
	jz 1f
	movl	%edi, %eax
	jmp	done_write
1:
	movl	%eax, %ecx
	movb	$2, %al		/* set CF */
	lcall   *%cs:MSDOS_lw_entry-bios_f000
#	jmp	done_write
done_write:
	popl	%ecx
	popl	%edi
	popl	%esi
	iret

        .align 4,0
MSDOS_lw_entry:
        .globl  MSDOS_lw_entry_ip
MSDOS_lw_entry_ip:
        .word 0
        .globl  MSDOS_lw_entry_cs
MSDOS_lw_entry_cs:
        .word 0
