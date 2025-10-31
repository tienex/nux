# Image Loader Format Documentation

This document provides references for implementing image format loaders in APXH.

## Universal Resource System

APXH implements a universal resource system based on Classic Macintosh resource fork format,
enabling cross-platform resource embedding in all executable formats.

### Resource Embedding Strategies

**Formats WITH Native Resources** (e.g., PE, Mach-O, OS/2 ELF):
- Embed universal resource fork as AUR (APXH Universal Resource) resource within native system
- Use hybrid strategy that exposes both native and universal resources
- AUR type codes: "AUR " (32-bit), "Au" (16-bit), "APXHURSC" (64-bit)
- Implementation: `ResourceStrategyBoth` with `FindUniversalResourceFork()`

**Formats WITHOUT Native Resources** (e.g., generic ELF, COFF, a.out):
- Store universal resource fork in dedicated section/segment
- Section names: `.axursrc` (ELF), `.rsrc` (COFF), `__apxh_uresource` (Mach-O segment)
- Implementation: `ResourceStrategyDirect` with `FindUniversalResourceFork()`

**Special Case: OS/2 PowerPC ELF** (hybrid):
- Native resources via `SHT_RES` sections or `PT_RES` program headers
- OS/2 resource format with collections, items, and locale information
- Supports both 32-bit and 64-bit ELF
- Fallback to `.axursrc` section for universal resources
- Implementation: `ResourceStrategyBoth` with OS/2 resource parser

### Resource Support Status

| Format | Native Resources | Status | Section/Segment Name |
|--------|------------------|--------|---------------------|
| PE     | ✅ Yes | ✅ Implemented | Resource directory (native) + `.axursrc` |
| Mach-O | ✅ Yes | ✅ Implemented | `__RSRC` segment (native) + `__apxh_uresource` |
| ELF    | ✅ OS/2 | ✅ Implemented | `SHT_RES`/`PT_RES` (OS/2 native) + `.axursrc` |
| COFF   | ❌ No  | ✅ Implemented | `.axursrc` section |
| XCOFF  | ❌ No  | ✅ Implemented | `.axursrc` section (AIX) |
| ECOFF  | ❌ No  | ✅ Implemented | `.axursrc` section (DEC/SGI) |
| a.out  | ❌ No  | ✅ Implemented | Symbol-based (`__apxh_uresource_start/size`) |
| LE/LX  | ⚠️ OS/2 | ⚠️ TODO | Native resource table + `.axursrc` |
| NLM    | ⚠️ NetWare | ⚠️ TODO | NetWare resources or `.axursrc` |
| Other  | ❌ No  | ⚠️ TODO | Format-specific (Hunk, Atari, PDP-10, etc.) |

### Implementation Guide

For formats WITHOUT native resources:
1. Add `#include <ananke/resource.h>` and `#include "imgresource.h"`
2. Implement section finder: `FormatFindSection(ImageBase, SectionName, Data, Size)`
3. Implement GetResource using `FindUniversalResourceFork()` with `ResourceStrategyDirect`
4. Implement GetResourceEnumerator similarly
5. Update vtable to include resource methods

Example implementation pattern (see `elf.c` for reference):
```c
static HRESULT FormatGetResource(...) {
  Status = FindUniversalResourceFork(ImageBase, ResourceStrategyDirect,
                                     NULL, FormatFindSection, ".axursrc",
                                     &ResourceFork, &Size, &NeedsFree);
  if (Status == S_OK) {
    return CreateImageResource(ResourceFork, TypeCode, Id, Name, Resource);
  }
  return Status;
}
```

**Special Case: Formats Without Sections (a.out)**

For formats that lack section headers, use symbol-based resource location:
1. Define special symbols when linking:
   - `__apxh_uresource_start`: Points to resource fork start address
   - `__apxh_uresource_size`: Value equals resource fork size
2. Implement finder that looks up these symbols using `GetSymbolByName()`
3. Use the finder with `FindUniversalResourceFork()` as usual

See `aout.c` for a complete symbol-based implementation example.

---

## Initialization and Termination Functions

APXH loaders support extracting initialization and termination function information from
executable formats, enabling proper startup/shutdown sequences for libraries and applications.

### Init/Fini Support Status

