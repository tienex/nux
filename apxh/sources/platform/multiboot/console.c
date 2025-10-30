/** @file
  APXH Multiboot Environment Functions

  Provides basic environment functions for Multiboot platform, including
  VGA text mode console output, serial port I/O, and exit handling.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

#define SERIAL_PORT 0x3f8

/**
  Input byte from I/O port.

  Reads a byte from specified I/O port using x86 IN instruction.

  @param[in] Port  I/O port number.

  @return Byte value read from port.
**/
int
Inb (
  IN INT32 Port
  )
{
INT32 Ret;

  asm volatile ("xor %%eax, %%eax; inb %%dx, %%al":"=a" (Ret):"d" (Port));
  return Ret;
}

/**
  Output byte to I/O port.

  Writes a byte to specified I/O port using x86 OUT instruction.

  @param[in] Port  I/O port number.
  @param[in] Val   Byte value to write.
**/
VOID
Outb (
  IN INT32 Port,
  IN INT32 Val
  )
{
  asm volatile ("outb %%al, %%dx"::"d" (Port), "a" (Val));
}

/**
  Output character to console.

  Outputs character to both VGA text mode display (0xB8000) and
  serial port (COM1). Implements basic scrolling and newline handling.

  @param[in] C  Character to output.
**/
VOID
Putchar (
  IN INT32 C
  )
{
  CONST UINT8 *VPtr = (CONST VOID *) 0xb8000;
  static int gInit = 0;
  static int gX = 0;
  static int gY = 0;

  if (!gInit)
    {
INT32 i;
      for (i = 0; i < 80 * 25; i++)
	*(UINT8 *) (VPtr + i * 2) = 0;

      Outb (SERIAL_PORT + 1, 0);
      Outb (SERIAL_PORT + 3, 0x80);
      Outb (SERIAL_PORT + 0, 3);
      Outb (SERIAL_PORT + 1, 0);
      Outb (SERIAL_PORT + 3, 3);
      Outb (SERIAL_PORT + 2, 0xc7);
      Outb (SERIAL_PORT + 4, 0xb);

      gInit = 1;
    }

  while (!(Inb (SERIAL_PORT + 5) & 0x20));
  Outb (SERIAL_PORT, C);

  if (C == '\n')
    {
      gY += gX / 80 + 1;
      gX = 0;
      return;
    }

  if (80 * gY + gX >= 80 * 25)
    {
      INT32 i;
      memmove ((VOID *) VPtr, (VOID *) VPtr + 80 * 2, 80 * 2 * (25 - 1));
      for (i = 0; i < 80; i++)
	*(UINT8 *) (VPtr + 80 * 2 * (25 - 1) + i * 2) = 0;
      gY = 25 - 1;
      gX = 0;
    }

  *(UINT8 *) ((VOID *) 0xb8000 + gX++ * 2 + gY * 80 * 2) = C;
  return;
}


/**
  Exit bootloader.

  Terminates bootloader execution. On Multiboot platform, enters
  infinite loop.

  @param[in] Status  Exit status code.
**/
VOID
Exit (
  IN INT32  Status
  )
{
  printf ("exit(%d) called.\n", Status);

  while (1);
}
