OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
   /* kernel code */
   .text 0x00100000 : AT (0x00100000)
   {
      *(.text._start)
      *(.text)
      _ecode = .;
   }

   /* kernel data */
   .data . :
   {
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

}
