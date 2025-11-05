# VINIL Implementation Guide

## Current Status

**Date**: 2025-11-02
**Version**: 0.1.0
**Completion**: ~5%

### What's Complete ✓

1. **Project Structure**
   - Directory layout created
   - Build system (Makefile)
   - Public API headers
   - Documentation framework

2. **Memory Management** (100% Complete)
   - Pool-based allocator
   - Page management
   - Error handling via setjmp/longjmp
   - ~220 lines of code
   - Files:
     - `include/vinil/memory.h`
     - `src/memory.c`

3. **Core API** (Stub Complete)
   - Context management
   - Program lifecycle
   - Execution interface
   - Error handling
   - Files:
     - `include/vinil/vinil.h`
     - `src/vinil.c`

4. **Build System**
   - Makefile with shared/static library targets
   - Example program
   - Successfully builds and runs

### What's Next ⏳

The following components need to be extracted from GLES20 and adapted for VINIL:

## Phase 1: Core IL Infrastructure (Weeks 1-2)

### 1. Type System (~2,000 lines)
**Source**: `ananke/contrib/libs/gles20/gl/frontend/types.{h,c}`

**Tasks**:
- [ ] Extract type definitions:
  - Scalar types: bool, int, float
  - Vector types: vec2, vec3, vec4
  - Matrix types: mat2, mat3, mat4
  - Array and struct types
- [ ] Remove GLES-specific types (samplers can be optional)
- [ ] Create `include/vinil/types.h`
- [ ] Create `src/types.c`

**Key Structures**:
```c
typedef enum vinil_type_value {
    VINIL_TYPE_VOID,
    VINIL_TYPE_BOOL,
    VINIL_TYPE_INT,
    VINIL_TYPE_FLOAT,
    VINIL_TYPE_VEC2,
    VINIL_TYPE_VEC3,
    VINIL_TYPE_VEC4,
    // ... etc
} vinil_type_value;

typedef enum vinil_precision {
    VINIL_PRECISION_LOW,
    VINIL_PRECISION_MEDIUM,
    VINIL_PRECISION_HIGH
} vinil_precision;
```

### 2. IL Definitions (~27,000 lines - core ~5,000 needed)
**Source**: `ananke/contrib/libs/gles20/gl/frontend/il.{h,c}`

**Tasks**:
- [ ] Extract opcode definitions (50+ opcodes)
- [ ] Extract instruction structures:
  - `InstBase`, `InstUnary`, `InstBinary`, `InstTernary`
  - `InstBranch`, `InstCond`, `InstTex`
- [ ] Extract block and control flow structures
- [ ] Extract variable/register definitions
- [ ] Extract shader program structure
- [ ] Create `include/vinil/il.h`
- [ ] Create `src/il.c`

**Key Opcodes to Extract**:
```
Arithmetic:   ADD, SUB, MUL, MAD, MIN, MAX, ABS, NEG
Dot Products: DP2, DP3, DP4, DPH
Vector Ops:   XPD, LRP
Math:         SIN, COS, EXP, LOG, POW, RCP, RSQ
Control Flow: IF/ELSE/ENDIF, LOOP/ENDLOOP, REP/ENDREP, BRK
Branching:    BRA, CAL/RET, SCC
Comparison:   SGE, SLT, SEQ, SNE
```

**Key Structures**:
```c
typedef enum vinil_opcode {
    VINIL_OP_ADD,
    VINIL_OP_SUB,
    VINIL_OP_MUL,
    // ... 50+ opcodes
} vinil_opcode;

typedef struct vinil_instruction {
    vinil_opcode opcode;
    // operands, masks, flags
} vinil_instruction;

typedef struct vinil_block {
    vinil_instruction* first;
    vinil_instruction* last;
    struct vinil_block* next;
} vinil_block;

typedef struct vinil_shader_program {
    vinil_memory_pool* memory;
    vinil_block* blocks;
    // variables, labels, etc.
} vinil_shader_program;
```

### 3. Linker (~2,000 lines)
**Source**: `ananke/contrib/libs/gles20/gl/frontend/linker.{h,c}`

**Tasks**:
- [ ] Extract binary format definitions
- [ ] Extract segment management (code, data, bss)
- [ ] Extract symbol resolution
- [ ] Remove graphics-specific metadata (or make optional)
- [ ] Create `src/linker.h`
- [ ] Create `src/linker.c`

