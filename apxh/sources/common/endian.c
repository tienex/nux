/** @file
  APXH Endianness Conversion

  Implements endianness conversion for boot structures.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/endian.h>

/**
  Convert APXH_BOOT_INFO structure to target endianness.

  @param[in,out] BootInfo      Boot info structure to convert.
  @param[in]     TargetEndian  Target endianness.
**/
VOID
ConvertBootInfoEndianness (
  IN OUT APXH_BOOT_INFO  *BootInfo,
  IN     IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    // No conversion needed
    return;
  }

  // Convert all UINT64 fields
  BootInfo->Magic = Swap64(BootInfo->Magic);
  BootInfo->MaxRamPfn = Swap64(BootInfo->MaxRamPfn);
  BootInfo->MaxPfn = Swap64(BootInfo->MaxPfn);
  BootInfo->NumRegions = Swap64(BootInfo->NumRegions);
  BootInfo->UserEntry = Swap64(BootInfo->UserEntry);

  // Convert framebuffer descriptor
  if (BootInfo->FramebufferDesc.Type != FB_INVALID) {
    BootInfo->FramebufferDesc.Type = Swap32(BootInfo->FramebufferDesc.Type);
    BootInfo->FramebufferDesc.PhysicalAddress = Swap64(BootInfo->FramebufferDesc.PhysicalAddress);
    BootInfo->FramebufferDesc.Width = Swap32(BootInfo->FramebufferDesc.Width);
    BootInfo->FramebufferDesc.Height = Swap32(BootInfo->FramebufferDesc.Height);
    BootInfo->FramebufferDesc.Pitch = Swap32(BootInfo->FramebufferDesc.Pitch);
    BootInfo->FramebufferDesc.Bpp = Swap32(BootInfo->FramebufferDesc.Bpp);
    BootInfo->FramebufferDesc.RedMask = Swap32(BootInfo->FramebufferDesc.RedMask);
    BootInfo->FramebufferDesc.GreenMask = Swap32(BootInfo->FramebufferDesc.GreenMask);
    BootInfo->FramebufferDesc.BlueMask = Swap32(BootInfo->FramebufferDesc.BlueMask);
    BootInfo->FramebufferDesc.RedShift = Swap32(BootInfo->FramebufferDesc.RedShift);
    BootInfo->FramebufferDesc.GreenShift = Swap32(BootInfo->FramebufferDesc.GreenShift);
    BootInfo->FramebufferDesc.BlueShift = Swap32(BootInfo->FramebufferDesc.BlueShift);
  }

  // Convert platform descriptor
  BootInfo->PlatformDesc.Type = Swap32(BootInfo->PlatformDesc.Type);
  BootInfo->PlatformDesc.PlatformPointer = Swap64(BootInfo->PlatformDesc.PlatformPointer);

  // Convert kernel TLS info
  BootInfo->KernelTls.InitializedDataVaddr = Swap64(BootInfo->KernelTls.InitializedDataVaddr);
  BootInfo->KernelTls.InitializedDataSize = Swap64(BootInfo->KernelTls.InitializedDataSize);
  BootInfo->KernelTls.TotalSize = Swap64(BootInfo->KernelTls.TotalSize);

  // Convert user TLS info
  BootInfo->UserTls.InitializedDataVaddr = Swap64(BootInfo->UserTls.InitializedDataVaddr);
  BootInfo->UserTls.InitializedDataSize = Swap64(BootInfo->UserTls.InitializedDataSize);
  BootInfo->UserTls.TotalSize = Swap64(BootInfo->UserTls.TotalSize);

  // Convert architecture and mixed-mode info
  BootInfo->KernelArchitecture = Swap32(BootInfo->KernelArchitecture);
  BootInfo->UserArchitecture = Swap32(BootInfo->UserArchitecture);
  BootInfo->HostArchitecture = Swap32(BootInfo->HostArchitecture);
  BootInfo->MixedModeFlags = Swap32(BootInfo->MixedModeFlags);

  // Convert endianness info
  BootInfo->KernelEndianness = Swap32(BootInfo->KernelEndianness);
  BootInfo->UserEndianness = Swap32(BootInfo->UserEndianness);
  BootInfo->MixedEndian = Swap32(BootInfo->MixedEndian);
}

/**
  Convert APXH_REGION structure to target endianness.

  @param[in,out] Region        Memory region to convert.
  @param[in]     TargetEndian  Target endianness.
**/
VOID
ConvertRegionEndianness (
  IN OUT APXH_REGION    *Region,
  IN     IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    // No conversion needed
    return;
  }

  Region->Type = Swap32(Region->Type);
  Region->Pfn = Swap64(Region->Pfn);
  Region->Length = Swap64(Region->Length);
}

/**
  Convert APXH_BATREE header to target endianness.

  @param[in,out] BatreeHeader  BAtree header to convert.
  @param[in]     TargetEndian  Target endianness.
**/
VOID
ConvertBatreeHeaderEndianness (
  IN OUT APXH_BATREE    *BatreeHeader,
  IN     IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    // No conversion needed
    return;
  }

  BatreeHeader->Magic = Swap64(BatreeHeader->Magic);
  // Version and Order are UINT8, no swap needed
  BatreeHeader->Offset = Swap16(BatreeHeader->Offset);
  BatreeHeader->Size = Swap64(BatreeHeader->Size);
}
