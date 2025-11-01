# OpenCL and HIP Implementation Feasibility Study

**Document**: Analysis of implementing OpenCL and HIP compute frameworks on top of the Vincent GLES20 shader infrastructure

**Date**: 2025-11-01

**Status**: Feasibility Analysis

---

## Executive Summary

This document analyzes the feasibility of implementing OpenCL and HIP compute APIs on top of the Vincent GLES20 shader infrastructure. The analysis concludes that:

- **OpenCL feasibility**: 7/10 difficulty - Challenging but doable (3-6 months)
- **HIP feasibility**: 8-9/10 difficulty - Very challenging (6-12 months)
- **Recommendation**: Extract common IL/JIT/interpreter infrastructure and build OpenCL/HIP as peers to GLES20, rather than layering compute on top of graphics

---

## Table of Contents

1. [Current GLES20 Infrastructure](#current-gles20-infrastructure)
2. [OpenCL Feasibility Analysis](#opencl-feasibility-analysis)
3. [HIP Feasibility Analysis](#hip-feasibility-analysis)
4. [Recommended Architecture](#recommended-architecture)
5. [Implementation Path](#implementation-path)
6. [Practical Recommendations](#practical-recommendations)

---

## Current GLES20 Infrastructure

### What We Have Built

The Vincent GLES20 implementation provides a strong foundation with:

**✓ Shader Intermediate Language (IL)**
- 50+ opcodes covering arithmetic, vector, math, texture, and control flow
- Full instruction set documented in `ananke/contrib/libs/gles20/gl/frontend/il.h`
- Support for vertex and fragment shader execution models

**✓ JIT Compiler** (`ananke/contrib/libs/gles20/gl/backend/sljit.c`)
- Based on sljit (Simple JIT library)
- Multi-architecture support: i386, x32, RISC-V 32/64
- 2,796 lines of optimized code generation
- Full opcode coverage with hardware acceleration

**✓ Interpreter Fallback** (`ananke/contrib/libs/gles20/gl/backend/interpreter.c`)
- 1,108 lines of portable C code
- Instruction pointer-based execution model
- Complete control flow implementation
- Fallback when JIT unavailable or disabled

**✓ Control Flow**
- IF/ELSE/ENDIF: Conditional branching
- LOOP/ENDLOOP: Infinite loops
- REP/ENDREP: Counted loops
- BRK: Loop exit
- CAL/RET: Subroutine calls with 32-level call stack
- BRA: Conditional branching with labels
- SCC: Condition code register

**✓ Vector Operations**
- vec4 arithmetic (ADD, SUB, MUL, MAD, MIN, MAX)
- Dot products (DP2, DP3, DP4, DPH)
- Cross product (XPD)
- Linear interpolation (LRP)
- Comparison operations (SGE, SLT, SLE, SGT, CMP)

**✓ Math Library**
- Transcendental functions: SIN, COS, SCS
- Exponential/logarithmic: EXP, LOG, EX2, LG2, POW
- Utility: ABS, FLR, FRC, RCP, RSQ, SSG

**✓ Memory Access**
- Uniforms: Read-only shader parameters
- Temps: Temporary registers
- Attributes: Per-vertex inputs (vertex shaders)
- Varyings: Interpolated vertex→fragment data
- Textures: 2D, 3D, Cube sampling with filtering

### Architecture Overview

```
┌─────────────────────────────────────────────┐
│         GLES20 Graphics API                 │
└──────────────────┬──────────────────────────┘
                   │
       ┌───────────▼──────────┐
       │   GLSL Compiler      │
       │   (Frontend)         │
       └───────────┬──────────┘
                   │
       ┌───────────▼──────────┐
       │    Shader IL         │
       │    (Linker)          │
       └─────┬────────────┬───┘
             │            │
    ┌────────▼──┐    ┌───▼────────┐
    │   JIT     │    │ Interpreter│
    │ (sljit)   │    │ Fallback   │
    └────────┬──┘    └───┬────────┘
             │            │
             └──────┬─────┘
                    │
        ┌───────────▼───────────┐
        │  Native Execution     │
        │  (Vertex/Fragment)    │
        └───────────────────────┘
```

---

## OpenCL Feasibility Analysis

### Difficulty Rating: 7/10 (Challenging but Doable)

### What Maps Well ✓

**1. Basic Execution Model**
- OpenCL kernel ≈ GLES20 shader program
- Instruction-by-instruction execution already implemented
- Control flow semantics are compatible

**2. Arithmetic and Math Operations**
- Most OpenCL built-in math functions already present
- Vector operations (vec4 ≈ float4)
- Fast math approximations available

**3. Control Flow**
- Conditional branching (if/else)
- Loops (for, while)
- Function calls
- All recently implemented with full semantics

**4. Vectorization**
- vec4 operations can map to OpenCL vector types
- SIMD-friendly instruction set

**5. JIT Infrastructure**
- sljit provides portable code generation
- Already supports multiple architectures
- Can be extended for compute-specific optimizations

### Major Challenges ✗

#### 1. Execution Model Mismatch (HARD)

**GLES20 Model:**
```
Vertex Shader  →  Rasterization  →  Fragment Shader
(per vertex)                         (per pixel)

Linear graphics pipeline
Implicit parallelism
No explicit thread management
```

**OpenCL Model:**
```
3D Grid of Work-Items
├── Work-Group [0,0,0] (threads: [0-N])
├── Work-Group [0,0,1] (threads: [0-N])
├── ...
└── Work-Group [X,Y,Z] (threads: [0-N])

Explicit parallelism
Work-item/work-group hierarchy
Requires scheduler and thread pool
```

**Solution Required:**
- Work-group scheduler
- Thread pool implementation
- 3D index calculation (get_global_id, get_local_id, get_group_id)
- Work division and load balancing

**Estimated Effort:** ~2,000 lines of code

---

#### 2. Memory Model Mismatch (VERY HARD)

**OpenCL Memory Hierarchy:**
```c
__global   float *g;  // Global memory (all work-items, requires sync)
__local    float *l;  // Shared within work-group (fast, needs barriers)
__private  float *p;  // Per work-item (registers/stack)
__constant float *c;  // Read-only global constants
```

**GLES20 Memory Model:**
```c
uniform    vec4 u;    // Read-only shader parameters (≈ __constant)
varying    vec4 v;    // Interpolated vertex→fragment data
attribute  vec4 a;    // Per-vertex input data
temp       vec4 t;    // Temporary registers (≈ __private)
```

**Critical Missing Piece: `__local` Memory**

`__local` memory is essential for OpenCL performance:
- Shared scratchpad within work-group
- Much faster than global memory
- Requires explicit synchronization
- Key to algorithms like reduction, scan, FFT

**Problems:**
1. No shared memory concept in GLES20
2. No synchronization primitives
3. No memory coherency guarantees
4. No cache control

**Solution Required:**
- Allocate per-work-group shared memory
- Implement memory fence operations
- Ensure cache coherency across threads
- Synchronization barriers (see challenge #3)

**Estimated Effort:** ~1,000 lines of code

---

#### 3. Synchronization Primitives (HARD)

**OpenCL Requirements:**
```c
// Work-group barrier - all threads wait
barrier(CLK_LOCAL_MEM_FENCE);
barrier(CLK_GLOBAL_MEM_FENCE);

// Memory fences - ensure memory visibility
mem_fence(CLK_LOCAL_MEM_FENCE);
read_mem_fence(CLK_LOCAL_MEM_FENCE);
write_mem_fence(CLK_LOCAL_MEM_FENCE);

// Atomic operations - thread-safe updates
atomic_add(&counter, 1);
atomic_cmpxchg(&lock, 0, 1);
atomic_min(&result, value);
```

**GLES20 Capabilities:**
- **NOTHING** - Graphics pipeline is implicitly synchronized
- No barrier instructions
- No atomic operations
- No memory ordering guarantees

**Solution Required:**
- Implement barrier() as IL instruction
- Runtime support for thread synchronization
- Atomic operation primitives
- Memory ordering semantics

**Implementation Challenges:**
- Barrier requires suspending work-group execution
- Must track thread arrival count
- Resume all threads once barrier reached
- Handle deadlocks and timeouts
- Atomic ops require hardware support or locks

**Estimated Effort:** ~500 lines of code (barrier), ~800 lines (atomics)

---

#### 4. Missing IL Opcodes

**Required Additions:**

| Opcode | Purpose | Complexity |
|--------|---------|------------|
| `barrier` | Work-group synchronization | Very Hard |
| `atomic_add/sub/xchg/cmpxchg/min/max` | Thread-safe operations | Hard |
| `fence` | Memory ordering | Medium |
| `get_global_id(dim)` | Work-item global index | Easy |
| `get_local_id(dim)` | Work-item local index | Easy |
| `get_group_id(dim)` | Work-group index | Easy |
| `get_global_size(dim)` | Total work-items | Easy |
| `get_local_size(dim)` | Work-group size | Easy |
| `convert_int/uint/float/double` | Type conversions | Medium |
| `vload/vstore` | Vector memory access | Medium |
| `select` | Conditional select | Easy |

**Estimated Effort:** ~1,000 lines of IL definitions and implementations

---

#### 5. Type System Gap

**OpenCL Type System:**
```c
// Scalar types
char, uchar, short, ushort, int, uint, long, ulong
float, double, half

// Vector types (2, 3, 4, 8, 16 elements)
int2, int4, int8, int16
float2, float4, float8, float16

// Pointers
__global float *ptr;
__local int *shared;

// Structures
struct Point { float x, y, z; };

// Arrays
float data[1024];
```

**GLES20 Type System:**
```c
// Primarily vec4 floats
vec4

// Limited integer support
// No pointer types
// No struct types
// No array types (except constant arrays)
```

**Solution Required:**
- Extend IL to support scalar types
- Add vector type variants (int4, float4, etc.)
- Implement pointer semantics
- Struct and array support
- Type conversion operations

**Estimated Effort:** ~2,000 lines of code

---

#### 6. Work-Group Scheduler

**Requirements:**

A complete work-group scheduler must:

1. **Divide global work into work-groups**
   ```c
   // Example: 1024x1024 global size, 16x16 local size
   // Results in 64x64 = 4,096 work-groups
   ```

2. **Schedule work-groups across CPU cores**
   - Thread pool management
   - Load balancing
   - Affinity control

3. **Manage `__local` memory per work-group**
   - Allocate shared memory
   - Initialize before execution
   - Cleanup after completion

4. **Execute work-items within work-group**
   - Launch threads
   - Pass global/local IDs
   - Wait for completion

5. **Handle synchronization**
   - Barrier implementation
   - Thread wake/sleep
   - Deadlock detection

6. **Memory transfers**
   - Host ↔ Device copies
   - Async operations
   - Event tracking

**Estimated Effort:** ~2,000 lines of runtime scheduler code

---

### Implementation Estimate

| Component | Lines of Code | Difficulty | Time |
|-----------|---------------|------------|------|
| Work-group scheduler | ~2,000 | Hard | 3-4 weeks |
| Barrier implementation | ~500 | Very Hard | 2-3 weeks |
| Atomic operations | ~800 | Hard | 2 weeks |
| `__local` memory management | ~1,000 | Hard | 2-3 weeks |
| OpenCL C frontend | ~5,000 | Medium | 6-8 weeks |
| Built-in function library | ~3,000 | Medium | 4-5 weeks |
| Buffer/memory management | ~1,500 | Medium | 2-3 weeks |
| Type system extensions | ~2,000 | Medium | 3-4 weeks |
| IL extensions | ~1,000 | Medium | 2 weeks |
| **TOTAL** | **~16,800** | **7/10** | **3-6 months** |

---

## HIP Feasibility Analysis

### Difficulty Rating: 8-9/10 (Very Challenging)

HIP (Heterogeneous-compute Interface for Portability) is AMD's CUDA-compatible compute API. Implementing HIP has **all the challenges of OpenCL** plus additional complexity:

### Additional Challenges Beyond OpenCL

#### 1. CUDA-like Semantics
```c
// HIP/CUDA terminology
Grid → Blocks → Threads

// vs OpenCL terminology
NDRange → Work-Groups → Work-Items

// More complex launch syntax
hipLaunchKernelGGL(kernel, blocks, threads, sharedMem, stream, args...);
kernel<<<blocks, threads, sharedMem, stream>>>(args...);
```

#### 2. Device Management
```c
hipSetDevice(deviceId);
hipGetDeviceProperties(&props, deviceId);
hipDeviceSynchronize();
hipDeviceReset();
```

#### 3. Stream/Event Model
```c
hipStream_t stream;
hipStreamCreate(&stream);
hipEventCreate(&event);
hipEventRecord(event, stream);
hipEventSynchronize(event);
hipStreamWaitEvent(stream, event, 0);
```

#### 4. Complex Memory Model
```c
// Unified memory
__managed__ float *unified;

// Peer-to-peer access
hipDeviceEnablePeerAccess(peerDevice, 0);

// Memory pools
hipMemPool_t pool;
```

#### 5. Texture and Surface Objects
```c
hipTextureObject_t texObj;
hipSurfaceObject_t surfObj;
// Hardware texture cache utilization
```

#### 6. Warp-level Primitives
```c
__shfl_sync(mask, var, srcLane);
__ballot_sync(mask, predicate);
__any_sync(mask, predicate);
__all_sync(mask, predicate);
```

### Implementation Estimate

| Component | Lines of Code | Difficulty | Time |
|-----------|---------------|------------|------|
| **OpenCL base** | ~16,800 | 7/10 | 3-6 months |
| Device management | ~1,500 | Medium | 2-3 weeks |
| Stream/event system | ~2,500 | Hard | 4-5 weeks |
| Unified memory | ~2,000 | Very Hard | 3-4 weeks |
| Texture objects | ~1,500 | Hard | 2-3 weeks |
| Warp primitives | ~1,000 | Medium | 2 weeks |
| HIP C++ frontend | ~3,000 | Medium | 4-5 weeks |
| **TOTAL** | **~28,300** | **8-9/10** | **6-12 months** |

---

## Recommended Architecture

### Don't Layer Compute on Graphics

**Current Architecture:**
```
    ┌──────────────┐
    │   GLES20     │
    │   (Graphics) │
    └──────┬───────┘
           │
    ┌──────▼────────────────┐
    │  IL + JIT + Interp    │
    └───────────────────────┘
```

**❌ Bad Approach: Force compute onto graphics**
```
    ┌──────────────┐
    │   GLES20     │
    │   (Graphics) │
    └──────┬───────┘
           │
    ┌──────▼────────────────┐
    │  IL + JIT + Interp    │
    └──────┬────────────────┘
           │
    ┌──────▼────────────────┐
    │  OpenCL/HIP Wrapper   │
    │  (Awkward mapping)    │
    └───────────────────────┘
```

**✓ Better Approach: Shared Infrastructure**
```
    ┌──────────┐  ┌──────────┐  ┌──────────┐
    │  GLES20  │  │  OpenCL  │  │   HIP    │
    │(Graphics)│  │ (Compute)│  │ (Compute)│
    └─────┬────┘  └─────┬────┘  └─────┬────┘
          │             │              │
          └─────────────┼──────────────┘
                        ▼
            ┌───────────────────────┐
            │   Common IL Backend   │
            │  • Shader IL (base)   │
            │  • Compute extensions │
            │  • sljit JIT          │
            │  • Interpreter        │
            │  • Math library       │
            └───────────────────────┘
                        │
                ┌───────┴───────┐
                ▼               ▼
            ┌────────┐      ┌────────┐
            │  JIT   │      │ Interp │
            │  Code  │      │ Engine │
            └────────┘      └────────┘
```

### Benefits of Shared Architecture

**1. Code Reuse**
- 70% of compute infrastructure already exists
- IL, JIT, interpreter are architecture-neutral
- Math library, control flow, execution engine all reusable

**2. Maintainability**
- Single execution engine to optimize
- Shared bug fixes across APIs
- Unified testing infrastructure

**3. Performance**
- JIT optimizations benefit all APIs
- Architecture-specific tuning once
- No graphics API overhead for compute

**4. Flexibility**
- Can disable graphics if only compute needed
- Can disable compute if only graphics needed
- Mix graphics and compute in same application

**5. Future-Proof**
- Easy to add new APIs (SYCL, Metal Compute, etc.)
- Clean separation of concerns
- Incremental feature additions

---

## Implementation Path

### Phase 1: Extract Core (2-3 weeks)

**Goal:** Create `libvincent-compute` shared library

**Tasks:**
1. Extract IL definitions
   - `ananke/contrib/libs/gles20/gl/frontend/il.h` → `libvincent-compute/il.h`
   - Opcode definitions
   - Instruction structures
   - Block and program structures

2. Extract JIT compiler
   - `ananke/contrib/libs/gles20/gl/backend/sljit.c` → `libvincent-compute/jit.c`
   - Remove graphics-specific assumptions
   - Generalize register allocation
   - Make texture sampling optional

3. Extract interpreter
   - `ananke/contrib/libs/gles20/gl/backend/interpreter.c` → `libvincent-compute/interpreter.c`
   - Remove varying/attribute references
   - Generalize memory access

4. Create common API
   ```c
   // libvincent-compute/vincent_compute.h

   typedef struct VincentProgram VincentProgram;
   typedef struct VincentExecutable VincentExecutable;

   VincentProgram* vincent_program_create(void);
   void vincent_program_add_instruction(VincentProgram*, Inst*);
   VincentExecutable* vincent_program_compile(VincentProgram*);
   void vincent_program_execute(VincentExecutable*, void* context);
   ```

**Deliverables:**
- `libvincent-compute.so` / `libvincent-compute.a`
- Clean API header
- GLES20 refactored to use shared library

---

### Phase 2: Add Compute Extensions (2-3 months)

**Goal:** Extend IL for compute workloads

**Tasks:**

1. **Type System Extensions** (~3 weeks)
   ```c
   // Add scalar and vector types
   typedef enum {
       TYPE_FLOAT,
       TYPE_INT,
       TYPE_UINT,
       TYPE_DOUBLE,
       TYPE_VEC2,
       TYPE_VEC4,
       TYPE_VEC8,
       // ...
   } VincentType;

   // Add to instruction definitions
   struct Inst {
       VincentType srcType;
       VincentType dstType;
       // ...
   };
   ```

2. **Pointer Operations** (~2 weeks)
   ```c
   // New opcodes
   case OpcodeLoad:     // Load from pointer
   case OpcodeStore:    // Store to pointer
   case OpcodePtrAdd:   // Pointer arithmetic
   ```

3. **Memory Model** (~3 weeks)
   ```c
   typedef enum {
       ADDR_SPACE_PRIVATE,
       ADDR_SPACE_GLOBAL,
       ADDR_SPACE_LOCAL,
       ADDR_SPACE_CONSTANT
   } AddressSpace;

   typedef struct {
       void* base;
       size_t size;
       AddressSpace space;
   } VincentBuffer;
   ```

4. **Synchronization** (~4 weeks)
   ```c
   // Barrier instruction
   case OpcodeBarrier: {
       uint32_t flags = inst->barrier.flags;
       vincent_barrier_wait(context, flags);
       break;
   }

   // Fence instruction
   case OpcodeFence: {
       uint32_t flags = inst->fence.flags;
       vincent_memory_fence(context, flags);
       break;
   }

   // Atomic operations
   case OpcodeAtomicAdd:
   case OpcodeAtomicSub:
   case OpcodeAtomicCmpXchg:
   // ...
   ```

5. **Work-Item Builtins** (~1 week)
   ```c
   case OpcodeGetGlobalId: {
       uint32_t dim = inst->builtin.dimension;
       result = context->globalId[dim];
       break;
   }

   case OpcodeGetLocalId: {
       uint32_t dim = inst->builtin.dimension;
       result = context->localId[dim];
       break;
   }
   // ... similar for other IDs
   ```

6. **Runtime Infrastructure** (~4 weeks)
   ```c
   typedef struct {
       // Work dimensions
       size_t globalSize[3];
       size_t localSize[3];
       size_t numGroups[3];

       // Current work-item
       size_t globalId[3];
       size_t localId[3];
       size_t groupId[3];

       // Memory
       VincentBuffer* globalMem;
       VincentBuffer* localMem;
       VincentBuffer* constantMem;
       void* privateMem;

       // Synchronization
       pthread_barrier_t* barrier;
       pthread_mutex_t* locks;
   } VincentComputeContext;

   // Work-group scheduler
   void vincent_launch_kernel(
       VincentExecutable* kernel,
       size_t globalSize[3],
       size_t localSize[3],
       VincentBuffer** args,
       size_t numArgs
   );
   ```

**Deliverables:**
- Extended IL with compute instructions
- Compute context and runtime
- Work-group scheduler
- Barrier and atomic implementations

---

### Phase 3: OpenCL Frontend (3-4 months)

**Goal:** Implement OpenCL 1.2 API

**Tasks:**

1. **Platform Layer** (~2 weeks)
   ```c
   cl_int clGetPlatformIDs(
       cl_uint num_entries,
       cl_platform_id* platforms,
       cl_uint* num_platforms
   );

   cl_int clGetDeviceIDs(
       cl_platform_id platform,
       cl_device_type device_type,
       cl_uint num_entries,
       cl_device_id* devices,
       cl_uint* num_devices
   );
   ```

2. **Context and Command Queue** (~2 weeks)
   ```c
   cl_context clCreateContext(...);
   cl_command_queue clCreateCommandQueue(...);
   ```

3. **Memory Objects** (~3 weeks)
   ```c
   cl_mem clCreateBuffer(
       cl_context context,
       cl_mem_flags flags,
       size_t size,
       void* host_ptr,
       cl_int* errcode_ret
   );

   cl_int clEnqueueReadBuffer(...);
   cl_int clEnqueueWriteBuffer(...);
   cl_int clEnqueueCopyBuffer(...);
   ```

4. **Program and Kernel Objects** (~4 weeks)
   ```c
   cl_program clCreateProgramWithSource(...);
   cl_int clBuildProgram(...);  // Compile OpenCL C to Vincent IL
   cl_kernel clCreateKernel(...);
   cl_int clSetKernelArg(...);
   ```

5. **OpenCL C Compiler** (~6-8 weeks)
   ```c
   // Parse OpenCL C
   OpenCLAST* parse_opencl_c(const char* source);

   // Translate to Vincent IL
   VincentProgram* ast_to_vincent_il(OpenCLAST* ast);
   ```

   Options:
   - Write custom parser (hard, ~6-8 weeks)
   - Use Clang/LLVM (moderate, ~4-6 weeks)
   - Use libclc (easier, ~2-3 weeks)

6. **Kernel Execution** (~2 weeks)
   ```c
   cl_int clEnqueueNDRangeKernel(
       cl_command_queue command_queue,
       cl_kernel kernel,
       cl_uint work_dim,
       const size_t* global_work_offset,
       const size_t* global_work_size,
       const size_t* local_work_size,
       cl_uint num_events_in_wait_list,
       const cl_event* event_wait_list,
       cl_event* event
   ) {
       // Map to vincent_launch_kernel
       vincent_launch_kernel(
           kernel->executable,
           global_work_size,
           local_work_size,
           kernel->args,
           kernel->numArgs
       );
   }
   ```

7. **Built-in Functions** (~4 weeks)
   ```c
   // Math: sin, cos, exp, log, pow, sqrt, rsqrt, ...
   // Integer: abs, clz, popcount, mul_hi, ...
   // Geometric: dot, cross, length, normalize, ...
   // Common: clamp, mix, step, smoothstep, ...
   // Relational: isequal, isgreater, isless, ...
   // Vector: shuffle, select, ...
   // Sync: barrier, mem_fence, atomic_*, ...
   ```

**Deliverables:**
- OpenCL 1.2 API implementation
- OpenCL C compiler
- Built-in function library
- Conformance test suite results

---

### Phase 4: HIP Frontend (3-4 months)

**Goal:** Implement HIP API (subset compatible with CUDA)

**Tasks:**

1. **Device Management** (~2 weeks)
   ```c
   hipError_t hipGetDeviceCount(int* count);
   hipError_t hipSetDevice(int deviceId);
   hipError_t hipGetDeviceProperties(hipDeviceProp_t* prop, int deviceId);
   hipError_t hipDeviceSynchronize(void);
   ```

2. **Memory Management** (~3 weeks)
   ```c
   hipError_t hipMalloc(void** ptr, size_t size);
   hipError_t hipFree(void* ptr);
   hipError_t hipMemcpy(void* dst, const void* src, size_t size, hipMemcpyKind kind);
   hipError_t hipMemcpyAsync(void* dst, const void* src, size_t size, hipMemcpyKind kind, hipStream_t stream);
   hipError_t hipMemset(void* ptr, int value, size_t size);
   ```

3. **Stream and Event Management** (~3 weeks)
   ```c
   hipError_t hipStreamCreate(hipStream_t* stream);
   hipError_t hipStreamDestroy(hipStream_t stream);
   hipError_t hipStreamSynchronize(hipStream_t stream);
   hipError_t hipEventCreate(hipEvent_t* event);
   hipError_t hipEventRecord(hipEvent_t event, hipStream_t stream);
   hipError_t hipEventSynchronize(hipEvent_t event);
   hipError_t hipEventElapsedTime(float* ms, hipEvent_t start, hipEvent_t stop);
   ```

4. **Kernel Launch** (~2 weeks)
   ```c
   // HIP launch syntax (C++)
   __global__ void kernel(float* out, float* in, int n);

   hipLaunchKernelGGL(
       kernel,
       dim3(blocks),
       dim3(threads),
       0,  // sharedMem
       0,  // stream
       d_out, d_in, n
   );
   ```

5. **HIP C++ Compiler** (~6-8 weeks)
   ```c
   // Parse HIP C++ with __global__, __device__, __host__ attributes
   // Translate to Vincent IL
   // Handle template instantiation
   // Support C++ features used in kernels
   ```

   Options:
   - Extend OpenCL compiler (moderate, ~4-6 weeks)
   - Use hipcc/Clang (easier, ~2-3 weeks)
   - Write custom (hard, ~8-10 weeks)

6. **Texture and Surface Support** (~2 weeks)
   ```c
   hipError_t hipCreateTextureObject(hipTextureObject_t* pTexObject, ...);
   hipError_t hipCreateSurfaceObject(hipSurfaceObject_t* pSurfObject, ...);
   ```

7. **Warp-level Primitives** (~2 weeks)
   ```c
   __device__ int __shfl_sync(unsigned mask, int var, int srcLane, int width);
   __device__ unsigned __ballot_sync(unsigned mask, int predicate);
   __device__ int __any_sync(unsigned mask, int predicate);
   __device__ int __all_sync(unsigned mask, int predicate);
   ```

**Deliverables:**
- HIP API implementation
- HIP C++ compiler
- Stream/event system
- Compatibility with CUDA examples

---

## Practical Recommendations

### Short Term: Quick and Dirty Compute

**If you need simple compute NOW:**

Use shader IL directly for embarrassingly parallel workloads:

```c
// Map compute to graphics API
// Treat work-items as vertices or fragments

// Example: Vector addition
void vector_add_kernel(float* out, float* a, float* b, int n) {
    // Compile to shader IL
    // Execute as "vertex shader" with n vertices
    // Each "vertex" = one work-item
    // Output to "varying" = output buffer
}

// Launch
vincent_launch_as_vertex_shader(kernel, n);
```

**Limitations:**
- ❌ No synchronization
- ❌ No shared memory (`__local`)
- ❌ No atomic operations
- ❌ Graphics API overhead
- ✓ Works for map, filter, reduce (some cases)
- ✓ Quick prototype

**Use Cases:**
- Image processing (per-pixel operations)
- Matrix operations (element-wise)
- Simple transformations
- Embarrassingly parallel algorithms

---

### Long Term: Proper Compute Support

**For production compute capabilities:**

1. **Extract common backend** (2-3 weeks)
   - Create `libvincent-compute` from IL/JIT/interpreter
   - Refactor GLES20 to use shared library
   - Clean API boundaries

2. **Add compute extensions** (2-3 months)
   - Extended type system
   - Pointer operations
   - Memory model (global/local/private)
   - Synchronization (barrier, atomic, fence)
   - Work-item builtins

3. **Build OpenCL** (3-4 months)
   - OpenCL 1.2 API
   - OpenCL C compiler
   - Built-in functions
   - Conformance testing

4. **Optional: Build HIP** (3-4 months)
   - HIP API
   - HIP C++ compiler
   - Stream/event model
   - CUDA compatibility

---

### Key Insight

**The value isn't "OpenCL on GLES20"**

**The value is "OpenCL and GLES20 sharing a portable execution engine"**

The current IL + JIT + interpreter infrastructure we built is **70% of what you need** for a compute framework. The remaining 30% (synchronization, memory model, scheduler) is **compute-specific** and shouldn't pollute the graphics API.

By extracting the common backend, you get:
- ✓ Clean separation of graphics and compute
- ✓ Maximum code reuse
- ✓ No graphics overhead for compute
- ✓ No compute overhead for graphics
- ✓ Future-proof architecture
- ✓ Easy to add new APIs (Vulkan Compute, Metal Compute, SYCL)

---

## Conclusion

**OpenCL feasibility:** 7/10 - Doable in 3-6 months
**HIP feasibility:** 8-9/10 - Challenging, 6-12 months
**Recommended approach:** Extract shared backend, build compute as peer to graphics

The foundation exists. The challenge is compute-specific features (synchronization, memory model, scheduler), not the execution engine itself.

---

## References

- Vincent GLES20 implementation: `ananke/contrib/libs/gles20/`
- Shader IL: `ananke/contrib/libs/gles20/gl/frontend/il.h`
- JIT compiler: `ananke/contrib/libs/gles20/gl/backend/sljit.c` (2,796 lines)
- Interpreter: `ananke/contrib/libs/gles20/gl/backend/interpreter.c` (1,108 lines)
- sljit library: `ananke/contrib/libs/sljit/`

**Related commits:**
- `c1da21a` - Implement FULL control flow in shader IL interpreter
- `0b44c01` - Complete 100% opcode coverage for fragment shader interpreter
- `b43ed89` - Add ALL remaining opcodes to vertex shader interpreter

**OpenCL Specification:** https://registry.khronos.org/OpenCL/
**HIP Documentation:** https://rocm.docs.amd.com/projects/HIP/
**SYCL Specification:** https://registry.khronos.org/SYCL/

---

**Document Version:** 1.0
**Last Updated:** 2025-11-01
**Author:** Claude (Anthropic)
**Status:** Complete
