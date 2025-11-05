# VINIL JIT Backend Design Document

## Executive Summary

This document outlines the design and implementation strategy for the VINIL JIT (Just-In-Time) compiler backend. The JIT backend will compile VINIL IL instructions to native machine code at runtime for significantly improved performance compared to the interpreter.

## Current Status

- ✅ **Interpreter Backend**: Fully implemented with 50+ opcodes
- ✅ **Binary Serialization**: Complete save/load functionality
- ✅ **COM Architecture**: Clean interfaces with COBJMACROS support
- 🔄 **JIT Backend**: Planned (this document)
- 📋 **AOT Backend**: Future work

## Architecture Overview

### Backend Selection Flow

```
IVinilContext_Execute(program, backend, ...)
  └─> switch (backend)
       ├─> VinilBackendInterpreter → InterpreterExecute()
       ├─> VinilBackendJit        → JitExecute()      [NEW]
       └─> VinilBackendAot        → AotExecute()      [FUTURE]
```

### JIT Compilation Pipeline

```
VINIL IL Program
  └─> JIT Compiler
       ├─> Analyze Instructions
       ├─> Allocate Registers
       ├─> Generate Native Code
       └─> Create Executable Function
            └─> Execute Directly on CPU
```

## Implementation Options

### Option 1: SLJIT Integration (Recommended)

**Pros:**
- Portable across x86, x86-64, ARM, ARM64, MIPS, PowerPC, RISC-V
- Battle-tested (used in LuaJIT, PCRE2, Zend PHP JIT)
- MIT license (compatible with CDDL)
- Lightweight (~30KB compiled)
- Already used in GLES20 shader JIT

**Cons:**
- External dependency (but already in project)
- Learning curve for SLJIT API
- ~30KB additional library size

**Integration Path:**
1. Use existing SLJIT from `/ananke/contrib/libs/sljit`
2. Create `vinil/src/jit_sljit.c` using GLES20's implementation as reference
3. Implement VINIL IL → SLJIT translation

### Option 2: Custom Lightweight JIT

**Pros:**
- No external dependencies
- Complete control over code generation
- Minimal size (~5-10KB for x86-64 only)
- Educational value

**Cons:**
- Only supports one architecture initially (x86-64)
- More development time
- Less battle-tested
- Manual register allocation and calling conventions
- Need to handle executable memory permissions

### Option 3: DynASM (from LuaJIT)

**Pros:**
- Very lightweight
- Excellent code generation quality
- Used in LuaJIT (proven performance)

**Cons:**
- Preprocessor-based (complicates build)
- Architecture-specific implementations
- More complex integration

### Decision: Use SLJIT

**Rationale:**
- Already in project (GLES20 uses it)
- Cross-platform out of the box
- Clean C API
- Proven track record
- Reasonable size overhead
- Faster time to implementation

## SLJIT Integration Architecture

### Core Components

#### 1. JIT Compilation Context

```c
typedef struct {
    struct sljit_compiler *Compiler;
    IVinilMemoryPool *MemoryPool;
    IVinilProgram *Program;

    /* Register allocation */
    INT32 NextFloatReg;
    INT32 NextIntReg;

    /* Code generation state */
    VOID *NativeCode;
    UINTN CodeSize;

    /* Variable to register mapping */
    UINT32 VarToReg[256];
} VINIL_JIT_CONTEXT;
```

#### 2. Opcode Translation Table

Each VINIL opcode maps to a code generation function:

```c
typedef HRESULT (*JIT_OPCODE_GENERATOR)(
    VINIL_JIT_CONTEXT *Context,
    VINIL_INSTRUCTION_NODE *Instruction
);

static JIT_OPCODE_GENERATOR gJitGenerators[VINIL_OP_COUNT] = {
    JitGenMov,    // VINIL_OP_MOV
    JitGenAdd,    // VINIL_OP_ADD
    JitGenSub,    // VINIL_OP_SUB
    JitGenMul,    // VINIL_OP_MUL
    // ... etc
};
```

#### 3. Register Allocation Strategy

**SLJIT Register Types:**
- `SLJIT_R0-R9`: Scratch registers (caller-saved)
- `SLJIT_S0-S9`: Saved registers (callee-saved)
- `SLJIT_FR0-FR6`: Floating-point registers

**VINIL Mapping:**
- `SLJIT_S0`: Execution context pointer (always)
- `SLJIT_FR0-FR3`: Vector registers (float4 as 4×float)
- `SLJIT_R0-R2`: Integer temp registers

#### 4. Calling Convention

```c
typedef void (*VINIL_JIT_FUNCTION)(VINIL_EXECUTION_STATE *State);
```

**Function prologue:**
1. Save callee-saved registers
2. Set up stack frame
3. Load context pointer to `SLJIT_S0`

**Function epilogue:**
1. Restore callee-saved registers
2. Return

## Implementation Phases

### Phase 1: Infrastructure (Week 1)

**Files:**
- `src/jit_sljit.c` - JIT compiler implementation
- `src/vinil_internal.h` - Add JIT structures

**Tasks:**
- Set up SLJIT integration
- Create JIT context management
- Implement prologue/epilogue generation
- Add to Context_Execute backend selection
- Basic smoke test (empty program)

