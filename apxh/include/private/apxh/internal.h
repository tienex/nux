/** @file
  APXH Internal API

  Main internal header that includes all APXH subsystem headers.
  This is the primary header included by APXH implementation files.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

//
// Standard Library
//

#include <assert.h>
#include <stdio.h>
#include <string.h>

//
// Platform Interfaces
//

#include <ananke/ananke.h>
#include <nux/framebuffer.h>
#include <apxh/apxh.h>

//
// APXH Subsystem Headers
//

#include <apxh/types.h>
#include <apxh/platform.h>
#include <apxh/vas.h>
#include <apxh/boot.h>
#include <apxh/imgload.h>

//
// Architecture-Specific Headers
//

#include <apxh/arch/pae.h>
#include <apxh/arch/pae64.h>
#include <apxh/arch/sv48.h>

//
// Constants
//

#define BOOTMEM MB(512)  ///< Maximum boot memory: 512MB
#define PFNMAP_ENTRY_SIZE 64

#define PAGE_SHIFT 12
#define PAGE_SIZE (1LL << PAGE_SHIFT)
#define PAGE_MASK ~(PAGE_SIZE - 1)

#define PAGE2M_SHIFT 21
#define PAGE2M_SIZE (1LL << PAGE2M_SHIFT)
#define PAGE2M_MASK ~(PAGE2M_SIZE - 1)

//
// Macros
//

#define MB(_x) ((UINTN)(_x) << 20)
#define BITMAP_SZ(_s) ((_s) >> 3)
#define PAGEMAP_SZ(_s) BITMAP_SZ((_s) >> PAGE_SHIFT)
#define PAGE_FLOOR(_a) ((_a) & PAGE_MASK)
#define PAGE_CEILING(_a) (((_a) + PAGE_SIZE - 1) & PAGE_MASK)
#define IS_PAGE_ALIGNED(_a) (((_a) & ~PAGE_MASK) == 0)
