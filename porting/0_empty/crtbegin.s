	.file	"crtbegin.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.file 0 "/home/glguida/murgia/nux/porting/0_empty" "../../libec/crtbegin.c"
	.section	,init,"ax",@progbits
	.align	1
	.type	__do_global_ctors_aux, @function
__do_global_ctors_aux:
.LFB0:
	.file 1 "../../libec/crtbegin.c"
	.loc 1 13 1
	.cfi_startproc
	.loc 1 14 3
	.loc 1 16 3
	.loc 1 16 7 is_stmt 0
	lui	a4,%hi(__initialized.1)
	.loc 1 16 6
	lw	a5,%lo(__initialized.1)(a4)
	bne	a5,zero,.L9
	.loc 1 19 3 is_stmt 1
.LBB2:
	.loc 1 21 48 is_stmt 0
	lui	a5,%hi(__CTOR_LIST_END__-8)
	ld	a5,%lo(__CTOR_LIST_END__-8)(a5)
.LBE2:
	.loc 1 19 17
	li	a3,1
	sw	a3,%lo(__initialized.1)(a4)
	.loc 1 21 3 is_stmt 1
.LBB3:
	.loc 1 21 8
.LVL0:
	.loc 1 21 51
	beq	a5,zero,.L9
.LBE3:
	.loc 1 13 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sd	s0,0(sp)
	.cfi_offset 8, -16
.LBB4:
	.loc 1 21 22
	lui	s0,%hi(__CTOR_LIST_END__-8)
.LBE4:
	.loc 1 13 1
	sd	ra,8(sp)
	.cfi_offset 1, -8
.LBB5:
	.loc 1 21 22
	addi	s0,s0,%lo(__CTOR_LIST_END__-8)
.LVL1:
.L3:
	.loc 1 23 5 is_stmt 1 discriminator 3
	.loc 1 21 61 is_stmt 0 discriminator 3
	addi	s0,s0,-8
.LVL2:
	.loc 1 23 6 discriminator 3
	jalr	a5
.LVL3:
	.loc 1 21 61 is_stmt 1 discriminator 3
	.loc 1 21 51 discriminator 3
	.loc 1 21 48 is_stmt 0 discriminator 3
	ld	a5,0(s0)
	.loc 1 21 51 discriminator 3
	bne	a5,zero,.L3
.LBE5:
	.loc 1 25 1
	ld	ra,8(sp)
	.cfi_restore 1
	ld	s0,0(sp)
	.cfi_restore 8
.LVL4:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
.LVL5:
.L9:
	ret
	.cfi_endproc
.LFE0:
	.size	__do_global_ctors_aux, .-__do_global_ctors_aux
	.section	.fini,"ax",@progbits
	.align	1
	.type	__do_global_dtors_aux, @function
__do_global_dtors_aux:
.LFB1:
	.loc 1 32 1 is_stmt 1
	.cfi_startproc
	.loc 1 33 3
	.loc 1 35 3
.LVL6:
.LBB6:
	.loc 1 38 48
.LBE6:
	.loc 1 32 1 is_stmt 0
	addi	sp,sp,-32
	.cfi_def_cfa_offset 32
	sd	s0,16(sp)
	sd	s1,8(sp)
	.cfi_offset 8, -16
	.cfi_offset 9, -24
.LBB7:
	.loc 1 38 48
	lui	s0,%hi(__DTOR_LIST__+8)
	lui	s1,%hi(__DTOR_LIST_END__)
.LBE7:
	.loc 1 32 1
	sd	ra,24(sp)
	.cfi_offset 1, -8
.LBB8:
	.loc 1 38 48
	addi	a5,s0,%lo(__DTOR_LIST__+8)
	addi	s1,s1,%lo(__DTOR_LIST_END__)
	bgeu	a5,s1,.L13
	.loc 1 38 22
	addi	s0,s0,%lo(__DTOR_LIST__+8)
.LVL7:
.L15:
	.loc 1 40 7 is_stmt 1 discriminator 3
	.loc 1 40 8 is_stmt 0 discriminator 3
	ld	a5,0(s0)
	.loc 1 38 70 discriminator 3
	addi	s0,s0,8
