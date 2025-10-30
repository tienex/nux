/** @file
  NUX Symbol Resolution

  Provides kernel symbol table lookup for translating addresses
  to symbol names. Uses linker-provided symbol table sections.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/internal.h>

/**
  Kernel Symbol Table Entry

  Structure matching linker-generated symbol table format.
**/
typedef struct _KSYM
{
  UINTN        Addr;    ///< Symbol address
  CONST CHAR8  *Name;   ///< Symbol name
} KSYM, *PKSYM, *PCKSYM;

extern KSYM _ksym_start[];
extern KSYM _ksym_end[];

/**
  Resolve address to symbol name.

  Searches kernel symbol table for the symbol whose address is
  closest to but not greater than the specified address.

  @param[in] Addr  Address to resolve.

  @return Symbol name string, or "unknown" if not found.
**/
CONST CHAR8 *
NuxSymbolResolve (
  IN UINTN  Addr
  )
{
  CONST KSYM *Sym = _ksym_start;
  CONST KSYM *Last = NULL;

  while (Sym->Addr != 0)
    {
      if (Sym->Addr > Addr)
	break;
      Last = Sym++;
    }

  if (Sym->Addr == 0)
    Last = NULL;

  if (Last == NULL)
    return "unknown";
  else
    return Last->Name;
}

//
// Legacy Type and Function Wrappers (for backward compatibility)
//

/** @deprecated Use KSYM instead **/
struct ksym {
  unsigned long addr;
  CONST char *name;
};

/** @deprecated Use NuxSymbolResolve instead **/
CONST char *nux_symresolve (unsigned long addr) {
  return NuxSymbolResolve (addr);
}
