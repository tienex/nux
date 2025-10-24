/** @file
  ACPI Table Support

  Provides ACPI (Advanced Configuration and Power Interface) table
  parsing and initialization. Discovers and processes MADT (APIC),
  HPET, and other system description tables.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <string.h>
#include <nux/defs.h>
#include <nux/types.h>
#include <nux/nux.h>

#include "acpitbl.h"
#include "internal.h"

#define ACPI_MAX_TBL (16 << 12)

static paddr_t gPaRootTable;
static paddr_t gPaApicTable;
static paddr_t gPaHpetTable;

/**
  Load ACPI table.

  Maps and validates an ACPI table, checking signature and checksum.

  @param[in] Pa  Physical address of table.

  @return Pointer to mapped table, or NULL if invalid.
**/
static VOID *
LoadTable (
  IN paddr_t  Pa
  )
{
  INT32 i;
  UINT8 Sum, *pPtr;
  struct acpi_thdr *pTbl;

  pTbl = (struct acpi_thdr *) KvaMapPhysical (Pa, ACPI_MAX_TBL, HAL_PTE_P);

  if (pTbl->length >= ACPI_MAX_TBL)
    {
      error ("Table %4.4s [%6.6s %8.8s rev%d] size %d > ACPI_MAX_TBL. Skipping checks.",
	     pTbl->signature, pTbl->oemid, pTbl->oemtableid, pTbl->oemrevision,
	     pTbl->length);
      return pTbl;
    }

  Sum = 0;
  pPtr = (UINT8 *) pTbl;

  for (i = 0; i < pTbl->length; i++)
    {
      Sum += pPtr[i];
    }
  if (Sum != 0)
    {
      warn ("Wrong checksum %d != 0 for ACPI table", Sum);
      KvaUnmap (pTbl, ACPI_MAX_TBL);
      return NULL;
    }

  debug ("loaded table '%4.4s' [%6.6s %8.8s rev%d]", pTbl->signature,
	 pTbl->oemid, pTbl->oemtableid, pTbl->oemrevision);
  return pTbl;
}

/**
  Unload ACPI table.

  Unmaps a previously loaded ACPI table.

  @param[in] pTbl  Pointer to mapped table.
**/
static VOID
UnloadTable (
  IN VOID  *pTbl
  )
{
  KvaUnmap (pTbl, ACPI_MAX_TBL);
}

/**
  Print ACPI table info.

  Logs table signature and OEM information.

  @param[in] pTbl  Pointer to table header.
**/
static VOID
PrintTable (
  IN struct acpi_thdr  *pTbl
  )
{
  info ("TABLE '%4.4s' [%6.6s %8.8s rev%d]", pTbl->signature, pTbl->oemid,
	pTbl->oemtableid, pTbl->oemrevision);
}

/**
  Initialize ACPI.

  Parses RSDP and root system description table (RSDT/XSDT),
  discovering and cataloging system tables.

  @param[in] Root  Physical address of RSDP.
**/
VOID
AcpiInitialize (
  IN paddr_t  Root
  )
{
  VOID *pPtr;
  size_t EntryLen;
  INT64 Length;
  paddr_t PaSdt;
  struct acpi_rsdp_thdr *pRsdp;
  struct acpi_thdr *pRootTable, *pSdTable;

  pRsdp = (struct acpi_rsdp_thdr *) KvaMapPhysical (Root, ACPI_MAX_TBL, HAL_PTE_P);

  info ("TABLE: '%8.8s' [%6.6s] rev: %d", pRsdp->signature, pRsdp->oemid,
	pRsdp->revision);

  if (pRsdp->revision == 0)
    {
      PaSdt = pRsdp->rsdt;
      debug ("SDT found at addr %" PRIx64, PaSdt);
      EntryLen = 4;
    }
  else
    {
      PaSdt = pRsdp->xsdt;
      debug ("XSDT found at addr %" PRIx64, PaSdt);
      EntryLen = 8;
    }

  KvaUnmap (pRsdp, ACPI_MAX_TBL);

  gPaRootTable = PaSdt;
  pRootTable = LoadTable (PaSdt);

  /* Iterate through ACPI tables. */
  pPtr = (VOID *) (pRootTable + 1);
  Length = (INT64) pRootTable->length - sizeof (*pRootTable);
  while (Length > 0)
    {
      PaSdt = EntryLen == 8 ? *(UINT64 *) pPtr : *(UINT32 *) pPtr;
      pSdTable = LoadTable (PaSdt);

      PrintTable (pSdTable);

      if (!memcmp (pSdTable->signature, "APIC", 4))
	gPaApicTable = PaSdt;
      else if (!memcmp (pSdTable->signature, "HPET", 4))
	gPaHpetTable = PaSdt;

      UnloadTable (pSdTable);
      Length -= EntryLen;
      pPtr += EntryLen;
    }

  UnloadTable (pRootTable);

  debug ("RDST table at pa %" PRIx64, gPaRootTable);
  debug ("APIC table at pa %" PRIx64, gPaApicTable);
  debug ("HPET table at pa %" PRIx64, gPaHpetTable);
}

