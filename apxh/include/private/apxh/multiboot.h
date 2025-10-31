/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/
#pragma once

/* multiboot.h - Multiboot header file. */
/* Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 *  DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#define __apxh_multiboot_h__ 1

/* How many bytes from the start of the file we search for the header. */
#define MULTIBOOT_SEARCH                        8192
#define __apxh_multiboot_h___ALIGN                  4

/* The magic field should contain this. */
#define __apxh_multiboot_h___MAGIC                  0x1BADB002

/* This should be in %eax. */
#define MULTIBOOT_BOOTLOADER_MAGIC              0x2BADB002

/* Alignment of multiboot modules. */
#define MULTIBOOT_MOD_ALIGN                     0x00001000

/* Alignment of the multiboot info structure. */
#define MULTIBOOT_INFO_ALIGN                    0x00000004

/* Flags set in the 'flags' member of the multiboot header. */

/* Align all boot modules on i386 page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN                    0x00000001

/* Must pass memory information to OS. */
#define MULTIBOOT_MEMORY_INFO                   0x00000002

/* Must pass video information to OS. */
#define MULTIBOOT_VIDEO_MODE                    0x00000004

/* This flag indicates the use of the address fields in the header. */
#define MULTIBOOT_AOUT_KLUDGE                   0x00010000

/* Flags to be set in the 'flags' member of the multiboot info structure. */

/* is there basic lower/upper memory information? */
#define MULTIBOOT_INFO_MEMORY                   0x00000001
/* is there a boot device set? */
#define MULTIBOOT_INFO_BOOTDEV                  0x00000002
/* is the command-line defined? */
#define MULTIBOOT_INFO_CMDLINE                  0x00000004
/* are there modules to do something with? */
#define MULTIBOOT_INFO_MODS                     0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_AOUT_SYMS                0x00000010
/* is there an ELF section header table? */
#define MULTIBOOT_INFO_ELF_SHDR                 0X00000020

/* is there a full memory map? */
#define MULTIBOOT_INFO_MEM_MAP                  0x00000040

/* Is there drive info? */
#define MULTIBOOT_INFO_DRIVE_INFO               0x00000080

/* Is there a config table? */
#define MULTIBOOT_INFO_CONFIG_TABLE             0x00000100

/* Is there a boot loader name? */
#define MULTIBOOT_INFO_BOOT_LOADER_NAME         0x00000200

/* Is there a APM table? */
#define MULTIBOOT_INFO_APM_TABLE                0x00000400

/* Is there video information? */
#define MULTIBOOT_INFO_VBE_INFO                 0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO         0x00001000

#ifndef ASM_FILE

typedef UINT8 MULTIBOOT_UINT8;
typedef unsigned short MULTIBOOT_UINT16;
typedef unsigned int MULTIBOOT_UINT32;
typedef unsigned long long MULTIBOOT_UINT64;

struct MULTIBOOT_HEADER
{
  /* Must be MULTIBOOT_MAGIC - see above. */
  MULTIBOOT_UINT32 Magic;

  /* Feature flags. */
  MULTIBOOT_UINT32 Flags;

  /* The above fields plus this one must equal 0 mod 2^32. */
  MULTIBOOT_UINT32 Checksum;

  /* These are only valid if MULTIBOOT_AOUT_KLUDGE is set. */
  MULTIBOOT_UINT32 HeaderAddr;
  MULTIBOOT_UINT32 LoadAddr;
  MULTIBOOT_UINT32 LoadEndAddr;
  MULTIBOOT_UINT32 BssEndAddr;
  MULTIBOOT_UINT32 EntryAddr;

  /* These are only valid if MULTIBOOT_VIDEO_MODE is set. */
  MULTIBOOT_UINT32 ModeType;
  MULTIBOOT_UINT32 Width;
  MULTIBOOT_UINT32 Height;
  MULTIBOOT_UINT32 Depth;
};

/* The symbol table for a.out. */
struct MULTIBOOT_AOUT_SYMBOL_TABLE
{
  MULTIBOOT_UINT32 TabSize;
  MULTIBOOT_UINT32 StrSize;
  MULTIBOOT_UINT32 Addr;
  MULTIBOOT_UINT32 Reserved;
};
typedef struct MULTIBOOT_AOUT_SYMBOL_TABLE MULTIBOOT_AOUT_SYMBOL_TABLE;

/* The section header table for ELF. */
struct MULTIBOOT_ELF_SECTION_HEADER_TABLE
{
  MULTIBOOT_UINT32 Num;
  MULTIBOOT_UINT32 Size;
  MULTIBOOT_UINT32 Addr;
  MULTIBOOT_UINT32 Shndx;
};
typedef struct MULTIBOOT_ELF_SECTION_HEADER_TABLE
  MULTIBOOT_ELF_SECTION_HEADER_TABLE;

