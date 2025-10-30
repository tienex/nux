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
unsigned long
EfiAllocateMaxAddr (
  IN unsigned long  MaxAddr
  )
{
  EFI_STATUS efi_status;
  VOID *pAddr;

  efi_status = uefi_call_wrapper (BS->AllocatePages, 4,
				  AllocateMaxAddress,
				  EfiLoaderData, 1, &MaxAddr);
  if (EFI_ERROR (efi_status))
    {
      Print (L"Allocate Pages Failed: %r\n", efi_status);
      exit (-1);
    }

  pAddr = (VOID *) MaxAddr;
  memset (pAddr, 0, 4096);
  return (unsigned long) pAddr;
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
static unsigned long gPayloadSize;

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
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *pInfo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *pGop;
  UINT32 RMask, GMask, BMask, Bpp, Pitch, Width, Height;
  UINT64 Addr, Size;

  Rc = LibLocateProtocol (&GraphicsOutputProtocol, (void **) &pGop);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Cannot locate Graphic Output Proto (%r)\n", Rc);
      return Rc;
    }

  if (pGop->Mode == NULL)
    {
      Print (L"Mode not found in GOP.\n");
      return EFI_SUCCESS;
    }

  UINTN i;
  UINTN MMax = 0;
  UINTN RMax = 0;
  UINTN IMax = pGop->Mode->MaxMode;

  /* Search for highest (horizontal) resolution. */
  for (i = 0; i < IMax; i++)
    {
      UINTN InfoSz;
      Rc = uefi_call_wrapper (pGop->QueryMode, 4, pGop, i, &InfoSz, &pInfo);
      if (Rc != EFI_SUCCESS)
	continue;

      Print (L"EFI GFX Mode %d: %ldx%ld.\n",
	     i, pInfo->HorizontalResolution, pInfo->VerticalResolution);

      if ((UINTN) pInfo->HorizontalResolution * pInfo->VerticalResolution >
	  RMax)
	{
	  RMax =
	    (UINTN) pInfo->HorizontalResolution * pInfo->VerticalResolution;
	  MMax = i;
	}
    }

  Print (L"Setting mode %d\n", MMax);
  Rc = uefi_call_wrapper (pGop->SetMode, 2, pGop, MMax);

  pInfo = pGop->Mode->Info;
  if (pInfo == NULL)
    {
      Print (L"Info not found in GOP.\n");
      return EFI_SUCCESS;
    }

  switch (pInfo->PixelFormat)
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

	RMask = pInfo->PixelInformation.RedMask;
	GMask = pInfo->PixelInformation.GreenMask;
	BMask = pInfo->PixelInformation.BlueMask;

	Mask = (RMask | GMask | BMask
		| pInfo->PixelInformation.ReservedMask);
	Bpp = __builtin_popcountl ((long) Mask);
	break;
      }
    case PixelBltOnly:
    default:
      Print (L"No Framebuffer (pixel format is %d)\n", pInfo->PixelFormat);
      return EFI_SUCCESS;
    }

  Addr = pGop->Mode->FrameBufferBase;
  Size = pGop->Mode->FrameBufferSize;
  Pitch = (UINT32) ((UINT64) pInfo->PixelsPerScanLine * Bpp / 8);
  Width = pInfo->HorizontalResolution;
  Height = pInfo->VerticalResolution;

  Print (L"Framebuffer found:\n"
	 "        ADDR: %lx\n        SIZE: %lx\n"
	 "        WIDTH: %d\n        HEIGHT: %d\n"
	 "        PITCH: %d\n        BPP: %d\n"
	 "        RMASK: %08lx        GMASK: %08lx        BMASK: %08lx\n",
	 Addr, Size, Width, Height, Pitch, Bpp, RMask, GMask, BMask);

  apxhefi_add_framebuffer (Addr, Size, Width, Height, Pitch, Bpp, RMask,
			   GMask, BMask);

}

