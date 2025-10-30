/** @file
  APXH EFI Main Entry Point

  Main UEFI application entry point for APXH bootloader. Initializes
  UEFI boot environment, loads ELF payloads from filesystem, retrieves
  memory map and graphics framebuffer, and launches bootloader.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <efi.h>
#include <efilib.h>

#include <apxh/uefi/internal.h>

static EFI_HANDLE gImageHandle;
static EFI_LOADED_IMAGE *gpImg = NULL;

/**
  Allocate page at maximum address.

  Allocates a single page below the specified maximum address using
  AllocateMaxAddress strategy. Used for bootloader memory allocation
  to ensure boot structures fit below memory constraints.

  @param[in] MaxAddr  Maximum physical address for allocation.

  @return Physical address of allocated page.
**/
UINTN
EfiAllocateMaxAddr (
  IN UINTN  MaxAddr
  )
{
  EFI_STATUS EfiStatus;
  VOID *Addr;

  EfiStatus = uefi_call_wrapper (BS->AllocatePages, 4,
				  AllocateMaxAddress,
				  EfiLoaderData, 1, &MaxAddr);
  if (EFI_ERROR (EfiStatus))
    {
      Print (L"Allocate Pages Failed: %r\n", EfiStatus);
      exit (-1);
    }

  Addr = (VOID *) MaxAddr;
  memset (Addr, 0, 4096);
  return (UINTN) Addr;
}

/**
  Output character.

  Outputs a character to EFI console using Print service.

  @param[in] C  Character to output.

  @return Character written (same as input).
**/
int
Putchar (
  IN int  C
  )
{
  Print (L"%c", C);
}

static VOID *gpPayloadStart;
static UINTN gPayloadSize;

/**
  Get framebuffer from Graphics Output Protocol.

  Queries EFI Graphics Output Protocol (GOP) for available video modes,
  selects the highest resolution mode, and retrieves framebuffer
  information (address, size, pixel format, dimensions).

  @retval EFI_SUCCESS  Framebuffer info retrieved successfully.
  @retval other        GOP protocol not found or mode query failed.
**/
EFI_STATUS
EfiGetFramebuffer (
  VOID
  )
{
  extern EFI_GUID GraphicsOutputProtocol;
  EFI_STATUS Rc;
  UINTN InfoSize;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  UINT32 RMask, GMask, BMask, Bpp, Pitch, Width, Height;
  UINT64 Addr, Size;

  Rc = LibLocateProtocol (&GraphicsOutputProtocol, (VOID **) &Gop);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Cannot locate Graphic Output Proto (%r)\n", Rc);
      return Rc;
    }

  if (Gop->Mode == NULL)
    {
      Print (L"Mode not found in GOP.\n");
      return EFI_SUCCESS;
    }

  UINTN i;
  UINTN MMax = 0;
  UINTN RMax = 0;
  UINTN IMax = Gop->Mode->MaxMode;

  /* Search for highest (horizontal) resolution. */
  for (i = 0; i < IMax; i++)
    {
      UINTN InfoSz;
      Rc = uefi_call_wrapper (Gop->QueryMode, 4, Gop, i, &InfoSz, &Info);
      if (Rc != EFI_SUCCESS)
	continue;

      Print (L"EFI GFX Mode %d: %ldx%ld.\n",
	     i, Info->HorizontalResolution, Info->VerticalResolution);

      if ((UINTN) Info->HorizontalResolution * Info->VerticalResolution >
	  RMax)
	{
	  RMax =
	    (UINTN) Info->HorizontalResolution * Info->VerticalResolution;
	  MMax = i;
	}
    }

  Print (L"Setting mode %d\n", MMax);
  Rc = uefi_call_wrapper (Gop->SetMode, 2, Gop, MMax);

  Info = Gop->Mode->Info;
  if (Info == NULL)
    {
      Print (L"Info not found in GOP.\n");
      return EFI_SUCCESS;
    }

  switch (Info->PixelFormat)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
      RMask = 0x0000ff;
      GMask = 0x00ff00;
      BMask = 0xff0000;
      Bpp = 32;
      break;

    case PixelBlueGreenRedReserved8BitPerColor:
      RMask = 0xff0000;
      GMask = 0x00ff00;
      BMask = 0x0000ff;
      Bpp = 32;
      break;

    case PixelBitMask:
      {
	UINT32 Mask;

	RMask = Info->PixelInformation.RedMask;
	GMask = Info->PixelInformation.GreenMask;
	BMask = Info->PixelInformation.BlueMask;

	Mask = (RMask | GMask | BMask
		| Info->PixelInformation.ReservedMask);
	Bpp = __builtin_popcountl ((long) Mask);
	break;
      }
    case PixelBltOnly:
    default:
      Print (L"No Framebuffer (pixel format is %d)\n", Info->PixelFormat);
      return EFI_SUCCESS;
    }

  Addr = Gop->Mode->FrameBufferBase;
  Size = Gop->Mode->FrameBufferSize;
  Pitch = (UINT32) ((UINT64) Info->PixelsPerScanLine * Bpp / 8);
  Width = Info->HorizontalResolution;
  Height = Info->VerticalResolution;

  Print (L"Framebuffer found:\n"
	 "        ADDR: %lx\n        SIZE: %lx\n"
	 "        WIDTH: %d\n        HEIGHT: %d\n"
	 "        PITCH: %d\n        BPP: %d\n"
	 "        RMASK: %08lx        GMASK: %08lx        BMASK: %08lx\n",
	 Addr, Size, Width, Height, Pitch, Bpp, RMask, GMask, BMask);

  ApxhEfiAddFramebuffer (Addr, Size, Width, Height, Pitch, Bpp, RMask,
			   GMask, BMask);

}

