# VINIL Opcode Implementation Status

This document tracks the implementation status of each opcode in both the JIT compiler and interpreter backends. **Both backends must implement every opcode** to ensure correctness and feature parity.

## Legend

- ✓ = Implemented and tested
- 🔄 = In progress
- ⏳ = Planned
- ❌ = Not applicable for this backend
- 🔶 = Partially implemented

## Implementation Matrix

| Opcode | Category | JIT Status | Interp Status | Graphics | Compute | Priority | Notes |
|--------|----------|:----------:|:-------------:|:--------:|:-------:|:--------:|-------|
| **ARITHMETIC OPERATIONS** |
| `ABS` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Absolute value |
| `ADD` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Vector/scalar add |
| `SUB` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Vector/scalar subtract |
| `MUL` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Vector/scalar multiply |
| `DIV` | Arithmetic | ⏳ | ⏳ | - | ✓ | HIGH | Division (compute) |
| `MAD` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Multiply-add (fused) |
| `MIN` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Component-wise minimum |
| `MAX` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Component-wise maximum |
| `NEG` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | HIGH | Negate |
| `FRC` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Fractional part |
| `FLR` | Arithmetic | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Floor |
| `MOD` | Arithmetic | ⏳ | ⏳ | - | ✓ | MEDIUM | Modulo (compute) |
| **VECTOR OPERATIONS** |
| `DP2` | Vector | ⏳ | ⏳ | ✓ | ✓ | HIGH | 2-component dot product |
| `DP3` | Vector | ⏳ | ⏳ | ✓ | ✓ | HIGH | 3-component dot product |
| `DP4` | Vector | ⏳ | ⏳ | ✓ | ✓ | HIGH | 4-component dot product |
| `DPH` | Vector | ⏳ | ⏳ | ✓ | - | MEDIUM | Homogeneous dot product |
| `XPD` | Vector | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Cross product (3D) |
| `LRP` | Vector | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Linear interpolation |
| `DST` | Vector | ⏳ | ⏳ | ✓ | - | LOW | Distance vector |
| `MOV` | Vector | ⏳ | ⏳ | ✓ | ✓ | HIGH | Move/copy |
| `SWZ` | Vector | ⏳ | ⏳ | ✓ | ✓ | HIGH | Swizzle |
| **TRANSCENDENTAL MATH** |
| `SIN` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Sine |
| `COS` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Cosine |
| `TAN` | Transcendental | ⏳ | ⏳ | - | ✓ | MEDIUM | Tangent |
| `ASIN` | Transcendental | ⏳ | ⏳ | - | ✓ | MEDIUM | Arc sine |
| `ACOS` | Transcendental | ⏳ | ⏳ | - | ✓ | MEDIUM | Arc cosine |
| `ATAN` | Transcendental | ⏳ | ⏳ | - | ✓ | MEDIUM | Arc tangent |
| `ATAN2` | Transcendental | ⏳ | ⏳ | - | ✓ | MEDIUM | Arc tangent (2-arg) |
| `SCS` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Simultaneous sin/cos |
| `EXP` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Exponential (base e) |
| `EX2` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Exponential (base 2) |
| `LOG` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Logarithm (base e) |
| `LG2` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Logarithm (base 2) |
| `POW` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Power |
| `RCP` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Reciprocal |
| `RSQ` | Transcendental | ⏳ | ⏳ | ✓ | ✓ | HIGH | Reciprocal square root |
| `SQRT` | Transcendental | ⏳ | ⏳ | - | ✓ | HIGH | Square root |
| **COMPARISON OPERATIONS** |
| `SEQ` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on equal |
| `SNE` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on not equal |
| `SLT` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on less than |
| `SLE` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on less or equal |
| `SGT` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on greater than |
| `SGE` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set on greater or equal |
| `CMP` | Comparison | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Compare select |
| `SELECT` | Comparison | ⏳ | ⏳ | - | ✓ | MEDIUM | Conditional select |
| `SSG` | Comparison | ⏳ | ⏳ | ✓ | ✓ | LOW | Sign |
| **CONTROL FLOW** |
| `IF` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Conditional begin |
| `ELSE` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Conditional else |
| `ENDIF` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Conditional end |
| `LOOP` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Infinite loop begin |
| `ENDLOOP` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Loop end |
| `REP` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Counted loop begin |
| `ENDREP` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Counted loop end |
| `BRK` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Break from loop |
| `CONT` | Control Flow | ⏳ | ⏳ | - | ✓ | HIGH | Continue loop |
| `BRA` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Conditional branch |
| `CAL` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Call subroutine |
| `RET` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | HIGH | Return |
| `SCC` | Control Flow | ⏳ | ⏳ | ✓ | ✓ | MEDIUM | Set condition code |
| `KIL` | Control Flow | ⏳ | ❌ | ✓ | - | MEDIUM | Kill fragment |
| **TEXTURE OPERATIONS** |
| `TEX` | Texture | ⏳ | ⏳ | ✓ | - | HIGH | Texture sample |
| `TXB` | Texture | ⏳ | ⏳ | ✓ | - | MEDIUM | Texture with bias |
| `TXL` | Texture | ⏳ | ⏳ | ✓ | - | MEDIUM | Texture with LOD |
| `TXP` | Texture | ⏳ | ⏳ | ✓ | - | MEDIUM | Texture with projection |
| **MEMORY OPERATIONS (Compute)** |
| `LOAD_GLOBAL` | Memory | ⏳ | ⏳ | - | ✓ | HIGH | Load from global memory |
| `STORE_GLOBAL` | Memory | ⏳ | ⏳ | - | ✓ | HIGH | Store to global memory |
| `LOAD_LOCAL` | Memory | ⏳ | ⏳ | - | ✓ | HIGH | Load from local memory |
| `STORE_LOCAL` | Memory | ⏳ | ⏳ | - | ✓ | HIGH | Store to local memory |
| `LOAD_PRIVATE` | Memory | ⏳ | ⏳ | - | ✓ | MEDIUM | Load from private memory |
| `STORE_PRIVATE` | Memory | ⏳ | ⏳ | - | ✓ | MEDIUM | Store to private memory |
| `LOAD_CONSTANT` | Memory | ⏳ | ⏳ | - | ✓ | HIGH | Load from constant memory |
| `VLOAD` | Memory | ⏳ | ⏳ | - | ✓ | MEDIUM | Vector load |
| `VSTORE` | Memory | ⏳ | ⏳ | - | ✓ | MEDIUM | Vector store |
| **WORK-ITEM BUILTINS (Compute)** |
| `GET_GLOBAL_ID` | Work-Item | ⏳ | ⏳ | - | ✓ | HIGH | Get global work-item ID |
| `GET_LOCAL_ID` | Work-Item | ⏳ | ⏳ | - | ✓ | HIGH | Get local work-item ID |
| `GET_GROUP_ID` | Work-Item | ⏳ | ⏳ | - | ✓ | HIGH | Get work-group ID |
| `GET_GLOBAL_SIZE` | Work-Item | ⏳ | ⏳ | - | ✓ | HIGH | Get global size |
| `GET_LOCAL_SIZE` | Work-Item | ⏳ | ⏳ | - | ✓ | HIGH | Get local size |
| `GET_NUM_GROUPS` | Work-Item | ⏳ | ⏳ | - | ✓ | MEDIUM | Get number of groups |
| `GET_WORK_DIM` | Work-Item | ⏳ | ⏳ | - | ✓ | MEDIUM | Get work dimensions |
| `GET_GLOBAL_OFFSET` | Work-Item | ⏳ | ⏳ | - | ✓ | LOW | Get global offset |
| **SYNCHRONIZATION (Compute)** |
| `BARRIER` | Sync | ⏳ | ⏳ | - | ✓ | HIGH | Work-group barrier |
| `MEM_FENCE` | Sync | ⏳ | ⏳ | - | ✓ | HIGH | Memory fence |
| `READ_MEM_FENCE` | Sync | ⏳ | ⏳ | - | ✓ | MEDIUM | Read memory fence |
| `WRITE_MEM_FENCE` | Sync | ⏳ | ⏳ | - | ✓ | MEDIUM | Write memory fence |
| **ATOMIC OPERATIONS (Compute)** |
| `ATOMIC_ADD` | Atomic | ⏳ | ⏳ | - | ✓ | HIGH | Atomic add |
| `ATOMIC_SUB` | Atomic | ⏳ | ⏳ | - | ✓ | HIGH | Atomic subtract |
| `ATOMIC_XCHG` | Atomic | ⏳ | ⏳ | - | ✓ | HIGH | Atomic exchange |
| `ATOMIC_CMPXCHG` | Atomic | ⏳ | ⏳ | - | ✓ | HIGH | Atomic compare-exchange |
| `ATOMIC_INC` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic increment |
| `ATOMIC_DEC` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic decrement |
| `ATOMIC_MIN` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic minimum |
| `ATOMIC_MAX` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic maximum |
| `ATOMIC_AND` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic bitwise AND |
| `ATOMIC_OR` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic bitwise OR |
| `ATOMIC_XOR` | Atomic | ⏳ | ⏳ | - | ✓ | MEDIUM | Atomic bitwise XOR |
| **ADDRESS REGISTER (Graphics)** |
| `ARL` | Address | ⏳ | ⏳ | ✓ | - | MEDIUM | Address register load |

