/** @file
  NUX Symbol Resolution

  Provides kernel symbol table lookup for translating addresses
  to symbol names. Uses linker-provided symbol table sections.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "internal.h"

/**
  Kernel symbol table entry.

  Structure matching linker-generated symbol table format.
**/
typedef struct _KSYM {
  UINTN Addr;
  CONST CHAR8 *pName;
} KSYM;

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
  CONST KSYM *pSym = _ksym_start;
  CONST KSYM *pLast = NULL;

  while (pSym->Addr != 0)
    {
      if (pSym->Addr > Addr)
	break;
      pLast = pSym++;
    }

  if (pSym->Addr == 0)
    pLast = NULL;

  if (pLast == NULL)
    return "unknown";
  else
    return pLast->pName;
}

//
// Legacy Type and Function Wrappers (for backward compatibility)
//

/** @deprecated Use KSYM instead **/
struct ksym {
  unsigned long addr;
  const char *name;
};

/** @deprecated Use NuxSymbolResolve instead **/
const char *nux_symresolve (unsigned long addr) {
  return NuxSymbolResolve (addr);
}
