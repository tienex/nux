OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv",
	      "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	. = 0x82000000;
	.text :
	{
		KEEP(crt0.o(.text))
		*(.text)
	}
	.data :
	{
		*(.data)
		*(.bss)
	}
}
