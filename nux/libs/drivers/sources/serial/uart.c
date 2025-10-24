/** @file
  x86 Serial Port Driver

  Provides serial port initialization and output for debugging.
  Configures COM1 (0x3F8) for 115200 8N1 operation.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "internal.h"

#define SERIAL_PORT  0x3F8  ///< COM1 base I/O port

/**
  Initialize the serial port.

  Configures COM1 for 115200 baud, 8 data bits, no parity, 1 stop bit.
  Disables interrupts and enables FIFO.
**/
VOID
SerialInitialize (
  VOID
  )
{
  OutB (SERIAL_PORT + 1, 0);     // Disable interrupts
  OutB (SERIAL_PORT + 3, 0x80);  // Enable DLAB (set baud rate divisor)
  OutB (SERIAL_PORT + 0, 3);     // Set divisor to 3 (38400 baud)
  OutB (SERIAL_PORT + 1, 0);     // High byte of divisor
  OutB (SERIAL_PORT + 3, 3);     // 8 bits, no parity, one stop bit
  OutB (SERIAL_PORT + 2, 0xC7);  // Enable FIFO, clear, 14-byte threshold
  OutB (SERIAL_PORT + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

/**
  Output a character to the serial port.

  Waits for the transmit buffer to be empty, then sends the character.

  @param[in] Ch  Character to output.
**/
VOID
SerialPutChar (
  IN INT32  Ch
  )
{
  // Wait for transmit buffer to be empty
  while (!(InB (SERIAL_PORT + 5) & 0x20))
    ;

  OutB (SERIAL_PORT, Ch);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use SerialInitialize instead **/
void serial_init (void) {
  SerialInitialize ();
}

/** @deprecated Use SerialPutChar instead **/
void serial_putchar (int c) {
  SerialPutChar (c);
}
