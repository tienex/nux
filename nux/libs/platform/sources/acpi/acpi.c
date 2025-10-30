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

#include <platform/acpi/tables.h>
#include <platform/acpi/internal.h>

#define ACPI_MAX_TBL (16 << 12)

static PHYSICAL_ADDRESS gPaRootTable;
static PHYSICAL_ADDRESS gPaApicTable;
static PHYSICAL_ADDRESS gPaHpetTable;

/**
  Load ACPI table.

  Maps and validates an ACPI table, checking signature and checksum.

  @param[in] Pa  Physical address of table.

  @return Pointer to mapped table, or NULL if invalid.
**/
static VOID *
LoadTable (
  IN PHYSICAL_ADDRESS  Pa
  )
{
  INT32 i;
  UINT8 Sum, *Ptr;
  struct acpi_thdr *Tbl;

  Tbl = (struct acpi_thdr *) KvaMapPhysical (Pa, ACPI_MAX_TBL, HAL_PTE_P);

  if (Tbl->length >= ACPI_MAX_TBL)
    {
      error ("Table %4.4s [%6.6s %8.8s rev%d] size %d > ACPI_MAX_TBL. Skipping checks.",
	     Tbl->signature, Tbl->oemid, Tbl->oemtableid, Tbl->oemrevision,
	     Tbl->length);
      return Tbl;
    }

  Sum = 0;
  Ptr = (UINT8 *) Tbl;

  for (i = 0; i < Tbl->length; i++)
    {
      Sum += Ptr[i];
    }
  if (Sum != 0)
    {
      warn ("Wrong checksum %d != 0 for ACPI table", Sum);
      KvaUnmap (Tbl, ACPI_MAX_TBL);
      return NULL;
    }

  debug ("loaded table '%4.4s' [%6.6s %8.8s rev%d]", Tbl->signature,
	 Tbl->oemid, Tbl->oemtableid, Tbl->oemrevision);
  return Tbl;
}

/**
  Unload ACPI table.

  Unmaps a previously loaded ACPI table.

  @param[in] pTbl  Pointer to mapped table.
**/
static VOID
UnloadTable (
  IN VOID  *Tbl
  )
{
  KvaUnmap (Tbl, ACPI_MAX_TBL);
}

/**
  Print ACPI table info.

  Logs table signature and OEM information.

  @param[in] pTbl  Pointer to table header.
**/
static VOID
PrintTable (
  IN struct acpi_thdr  *Tbl
  )
{
  info ("TABLE '%4.4s' [%6.6s %8.8s rev%d]", Tbl->signature, Tbl->oemid,
	Tbl->oemtableid, Tbl->oemrevision);
}