| Format | Init/Fini Mechanism | Status | Sections/Mechanisms |
|--------|---------------------|--------|---------------------|
| Mach-O | ✅ Sections | ✅ Implemented | `__DATA,__mod_init_func`, `__DATA,__mod_term_func` |
| PE     | ✅ TLS Callbacks | ✅ Implemented | TLS directory (`AddressOfCallBacks`) |
| ELF (OS/2) | ✅ Dynamic Tags | ✅ Implemented | `DT_INIT`, `DT_FINI`, `DT_INITTERM`, `DT_IT` |
| ELF (generic) | ✅ Sections | ✅ Implemented | `.init`, `.fini`, `.ctors`, `.dtors` |
| COFF   | ✅ Sections | ✅ Implemented | `.init`, `.fini`, `.ctors`, `.dtors` |
| XCOFF  | ✅ Sections | ✅ Implemented | `.init`, `.fini`, `.ctors`, `.dtors` (AIX) |
| ECOFF  | ✅ Sections | ✅ Implemented | `.init`, `.fini`, `.ctors`, `.dtors` (DEC/SGI) |
| a.out  | ⚠️ Limited | ⚠️ Partial | Dynamic linker support via `__DYNAMIC` |
| LE/LX  | ⚠️ OS/2 | ⚠️ TODO | OS/2 DLL initialization/termination |
| PEF    | ⚠️ PowerPC | ⚠️ TODO | PowerPC fragment init/term |

### Implementation Patterns

#### 1. **Mach-O (macOS/iOS)**: Function Pointer Arrays
```c
// __DATA,__mod_init_func: Array of init function pointers (called before main)
// __DATA,__mod_term_func: Array of term function pointers (called at exit)
MachoGetInitFini(ImageBase, &InitFuncs, &NumInitFuncs, &TermFuncs, &NumTermFuncs);
```

#### 2. **PE (Windows)**: TLS Callbacks
```c
// TLS directory contains AddressOfCallBacks (array of callback functions)
// Callbacks are called on process/thread attach/detach
PeGetTlsInfo(ImageBase, &TlsInfo);  // TlsInfo.CallbacksAddr points to callback array
```

#### 3. **ELF**: Multiple Mechanisms
**OS/2 PowerPC ELF** (dynamic tags):
```c
// DT_INIT:     Single init function address
// DT_FINI:     Single fini function address
// DT_INITTERM: Combined init/term function (OS/2 specific)
// DT_IT:       Init/term type (IT_GLOBAL, IT_INSTANCE, IT_THREAD)
// DT_ITPRTY:   Priority (0 = highest)
GetOs2InitFiniInfo(ImageBase, &InitAddr, &FiniAddr, &InitTermAddr, &InitType, &TermType, &Priority);
```

**Generic ELF** (sections):
```c
// .init:  Initialization code section
// .fini:  Finalization code section
// .ctors: Constructor function pointer array (C++)
// .dtors: Destructor function pointer array (C++)
```

#### 4. **COFF Family** (COFF/XCOFF/ECOFF): Standard Sections
```c
// All three formats use identical section-based approach:
CoffGetInitFini(ImageBase, &InitSection, &InitSize, &FiniSection, &FiniSize,
                &CtorsArray, &NumCtors, &DtorsArray, &NumDtors);
```

**Section Types:**
- `.init` / `.fini`: Code sections containing initialization/finalization code
- `.ctors` / `.dtors`: Arrays of function pointers (C++ global constructors/destructors)

#### 5. **a.out**: Dynamic Linker Support
```c
// If __DYNAMIC symbol present, dynamic linker handles init/fini
// Static executables may use custom sections (non-standard)
AoutGetDynamic(ImageBase, &Dynamic);  // Returns AOUT_LINK_DYNAMIC structure
```

### Execution Order

Typical initialization sequence (loader responsibility):
1. **Load executable and dependencies**
2. **Apply relocations**
3. **Execute init functions** (in dependency order):
   - Library-level init (IT_GLOBAL for OS/2)
   - Per-instance init (IT_INSTANCE for OS/2)
   - TLS callbacks with DLL_PROCESS_ATTACH (PE)
   - C++ global constructors (.ctors)
4. **Call entry point** (main/WinMain/etc.)

Typical termination sequence:
1. **Return from entry point**
2. **Execute fini functions** (reverse dependency order):
   - C++ global destructors (.dtors)
   - TLS callbacks with DLL_PROCESS_DETACH (PE)
   - Per-instance term (IT_INSTANCE for OS/2)
   - Library-level term (IT_GLOBAL for OS/2)
