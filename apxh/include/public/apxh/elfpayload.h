/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/
#pragma once

#include <ananke/ananke.h>

/*

  ELF Payload.

*/

/* Get payload address. */
VOID *PayloadGet (IN UINT32 Index, OUT OPTIONAL UINTN *Size);


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
ANX_PACK_PUSH(1)
typedef struct _PAYLOAD_HDR
{
  UINT64 Magic;
  UINT64 Filename;
  UINT32 Size;
} ANX_PACKED PAYLOAD_HDR, *PPAYLOAD_HDR, *PCPAYLOAD_HDR;

ANX_PACK_POP()



