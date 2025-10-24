# NUX COM Migration Examples

This guide provides practical, real-world examples of migrating from legacy NUX APIs to the new COM-style interfaces.

## Table of Contents
1. [Quick Start](#quick-start)
2. [Memory Management Examples](#memory-management-examples)
3. [CPU Operations Examples](#cpu-operations-examples)
4. [Synchronization Examples](#synchronization-examples)
5. [User Space Examples](#user-space-examples)
6. [Performance Measurement Examples](#performance-measurement-examples)
7. [Complete Kernel Module Example](#complete-kernel-module-example)

---

## Quick Start

### Option 1: No Changes (Recommended Initially)

Your existing code continues to work without any modifications:

```c
// This still works exactly as before
hal_putchar('H');
pfn_t pfn = pfn_alloc(0);
spinlock(&my_lock);
```

### Option 2: Gradual Migration

Mix old and new APIs during transition:

```c
// Use new APIs where convenient
SpinLockAcquire(&my_lock);
PFN pfn = pfn_alloc(0);  // Still using legacy, that's fine

// Old API
hal_putchar('H');
```

### Option 3: Full COM Adoption

Use COM interfaces directly:

```c
// Access global interfaces
extern IHal *gpHal;
extern INux *gpNux;

// Get sub-interfaces
IHalCpu *pHalCpu;
gpHal->lpVtbl->GetCpuInterface(gpHal, &pHalCpu);

// Use methods
pHalCpu->lpVtbl->PutChar(pHalCpu, 'H');
```

---

## Memory Management Examples

### Example 1: Physical Memory Allocation

**Legacy Code:**
```c
pfn_t pfn;
void *page;

// Allocate a physical page
pfn = pfn_alloc(0);
if (pfn == PFN_INVALID) {
    error("Failed to allocate page");
    return -1;
}

// Get temporary access to the page
page = pfn_get(pfn);
memset(page, 0, PAGE_SIZE);
pfn_put(pfn, page);
```

**New Code (Simple Migration):**
```c
PFN pfn;
VOID *pPage;

// Allocate a physical page
pfn = pfn_alloc(0);  // Still works!
if (pfn == PFN_INVALID) {
    error("Failed to allocate page");
    return -1;
}

// Get temporary access to the page
pPage = pfn_get(pfn);  // Still works!
memset(pPage, 0, PAGE_SIZE);
pfn_put(pfn, pPage);
```

**New Code (Full COM):**
```c
INuxMemory *pMemory;
PFN pfn;
VOID *pPage;

// Get memory interface
gpNux->lpVtbl->GetMemoryInterface(gpNux, &pMemory);

// Allocate a physical page
pfn = pMemory->lpVtbl->PfnAllocate(pMemory, 0);
if (pfn == PFN_INVALID) {
    error("Failed to allocate page");
    return -1;
}

// Get temporary access
pPage = pMemory->lpVtbl->PfnGet(pMemory, pfn);
memset(pPage, 0, PAGE_SIZE);
pMemory->lpVtbl->PfnPut(pMemory, pfn, pPage);
```

### Example 2: Kernel Memory Allocation

**Legacy Code:**
```c
vaddr_t buffer;
size_t size = 4096 * 10;  // 10 pages

buffer = kmem_alloc(0, size);
if (buffer == VADDR_INVALID) {
    error("Failed to allocate kernel memory");
    return NULL;
}

// Use the buffer
memset((void *)buffer, 0, size);

// Free when done
kmem_free(0, buffer, size);
```

**New Code (Simple Migration):**
```c
VIRTUAL_ADDRESS buffer;
UINTN size = 4096 * 10;  // 10 pages

buffer = kmem_alloc(0, size);  // Still works!
if (buffer == VADDR_INVALID) {
    error("Failed to allocate kernel memory");
    return NULL;
}

memset((VOID *)buffer, 0, size);
kmem_free(0, buffer, size);
```

**New Code (Full COM):**
```c
INuxKmem *pKmem;
VIRTUAL_ADDRESS buffer;
UINTN size = 4096 * 10;

gpNux->lpVtbl->GetKmemInterface(gpNux, &pKmem);

buffer = pKmem->lpVtbl->Allocate(pKmem, 0, size);
if (buffer == VADDR_INVALID) {
    error("Failed to allocate kernel memory");
    return NULL;
}

memset((VOID *)buffer, 0, size);
pKmem->lpVtbl->Free(pKmem, 0, buffer, size);
```

### Example 3: Virtual Memory Mapping

**Legacy Code:**
```c
vaddr_t va;
pfn_t pfn;
void *ptr;

// Allocate kernel virtual address
va = kva_alloc(PAGE_SIZE);
if (va == VADDR_INVALID) {
    return -1;
}

// Allocate physical page
pfn = pfn_alloc(0);

// Map it
kmap_map(va, pfn, HAL_PTE_P | HAL_PTE_W);
kmap_commit();

// Use it
ptr = (void *)va;
memset(ptr, 0, PAGE_SIZE);

// Cleanup
kmap_unmap(va);
kva_free(va, PAGE_SIZE);
pfn_free(pfn);
```

**New Code (Full COM):**
```c
INuxKva *pKva;
INuxKmap *pKmap;
INuxMemory *pMemory;
VIRTUAL_ADDRESS va;
PFN pfn;
VOID *pPtr;

// Get interfaces
gpNux->lpVtbl->GetKvaInterface(gpNux, &pKva);
gpNux->lpVtbl->GetKmapInterface(gpNux, &pKmap);
gpNux->lpVtbl->GetMemoryInterface(gpNux, &pMemory);

// Allocate virtual address
va = pKva->lpVtbl->Allocate(pKva, PAGE_SIZE);
if (va == VADDR_INVALID) {
    return -1;
}

// Allocate physical page
pfn = pMemory->lpVtbl->PfnAllocate(pMemory, 0);

// Map it
pKmap->lpVtbl->Map(pKmap, va, pfn, HAL_PTE_P | HAL_PTE_W);
pKmap->lpVtbl->Commit(pKmap);

// Use it
pPtr = (VOID *)va;
memset(pPtr, 0, PAGE_SIZE);

// Cleanup
pKmap->lpVtbl->Unmap(pKmap, va);
pKva->lpVtbl->Free(pKva, va, PAGE_SIZE);
pMemory->lpVtbl->PfnFree(pMemory, pfn);
```

---

## CPU Operations Examples

### Example 4: Inter-Processor Interrupts

**Legacy Code:**
```c
// Send IPI to CPU 2
cpu_ipi(2);

// Broadcast IPI to all CPUs
cpu_ipi_broadcast();

// Send NMI to all but current CPU
cpu_nmi_allbutself();
```

**New Code (Simple Migration):**
```c
// Send IPI to CPU 2
cpu_ipi(2);  // Still works!

// Broadcast IPI to all CPUs
cpu_ipi_broadcast();

// Send NMI to all but current CPU
cpu_nmi_allbutself();
```

**New Code (Full COM):**
```c
INuxCpu *pCpu;

gpNux->lpVtbl->GetCpuInterface(gpNux, &pCpu);

// Send IPI to CPU 2
pCpu->lpVtbl->SendIpi(pCpu, 2);

// Broadcast IPI to all CPUs
pCpu->lpVtbl->BroadcastIpi(pCpu);

// Send NMI to all but current CPU
pCpu->lpVtbl->BroadcastNmiAllButSelf(pCpu);
```

### Example 5: TLB Management

**Legacy Code:**
```c
cpumask_t mask = 0x0F;  // CPUs 0-3

// Flush TLB on specific CPUs
cpu_tlbflush_mask(mask);

// Flush TLB on all CPUs and wait
cpu_tlbflush_broadcast_sync();

// Update kernel TLB
cpu_ktlb_update();
```

**New Code (Full COM):**
```c
INuxCpu *pCpu;
CPU_MASK mask = 0x0F;

gpNux->lpVtbl->GetCpuInterface(gpNux, &pCpu);

// Flush TLB on specific CPUs
pCpu->lpVtbl->FlushTlbMask(pCpu, mask);

// Flush TLB on all CPUs and wait
pCpu->lpVtbl->BroadcastFlushTlbSync(pCpu);

// Update kernel TLB
pCpu->lpVtbl->UpdateKernelTlb(pCpu);
```

---

## Synchronization Examples

### Example 6: Spinlock Usage

**Legacy Code:**
```c
lock_t my_lock = LOCK_INITIALIZER;

void protected_operation(void) {
    spinlock(&my_lock);
    // Critical section
    critical_work();
    spinunlock(&my_lock);
}
```

**New Code (Simple Migration):**
```c
SPINLOCK MyLock = LOCK_INITIALIZER;

VOID ProtectedOperation(VOID) {
    SpinLockAcquire(&MyLock);
    // Critical section
    CriticalWork();
    SpinLockRelease(&MyLock);
}
```

### Example 7: Read-Write Lock

**Legacy Code:**
```c
rwlock_t rw_lock = RWLOCK_INITIALIZER;

void reader(void) {
    rwlock_rlock(&rw_lock);
    // Read-only access
    read_data();
    rwlock_runlock(&rw_lock);
}

void writer(void) {
    rwlock_wlock(&rw_lock);
    // Write access
    write_data();
    rwlock_wunlock(&rw_lock);
}
```

**New Code (Simple Migration):**
```c
RWLOCK RwLock = RWLOCK_INITIALIZER;

VOID Reader(VOID) {
    RwLockAcquireRead(&RwLock);
    // Read-only access
    ReadData();
    RwLockReleaseRead(&RwLock);
}

VOID Writer(VOID) {
    RwLockAcquireWrite(&RwLock);
    // Write access
    WriteData();
    RwLockReleaseWrite(&RwLock);
}
```

---

## User Space Examples

### Example 8: User Address Validation and Copy

**Legacy Code:**
```c
bool copy_from_user(void *dst, uaddr_t src, size_t size) {
    if (!uaddr_validrange(src, size)) {
        return false;
    }

    return uaddr_copyfrom(dst, src, size, NULL);
}
```

**New Code (Full COM):**
```c
BOOLEAN CopyFromUser(VOID *pDst, USER_ADDRESS Src, UINTN Size) {
    INuxUaddr *pUaddr;

    gpNux->lpVtbl->GetUaddrInterface(gpNux, &pUaddr);

    if (!pUaddr->lpVtbl->ValidRange(pUaddr, Src, Size)) {
        return FALSE;
    }

    return pUaddr->lpVtbl->CopyFrom(pUaddr, pDst, Src, Size, NULL);
}
```

### Example 9: User Context Manipulation

**Legacy Code:**
```c
void setup_user_context(uctxt_t *uctxt, vaddr_t entry, vaddr_t stack) {
    uctxt_init(uctxt, entry, stack, 0);
    uctxt_seta0(uctxt, 42);  // Pass argument
    uctxt_seta1(uctxt, 123);
}

vaddr_t get_user_ip(uctxt_t *uctxt) {
    return uctxt_getip(uctxt);
}
```

**New Code (Full COM):**
```c
VOID SetupUserContext(UCTXT *pUctxt, VIRTUAL_ADDRESS Entry, VIRTUAL_ADDRESS Stack) {
    INuxUctxt *pUctxtIf;

    gpNux->lpVtbl->GetUctxtInterface(gpNux, &pUctxtIf);

    pUctxtIf->lpVtbl->Init(pUctxtIf, pUctxt, Entry, Stack, 0);
    pUctxtIf->lpVtbl->SetA0(pUctxtIf, pUctxt, 42);  // Pass argument
    pUctxtIf->lpVtbl->SetA1(pUctxtIf, pUctxt, 123);
}

VIRTUAL_ADDRESS GetUserIp(UCTXT *pUctxt) {
    INuxUctxt *pUctxtIf;

    gpNux->lpVtbl->GetUctxtInterface(gpNux, &pUctxtIf);
    return pUctxtIf->lpVtbl->GetIp(pUctxtIf, pUctxt);
}
```

---

## Performance Measurement Examples

### Example 10: Using Performance Counters

**Legacy Code:**
```c
nuxperf_t __perf page_alloc_counter = {
    .name = "page_allocations",
    .val = 0
};

void my_alloc_function(void) {
    nuxperf_inc(&page_alloc_counter);
    // ... allocation logic
}

void print_stats(void) {
    nuxperf_print();
}
```

**New Code (Simple Migration):**
```c
NUXPERF_COUNTER __perf PageAllocCounter = {
    .pName = "page_allocations",
    .Value = 0
};

VOID MyAllocFunction(VOID) {
    NuxPerfCounterIncrement(&PageAllocCounter);
    // ... allocation logic
}

VOID PrintStats(VOID) {
    NuxPerfCounterPrint();
}
```

### Example 11: Using Performance Measures

**Legacy Code:**
```c
DEFINE_MEASURE(lock_wait_time);

void acquire_with_measurement(lock_t *lock) {
    uint64_t start = timer_gettime();
    spinlock(lock);
    uint64_t elapsed = timer_gettime() - start;
    nuxmeasure_add(&lock_wait_time, elapsed);
}
```

**New Code (Simple Migration):**
```c
DEFINE_MEASURE(LockWaitTime);

VOID AcquireWithMeasurement(SPINLOCK *pLock) {
    UINT64 start = timer_gettime();
    SpinLockAcquire(pLock);
    UINT64 elapsed = timer_gettime() - start;
    NuxPerfMeasureAdd(&LockWaitTime, elapsed);
}
```

---

## Complete Kernel Module Example

### Example 12: Complete Driver Module

**Legacy Code:**
```c
#include <nux/nux.h>
#include <nux/locks.h>

static lock_t driver_lock = LOCK_INITIALIZER;
static void *device_buffer = NULL;
static size_t buffer_size = PAGE_SIZE * 4;

int driver_init(void) {
    vaddr_t va;
    pfn_t pfn;

    // Allocate buffer
    va = kmem_alloc(0, buffer_size);
    if (va == VADDR_INVALID) {
        error("Failed to allocate buffer");
        return -1;
    }

    device_buffer = (void *)va;
    info("Driver initialized");
    return 0;
}

void driver_operation(void) {
    spinlock(&driver_lock);

    // Protected operation
    memset(device_buffer, 0, buffer_size);

    spinunlock(&driver_lock);
}

void driver_cleanup(void) {
    if (device_buffer) {
        kmem_free(0, (vaddr_t)device_buffer, buffer_size);
        device_buffer = NULL;
    }
}
```

**New Code (Full COM - Recommended for New Code):**
```c
#include <nux/nux.h>
#include <nux/locks.h>

static SPINLOCK gDriverLock = LOCK_INITIALIZER;
static VOID *gpDeviceBuffer = NULL;
static UINTN gBufferSize = PAGE_SIZE * 4;

INT32 DriverInit(VOID) {
    INuxKmem *pKmem;
    VIRTUAL_ADDRESS va;

    // Get kernel memory interface
    gpNux->lpVtbl->GetKmemInterface(gpNux, &pKmem);

    // Allocate buffer
    va = pKmem->lpVtbl->Allocate(pKmem, 0, gBufferSize);
    if (va == VADDR_INVALID) {
        error("Failed to allocate buffer");
        return -1;
    }

    gpDeviceBuffer = (VOID *)va;
    info("Driver initialized");
    return 0;
}

VOID DriverOperation(VOID) {
    SpinLockAcquire(&gDriverLock);

    // Protected operation
    memset(gpDeviceBuffer, 0, gBufferSize);

    SpinLockRelease(&gDriverLock);
}

VOID DriverCleanup(VOID) {
    INuxKmem *pKmem;

    if (gpDeviceBuffer) {
        gpNux->lpVtbl->GetKmemInterface(gpNux, &pKmem);
        pKmem->lpVtbl->Free(pKmem, 0, (VIRTUAL_ADDRESS)gpDeviceBuffer, gBufferSize);
        gpDeviceBuffer = NULL;
    }
}
```

**New Code (Mixed Approach - Pragmatic):**
```c
#include <nux/nux.h>
#include <nux/locks.h>

static SPINLOCK gDriverLock = LOCK_INITIALIZER;
static VOID *gpDeviceBuffer = NULL;
static UINTN gBufferSize = PAGE_SIZE * 4;

INT32 DriverInit(VOID) {
    VIRTUAL_ADDRESS va;

    // Use legacy API - it's fine during transition!
    va = kmem_alloc(0, gBufferSize);
    if (va == VADDR_INVALID) {
        error("Failed to allocate buffer");
        return -1;
    }

    gpDeviceBuffer = (VOID *)va;
    info("Driver initialized");
    return 0;
}

VOID DriverOperation(VOID) {
    // Use new API where it's clearer
    SpinLockAcquire(&gDriverLock);

    memset(gpDeviceBuffer, 0, gBufferSize);

    SpinLockRelease(&gDriverLock);
}

VOID DriverCleanup(VOID) {
    if (gpDeviceBuffer) {
        // Legacy API still works
        kmem_free(0, (VIRTUAL_ADDRESS)gpDeviceBuffer, gBufferSize);
        gpDeviceBuffer = NULL;
    }
}
```

---

## Migration Strategy Recommendations

### For Existing Code
1. **Don't rush** - Your code works as-is
2. **Start simple** - Use new type names (VOID, UINTN, etc.)
3. **Update gradually** - Replace functions as you refactor
4. **Test frequently** - Ensure compatibility at each step

### For New Code
1. **Use COM interfaces** - Start with the new architecture
2. **Follow NT style** - PascalCase, proper annotations
3. **Document thoroughly** - Use UEFI-style comments
4. **Leverage type safety** - Use new types consistently

### Best Practices
1. **Mix approaches freely** - Use what makes sense
2. **Inline wrappers are free** - No performance cost
3. **Reference docs** - Check TRANSFORMATION_GUIDE.md
4. **Ask questions** - Documentation is comprehensive

---

## Common Patterns

### Pattern 1: Initialize-Use-Cleanup
```c
VOID MyFunction(VOID) {
    INuxKmem *pKmem;
    VIRTUAL_ADDRESS buffer;

    // Initialize
    gpNux->lpVtbl->GetKmemInterface(gpNux, &pKmem);
    buffer = pKmem->lpVtbl->Allocate(pKmem, 0, PAGE_SIZE);

    // Use
    ProcessBuffer((VOID *)buffer);

    // Cleanup
    pKmem->lpVtbl->Free(pKmem, 0, buffer, PAGE_SIZE);
}
```

### Pattern 2: Error Handling
```c
BOOLEAN MyOperation(VOID) {
    INuxMemory *pMemory;
    PFN pfn;

    gpNux->lpVtbl->GetMemoryInterface(gpNux, &pMemory);

    pfn = pMemory->lpVtbl->PfnAllocate(pMemory, 0);
    if (pfn == PFN_INVALID) {
        error("Allocation failed");
        return FALSE;
    }

    // Success path
    pMemory->lpVtbl->PfnFree(pMemory, pfn);
    return TRUE;
}
```

### Pattern 3: Interface Caching
```c
// Cache frequently-used interfaces
static INuxMemory *gpCachedMemory = NULL;

VOID InitCache(VOID) {
    gpNux->lpVtbl->GetMemoryInterface(gpNux, &gpCachedMemory);
}

VOID FastAlloc(VOID) {
    PFN pfn = gpCachedMemory->lpVtbl->PfnAllocate(gpCachedMemory, 0);
    // ... use pfn
}
```

---

## Troubleshooting

### Issue: Undefined types
**Solution**: Include `<nux/types.h>` or use legacy types

### Issue: Vtable syntax is verbose
**Solution**: Use legacy wrappers or create helper macros

### Issue: Global interface pointers not found
**Solution**: Ensure proper initialization in your kernel

### Issue: Performance concerns
**Solution**: Inline wrappers have zero overhead; COM calls are as fast as function pointers

---

**Document Version**: 1.0
**Last Updated**: October 24, 2025
**See Also**: TRANSFORMATION_GUIDE.md, COM_ARCHITECTURE.md