/**
  Load ELF payload from filesystem.

  Opens and reads ELF payload file from EFI filesystem using Simple File
  protocol. Grows buffer as needed to accommodate payload size.

  @param[in]  Name  Wide-character filename (e.g., L"kernel.elf").
  @param[out] ppPtr  Pointer to receive payload buffer address.
  @param[out] Size  Pointer to receive payload size.

  @retval EFI_SUCCESS  Payload loaded successfully.
  @retval other        File not found or read failed.
**/
EFI_STATUS
EfiGetPayload (
  IN CHAR16          *Name,
  OUT VOID           **ppPtr,
  OUT UINTN  *Size
  )
{
  EFI_STATUS Rc;
  EFI_HANDLE Hdl;
  SIMPLE_READ_FILE RdHdl;
  EFI_DEVICE_PATH *FilePath;

  FilePath = FileDevicePath (gpImg->DeviceHandle, Name);

  Rc = OpenSimpleReadFile (TRUE, NULL, 0, &FilePath, &Hdl, &RdHdl);
  if (EFI_ERROR (Rc))
    {
      Print (L"OpenSimpleReadFile failed %r\n", Rc);
      return Rc;
    }

  Rc = EFI_SUCCESS;
  *ppPtr = NULL;
  *Size = 8 * 1024 * 1024;

  while (GrowBuffer (&Rc, ppPtr, *Size))
    {
      Print (L"GrowBuffer!\n");
      Rc = ReadSimpleReadFile (RdHdl, 0, Size, *ppPtr);
    }

  if (EFI_ERROR (Rc))
    {
      Print (L"ReadSimpleReadFile failed: %r\n", Rc);
      return Rc;
    }

  CloseSimpleReadFile (RdHdl);

  return EFI_SUCCESS;
}

/**
  Retrieve EFI memory map.

  Retrieves EFI memory map and converts it to bootloader memory region
  format. Classifies regions as RAM (available/busy) or other types based
  on EFI memory type.
**/
VOID
EfiGetMemoryMap (
  VOID
  )
{
  UINTN Num, i;
  UINTN Key;
  UINTN DescSize;
  UINT32 DescVer;
  EFI_MEMORY_DESCRIPTOR *Md, *Ptr;


  Md = LibMemoryMap (&Num, &Key, &DescSize, &DescVer);

  Print (L"Found %ld memory entries, Key: %ld, Size: %ld, Version: %d\n",
	 Num, Key, DescSize, DescVer);

  for (i = 0; i < Num; i++)
    {
INT32 Ram, Bsy;
      UINT32 Len;
      UINTN Pfn;

      Ptr = (VOID *) Md + i * DescSize;

      switch (Ptr->Type)
	{
	case EfiReservedMemoryType:
	case EfiUnusableMemory:
	case EfiACPIReclaimMemory:
	case EfiACPIMemoryNVS:
	case EfiMemoryMappedIO:
	  Ram = 0;
	  Bsy = 1;
	  break;

	case EfiLoaderCode:
	case EfiLoaderData:
	case EfiBootServicesCode:
	case EfiBootServicesData:
	case EfiConventionalMemory:
	  Ram = 1;
	  Bsy = 0;
	  break;

	case EfiRuntimeServicesCode:
	case EfiRuntimeServicesData:
	  Ram = 1;
	  Bsy = 1;
	  break;

	case EfiMemoryMappedIOPortSpace:
	case EfiPalCode:
	default:
	  continue;
	}

      Pfn = Ptr->PhysicalStart >> 12;
      Len = Ptr->NumberOfPages;

      ApxhEfiAddMemRegion (Ram, Bsy, Pfn, Len);
    }
}