struct MULTIBOOT_INFO
{
  /* Multiboot info version number */
  MULTIBOOT_UINT32 Flags;

  /* Available memory from BIOS */
  MULTIBOOT_UINT32 MemLower;
  MULTIBOOT_UINT32 MemUpper;

  /* "root" partition */
  MULTIBOOT_UINT32 BootDevice;

  /* Kernel command line */
  MULTIBOOT_UINT32 Cmdline;

  /* Boot-Module list */
  MULTIBOOT_UINT32 ModsCount;
  MULTIBOOT_UINT32 ModsAddr;

  union
  {
    MULTIBOOT_AOUT_SYMBOL_TABLE aout_sym;
    MULTIBOOT_ELF_SECTION_HEADER_TABLE elf_sec;
  } u;

  /* Memory Mapping buffer */
  MULTIBOOT_UINT32 MmapLength;
  MULTIBOOT_UINT32 MmapAddr;

  /* Drive Info buffer */
  MULTIBOOT_UINT32 DrivesLength;
  MULTIBOOT_UINT32 DrivesAddr;

  /* ROM configuration table */
  MULTIBOOT_UINT32 ConfigTable;

  /* Boot Loader Name */
  MULTIBOOT_UINT32 BootLoaderName;

  /* APM table */
  MULTIBOOT_UINT32 ApmTable;

  /* Video */
  MULTIBOOT_UINT32 VbeControlInfo;
  MULTIBOOT_UINT32 VbeModeInfo;
  MULTIBOOT_UINT16 VbeMode;
  MULTIBOOT_UINT16 VbeInterfaceSeg;
  MULTIBOOT_UINT16 VbeInterfaceOff;
  MULTIBOOT_UINT16 VbeInterfaceLen;

  MULTIBOOT_UINT64 FramebufferAddr;
  MULTIBOOT_UINT32 FramebufferPitch;
  MULTIBOOT_UINT32 FramebufferWidth;
  MULTIBOOT_UINT32 FramebufferHeight;
  MULTIBOOT_UINT8 FramebufferBpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT     2
  MULTIBOOT_UINT8 FramebufferType;
  union
  {
    struct
    {
      MULTIBOOT_UINT32 framebuffer_palette_addr;
      MULTIBOOT_UINT16 framebuffer_palette_num_colors;
    };
    struct
    {
      MULTIBOOT_UINT8 rpos;
      MULTIBOOT_UINT8 rsize;
      MULTIBOOT_UINT8 gpos;
      MULTIBOOT_UINT8 gsize;
      MULTIBOOT_UINT8 bpos;
      MULTIBOOT_UINT8 bsize;
    };
  };
};
typedef struct MULTIBOOT_INFO MULTIBOOT_INFO;

struct MULTIBOOT_COLOR
{
  MULTIBOOT_UINT8 Red;
  MULTIBOOT_UINT8 Green;
  MULTIBOOT_UINT8 Blue;
};

ANX_PACK_PUSH(1)
struct MULTIBOOT_MMAP_ENTRY
{
  MULTIBOOT_UINT32 Size;
  MULTIBOOT_UINT64 Addr;
  MULTIBOOT_UINT64 Len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
  MULTIBOOT_UINT32 Type;
};
ANX_PACK_POP()
typedef struct MULTIBOOT_MMAP_ENTRY MULTIBOOT_MEMORY_MAP;

struct MULTIBOOT_MOD_LIST
{
  /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
  MULTIBOOT_UINT32 ModStart;
  MULTIBOOT_UINT32 ModEnd;

  /* Module command line */
  MULTIBOOT_UINT32 Cmdline;

  /* padding to take it to 16 bytes (must be zero) */
  MULTIBOOT_UINT32 Pad;
};
typedef struct MULTIBOOT_MOD_LIST MULTIBOOT_MODULE;

/* APM BIOS info. */
struct MULTIBOOT_APM_INFO
{
  MULTIBOOT_UINT16 version;
  MULTIBOOT_UINT16 cseg;
  MULTIBOOT_UINT32 offset;
  MULTIBOOT_UINT16 cseg_16;
  MULTIBOOT_UINT16 dseg;
  MULTIBOOT_UINT16 Flags;
  MULTIBOOT_UINT16 cseg_len;
  MULTIBOOT_UINT16 cseg_16_len;
  MULTIBOOT_UINT16 dseg_len;
};

#endif /* ! ASM_FILE */