**Key Structures**:
```c
typedef struct vinil_segment {
    void* base;
    vinil_size size;
} vinil_segment;

typedef struct vinil_binary {
    vinil_segment code;
    vinil_segment data;
    vinil_size bss_size;
} vinil_binary;
```

## Phase 2: Execution Backends (Weeks 2-3)

### 4. Interpreter (~1,100 lines)
**Source**: `ananke/contrib/libs/gles20/gl/backend/interpreter.{h,c}`

**Tasks**:
- [ ] Extract instruction dispatch loop
- [ ] Extract control flow stack
- [ ] Extract call stack (32 levels)
- [ ] Extract register/variable access
- [ ] Remove graphics context dependencies
- [ ] Create generic execution context
- [ ] Create `src/interpreter.h`
- [ ] Create `src/interpreter.c`

**Key Structures**:
```c
typedef enum vinil_cf_type {
    VINIL_CF_IF,
    VINIL_CF_LOOP,
    VINIL_CF_REP
} vinil_cf_type;

typedef struct vinil_cf_frame {
    vinil_cf_type type;
    vinil_size start_ip;
    vinil_size end_ip;
    vinil_size else_ip;
    vinil_int32 counter;
} vinil_cf_frame;

typedef struct vinil_exec_context {
    vinil_binary* binary;
    void* registers;      /* temp storage */
    void* uniforms;       /* parameters */
    vinil_cf_frame cf_stack[32];
    vinil_size cf_depth;
    void* call_stack[32];
    vinil_size call_depth;
} vinil_exec_context;
```

### 5. JIT Compiler (~2,800 lines)
**Source**: `ananke/contrib/libs/gles20/gl/backend/sljit.c`

**Tasks**:
- [ ] Extract sljit integration
- [ ] Extract register allocation
- [ ] Extract opcode translation
- [ ] Extract control flow compilation
- [ ] Remove graphics context dependencies
- [ ] Create `src/jit.h`
- [ ] Create `src/jit.c`

**Dependencies**:
- sljit library (already present at `ananke/contrib/libs/sljit/`)

## Phase 3: Integration (Week 3)

### 6. Wire Everything Together

**Tasks**:
- [ ] Implement `vinil_program_compile()`:
  - Parse/validate IL
  - Run linker
  - Choose JIT or interpreter
  - Produce executable
- [ ] Implement `vinil_execute()`:
  - Setup execution context
  - Call JIT code or interpreter
  - Handle errors
- [ ] Create comprehensive tests
- [ ] Update documentation

## Phase 4: GLES20 Refactoring (Week 4)

### 7. Update GLES20 to Use VINIL

**Tasks**:
- [ ] Replace GLES20 IL with vinil IL
- [ ] Replace GLES20 memory with vinil memory
- [ ] Replace GLES20 JIT with vinil JIT
- [ ] Replace GLES20 interpreter with vinil interpreter
- [ ] Add graphics-specific wrapper layer
- [ ] Update GLES20 build system
- [ ] Test all existing shaders

**Migration Strategy**:
```c
// Old GLES20 code:
ShaderProgram* prog = GlesCreateShaderProgram(pool);

// New code using VINIL:
vinil_program* prog = vinil_program_create(ctx);
// Add GLES20-specific wrapper for texture ops, varying interpolation, etc.
```

## Phase 5: Compute Extensions (Months 2-3)

### 8. Add Compute-Specific Features

**New Opcodes to Add**:
```c
// Work-item builtins
VINIL_OP_GET_GLOBAL_ID,
VINIL_OP_GET_LOCAL_ID,
VINIL_OP_GET_GROUP_ID,
VINIL_OP_GET_GLOBAL_SIZE,
VINIL_OP_GET_LOCAL_SIZE,

// Memory operations
VINIL_OP_LOAD_GLOBAL,
VINIL_OP_STORE_GLOBAL,
VINIL_OP_LOAD_LOCAL,
VINIL_OP_STORE_LOCAL,

// Synchronization
VINIL_OP_BARRIER,
VINIL_OP_MEM_FENCE,

// Atomics
VINIL_OP_ATOMIC_ADD,
VINIL_OP_ATOMIC_SUB,
VINIL_OP_ATOMIC_XCHG,
VINIL_OP_ATOMIC_CMPXCHG,
VINIL_OP_ATOMIC_MIN,
VINIL_OP_ATOMIC_MAX,
```

