ENTRY(_start)
SECTIONS
{
   . = 0xffff800000000000 + SIZEOF_HEADERS;

   .text :
   {
       *(.text)
       *(.text.*)
   } : text

   .init :
   {
      KEEP (*crti.o(.init))
      KEEP (*crti?.o(.init))

      KEEP (*(EXCLUDE_FILE (*crtn.o *crtn?.o ) .init))
      KEEP (*(SORT(.init.*)))
      KEEP (*(.init))
   } : text

   .fini :
   {
      KEEP (*crti.o(.fini))
      KEEP (*crti?.o(.fini))

      KEEP (*(EXCLUDE_FILE (*crtn.o *crtn?.o ) .fini))
      KEEP (*(SORT(.fini.*)))
      KEEP (*(.fini))
   } : text

   .rodata ALIGN(4096) :
   {
      *(EXCLUDE_FILE (*__nux_syms.o) .rodata)
      *(EXCLUDE_FILE (*__nux_syms.o) .rodata.str?.*)
      *(EXCLUDE_FILE (*__nux_syms.o) .rodata.*)
   } : rodata

  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } : rodata

  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } : rodata

  .preinit_array (READONLY) :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } : rodata

  .ctors (READONLY) :
   {
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))

      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
    } : rodata

   .dtors (READONLY) :
   { 
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
   } : rodata

   .data ALIGN (4096) :
   {
      *(.data);
      *(.data.*);

      /* Performance Counters. */
      . = ALIGN (16);
      _nuxperf_start = .;
      *(.perfctr);
      _nuxperf_end = .;

      /* Performance Measures. */
      . = ALIGN (16);
      _nuxmeasure_start = .;
      *(.measure);
      _nuxmeasure_end = .;

      /*
	Custom data structures for projects using NUX.
      */
      . = ALIGN (16);
      _data_ext0_start = .;
      KEEP (*(.data_ext0));
      _data_ext0_end = .;

      . = ALIGN (16);
      _data_ext1_start = .;
      KEEP (*(.data_ext1));
      _data_ext1_end = .;

   } : data

   .bss :
   {
      _sbss = .;
      *(COMMON);
      *(.bss);
      _ebss = .;
   } : data

   .tdata :
   {
     PROVIDE_HIDDEN (__tdata_start = .);
     *(.tdata .tdata.* .gnu.linkonce.td.*)
   } : tdata
  .tbss : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } : tdata

   . += 4096;

   .kstack_bsp ALIGN(4096) :
   {
	*(.kstack)
   } : kstack1

   . += 4096;

   .kstack_ist1 ALIGN(4096) :
   {
	*(.kstack_ist1)
   } : kstack2

   . += 4096;

   .kstack_ist2 ALIGN(4096) :
   {
	*(.kstack_ist2)
   } : kstack3

   . += 4096;

   .kstack_ist3 ALIGN(4096) :
   {
	*(.kstack_ist3)
   } : kstack4

   .info ALIGN(4096) :
   {
	_info_start = .;
	. += 4096;
	_info_end = .;
   } : info

   .memregs ALIGN(4096) :
   {
	_memregs_start = .;
	. += 1 * 1024 * 1024;
	_memregs_end = .;
   } : memregs

   .stree ALIGN (4096) : AT(0)
   {
	_stree_start = .;
	. += 16 * 1024 * 1024;
	_stree_end = .;
   } : stree

   /* 64Mb Framebuffer. */
   .fbuffer ALIGN (4096) : AT(0)
   {
	_fbuf_start = .;
	. += 64 * 1024 * 1024;
	_fbuf_end = .;
   } : fbuf

   . += 4096;

   .ksyms ALIGN(4096) : AT(0)
   {
	_ksym_start = .;
	__nux_syms.o*(*.rodata);
	__nux_syms.o*(*.rodata.str?.*);
	__nux_syms.o*(*.rodata.*);
	_ksym_end = .;
   } : ksyms

   /* 512GB KMEM, 4k aligned. */
   .kmem ALIGN(4096) : AT (0)
   {
	_kmem_start = .;
	. += 512 * 1024 * 1024 * 1024;
	_kmem_end = .;
   } : kmem

   /* 512GB KVA, 4k aligned. */
   .kva ALIGN(4096): AT (0)
   {
	_kva_start = .;
	. += 512 * 1024 * 1024 * 1024;
	_kva_end = .;
   } : kva

   /* 256 Mb PFN Cache */
   .pfncache ALIGN(4096) : AT(0)
   {
	_pfncache_start = .;
	. += 256 * 1024 * 1024;
	_pfncache_end = .;
   } : pfncache


   /* 512 GB physmap, 512GB Aligned. */
   .physmap ALIGN(512 * 1024 * 1024 * 1024) : AT(0)
   {
	_physmap_start = .;
	. += 512 * 1024 * 1024 * 1024;
	_physmap_end = .;
   } : physmap

   /* 512 GB linear map, 512GB Aligned necessarily. */
   .linear ALIGN(512 * 1024 * 1024 * 1024) : AT(0)
   {
	_linear_start = .;
	. += 512 * 1024 * 1024 * 1024;
	_linear_end = .;

   } : linear
}

PHDRS
{
    text    PT_LOAD;
    rodata  PT_LOAD;
    data    PT_LOAD;
    tdata   PT_TLS;
    kstack1 PT_LOAD;
    kstack2 PT_LOAD;
    kstack3 PT_LOAD;
    kstack4 PT_LOAD;
    ksyms   PT_LOAD;
    info    0xAF100000; /* PHT_APXH_INFO */
    kmem    0xAF100008; /* PHT_APXH_TOPPTALLOC */
    kva     0xAF100008; /* PHT_APXH_TOPPTALLOC */
    pfncache 0xAF100005; /* PHT_APXH_PTALLOC */
    physmap 0xAF100002; /* PHT_APXH_PHYSMAP */
    stree   0xAF100004; /* PHT_APXH_STREE */
    fbuf    0xAF100006; /* PHT_APXH_FRAMEBUF */
    memregs 0xAF100007; /* PHT_APXH_REGIONS */
    linear  0xAF10FFFF; /* PHT_APXH_LINEAR */
}