**Deliverables:**
- JIT backend compiles and links
- Can generate/execute empty function
- Tests pass

### Phase 2: Core Arithmetic (Week 1)

**Opcodes:**
- MOV, ADD, SUB, MUL, MAD
- DP3, DP4

**Implementation:**
- Variable to register mapping
- Basic float operations
- Vector operations (component-wise)
- Memory load/store for registers

**Test:**
- Simple arithmetic programs
- Vector dot products
- Compare JIT vs interpreter results

### Phase 3: Extended Operations (Week 2)

**Opcodes:**
- DIV, MIN, MAX, ABS, NEG
- RSQ, RCP (reciprocal, reciprocal sqrt)
- FRC, FLR, CEIL (fractional, floor, ceiling)

**Implementation:**
- More complex ALU operations
- libm calls for transcendentals

**Test:**
- Mathematical operations
- Edge cases (NaN, Inf, denormals)

### Phase 4: Control Flow (Week 2)

**Opcodes:**
- IF, ELSE, ENDIF
- LOOP, ENDLOOP, BREAK
- RET

**Implementation:**
- Label management
- Jump generation
- Control flow stack

**Test:**
- Conditional branches
- Loops
- Nested control structures

### Phase 5: Optimization & Tuning (Week 3)

**Features:**
- Register pressure reduction
- Constant folding
- Dead code elimination
- Benchmark suite

**Test:**
- Performance comparison vs interpreter
- Stress tests
- Real-world shader programs

## Code Size Estimates

```
Component               Size
--------------------------------
SLJIT library          ~30 KB
JIT compiler core      ~15 KB
Opcode generators      ~20 KB
Register allocator     ~5 KB
Control flow           ~8 KB
--------------------------------
Total                  ~78 KB
```

With VINIL interpreter at 86KB, total JIT-enabled library: ~164KB

## Performance Expectations

Based on similar JIT compilers (LuaJIT, V8):

**Expected Speedup:**
- Simple arithmetic: 10-20× faster than interpreter
- Vector operations: 15-30× faster
- Complex shaders: 8-15× faster (due to overhead)

**Memory:**
- ~500 bytes per instruction for JIT code
- Typical shader (100 instructions): ~50KB compiled code

## Testing Strategy

### Unit Tests
- Per-opcode generation tests
- Register allocation tests
- Control flow tests

### Integration Tests
- Compare JIT vs interpreter results (exact match required)
- Test all 50+ opcodes through JIT path
- Edge case handling

### Performance Tests
- Microbenchmarks for each opcode
- Shader program benchmarks
- Memory usage profiling

### Stress Tests
- Long programs (1000+ instructions)
- Deep nesting
- All registers used
- Repeated compilation

## File Structure

```
vinil/
├── src/
│   ├── jit_sljit.c          # NEW: JIT backend implementation
│   ├── interpreter.c         # Existing interpreter
│   ├── vinil.c              # Backend capability reporting (update)
│   └── vinil_internal.h     # Add JIT structures
├── include/vinil/
│   └── vinil.h              # No changes needed (backend enum exists)
└── docs/
    └── JIT_BACKEND_DESIGN.md # This document
```

## Security Considerations

1. **Executable Memory**: Use platform-specific APIs
   - Linux: `mmap` with `PROT_EXEC`
   - Windows: `VirtualAlloc` with `PAGE_EXECUTE_READWRITE`
   - SLJIT handles this portably

2. **Code Validation**: Verify IL before JIT compilation

3. **Buffer Overflows**: SLJIT prevents typical code generation errors

4. **Side Channels**: Not a concern for graphics workloads

## Compatibility

**Platforms:**
- ✅ x86-64 (Linux, Windows, macOS)
- ✅ x86 (32-bit)
- ✅ ARM64 (Linux, iOS, Android)
- ✅ ARM (32-bit)
- ✅ MIPS, PowerPC, RISC-V (via SLJIT)

**Fallback:**
- If JIT not available: automatically use interpreter
- No API changes needed

## Future Enhancements

1. **Tier-2 Optimization**
   - Compile hot loops with aggressive optimization
   - Profile-guided optimization

2. **AOT Compilation**
   - Compile to object files
   - Link with application
   - Zero runtime compilation overhead

3. **Multi-Threading**
   - Parallel JIT compilation
   - Thread-safe code cache

4. **SIMD Vectorization**
   - Use SSE/AVX on x86
   - Use NEON on ARM
   - Process multiple work-items in parallel

## References

- SLJIT Documentation: http://sljit.sourceforge.net/
- GLES20 Shader JIT: `ananke/contrib/libs/gles20/gl/backend/sljit.c`
- LuaJIT: https://luajit.org/
- DynASM: https://luajit.org/dynasm.html

## Conclusion

The SLJIT-based JIT backend will provide:
- **10-20× performance improvement** over interpreter
- **Cross-platform support** for all major architectures
- **Reasonable size increase** (~78KB additional code)
- **Production quality** using battle-tested library
- **Fast implementation** (~2-3 weeks for full feature parity)

This positions VINIL as a high-performance IL for both graphics and compute workloads, suitable for embedded systems, mobile devices, and desktop platforms.