3. **Unload libraries**

### Platform-Specific Notes

**macOS/iOS (Mach-O):**
- `__mod_init_func` runs before `main()` or library use
- `__mod_term_func` runs at process exit via `atexit()`
- Used by C++ constructors and `__attribute__((constructor))`

**Windows (PE):**
- TLS callbacks run on every thread creation/destruction
- `DllMain(DLL_PROCESS_ATTACH)` for DLL initialization
- `DllMain(DLL_PROCESS_DETACH)` for DLL termination

**OS/2 (LE/LX and ELF):**
- Init/term types control when functions run:
  - `IT_GLOBAL`: First load / final unload (library-level)
  - `IT_INSTANCE`: Each process attach / detach
  - `IT_THREAD`: Each thread (reserved in Release 1)
- Priority determines execution order (0 = highest priority)

**Unix (ELF/COFF/ECOFF/XCOFF):**
- `.init` / `.fini` sections contain arbitrary code
- `.ctors` / `.dtors` are NULL-terminated function pointer arrays
- Linker combines all `.ctors` from object files into one array
- C++ runtime calls each constructor/destructor in order

---

## ELF (Executable and Linkable Format)

**Status:** ✅ Fully implemented with unwinding/symbol support

**Documentation:**
- System V ABI: https://www.sco.com/developers/gabi/
- ELF specification: http://www.sco.com/developers/gabi/latest/ch4.eheader.html
- Tool Interface Standard (TIS) ELF specification
- Linux Standard Base (LSB) Core Specification
- NetBSD `elf.h` header (included in `nux/contrib/include/elf.h`)

**Implementations:**
- `elf.c` - COM interface wrapper
- `elf_impl.c` - Core implementation with LoadElf32/LoadElf64
- `fatelf.c` - FatELF multi-architecture wrapper

**Features:**
- 32-bit and 64-bit support
- Program headers (PT_LOAD, PT_TLS, PT_GNU_EH_FRAME)
- Section headers (.eh_frame, .symtab, .dynsym, .strtab)
- Unwinding information extraction (DWARF eh_frame)
- Symbol lookup by address and name
- Endianness detection (little/big endian)
- Custom APXH program header types

---

## PE/COFF (Portable Executable / Common Object File Format)

**Status:** ⚠️ Detection implemented, needs interface update

**Documentation:**
- Microsoft PE/COFF Specification (docx): https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
- ECMA-335 (CLI) specification for PE/COFF headers
- Wine source code: https://source.winehq.org/

**Implementations:**
- `pe.c` - PE/COFF loader (needs IImageLoader interface update)

**Features:**
- PE32 (32-bit) and PE32+ (64-bit) support
- Section headers
- Data directories (imports, exports, TLS, resources)
- .pdata sections for x64 unwinding (SEH)
- COFF file format support

**TODO:**
- Update to IImageLoader interface with HRESULT
- Extract .pdata unwinding information
- Add symbol table support

---

## Mach-O (Mach Object)

**Status:** ⚠️ Detection implemented, needs interface update

**Documentation:**
- Apple's Mach-O Programming Topics (archive.org)
- macOS ABI Mach-O File Format Reference
- XNU kernel source: https://github.com/apple/darwin-xnu
- `<mach-o/loader.h>` header on macOS systems

**Implementations:**
- `macho.c` - Mach-O loader (needs IImageLoader interface update)

**Features:**
- Universal binaries (fat binaries)
- 32-bit and 64-bit Mach-O
- Load commands (LC_SEGMENT, LC_THREAD, LC_UNIXTHREAD)
- Dynamic linking support

**TODO:**
- Update to IImageLoader interface with HRESULT
- Add __eh_frame/__unwind_info support (compact unwinding)
- Add symbol table support (nlist structures)

---

## HP SOM (System Object Module)

**Status:** 🚧 Stub implementation

**Documentation:**
- **HP-UX a.out(4) manual page** - Primary SOM format specification
- PA-RISC Runtime Architecture document
- HP-UX include files: `som.h`, `lst.h`

**Implementations:**
- `som.c` - HP SOM loader stub

**Architecture:**
- PA-RISC (32-bit and 64-bit)
- Big-endian format

