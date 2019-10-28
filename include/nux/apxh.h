/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _APXH_H
#define _APXH_H

#include <stdint.h>

struct apxh_bootinfo
{
#define APXH_BOOTINFO_MAGIC 0xAF10B007
  uint64_t magic;
  uint64_t maxpfn;
  uint64_t acpi_rsdp;
} __attribute__((packed));

struct apxh_stree
{
#define APXH_STREE_MAGIC 0xAF1057EE
  uint64_t magic;
#define APXH_STREE_VERSION 0
  uint8_t version;
  uint8_t order;
  uint16_t offset;
  uint32_t size;
} __attribute__((packed));

#endif
