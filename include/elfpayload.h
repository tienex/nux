/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef ELF_PAYLOAD_H
#define ELF_PAYLOAD_H

#include <stddef.h>
#include <inttypes.h>

/*

  ELF Payload.

*/

/* Get payload address. */
void *payload_get (unsigned i, size_t *size);


/*

  Internal structures for ELF Payloads.
*/

/*
  The payload magic: DEC RAD-50 encoding of "nux-payload".
*/
#define ELFPAYLOAD_MAGIC 0x54a2f911659dece0LL

/*
  Payload header structure.

  Payloads are located at the end of the last kernel data/BSS
  address. The payload itself is prefixed by the following header.
*/
struct payload_hdr
{
  uint64_t magic;
  uint64_t filename;
  uint32_t size;
} __attribute__((packed));



#endif
