OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   /* kernel code */
   .text 0xc0100000 : AT (0x00100000)
   {
      *(.text)
   } : text

   .ctors :
   {
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))

      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
    } : text

   .dtors :
   { 
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
   } : text
   _ecode = .;

   .rodata ALIGN(4096) :
   {
      *(.rodata.str?.*);
      *(.rodata);       
   } : rodata


   .data ALIGN (4096) :
   {

      *(.data);
   } : data

   .bss :
   {
      _sbss = .;
      *(COMMON);
      *(.bss);
      _ebss = .;
   } : data

   /* First megabyte needed by HAL. */
   .physmap ALIGN(4096) : AT(0)
   {
	_physmap_start = .;
	. += 1 * 1024 * 1024;
	_physmap_end = .;
   } : physmap

   .pfncache ALIGN(4096) : AT(0)
   {
	_pfncache_start = .;
	. += 1 * 1024 * 1024;
	_pfncache_end = .;
   } : pfncache

   .info ALIGN (4096) : AT(0)
   {
	_info_start = .;
	. += 4096;
	_info_end = .;
   } : info

   .stree ALIGN (4096) : AT(0)
   {
	_stree_start = .;
	. += 16 * 1024 * 1024;
	_stree_end = .;
   } : stree

   .kmem ALIGN(4096) : AT (0)
   {
	_kmem_start = .;
	. += 512 * 1024 * 1024;
	_kmem_end = .;
   } : kmem

   .kva ALIGN(4096): AT (0)
   {
	_kva_start = .;
	. += 256 * 1024 * 1024;
	_kva_end = .;
   } : kva

   /* 16Mb Framebuffer. */
   .fbuffer ALIGN (4096) : AT(0)
   {
	_fbuf_start = .;
	. += 16 * 1024 * 1024;
	_fbuf_end = .;
   } : fbuf

   .linear ALIGN(8 * 1024 * 1024) : AT (0)
   {
	_linear_start = .;
	. += 8 * 1024 * 1024;
	_linear_end = .;

   } : linear
   _linear_l2table = (_linear_start + (_linear_start >> 9));
   _linear_l3table = (_linear_start + (_linear_start >> 9) + (_linear_start >> 18));

   _end = .;
}


PHDRS
{
    text    PT_LOAD;
    rodata  PT_LOAD;
    data    PT_LOAD;
    info    0xAF100000;
    kmem    0xAF100001;
    kva     0xAF100001;
    pfncache 0xAF100005;
    physmap 0xAF100002;
    stree   0xAF100004;
    fbuf    0xAF100006;
    linear  0xAF10FFFF;
}