**TODO:**
- Obtain HP-UX documentation and headers
- Implement full SOM format parsing
- Add support for shared libraries (SL)
- Update to IImageLoader interface

---

## OpenVMS Image Format

**Status:** 🚧 Stub implementation

**Documentation:**
- OpenVMS Internals and Data Structures Manual (IDSM)
- **Required header files:** `ihsdef.h` (Image Header Section) and `ihddef.h` (Image Header Descriptor)
- **Source:** https://www.digiater.nl/openvms/freeware/v80/symbols/symbols.zip
  1. Download `symbols.zip`
  2. Extract `symbols-src.zip` from the archive
  3. Look for `ihsdef.h` and `ihddef.h` in the include directory

**Header Structure:**
- IHS (Image Header Section) - Overall image structure
- IHD (Image Header Descriptor) - Per-section descriptors
- Image header size: 1024 bytes

**Implementations:**
- `vms.c` - OpenVMS image loader stub

**Architectures:**
- VAX
- Alpha (AXP)
- Itanium (limited support)

**TODO:**
- Extract ihsdef.h and ihddef.h from symbols package
- Port VMS header structures to ANANKE types
- Implement IHS/IHD parsing
- Add Image Section Descriptor (ISD) support
- Update to IImageLoader interface

---

## a.out (Assembler Output)

**Status:** ⚠️ Detection implemented, needs interface update

**Documentation:**
- BSD a.out(5) manual page
- 4.4BSD source code
- NetBSD/FreeBSD/OpenBSD kernel sources

**Implementations:**
- `aout.c` - a.out loader

**Variants:**
- OMAGIC (old impure format)
- NMAGIC (pure format)
- ZMAGIC (demand-paged format)
- QMAGIC (compact format)

**TODO:**
- Update to IImageLoader interface with HRESULT
- Add symbol table support (nlist structures)

---

## ECOFF (Extended COFF)

**Status:** ⚠️ Detection implemented, needs interface update

**Documentation:**
- MIPS ABI documentation
- Digital Unix (OSF/1) documentation
- Ultrix documentation

**Implementations:**
- `ecoff.c` - ECOFF loader

**Architectures:**
- MIPS
- Alpha (DEC Alpha)

**TODO:**
- Update to IImageLoader interface with HRESULT

---

## XCOFF (eXtended COFF)

**Status:** ⚠️ Detection implemented, needs interface update

**Documentation:**
- AIX documentation
- PowerPC ABI supplement

**Implementations:**
- `xcoff.c` - XCOFF loader

**Architectures:**
- PowerPC (32-bit and 64-bit)
- POWER

**TODO:**
- Update to IImageLoader interface with HRESULT
- Add TOC (Table of Contents) support

---

## PDP-10 SAV Format

**Status:** 🚧 Stub implementation (detection disabled)

**Documentation:**
- TOPS-10 Monitor Calls Reference Manual
- TOPS-20 Monitor Calls Reference Manual
- DEC PDP-10 System Reference Manual
- SAV file structure: Entry point + high/low segment descriptors + core image
- File extensions: `.SAV` (Save file), `.EXE` (Executable)

**File Structure:**
- Word 0: Entry point address (18-bit address in 36-bit word)
- Word 1: High segment origin and size
- Word 2: Low segment size
- Followed by program data in 36-bit words

**Implementations:**
- `pdp10.c` - PDP-10 SAV loader stub

**Architecture:**
- PDP-10 (36-bit words)
- Big-endian byte order
- 18-bit addressing (262,144 word address space)

**Notes:**
- 36-bit words are typically encoded as 5 bytes (rounded) in files
- Various encoding schemes exist (4-byte, 5-byte, 7-track tape, etc.)
- Supports programs for TOPS-10, TOPS-20, and ITS operating systems

**TODO:**
- Improve format detection (currently disabled due to weak heuristics)
- Implement 36-bit word decoding (5-byte encoding)
- Parse SAV header structure
- Add segment loading support
- Update to fully implement IImageLoader interface
- Research symbol table format (if any)

---

## Other Formats

### Xenix COFF
- **Implementation:** `xenix.c`
- **Status:** Stub implementation

### NLM (NetWare Loadable Module)
- **Implementation:** `nlm.c`
- **Status:** Detection implemented