**Work-Group Scheduler**:
```c
typedef struct vinil_workgroup_context {
    vinil_size global_id[3];
    vinil_size local_id[3];
    vinil_size group_id[3];
    vinil_size global_size[3];
    vinil_size local_size[3];

    void* global_mem;
    void* local_mem;
    void* private_mem;

    pthread_barrier_t* barrier;
} vinil_workgroup_context;

vinil_error vinil_launch_kernel(
    vinil_context* ctx,
    vinil_executable* kernel,
    vinil_size global_size[3],
    vinil_size local_size[3],
    void** args,
    vinil_size num_args
);
```

## Testing Strategy

### Unit Tests
1. Memory pool allocation/deallocation
2. IL instruction creation
3. Type system validation
4. Linker segment creation
5. Interpreter execution (per opcode)
6. JIT compilation (per opcode)

### Integration Tests
1. Simple programs (add, multiply)
2. Control flow (if/else, loops)
3. Function calls (CAL/RET)
4. Complex expressions
5. Existing GLES20 shaders

### Conformance Tests
1. GLES20 conformance suite
2. OpenCL conformance tests (future)

## File Structure

```
ananke/contrib/libs/vinil/
├── include/vinil/
│   ├── vinil.h          ✓ Complete
│   ├── memory.h         ✓ Complete
│   ├── types.h          ⏳ Next
│   └── il.h             ⏳ Next
├── src/
│   ├── vinil.c          ✓ Stub complete
│   ├── memory.c         ✓ Complete
│   ├── types.c          ⏳ Next
│   ├── il.c             ⏳ Next
│   ├── linker.h         ⏳ Next
│   ├── linker.c         ⏳ Next
│   ├── interpreter.h    ⏳ Next
│   ├── interpreter.c    ⏳ Next
│   ├── jit.h            ⏳ Next
│   └── jit.c            ⏳ Next
├── Makefile             ✓ Complete
├── README.md            ✓ Complete
├── IMPLEMENTATION.md    ✓ This file
└── example.c            ✓ Complete
```

## Dependencies

### Required for Core Functionality
- stdlib (malloc, free)
- string (memcpy, memset)
- math (sin, cos, exp, log, pow, sqrt)
- setjmp (longjmp for error handling)

### Required for JIT
- sljit library (already present)

### Required for Compute Extensions (Future)
- pthreads (work-group scheduling, barriers)

## Build Instructions

```bash
# Build library
cd ananke/contrib/libs/vinil
make clean
make

# Build and run example
make example
LD_LIBRARY_PATH=build/lib ./example

# Build with debugging
make CFLAGS="-Wall -Wextra -g -fPIC -Iinclude"
```

## Next Immediate Steps

1. **Extract types.h/types.c** (~1-2 days)
   - Copy from GLES20
   - Remove GLES-specific dependencies
   - Adapt to vinil naming conventions

2. **Extract core IL structures** (~2-3 days)
   - Focus on instruction and block structures
   - Defer full opcode implementations
   - Get basic structure in place

3. **Extract linker** (~2-3 days)
   - Binary format
   - Segment management
   - Basic linking

4. **Extract interpreter** (~3-4 days)
   - Core dispatch loop
   - Control flow
   - Register access
   - Test with simple programs

5. **Extract JIT** (~3-4 days)
   - sljit integration
   - Basic code generation
   - Test with simple programs

## Estimated Timeline

- **Phase 1** (Core IL): 2 weeks
- **Phase 2** (Execution): 2 weeks
- **Phase 3** (Integration): 1 week
- **Phase 4** (GLES20 Migration): 1 week
- **Phase 5** (Compute Extensions): 2-3 months

**Total for basic functionality**: ~6 weeks
**Total for OpenCL support**: ~4-5 months

## References

- Original GLES20: `ananke/contrib/libs/gles20/gl/`
- Feasibility Study: `docs/opencl-hip-feasibility.md`
- sljit: `ananke/contrib/libs/sljit/`