## Implementation Phases

### Phase 1: Graphics Core (Week 1-2)
**Goal**: Get basic graphics working
- Arithmetic: ADD, SUB, MUL, MAD, MIN, MAX, ABS, NEG, MOV
- Vector: DP3, DP4
- Control Flow: IF/ELSE/ENDIF, CAL/RET
- Math: SIN, COS, EXP, LOG, POW, RCP, RSQ

### Phase 2: Full Graphics (Week 3)
**Goal**: Complete GLES20 support
- Remaining arithmetic: FRC, FLR
- Remaining vector: DP2, DPH, XPD, LRP, DST, SWZ
- Remaining math: SCS, EX2, LG2
- Remaining control flow: LOOP/ENDLOOP, REP/ENDREP, BRK, BRA, SCC, KIL
- All comparisons: SEQ-SGE, CMP, SSG
- Textures: TEX, TXB, TXL, TXP
- Address: ARL

### Phase 3: Compute Basics (Week 4-5)
**Goal**: Basic compute operations
- Extended arithmetic: DIV, MOD
- Extended math: TAN, ASIN, ACOS, ATAN, ATAN2, SQRT
- Memory: LOAD/STORE_GLOBAL, LOAD_CONSTANT
- Work-item: GET_GLOBAL_ID, GET_LOCAL_ID, GET_GLOBAL_SIZE, GET_LOCAL_SIZE
- Control: CONT