.LVL8:
	.loc 1 40 8 discriminator 3
	jalr	a5
.LVL9:
	.loc 1 38 70 is_stmt 1 discriminator 3
	.loc 1 38 48 discriminator 3
	bltu	s0,s1,.L15
.LVL10:
.L13:
.LBE8:
	.loc 1 42 1 is_stmt 0
	ld	ra,24(sp)
	.cfi_restore 1
	ld	s0,16(sp)
	.cfi_restore 8
	ld	s1,8(sp)
	.cfi_restore 9
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE1:
	.size	__do_global_dtors_aux, .-__do_global_dtors_aux
	.section	.sbss,"aw",@nobits
	.align	2
	.type	__initialized.1, @object
	.size	__initialized.1, 4
__initialized.1:
	.zero	4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0xff
	.2byte	0x5
	.byte	0x1
	.byte	0x8
	.4byte	.Ldebug_abbrev0
	.byte	0x6
	.4byte	.LASF6
	.byte	0x1d
	.4byte	.LASF0
	.4byte	.LASF1
	.4byte	.LLRL4
	.8byte	0
	.4byte	.Ldebug_line0
	.byte	0x7
	.4byte	.LASF7
	.byte	0x1
	.byte	0x3
	.byte	0x10
	.4byte	0x3b
	.byte	0x1
	.4byte	0x2a
	.byte	0x2
	.4byte	0x40
	.byte	0x8
	.byte	0x9
	.4byte	0x36
	.4byte	0x4c
	.byte	0xa
	.byte	0
	.byte	0x1
	.4byte	0x41
	.byte	0x3
	.4byte	.LASF2
	.byte	0x6
	.4byte	0x4c
	.byte	0xb
	.4byte	.LASF8
	.byte	0x1
	.byte	0x7
	.byte	0x15
	.4byte	.LASF8
	.4byte	0x36
	.byte	0x3
	.4byte	.LASF3
	.byte	0x8
	.4byte	0x4c
	.byte	0xc
	.4byte	.LASF9
	.byte	0x1
	.byte	0x1f
	.byte	0x1
	.8byte	.LFB1
	.8byte	.LFE1-.LFB1
	.byte	0x1
	.byte	0x9c
	.4byte	0xb3
	.byte	0xd
	.4byte	.LASF4
	.byte	0x1
	.byte	0x21
	.byte	0xe
	.4byte	0xb3
	.byte	0
	.byte	0x4
	.4byte	.LLRL2
	.byte	0x5
	.string	"p"
	.byte	0x26
	.4byte	0xba
	.4byte	.LLST3
	.byte	0
	.byte	0
	.byte	0xe
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0x2
	.4byte	0x36
	.byte	0xf
	.4byte	.LASF10
	.byte	0x1
	.byte	0xc
	.byte	0x1
	.8byte	.LFB0
	.8byte	.LFE0-.LFB0
	.byte	0x1
	.byte	0x9c
	.byte	0x10
	.4byte	.LASF5
	.byte	0x1
	.byte	0xe
	.byte	0xe
	.4byte	0xb3
	.byte	0x9
	.byte	0x3
	.8byte	__initialized.1
	.byte	0x4
	.4byte	.LLRL0
	.byte	0x5
	.string	"p"
	.byte	0x15
	.4byte	0xba
	.4byte	.LLST1
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.byte	0x1
	.byte	0x26
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x2
	.byte	0xf
	.byte	0
	.byte	0xb
	.byte	0x21
	.byte	0x8
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x3
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0x21
	.byte	0x1
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0x21
	.byte	0x15
	.byte	0x49
	.byte	0x13
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x4
	.byte	0xb
	.byte	0x1
	.byte	0x55
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x5
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0x21
	.byte	0x1
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0x21
	.byte	0x16
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x6
	.byte	0x11
	.byte	0x1
	.byte	0x25
	.byte	0xe
	.byte	0x13
	.byte	0xb
	.byte	0x3
	.byte	0x1f
	.byte	0x1b
	.byte	0x1f
	.byte	0x55
	.byte	0x17
	.byte	0x11
	.byte	0x1
	.byte	0x10
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x7
	.byte	0x16
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x15
	.byte	0
	.byte	0x27
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x21
	.byte	0
	.byte	0
	.byte	0
	.byte	0xb
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x6e
	.byte	0xe
	.byte	0x49
	.byte	0x13
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0xc
	.byte	0x2e
	.byte	0x1
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x27
	.byte	0x19
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x7
	.byte	0x40
	.byte	0x18
	.byte	0x7c
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xd
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x1c
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xe
	.byte	0x24
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0x8
	.byte	0
	.byte	0
	.byte	0xf
	.byte	0x2e
	.byte	0x1
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x27
	.byte	0x19
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x7
	.byte	0x40
	.byte	0x18
	.byte	0x7c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x10
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loclists,"",@progbits
	.4byte	.Ldebug_loc3-.Ldebug_loc2
