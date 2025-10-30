/** @file
  APXH x86 PAE Paging Common Functions

  Common functions shared between PAE32 and PAE64 implementations,
  including CPU feature detection, PAT (Page Attribute Table) setup,
  and memory type conversions.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/x86/pae.h>

bool gNxEnabled;

/**
  Scan PAT (Page Attribute Table).

  Examines the current PAT configuration and displays entries for
  Uncacheable (UC), Write-Combining (WC), and Write-Back (WB) memory
  types.
**/
static VOID
ScanPatTable (
  VOID
  )
{
  bool WbSet = false;
  bool WcSet = false;
  bool UcSet = false;
  UINT64 Pat = Rdmsr (MSR_IA32_PAT);

  for (INT32 i = 0; i < 8; i++)
    {
      switch (Pat & 0x7)
	{
	case _MSR_IA32_PAT_UC:
	  if (!UcSet)
	    {
	      printf ("PAT Table: UC Entry at %d\n", i);
	      UcSet = true;
	    }
	  break;
	case _MSR_IA32_PAT_WC:
	  if (!WcSet)
	    {
	      printf ("PAT Table: WC Entry at %d\n", i);
	      WcSet = true;
	    }
	  break;
	case _MSR_IA32_PAT_WB:
	  if (!WbSet)
	    {
	      printf ("PAT TABLE: WB Entry at %d\n", i);
	      WbSet = true;
	    }
	  break;
	}
      Pat >>= 8;
    }
}

/**
  Configure PAT table.

  Sets up the Page Attribute Table with default entries plus
  Write-Combining at entry 7.
**/
VOID
SetupPatTable (
  VOID
  )
{
  /*
     Default PAT table, with added WC at 7 */
  Wrmsr (MSR_IA32_PAT, 0x0100040600070406LL);

  ScanPatTable ();
}

/**
  Convert memory type to PTE flags.

  Translates memory type enumeration to appropriate PAT, PCD, and PWT
  flags for page table entries.

  @param[in] Mt     Memory type (WC, WB, or UC).
  @param[in] Small  TRUE for 4KB pages, FALSE for 2MB/1GB pages.

  @return PTE flags for specified memory type.
**/
UINT32
MemtypeToFlags (
  IN MEMORY_TYPE  Mt,
  IN bool              Small
  )
{
  UINT32 Pat = Small ? PTE_PAT_4K : PTE_PAT_BIG;

  switch (Mt)
    {
    case MEMTYPE_WC:
      /* WC is 7 */
      return Pat | PTE_PCD | PTE_PWT;
      break;
    case MEMTYPE_WB:
      /* WB is 0 */
      return 0;
      break;
    case MEMTYPE_UC:
      return PTE_PCD | PTE_PWT;
      break;
    }
  return 0;
}


/**
  Check if CPU is Intel.

  Uses CPUID to determine if the processor is manufactured by Intel.

  @retval TRUE   CPU is Intel.
  @retval FALSE  CPU is not Intel.
**/
bool
CpuIsIntel (
  VOID
  )
{
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 0;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  // GenuineIntel?
  if (Ebx == 0x756e6547 && Ecx == 0x6c65746e && Edx == 0x49656e69)
    return true;
  else
    return false;

  return 1;
}

/**
  Get Intel CPU family.

  Retrieves the processor family value from CPUID.

  @return CPU family number.
**/
UINT32
IntelCpuFamily (
  VOID
  )
{
  UINT32 Family;
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 1;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  Family = (Eax & 0xf00) >> 8;
  Family |= (Eax & 0xf00000 >> 20);

  return Family;
}

/**
  Get Intel CPU model.

  Retrieves the processor model value from CPUID.

  @return CPU model number.
**/
UINT32
IntelCpuModel (
  VOID
  )
{
  UINT32 Model;
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 1;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  Model = (Eax & 0xf0) >> 4;
  Model |= (Eax & 0xf0000) >> 16;

  return Model;
}

/**
  Check if CPU supports PAE.

  Uses CPUID to determine if Physical Address Extension is supported.

  @retval TRUE   PAE is supported.
  @retval FALSE  PAE is not supported.
**/
bool
CpuSupportsPae (
  VOID
  )
{
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 1;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  return !!(Edx & (1 << 6));
}

/**
  Check if CPU supports long mode.

  Uses CPUID to determine if 64-bit long mode (AMD64) is supported.

  @retval TRUE   Long mode is supported.
  @retval FALSE  Long mode is not supported.
**/
bool
CpuSupportsLongmode (
  VOID
  )
{
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 0x80000001;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  return !!(Edx & (1 << 29));
}

/**
  Check if CPU supports 1GB pages.

  Uses CPUID to determine if 1GB page support is available.

  @retval TRUE   1GB pages are supported.
  @retval FALSE  1GB pages are not supported.
**/
bool
CpuSupports1gbPages (
  VOID
  )
{
  UINT32 Eax, Ebx, Ecx, Edx;

  Eax = 0x80000001;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  return !!(Edx & (1 << 26));
}

/**
  Check if CPU supports NX bit and enable it.

  Uses CPUID to check for NX (No-Execute) support and enables it
  via IA32_EFER MSR. On Intel CPUs, may need to clear XD disable
  bit in IA32_MISC_ENABLE MSR first.

  @retval TRUE   NX is supported and enabled.
  @retval FALSE  NX is not supported.
**/
bool
CpuSupportsNx (
  VOID
  )
{
  bool NxSupported;
  UINT64 Efer;
  UINT32 Eax, Ebx, Ecx, Edx;

  /* Intel CPUs might have disabled this in MSR. */
  if (CpuIsIntel ())
    {
      UINT32 Family = IntelCpuFamily ();
      UINT32 Model = IntelCpuModel ();

      if ((Family >= 6) && (Family > 6 || Model > 0xd))
	{
	  UINT64 MiscEnable;

	  MiscEnable = Rdmsr (MSR_IA32_MISC_ENABLE);
	  if (MiscEnable & _MSR_IA32_MISC_ENABLE_XD_DISABLE)
	    {
	      MiscEnable &= ~_MSR_IA32_MISC_ENABLE_XD_DISABLE;
	      Wrmsr (MSR_IA32_MISC_ENABLE, MiscEnable);
	    }
	}
    }
  Eax = 0x80000001;
  Ecx = 0;
  Cpuid (&Eax, &Ebx, &Ecx, &Edx);

  NxSupported = !!(Edx & (1 << 20));
  if (!NxSupported)
    return false;

  Efer = Rdmsr (MSR_IA32_EFER);
  Wrmsr (MSR_IA32_EFER, Efer | _MSR_IA32_EFER_NXE);
  Efer = Rdmsr (MSR_IA32_EFER);

  return !!(Efer & _MSR_IA32_EFER_NXE);
}
