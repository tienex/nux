/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _APXH_H
#define _APXH_H

#include <stdint.h>
#include <framebuffer.h>

struct apxh_pltdesc
{
#define PLT_UNKNOWN 0
#define PLT_ACPI 1
#define PLT_DTB 2
  uint64_t type;
  uint64_t pltptr;		/* ACPI or DTB root */
} __attribute__((packed));

struct apxh_tlsinfo
{
  uint64_t initvaddr;		/* Prototype Initialized Data Address. */
  uint64_t initsize;		/* Prototype Initialized Data Size. */
  uint64_t size;		/* Total Size, including per-thread BSS. */
};

struct apxh_bootinfo
{
#define APXH_BOOTINFO_MAGIC 0xAF10B007
  uint64_t magic;
  uint64_t maxrampfn;
  uint64_t maxpfn;
  uint64_t numregions;
  uint64_t uentry;
  struct fbdesc fbdesc;
  struct apxh_pltdesc pltdesc;
  struct apxh_tlsinfo ktls;
  struct apxh_tlsinfo utls;
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


struct apxh_region
{
#define APXH_REGION_UNKNOWN 0
#define APXH_REGION_RAM 1
#define APXH_REGION_MMIO 2
#define APXH_REGION_BSY 3
  uint64_t type:2;
  uint64_t pfn:62;
  uint64_t len;
} __attribute__((packed));

#endif