.Ldebug_loc2:
	.2byte	0x5
	.byte	0x8
	.byte	0
	.4byte	0
.Ldebug_loc0:
.LLST3:
	.byte	0x7
	.8byte	.LVL7
	.8byte	.LVL8
	.byte	0x1
	.byte	0x58
	.byte	0x7
	.8byte	.LVL8
	.8byte	.LVL9
	.byte	0x3
	.byte	0x78
	.byte	0x78
	.byte	0x9f
	.byte	0x7
	.8byte	.LVL9
	.8byte	.LVL10
	.byte	0x1
	.byte	0x58
	.byte	0
.LLST1:
	.byte	0x7
	.8byte	.LVL1
	.8byte	.LVL2
	.byte	0x1
	.byte	0x58
	.byte	0x7
	.8byte	.LVL2
	.8byte	.LVL3
	.byte	0x3
	.byte	0x78
	.byte	0x8
	.byte	0x9f
	.byte	0x7
	.8byte	.LVL3
	.8byte	.LVL4
	.byte	0x1
	.byte	0x58
	.byte	0
.Ldebug_loc3:
	.section	.debug_aranges,"",@progbits
	.4byte	0x3c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.LFB0
	.8byte	.LFE0-.LFB0
	.8byte	.LFB1
	.8byte	.LFE1-.LFB1
	.8byte	0
	.8byte	0
	.section	.debug_rnglists,"",@progbits
.Ldebug_ranges0:
	.4byte	.Ldebug_ranges3-.Ldebug_ranges2
.Ldebug_ranges2:
	.2byte	0x5
	.byte	0x8
	.byte	0
	.4byte	0
.LLRL0:
	.byte	0x6
	.8byte	.LBB2
	.8byte	.LBE2
	.byte	0x6
	.8byte	.LBB3
	.8byte	.LBE3
	.byte	0x6
	.8byte	.LBB4
	.8byte	.LBE4
	.byte	0x6
	.8byte	.LBB5
	.8byte	.LBE5
	.byte	0
.LLRL2:
	.byte	0x6
	.8byte	.LBB6
	.8byte	.LBE6
	.byte	0x6
	.8byte	.LBB7
	.8byte	.LBE7
	.byte	0x6
	.8byte	.LBB8
	.8byte	.LBE8
	.byte	0
.LLRL4:
	.byte	0x6
	.8byte	.LFB0
	.8byte	.LFE0
	.byte	0x6
	.8byte	.LFB1
	.8byte	.LFE1
	.byte	0
.Ldebug_ranges3:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF8:
	.string	"__DTOR_LIST__"
.LASF2:
	.string	"__CTOR_LIST_END__"
.LASF9:
	.string	"__do_global_dtors_aux"
.LASF3:
	.string	"__DTOR_LIST_END__"
.LASF5:
	.string	"__initialized"
.LASF7:
	.string	"fptr_t"
.LASF6:
	.string	"GNU C17 12.2.0 -mabi=lp64d -misa-spec=20191213 -march=rv64imafdc_zicsr_zifencei -g -O7 -fno-builtin -ffreestanding -fno-strict-aliasing"
.LASF4:
	.string	"__finished"
.LASF10:
	.string	"__do_global_ctors_aux"
	.section	.debug_line_str,"MS",@progbits,1
.LASF0:
	.string	"../../libec/crtbegin.c"
.LASF1:
	.string	"/home/glguida/murgia/nux/porting/0_empty"
	.ident	"GCC: (GNU) 12.2.0"
