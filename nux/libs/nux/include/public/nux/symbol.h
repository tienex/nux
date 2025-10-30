/** @file
  Symbol Resolution Utilities

  Provides runtime symbol resolution for debugging and diagnostic purposes.
  Converts memory addresses to human-readable symbol names by looking up
  addresses in the kernel symbol table.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_symbol_h__
#define __nux_symbol_h__

/**
  Resolve an address to a symbol name.

  Searches the kernel symbol table for the symbol that contains the
  specified address. Returns the name of the nearest symbol at or
  before the address.

  @param[in] Address  Memory address to resolve.

  @return Pointer to symbol name string, or NULL if not found.
          The returned string is statically allocated and should
          not be freed.
**/
CONST CHAR8 *NuxSymbolResolve (
  IN UINTN  Address
  );

#endif // _SYMBOL_H