/**
  Retrieve ACPI RSDP from EFI system table.

  Searches EFI system configuration table for ACPI 2.0 RSDP, falling
  back to ACPI 1.0 if not found. Stores pointer in platform descriptor.

  @retval EFI_SUCCESS  RSDP retrieved (may be NULL if not found).
**/
EFI_STATUS
EfiGetRsdp (
  VOID
  )
{
  EFI_GUID GuidRsdp20 = ACPI_20_TABLE_GUID;
  EFI_GUID GuidRsdp = ACPI_TABLE_GUID;
  VOID *Rsdp;

  LibGetSystemConfigurationTable (&GuidRsdp20, &Rsdp);
  if (Rsdp == NULL)
    LibGetSystemConfigurationTable (&GuidRsdp, &Rsdp);

  if (Rsdp == NULL)
    Print (L"No RSDP found!\n");

  ApxhEfiAddRsdp (Rsdp);
}


/**
  Main EFI application entry point.

  UEFI firmware entry point for APXH bootloader. Initializes EFI libraries,
  loads kernel and optional user payloads, retrieves memory map and
  graphics info, then launches main bootloader.

  @param[in] ImageHandle   EFI image handle.
  @param[in] SystemTable  EFI system table pointer.

  @retval EFI_SUCCESS  Bootloader launched successfully (does not return).
  @retval other        Initialization or payload loading failed.
**/
EFI_STATUS EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID *Ptr;
  EFI_STATUS Rc;
  EFI_GUID ImgProt = LOADED_IMAGE_PROTOCOL;

  InitializeLib (ImageHandle, SystemTable);
  gImageHandle = ImageHandle;

  Rc = uefi_call_wrapper (BS->OpenProtocol, 6, ImageHandle, &ImgProt, &Ptr,
			  ImageHandle, NULL,
			  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Open Protocol failed: %r\n", Rc);
      return Rc;
    }

  gpImg = Ptr;

  /*
     Get payloads.
   */
  Rc = EfiGetPayload (L"kernel.elf", &gpPayloadStart, &gPayloadSize);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Could not load kernel.elf: %r\n", Rc);
      return Rc;
    }
  ApxhEfiAddKernelPayload (gpPayloadStart, gPayloadSize);

  Rc = EfiGetPayload (L"user.elf", &gpPayloadStart, &gPayloadSize);
  if (Rc == EFI_SUCCESS)
    {
      Print (L"Loading user.elf");
      ApxhEfiAddUserPayload (gpPayloadStart, gPayloadSize);
    }

  /*
     Populate memory map.
   */
  EfiGetMemoryMap ();

  /*
     Get Framebuffer info.
   */
  EfiGetFramebuffer ();

  /*
     Get RSDP.
   */
  EfiGetRsdp ();

  /*
     Launch APXH.
   */
  main (0, 0);

  return EFI_SUCCESS;
}

/**
  Exit EFI boot services.

  Retrieves final memory map and calls ExitBootServices to transition
  from boot environment to runtime. After this call, boot-time services
  are no longer available.
**/
VOID
EfiExitBs (
  VOID
  )
{
  EFI_STATUS Rc;
  UINTN Num, i;
  UINTN Key;
  UINTN DescSize;
  UINT32 DescVer;
  EFI_MEMORY_DESCRIPTOR *Md;


  Md = LibMemoryMap (&Num, &Key, &DescSize, &DescVer);

  Rc = uefi_call_wrapper (BS->ExitBootServices, 2, gImageHandle, Key);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"EBS failed: %r\n", Rc);
      /* XXX: efi_exit */
      return;
    }
}

/**
  Exit EFI application.

  Stub function for exiting EFI application. Currently empty as control
  is transferred to kernel via MdEntry trampoline.

  @param[in] Status  Exit status code (unused).
**/
VOID
EfiExit (
  IN int  Status
  )
{
}