ENTRY(_start)
SECTIONS
{
   /* kernel code */
   .text 0x82000000 : AT (0x82000000)
   {
      *(.text._start)
      *(.text)
      _ecode = .;
   }

   /* kernel data */
   .data ALIGN(4096) :
   {
      _sdata = .;
      *(.rodata.str?.*);
      *(.rodata);
      *(.data);
      *(.shstrtab);
      _edata = .;
   }

   /* kernel BSS */
   .bss . :
   {
      *(COMMON);
      _sbss = .;
      *(.bss);
      _ebss = .;
   }

   .eh_frame . : {
      *(.eh_frame);
   }

  .ctors . :
  {
     KEEP (*crtbegin.o(.ctors))
     KEEP (*crtbegin?.o(.ctors))
     KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
     KEEP (*(SORT(.ctors.*)))
     KEEP (*(.ctors))
  }

  .dtors .  :
  {
     KEEP (*crtbegin.o(.dtors))
     KEEP (*crtbegin?.o(.dtors))
     KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
     KEEP (*(SORT(.dtors.*)))
     KEEP (*(.dtors))
  }

  _end = .;

  __global_pointer$ = _edata;

}
