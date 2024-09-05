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
  .preinit_array    :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  }
  .init_array    :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  }
  .fini_array    :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
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