/**
  Load ELF payload from filesystem.

  Opens and reads ELF payload file from EFI filesystem using Simple File
  protocol. Grows buffer as needed to accommodate payload size.

  @param[in]  pName  Wide-character filename (e.g., L"kernel.elf").
  @param[out] ppPtr  Pointer to receive payload buffer address.
  @param[out] pSize  Pointer to receive payload size.

  @retval EFI_SUCCESS  Payload loaded successfully.
  @retval other        File not found or read failed.
**/
EFI_STATUS
EfiGetPayload (
  IN CHAR16          *pName,
  OUT VOID           **ppPtr,
  OUT unsigned long  *pSize
  )
{
  EFI_STATUS Rc;
  EFI_HANDLE Hdl;
  SIMPLE_READ_FILE RdHdl;
  EFI_DEVICE_PATH *pFilePath;

  pFilePath = FileDevicePath (gpImg->DeviceHandle, pName);

  Rc = OpenSimpleReadFile (TRUE, NULL, 0, &pFilePath, &Hdl, &RdHdl);
  if (EFI_ERROR (Rc))
    {
      Print (L"OpenSimpleReadFile failed %r\n", Rc);
      return Rc;
    }

  Rc = EFI_SUCCESS;
  *ppPtr = NULL;
  *pSize = 8 * 1024 * 1024;

  while (GrowBuffer (&Rc, ppPtr, *pSize))
    {
      Print (L"GrowBuffer!\n");
      Rc = ReadSimpleReadFile (RdHdl, 0, pSize, *ppPtr);
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
  EFI_MEMORY_DESCRIPTOR *pMd, *pPtr;


  pMd = LibMemoryMap (&Num, &Key, &DescSize, &DescVer);

  Print (L"Found %ld memory entries, Key: %ld, Size: %ld, Version: %d\n",
	 Num, Key, DescSize, DescVer);

  for (i = 0; i < Num; i++)
    {
      int Ram, Bsy;
      unsigned Len;
      unsigned long Pfn;

      pPtr = (void *) pMd + i * DescSize;

      switch (pPtr->Type)
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

      Pfn = pPtr->PhysicalStart >> 12;
      Len = pPtr->NumberOfPages;

      apxhefi_add_memregion (Ram, Bsy, Pfn, Len);
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
  VOID *pRsdp;

  LibGetSystemConfigurationTable (&GuidRsdp20, &pRsdp);
  if (pRsdp == NULL)
    LibGetSystemConfigurationTable (&GuidRsdp, &pRsdp);

  if (pRsdp == NULL)
    Print (L"No RSDP found!\n");

  apxhefi_add_rsdp (pRsdp);
}


/**
  Main EFI application entry point.

  UEFI firmware entry point for APXH bootloader. Initializes EFI libraries,
  loads kernel and optional user payloads, retrieves memory map and
  graphics info, then launches main bootloader.

  @param[in] ImageHandle   EFI image handle.
  @param[in] pSystemTable  EFI system table pointer.

  @retval EFI_SUCCESS  Bootloader launched successfully (does not return).
  @retval other        Initialization or payload loading failed.
**/
EFI_STATUS EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *pSystemTable
  )
{
  VOID *pPtr;
  EFI_STATUS Rc;
  EFI_GUID ImgProt = LOADED_IMAGE_PROTOCOL;

  InitializeLib (ImageHandle, pSystemTable);
  gImageHandle = ImageHandle;

  Rc = uefi_call_wrapper (BS->OpenProtocol, 6, ImageHandle, &ImgProt, &pPtr,
			  ImageHandle, NULL,
			  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Open Protocol failed: %r\n", Rc);
      return Rc;
    }

  gpImg = pPtr;

  /*
     Get payloads.
   */
  Rc = EfiGetPayload (L"kernel.elf", &gpPayloadStart, &gPayloadSize);
  if (Rc != EFI_SUCCESS)
    {
      Print (L"Could not load kernel.elf: %r\n", Rc);
      return Rc;
    }
  apxhefi_add_kernel_payload (gpPayloadStart, gPayloadSize);

  Rc = EfiGetPayload (L"user.elf", &gpPayloadStart, &gPayloadSize);
  if (Rc == EFI_SUCCESS)
    {
      Print (L"Loading user.elf");
      apxhefi_add_user_payload (gpPayloadStart, gPayloadSize);
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
  apxh_main (0, 0);

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
  EFI_MEMORY_DESCRIPTOR *pMd;


  pMd = LibMemoryMap (&Num, &Key, &DescSize, &DescVer);

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

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use EfiAllocateMaxAddr instead **/
unsigned long efi_allocate_maxaddr (unsigned long maxaddr) {
  return EfiAllocateMaxAddr (maxaddr);
}

/** @deprecated Use Putchar instead **/
int putchar (int c) {
  return Putchar (c);
}

/** @deprecated Use EfiGetFramebuffer instead **/
EFI_STATUS efi_getframebuffer (void) {
  return EfiGetFramebuffer ();
}

/** @deprecated Use EfiGetPayload instead **/
EFI_STATUS efi_getpayload (CHAR16 *name, void **ptr, unsigned long *size) {
  return EfiGetPayload (name, ptr, size);
}

/** @deprecated Use EfiGetMemoryMap instead **/
void efi_getmemorymap (void) {
  EfiGetMemoryMap ();
}

/** @deprecated Use EfiGetRsdp instead **/
EFI_STATUS efi_getrsdp (void) {
  return EfiGetRsdp ();
}

/** @deprecated Use EfiMain instead **/
EFI_STATUS EFIAPI efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  return EfiMain (ImageHandle, SystemTable);
}

/** @deprecated Use EfiExitBs instead **/
void efi_exitbs (void) {
  EfiExitBs ();
}

/** @deprecated Use EfiExit instead **/
void efi_exit (int st) {
  EfiExit (st);
}