/**
  Scan MADT table.

  Parses Multiple APIC Description Table to discover Local APICs,
  I/O APICs, interrupt overrides, and NMI configurations.
**/
VOID
AcpiMadtScan (
  VOID
  )
{
  INT32 Len;
  UINT32 Flags, NumLapic = 0, NumIoapic = 0;
  UINT8 Type;
  paddr_t LapicAddr;
  struct acpi_madt *pAcpiMadt;

  union
  {
    UINT8 *pPtr;
    struct acpi_madt_lapic *pLapic;
    struct acpi_madt_ioapic *pIoapic;
    struct acpi_madt_lapicoverride *pLapicOvr;
    struct acpi_madt_lapicnmi *pLapicNmi;
    struct acpi_madt_intoverride *pIntOvr;
  } _;

#define madt_foreach(_cases)						\
	do {								\
		Len = pAcpiMadt->hdr.length - sizeof(*pAcpiMadt);	\
		_.pPtr = (UINT8 *) pAcpiMadt + sizeof(*pAcpiMadt);	\
		while (Len > 0) {					\
			Type = *_.pPtr;					\
			switch (Type) {					\
				_cases;					\
			}						\
			Len -= *(_.pPtr + 1);				\
			_.pPtr += *(_.pPtr + 1);			\
		}							\
	} while (0)

  pAcpiMadt = LoadTable (gPaApicTable);
  if (pAcpiMadt == NULL)
    {
      error ("Could not load ACPI MADT Table.");
      return;
    }

  LapicAddr = pAcpiMadt->lapic;

  /* Search for APICs. Output of this stage is number of Local
     and I/O APICs and Lapic address. */
  /* *INDENT-OFF* */
  madt_foreach({
      case ACPI_MADT_TYPE_LAPICOVERRIDE:
	info("ACPI MADT LAPICOVR %"PRIx64, _.pLapicOvr->address);
	LapicAddr = _.pLapicOvr->address;
	break;
      case ACPI_MADT_TYPE_LAPIC:
	if (_.pLapic->flags & ACPI_MADT_LAPIC_ENABLED)
	  {
	    info("ACPI MADT LAPIC %02d %02d %08x",
		 _.pLapic->lapicid, _.pLapic->acpiid, _.pLapic->flags);
	    NumLapic++;
	  }
	break;
      case ACPI_MADT_TYPE_IOAPIC:
	info("ACPI MADT IOAPIC %02d %08x %02d",
	       _.pIoapic->ioapicid, _.pIoapic->address, _.pIoapic->gsibase);
	NumIoapic++;
	break;
      case ACPI_MADT_TYPE_LSAPIC:
	{
	  static INT32 Warn = 0;
	  if (!Warn)
	    {
	      info("Warning: LSAPIC ENTRIES IGNORED");
	      Warn = 1;
	    }
	  break;
	}
      case ACPI_MADT_TYPE_LX2APIC:
	{
	  static INT32 Warn = 0;
	  if (!Warn)
	    {
	      info("Warning: X2APIC ENTRY IGNORED");
	      Warn = 1;
	    }
	}
	break;
      case ACPI_MADT_TYPE_IOSAPIC:
	{
	  static INT32 Warn = 0;
	  if (!Warn)
	    {
	      info("Warning: IOSAPIC ENTRY IGNORED");
	      Warn = 1;
	    }
	  break;
	}
      default:
	break;
    });
  /* *INDENT-ON* */
  if (NumLapic == 0)
    {
      info ("Warning: NO LOCAL APICS, ACPI SAYS");
      NumLapic = 1;
    }

  LapicInitialize (LapicAddr, NumLapic);
  IoapicInitialize (NumIoapic);

  /* Add APICs. Local and I/O APICs existence is notified to the
   * kernel after this. */
  NumIoapic = 0;
  /* *INDENT-OFF* */
  madt_foreach({
      case ACPI_MADT_TYPE_LAPIC:
	if (_.pLapic->flags & ACPI_MADT_LAPIC_ENABLED)
	  LapicAdd(_.pLapic->lapicid, _.pLapic->acpiid);
	break;
      case ACPI_MADT_TYPE_IOAPIC:
	IoapicAdd(NumIoapic, _.pIoapic->address, _.pIoapic->gsibase);
	NumIoapic++;
	break;
      default:
	break;
    });
  /* *INDENT-ON* */

  GsiInitialize ();
  /* *INDENT-OFF* */
  madt_foreach({
      case ACPI_MADT_TYPE_LAPICNMI:
	info ("ACPI MADT LAPICNMI LINT%01d FL:%04x PROC:%02d",
	       _.pLapicNmi->lint, _.pLapicNmi->flags, _.pLapicNmi->acpiid);
	/* Ignore IntiFlags as NMI vectors ignore
	 * polarity and trigger */
	LapicAddNmi(_.pLapicNmi->acpiid, _.pLapicNmi->lint);
	break;
      case ACPI_MADT_TYPE_LX2APICNMI:
	warn ("LX2APICNMI ENTRY IGNORED");
	break;
      case ACPI_MADT_TYPE_INTOVERRIDE:
	info ("ACPI MADT INTOVR BUS %02d IRQ: %02d GSI: %02d FL: %04x",
	       _.pIntOvr->bus, _.pIntOvr->irq, _.pIntOvr->gsi, _.pIntOvr->flags);
	Flags = _.pIntOvr->flags;
	switch (Flags & ACPI_MADT_TRIGGER_MASK) {
	case ACPI_MADT_TRIGGER_RESERVED:
	  warn ("reserved trigger value");
	  /* Passtrhough to edge. */
	case ACPI_MADT_TRIGGER_CONFORMS:
	  /* ISA is EDGE */
	case ACPI_MADT_TRIGGER_EDGE:
	  GsiSetup(_.pIntOvr->gsi, _.pIntOvr->irq, PLT_IRQ_EDGE);
	  break;
	case ACPI_MADT_TRIGGER_LEVEL:
	  switch(Flags & ACPI_MADT_POLARITY_MASK) {
	  case ACPI_MADT_POLARITY_RESERVED:
	    warn ("Warning: reserved polarity value");
	    /* Passthrough to Level Low */
	  case ACPI_MADT_POLARITY_CONFORMS:
	    /* Default for EISA is LOW */
	  case ACPI_MADT_POLARITY_ACTIVE_LOW:
	    GsiSetup(_.pIntOvr->gsi, _.pIntOvr->irq, PLT_IRQ_LVLLO);
	    break;
	  case ACPI_MADT_POLARITY_ACTIVE_HIGH:
	    GsiSetup(_.pIntOvr->gsi, _.pIntOvr->irq, PLT_IRQ_LVLHI);
	    break;
	  }
	  break;
	}
	break;
      default:
	break;
    });
  /* *INDENT-ON* */

  UnloadTable (pAcpiMadt);
}