### Phase 4: Advanced Compute (Week 6-8)
**Goal**: Full OpenCL support
- Memory: LOAD/STORE_LOCAL, LOAD/STORE_PRIVATE, VLOAD, VSTORE
- Work-item: GET_GROUP_ID, GET_NUM_GROUPS, GET_WORK_DIM, GET_GLOBAL_OFFSET
- Sync: BARRIER, MEM_FENCE, READ/WRITE_MEM_FENCE
- Atomics: All ATOMIC_* operations
- Comparison: SELECT

## Development Workflow

When implementing a new opcode:

1. **Add to IL header** (`include/vinil/il.h`)
   - Add opcode to enum
   - Update opcode info table

2. **Implement in Interpreter** (`src/interpreter.c`)
   - Add case to dispatch switch
   - Implement semantics in C
   - Add unit test
   - Mark as implemented in this document

3. **Implement in JIT** (`src/jit.c`)
   - Add case to JIT compiler
   - Generate native code using sljit
   - Add unit test
   - Mark as implemented in this document

4. **Verify Parity**
   - Run same test on both backends
   - Verify identical results
   - Update this document

## Testing Requirements

Each opcode must have:
- ✓ Unit test (isolated opcode testing)
- ✓ Integration test (in real program)
- ✓ Performance benchmark
- ✓ Both JIT and Interpreter tested

## Statistics

- **Total Opcodes**: 110
- **Graphics Opcodes**: 62
- **Compute Opcodes**: 48
- **Shared Opcodes**: 45
- **JIT Implemented**: 0 (0%)
- **Interpreter Implemented**: 0 (0%)
- **Both Implemented**: 0 (0%)

## Notes

- Graphics-only opcodes (KIL, texture ops) don't need compute support
- Compute-only opcodes don't need graphics support
- All shared opcodes must work in both contexts
- BARRIER/atomics require runtime support beyond just IL
- Texture ops require sampler hardware abstraction

---

**Last Updated**: 2025-11-02
**Status**: Initial Planning
