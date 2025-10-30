# NUX Subprojects - Transformation Status and Guidelines

This document explains the various subprojects in the NUX kernel library, their transformation status, and recommendations for each.

## Subproject Overview

```
nux/
├── include/nux/          ✅ TRANSFORMED (15 headers, all public APIs)
├── apxh/                 ⚠️  BOOTLOADER (minimal changes needed)
├── contrib/              ❌ EXTERNAL (do not transform)
├── example/              📝 SHOULD UPDATE (demonstration code)
├── libec/                ❌ STANDARD C LIB (do not transform)
├── libfdt/               ❌ EXTERNAL (do not transform)
├── libhal_riscv/         💡 IMPLEMENTATION (optional transformation)
├── libhal_x86/           💡 IMPLEMENTATION (optional transformation)
├── libnux/               💡 IMPLEMENTATION (optional transformation)
├── libnux_user/          💡 USER-SPACE (optional transformation)
├── libplt_acpi/          💡 IMPLEMENTATION (optional transformation)
└── libplt_sbi/           💡 IMPLEMENTATION (optional transformation)
```

---

## 1. Public Headers (include/nux/) - ✅ COMPLETE

### Status: FULLY TRANSFORMED

All 15 public API headers have been transformed to COM-style with NT coding conventions.

**Files Transformed:**
- combase.h, types.h, hal.h, plt.h, nux.h
- defs.h, locks.h, slab.h, slabinc.h, cpumask.h, cache.h
- apxh.h, nuxperf.h, symbol.h, nmiemul.h

**Impact**: All code that includes these headers automatically benefits from:
- Better documentation
- Type safety
- Modern coding standards
- 100% backward compatibility

---

## 2. External Dependencies (contrib/) - ❌ DO NOT TRANSFORM

### Submodules:
- **gnu-efi**: EFI development library (external project)
- **binutils**: Binary utilities (external project)
- **dtc**: Device Tree Compiler (external project)

**Recommendation**: Leave unchanged. These are external dependencies maintained by their respective projects.

---

## 3. Standard C Library (libec/) - ❌ DO NOT TRANSFORM

### Purpose:
Embedded C library based on NetBSD libc, providing standard C functionality.

**Files Include:**
- Standard C headers (stdint.h, limits.h, assert.h, etc.)
- String functions (string.h, strings.h)
- Memory functions (stdlib.h)
- Architecture-specific implementations

**Recommendation**: Leave unchanged. This is a standard C library that should maintain POSIX/ISO C compatibility. Transforming it would break portability and standard compliance.

---

## 4. Device Tree Library (libfdt/) - ❌ DO NOT TRANSFORM

### Purpose:
Flattened Device Tree manipulation library (external).

**Recommendation**: Leave unchanged. External library used for device tree parsing on ARM/RISC-V platforms.

---

## 5. APXH Bootloader (apxh/) - ⚠️ MINIMAL CHANGES

### Purpose:
Boot protocol implementation that loads the kernel.

**Current Status**: Uses boot protocol structures from apxh.h (now transformed).

**What Changed:**
- apxh.h header is now NT-style
- Boot structures use NT naming (APXH_BOOT_INFO, etc.)
- Legacy aliases maintain compatibility

**Recommendation**:
- Minimal changes needed
- Update structure usage to new names (optional)
- Legacy names still work
- Focus on functionality, not style

**Example Change (optional):**
```c
// Old
struct apxh_bootinfo *bi;

// New (optional)
APXH_BOOT_INFO *pBootInfo;

// Both work due to typedef
```

---

## 6. Example Code (example/) - 📝 SHOULD UPDATE

### Purpose:
Demonstration kernel and userspace programs.

**Current Status**: Uses legacy APIs.

**Recommendation**: **Update to demonstrate new APIs**

This is the perfect place to show developers how to use the new COM interfaces!

**Suggested Updates:**

1. **Create new examples showing COM usage:**
```c
// example/kern/main_com.c - New file showing COM style
#include <nux/nux.h>

INT32 main(INT32 argc, CHAR8 *argv[]) {
    INuxMemory *pMemory;
    PFN pfn;

    // Get memory interface
    gpNux->lpVtbl->GetMemoryInterface(gpNux, &pMemory);

    // Use COM interface
    pfn = pMemory->lpVtbl->PfnAllocate(pMemory, 0);
    info("Allocated PFN: %ld", pfn);

    return EXIT_IDLE;
}
```

2. **Keep existing examples for backward compatibility:**
   - Shows that old code still works
   - Demonstrates migration path

3. **Add side-by-side comparisons:**
   - example/kern/main_legacy.c (current style)
   - example/kern/main_modern.c (new COM style)
   - example/kern/main_mixed.c (mixed approach)

---

## 7. Implementation Libraries - 💡 OPTIONAL TRANSFORMATION

### 7.1 Main NUX Library (libnux/)

**Purpose**: Core NUX kernel library implementation.

**Files**: ~20 C files implementing the public APIs.

**Status**: Uses transformed headers, implements legacy functions.

**Transformation Options:**

**Option A: No Changes (Recommended Initially)**
- Implementation files work as-is
- They provide the legacy functions that the wrappers call
- Focus on functionality, not style

**Option B: Gradual Internal Cleanup**
- Update internal function names to NT style
- Keep legacy API functions as thin wrappers
- Improves code readability over time

**Option C: Full Transformation**
- Create COM interface implementations
- Implement vtables for all interfaces
- Significant effort, best done incrementally

**Recommendation**: Option A or B. The headers are already transformed, which is what matters for external users.

### 7.2 HAL Implementations (libhal_x86/, libhal_riscv/)