/**
  Initialize ACPI.

  Parses RSDP and root system description table (RSDT/XSDT),
  discovering and cataloging system tables.

  @param[in] Root  Physical address of RSDP.
**/
VOID
AcpiInitialize (
  IN PHYSICAL_ADDRESS  Root
  )
{
  VOID *Ptr;
  size_t EntryLen;
  INT64 Length;
  PHYSICAL_ADDRESS PaSdt;
  struct acpi_rsdp_thdr *Rsdp;
  struct acpi_thdr *RootTable, *SdTable;

  Rsdp = (struct acpi_rsdp_thdr *) KvaMapPhysical (Root, ACPI_MAX_TBL, HAL_PTE_P);

  info ("TABLE: '%8.8s' [%6.6s] rev: %d", Rsdp->signature, Rsdp->oemid,
	Rsdp->revision);

  if (Rsdp->revision == 0)
    {
      PaSdt = Rsdp->rsdt;
      debug ("SDT found at addr %" PRIx64, PaSdt);
      EntryLen = 4;
    }
  else
    {
      PaSdt = Rsdp->xsdt;
      debug ("XSDT found at addr %" PRIx64, PaSdt);
      EntryLen = 8;
    }

  KvaUnmap (Rsdp, ACPI_MAX_TBL);

  gPaRootTable = PaSdt;
  RootTable = LoadTable (PaSdt);

  /* Iterate through ACPI tables. */
  Ptr = (VOID *) (RootTable + 1);
  Length = (INT64) RootTable->length - sizeof (*RootTable);
  while (Length > 0)
    {
      PaSdt = EntryLen == 8 ? *(UINT64 *) Ptr : *(UINT32 *) Ptr;
      SdTable = LoadTable (PaSdt);

      PrintTable (SdTable);

      if (!memcmp (SdTable->signature, "APIC", 4))
	gPaApicTable = PaSdt;
      else if (!memcmp (SdTable->signature, "HPET", 4))
	gPaHpetTable = PaSdt;

      UnloadTable (SdTable);
      Length -= EntryLen;
      Ptr += EntryLen;
    }

  UnloadTable (RootTable);

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
  PHYSICAL_ADDRESS LapicAddr;
  struct acpi_madt *AcpiMadt;

  union
  {
    UINT8 *Ptr;
    struct acpi_madt_lapic *Lapic;
    struct acpi_madt_ioapic *Ioapic;
    struct acpi_madt_lapicoverride *LapicOvr;
    struct acpi_madt_lapicnmi *LapicNmi;
    struct acpi_madt_intoverride *IntOvr;
  } _;

#define madt_foreach(_cases)						\
	do {								\
		Len = AcpiMadt->hdr.length - sizeof(*AcpiMadt);	\
		_.Ptr = (UINT8 *) AcpiMadt + sizeof(*AcpiMadt);	\
		while (Len > 0) {					\
			Type = *_.Ptr;					\
			switch (Type) {					\
				_cases;					\
			}						\
			Len -= *(_.Ptr + 1);				\
			_.Ptr += *(_.Ptr + 1);			\
		}							\
	} while (0)

  AcpiMadt = LoadTable (gPaApicTable);
  if (AcpiMadt == NULL)
    {
      error ("Could not load ACPI MADT Table.");
      return;
    }

  LapicAddr = AcpiMadt->lapic;

  /* Search for APICs. Output of this stage is number of Local
     and I/O APICs and Lapic address. */
  /* *INDENT-OFF* */
  madt_foreach({
      case ACPI_MADT_TYPE_LAPICOVERRIDE:
	info("ACPI MADT LAPICOVR %"PRIx64, _.LapicOvr->address);
	LapicAddr = _.LapicOvr->address;
	break;
      case ACPI_MADT_TYPE_LAPIC:
	if (_.Lapic->flags & ACPI_MADT_LAPIC_ENABLED)
	  {
	    info("ACPI MADT LAPIC %02d %02d %08x",
		 _.Lapic->lapicid, _.Lapic->acpiid, _.Lapic->flags);
	    NumLapic++;
	  }
	break;
      case ACPI_MADT_TYPE_IOAPIC:
	info("ACPI MADT IOAPIC %02d %08x %02d",
	       _.Ioapic->ioapicid, _.Ioapic->address, _.Ioapic->gsibase);
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
	if (_.Lapic->flags & ACPI_MADT_LAPIC_ENABLED)
	  LapicAdd(_.Lapic->lapicid, _.Lapic->acpiid);
	break;
      case ACPI_MADT_TYPE_IOAPIC:
	IoapicAdd(NumIoapic, _.Ioapic->address, _.Ioapic->gsibase);
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
	       _.LapicNmi->lint, _.LapicNmi->flags, _.LapicNmi->acpiid);
	/* Ignore IntiFlags as NMI vectors ignore
	 * polarity and trigger */
	LapicAddNmi(_.LapicNmi->acpiid, _.LapicNmi->lint);
	break;
      case ACPI_MADT_TYPE_LX2APICNMI:
	warn ("LX2APICNMI ENTRY IGNORED");
	break;
      case ACPI_MADT_TYPE_INTOVERRIDE:
	info ("ACPI MADT INTOVR BUS %02d IRQ: %02d GSI: %02d FL: %04x",
	       _.IntOvr->bus, _.IntOvr->irq, _.IntOvr->gsi, _.IntOvr->flags);
	Flags = _.IntOvr->flags;
	switch (Flags & ACPI_MADT_TRIGGER_MASK) {
	case ACPI_MADT_TRIGGER_RESERVED:
	  warn ("reserved trigger value");
	  /* Passtrhough to edge. */
	case ACPI_MADT_TRIGGER_CONFORMS:
	  /* ISA is EDGE */
	case ACPI_MADT_TRIGGER_EDGE:
	  GsiSetup(_.IntOvr->gsi, _.IntOvr->irq, PLATFORM_IRQ_EDGE);
	  break;
	case ACPI_MADT_TRIGGER_LEVEL:
	  switch(Flags & ACPI_MADT_POLARITY_MASK) {
	  case ACPI_MADT_POLARITY_RESERVED:
	    warn ("Warning: reserved polarity value");
	    /* Passthrough to Level Low */
	  case ACPI_MADT_POLARITY_CONFORMS:
	    /* Default for EISA is LOW */
	  case ACPI_MADT_POLARITY_ACTIVE_LOW:
	    GsiSetup(_.IntOvr->gsi, _.IntOvr->irq, PLATFORM_IRQ_LVLLO);
	    break;
	  case ACPI_MADT_POLARITY_ACTIVE_HIGH:
	    GsiSetup(_.IntOvr->gsi, _.IntOvr->irq, PLATFORM_IRQ_LVLHI);
	    break;
	  }
	  break;
	}
	break;
      default:
	break;
    });
  /* *INDENT-ON* */

  UnloadTable (AcpiMadt);
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
  struct acpi_hpet *Hpet;

  if (gPaHpetTable == 0)
    {
      warn ("No HPET found");
      return FALSE;
    }

  Hpet = LoadTable (gPaHpetTable);
  if (Hpet == NULL)
    {
      error ("Error loading HPET table");
      return FALSE;
    }

  Rc = HpetInitialize (Hpet->address.address);

  UnloadTable (Hpet);

  return Rc;
}