### Linear Executable (LE/LX)
- **Implementation:** `le.c`
- **Status:** Detection implemented
- OS/2 and Windows VxD format

### PEF (Preferred Executable Format)
- **Implementation:** `pef.c`
- **Status:** Detection implemented
- Classic Mac OS PowerPC format

### Amiga Hunk
- **Implementation:** `hunk.c`
- **Status:** Detection implemented
- AmigaOS executable format

### Atari TOS
- **Implementation:** `atari.c`
- **Status:** Detection implemented
- Atari ST/TT/Falcon executable format

### Plan 9
- **Implementation:** `plan9.c`
- **Status:** Detection implemented
- Plan 9 a.out variant

---

## General Implementation Notes

### IImageLoader Interface

All loaders should implement the full `IImageLoader` COM interface defined in `imgload.h`:

```c
// Interface methods (all return HRESULT):
- Detect(ImageBase, ImageSize) -> S_OK if recognized, S_FALSE otherwise
- GetArch(ImageBase, &Architecture) -> Returns ARCH_*
- GetEndianness(ImageBase, &Endianness) -> Returns ImgEndianLittle/Big
- GetEntryPoint(ImageBase, &EntryPoint) -> Returns entry virtual address
- LoadImage(&Context) -> Loads image, populates context
- GetTlsInfo(ImageBase, &TlsInfo) -> Extracts TLS information (or S_FALSE)
- GetUnwindInfo(ImageBase, &UnwindInfo) -> Extracts unwinding data (or S_FALSE)
- GetSymbolByAddress(ImageBase, Address, &SymbolInfo) -> Symbol lookup
- GetSymbolByName(ImageBase, Name, &SymbolInfo) -> Symbol lookup

// IUnknown methods:
- QueryInterface(riid, &ppvObject)
- AddRef() -> Returns reference count
- Release() -> Returns reference count
```

### HRESULT Codes

Use standard COM HRESULT codes:
- `S_OK` - Success
- `S_FALSE` - Success with no data (e.g., no TLS, no symbols)
- `E_POINTER` - NULL pointer passed
- `E_INVALIDARG` - Invalid argument
- `E_OUTOFMEMORY` - Memory allocation failed
- `IMGLOAD_E_INVALID_FORMAT` - Invalid or corrupted image
- `IMGLOAD_E_UNSUPPORTED_ARCH` - Unsupported architecture
- `IMGLOAD_E_UNSUPPORTED_VERSION` - Unsupported format version
- `IMGLOAD_E_INVALID_HEADER` - Invalid header structure

### Unwinding Information

Different formats use different unwinding mechanisms:
- **ELF**: DWARF `.eh_frame` sections (Format = 0)
- **PE/COFF**: `.pdata` sections with RUNTIME_FUNCTION entries (Format = 1)
- **Mach-O**: `__eh_frame` or `__unwind_info` (compact unwinding, Format = 2)

### Symbol Information

Symbol tables vary by format:
- **ELF**: `.symtab` and `.dynsym` sections with ELF symbol structures
- **PE/COFF**: COFF symbol table with string table
- **Mach-O**: `nlist` symbol structures with string table
- **a.out**: `nlist` symbol structures

---

## Testing Strategy

1. **Format Detection:** Test with sample binaries for each architecture
2. **Architecture Detection:** Verify correct ARCH_* return values
3. **Endianness Detection:** Test on bi-endian architectures
4. **Entry Point:** Verify against `readelf -h`, `otool -h`, etc.
5. **Unwinding Info:** Test with exception-throwing programs
6. **Symbol Lookup:** Test with known function addresses/names

---

## Contributing

When implementing or updating loaders:

1. Update this documentation with references
2. Implement full IImageLoader interface
3. Use proper HRESULT return codes
4. Add unwinding info extraction if format supports it
5. Add symbol table support if format supports it
6. Test with real-world binaries
7. Update unit tests in `apxh/tests/`

---

## References for Multiple Formats

- **Generic references:**
  - John R. Levine, "Linkers and Loaders" (Morgan Kaufmann, 1999)
  - Wikipedia executable format articles
  - OSDev Wiki: https://wiki.osdev.org/

- **Source code references:**
  - GNU binutils: https://sourceware.org/git/binutils-gdb.git
  - LLVM: https://github.com/llvm/llvm-project
  - Radare2: https://github.com/radareorg/radare2
