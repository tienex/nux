/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __apxh_elfpayload_h__
#define __apxh_elfpayload_h__

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

/**
  Payload Header Structure

  Payloads are located at the end of the last kernel data/BSS
  address. The payload itself is prefixed by the following header.
**/
typedef struct _PAYLOAD_HDR
{
  UINT64 magic;
  UINT64 filename;
  UINT32 size;
} __attribute__((packed)) PAYLOAD_HDR, *PPAYLOAD_HDR, *PCPAYLOAD_HDR;

/** Legacy compatibility **/
#define payload_hdr PAYLOAD_HDR



#endif