/**
  Scan for HPET.

  Locates and initializes High Precision Event Timer from ACPI table.

  @retval TRUE   HPET found and initialized.
  @retval FALSE  HPET not found or initialization failed.
**/
BOOLEAN
AcpiHpetScan (
  VOID
  )
{
  BOOLEAN Rc;
  struct acpi_hpet *pHpet;

  if (gPaHpetTable == 0)
    {
      warn ("No HPET found");
      return FALSE;
    }

  pHpet = LoadTable (gPaHpetTable);
  if (pHpet == NULL)
    {
      error ("Error loading HPET table");
      return FALSE;
    }

  Rc = HpetInitialize (pHpet->address.address);

  UnloadTable (pHpet);

  return Rc;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use LoadTable instead **/
static void *load_table (paddr_t pa) {
  return LoadTable (pa);
}

/** @deprecated Use UnloadTable instead **/
static void unload_table (void *tbl) {
  UnloadTable (tbl);
}

/** @deprecated Use PrintTable instead **/
static void print_table (struct acpi_thdr *tbl) {
  PrintTable (tbl);
}

/** @deprecated Use AcpiInitialize instead **/
void acpi_init (paddr_t root) {
  AcpiInitialize (root);
}

/** @deprecated Use AcpiMadtScan instead **/
void acpi_madt_scan (void) {
  AcpiMadtScan ();
}

/** @deprecated Use AcpiHpetScan instead **/
bool acpi_hpet_scan (void) {
  return AcpiHpetScan ();
}

// Legacy global variable aliases
static paddr_t pa_root_table __attribute__((alias("gPaRootTable")));
static paddr_t pa_apic_table __attribute__((alias("gPaApicTable")));
static paddr_t pa_hpet_table __attribute__((alias("gPaHpetTable")));