**Purpose**: Hardware abstraction layer for x86 and RISC-V.

**Status**: Implements IHal* interfaces defined in hal.h.

**Transformation Approach:**

Current structure:
```c
// libhal_x86/cpu.c
void hal_putchar(char c) {
    // Implementation
}
```

Could evolve to:
```c
// libhal_x86/cpu.c
static VOID EFIAPI HalCpuPutChar(IN IHalCpu *This, IN CHAR8 Ch) {
    // Implementation
}

// Vtable
static IHalCpuVtbl gHalCpuVtbl = {
    // IUnknown methods
    .QueryInterface = HalCpuQueryInterface,
    .AddRef = HalCpuAddRef,
    .Release = HalCpuRelease,
    // IHalCpu methods
    .PutChar = HalCpuPutChar,
    // ... other methods
};

// Legacy wrapper
void hal_putchar(char c) {
    HalCpuPutChar(&gHalCpu, c);
}
```

**Recommendation**: Transform incrementally, one interface at a time.

### 7.3 Platform Implementations (libplt_acpi/, libplt_sbi/)

**Purpose**: Platform layer for ACPI (x86) and SBI (RISC-V).

**Status**: Implements IPlt* interfaces defined in plt.h.

**Recommendation**: Similar to HAL - transform incrementally if desired, but not required for functionality.

### 7.4 User-Space Library (libnux_user/)

**Purpose**: User-space system call interface.

**Transformation Priority**: Medium

This library defines the user-space ABI. Changes here affect:
- Application compatibility
- System call interface
- User/kernel interaction

**Recommendation**:
- Keep ABI stable
- Can update internal implementation
- Add new COM-style APIs alongside legacy

---

## Transformation Priority Matrix

| Subproject | Priority | Effort | Impact | Recommendation |
|------------|----------|--------|--------|----------------|
| include/nux/ | CRITICAL | HIGH | HIGH | ✅ DONE |
| example/ | HIGH | LOW | HIGH | 📝 DO IT |
| libnux/ | MEDIUM | HIGH | MEDIUM | Later |
| libhal_*/ | MEDIUM | HIGH | MEDIUM | Later |
| libplt_*/ | MEDIUM | MEDIUM | MEDIUM | Later |
| libnux_user/ | MEDIUM | MEDIUM | HIGH | Careful |
| apxh/ | LOW | LOW | LOW | Optional |
| libec/ | NONE | N/A | N/A | ❌ Don't |
| libfdt/ | NONE | N/A | N/A | ❌ Don't |
| contrib/ | NONE | N/A | N/A | ❌ Don't |

---

## Recommended Next Steps

### Immediate (High Value, Low Effort)

1. **Update example/kern/main.c**
   - Add COM interface usage examples
   - Show best practices
   - Demonstrate new APIs

2. **Create example/docs/MIGRATION.md**
   - Document migration for kernel developers
   - Show before/after examples
   - Link to MIGRATION_EXAMPLES.md

3. **Add example/kern/main_com_demo.c**
   - Pure COM style example
   - Showcase all interfaces
   - Educational reference

### Short-term (Medium Priority)

4. **Update libnux/ internal code style**
   - Internal functions to NT style
   - Keep legacy APIs as wrappers
   - Improve maintainability

5. **Add COM vtable implementations**
   - Implement IHal vtable in libhal_*
   - Implement IPlt vtable in libplt_*
   - Implement INux vtable in libnux
   - Make global interface pointers available

### Long-term (Lower Priority)

6. **Full implementation transformation**
   - Complete COM implementation in all libs
   - Comprehensive test suite
   - Performance validation

---

## Build System Considerations

### Current State
The build system (autoconf/make) works with transformed headers because:
- Backward compatibility is 100%
- Inline wrappers compile away
- No ABI changes

### Future Considerations

If transforming implementations:

1. **Maintain Build Compatibility**
   - Keep existing build targets
   - Add new COM-enabled builds (optional)
   - Test both configurations

2. **Linker Symbols**
   - Legacy functions must remain exported
   - New interfaces can be internal
   - Symbol versioning for ABI stability

3. **Dependencies**
   - Headers depend on combase.h
   - Implementations depend on headers
   - External code depends on legacy APIs

---

## Testing Strategy

### For Transformed Headers (Done)
- ✅ Headers compile cleanly
- ✅ Legacy code still works
- ✅ New APIs available

### For Example Code (To Do)
- Test legacy examples still build
- Test new COM examples work
- Verify mixed approach works

### For Implementation Libraries (Future)
- Unit tests for each interface
- Integration tests for subsystems
- Performance regression tests
- ABI compatibility tests

---

## Summary

### What's Done ✅
- All public headers transformed (include/nux/)
- Full backward compatibility maintained
- Comprehensive documentation created

### What's Recommended 📝
- Update example/ to demonstrate new APIs
- Show both legacy and modern approaches
- Provide migration examples

### What's Optional 💡
- Transform implementation files (libnux/, libhal_*, libplt_*)
- Internal code cleanup
- Full COM vtable implementation

### What Not To Touch ❌
- External dependencies (contrib/)
- Standard C library (libec/)
- Device tree library (libfdt/)

---

## Contact and Resources

- **Main Documentation**: TRANSFORMATION_GUIDE.md
- **Migration Guide**: MIGRATION_EXAMPLES.md
- **Architecture**: COM_ARCHITECTURE.md
- **Summary**: TRANSFORMATION_SUMMARY.md

The header transformation is complete and provides immediate value. Implementation transformation is optional and can be done incrementally based on project needs and priorities.

---

**Document Version**: 1.0
**Last Updated**: October 24, 2025
**Status**: Headers complete, implementation optional
