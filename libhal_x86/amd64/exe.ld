ENTRY(_start)
SECTIONS
{
    . = 0xffff800000000000;

    .text : 
    {
        *(.text)
        *(.rodata*)
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

   .info ALIGN(4096) :
   {
	_info_start = .;
	. += 4096;
	_info_end = .;
   } : info

   .stree ALIGN (4096) : AT(0)
   {
	_stree_start = .;
	. += 3 * 1024 * 1024;
	_stree_end = .;
   } : stree

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
    info    0xAF100000;
    kmem    0xAF100001;
    kva     0xAF100001;
    pfncache 0xAF100005;
    physmap 0xAF100002;
    stree   0xAF100004;
    linear  0xAF10FFFF;
}
