#include <machine/asm.h>
	
#define MULTIBOOT_HDRSIGN 0x1badb002 
#define MULTIBOOT_REGSIGN 0x2badb002
#define _BOOT_FLAGS       0x00000003

#define MBIOFFSET_FLAGS   0
#define _MBIF_CMDLINE     (1 << 2)
#define MBIOFFSET_CMDLINE 16

ENTRY(start)
ENTRY(_start)
	jmp multiboot_entry

	.align 4
multiboot_header:
	.long MULTIBOOT_HDRSIGN
	.long _BOOT_FLAGS
	.long -(MULTIBOOT_HDRSIGN+_BOOT_FLAGS)
	.long multiboot_header
	.long _start
	.long _edata
	.long _end
	.long multiboot_entry

multiboot_entry:
	movl $stacktop, %esp

	/* Clear flags (cld and cli) */
	pushl $0
	popf

	/* Check Multiboot */
	cmpl $MULTIBOOT_REGSIGN, %eax
	jne badmultiboot

	/* Save Multiboot CMDLINE */
	mov MBIOFFSET_FLAGS(%ebx), %eax
	and $_MBIF_CMDLINE, %eax
	jz 1f
	mov MBIOFFSET_CMDLINE(%ebx), %esi
	mov $__cmdline, %edi
	mov $256, %ecx
	rep movsb

	call _C_LABEL(_clear_bss)


	push %ebx
	call _C_LABEL(parse_multiboot)
	add $4, %esp

	/* init hal */
//	call _C_LABEL(i386_init)

	push $__cmdline
	call ___start
	add $4, %esp
halt:
	pushl $haltstring
	call printf
	add $4, %esp
loop: 
	cli
	hlt
	jmp loop

badmultiboot:
	lea badbootstring, %esi
	call print
	jmp loop


print:
	movl $0x1000, %ecx
	movb $0x87, %ah
	movl $0xb8000, %edi
1:	lodsb
	stosw
	cmp $0, %al
	loopne 1b
	ret

ENTRY(_clear_bss)
	push %edi
	mov $0x0, %eax
	mov $_ebss, %ecx
	sub $_sbss, %ecx
	mov $_sbss, %edi
	rep stosb
	pop %edi
	ret
END(_clear_bss)

	.align 64
_bios_idtr:
	.word 1023
	.long 0

	.align 64
_saved_idtr:
	.skip 6
	
_saved_esp:
	.long 0

	.data
haltstring: 
	.asciz "System Halted."
haltstring_size = . - haltstring
	
badbootstring: 
	.asciz "Multiboot compliant bootloader needed."
badbootstring_size = . - badbootstring

__mbi:
	.long 0

__cmdline:
	.skip 256

	.align 4096
stackbtm:
	.skip 4096
stacktop:

