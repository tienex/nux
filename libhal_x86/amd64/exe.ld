ENTRY(_start)
SECTIONS
{
   . = 0xffff800000000000 + SIZEOF_HEADERS;

   .text :
   {
       *(.text)
       *(.text.*)
   } : text
      _ecode = .;	

   .init :
   {
      KEEP (*crti.o(.init))
      KEEP (*crti?.o(.init))

      KEEP (*(EXCLUDE_FILE (*crtn.o *crtn?.o ) .init))
      KEEP (*(SORT(.init.*)))
      KEEP (*(.init))
   } :text

   .fini :
   {
      KEEP (*crti.o(.fini))
      KEEP (*crti?.o(.fini))

      KEEP (*(EXCLUDE_FILE (*crtn.o *crtn?.o ) .fini))
      KEEP (*(SORT(.fini.*)))
      KEEP (*(.fini))
   } :text

   .rodata ALIGN(4096) :
   {
      *(.rodata);
      *(.rodata.*)
   } : rodata

   .data ALIGN (4096) :
   {

      *(.data);
      *(.data.*);
   } : data

   .ctors :
   {
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))

      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
    } : data

   .dtors :
   { 
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
   } : data

   .bss :
   {
      _sbss = .;
      *(COMMON);
      *(.bss);
      _ebss = .;
   } : data

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

   /* 32Mb Framebuffer. */
   .fbuffer ALIGN (4096) : AT(0)
   {
	_fbuf_start = .;
	. += 32 * 1024 * 1024;
	_fbuf_end = .;
   } : fbuf

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
    kstack1  PT_LOAD;
    kstack2  PT_LOAD;
    kstack3  PT_LOAD;
    kstack4  PT_LOAD;
    info    0xAF100000; /* PHT_APXH_INFO */
    kmem    0xAF100001; /* PHT_APXH_EMPTY */
    kva     0xAF100001; /* PHT_APXH_EMPTY */
    pfncache 0xAF100005; /* PHT_APXH_PTALLOC */
    physmap 0xAF100002; /* PHT_APXH_PHYSMAP */
    stree   0xAF100004; /* PHT_APXH_STREE */
    fbuf    0xAF100006; /* PHT_APXH_FRAMEBUF */
    memregs 0xAF100007; /* PHT_APXH_REGIONS */
    linear  0xAF10FFFF; /* PHT_APXH_LINEAR */
}
