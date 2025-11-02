/** @file
  VINIL IL Program Builder

  COM interface for programmatically constructing IL programs.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/vinil.h>
#include <vinil/il.h>
#include <ananke/com.h>

//
// GUID
//

ANX_DEFINE_GUID(IID_IVinilBuilder, 0x78901234, 0x7890, 0x7890, 0x78, 0x90, 0x12, 0x34, 0xAB, 0xCD, 0xEF, 0x56);

//
// Variable Types
//

typedef enum _VINIL_VARIABLE_TYPE {
    VinilVariableTypeFloat    = 0,
    VinilVariableTypeFloat2   = 1,
    VinilVariableTypeFloat3   = 2,
    VinilVariableTypeFloat4   = 3,
    VinilVariableTypeInt      = 4,
    VinilVariableTypeInt2     = 5,
    VinilVariableTypeInt3     = 6,
    VinilVariableTypeInt4     = 7,
    VinilVariableTypeMat4     = 8,
} VINIL_VARIABLE_TYPE;

//
// IVinilBuilder Interface
//

ANX_BEGIN_INTERFACE(IVinilBuilder, IUnknown, IID_IVinilBuilder, "78901234-7890-7890-7890-1234ABCDEF56")
    /**
      Create a variable (register).

      @param[in]   Type     Variable type.
      @param[in]   Name     Variable name (optional, can be NULL).
      @param[out]  Variable Created variable interface.

      @retval  S_OK           Success.
      @retval  E_POINTER      Invalid pointer.
      @retval  E_OUTOFMEMORY  Memory allocation failed.
    **/
    ANX_IFACE_METHOD(HRESULT, CreateVariable, (VINIL_VARIABLE_TYPE Type, CONST CHAR8 *Name, IVinilVariable **Variable))

    /**
      Create a basic block.

      @param[out]  Block  Created block interface.

      @retval  S_OK           Success.
      @retval  E_POINTER      Invalid pointer.
      @retval  E_OUTOFMEMORY  Memory allocation failed.
    **/
    ANX_IFACE_METHOD(HRESULT, CreateBlock, (IVinilBlock **Block))

    /**
      Set current insertion block.

      @param[in]  Block  Block to insert instructions into.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, SetInsertBlock, (IVinilBlock *Block))

    /**
      Build ADD instruction: dst = src1 + src2.

      @param[in]  Dst   Destination variable.
      @param[in]  Src1  First source variable.
      @param[in]  Src2  Second source variable.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, BuildAdd, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2))

    /**
      Build SUB instruction: dst = src1 - src2.
    **/
    ANX_IFACE_METHOD(HRESULT, BuildSub, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2))

    /**
      Build MUL instruction: dst = src1 * src2.
    **/
    ANX_IFACE_METHOD(HRESULT, BuildMul, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2))

    /**
      Build MAD instruction: dst = src1 * src2 + src3.
    **/
    ANX_IFACE_METHOD(HRESULT, BuildMad, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2, IVinilVariable *Src3))

    /**
      Build MOV instruction: dst = src.
    **/
    ANX_IFACE_METHOD(HRESULT, BuildMov, (IVinilVariable *Dst, IVinilVariable *Src))

    /**
      Build DP3 instruction: dst = dot(src1.xyz, src2.xyz).
    **/
    ANX_IFACE_METHOD(HRESULT, BuildDp3, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2))

    /**
      Build DP4 instruction: dst = dot(src1, src2).
    **/
    ANX_IFACE_METHOD(HRESULT, BuildDp4, (IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2))

    /**
      Build RET instruction (return from program/function).

      @retval  S_OK  Success.
    **/
    HRESULT (STDMETHODCALLTYPE *BuildRet)(void* This);

    /**
      Finalize and get the built program.

      @param[out]  Program  Built IL program interface.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_FAIL     Finalization failed.
    **/
    ANX_IFACE_METHOD(HRESULT, Finalize, (IVinilProgram **Program))
ANX_END_INTERFACE(IVinilBuilder, IID_IVinilBuilder)

//
// COBJMACROS
//

#ifdef COBJMACROS

#define IVinilBuilder_QueryInterface(This, riid, ppv) \
    (This)->lpVtbl->QueryInterface(This, riid, ppv)
#define IVinilBuilder_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IVinilBuilder_Release(This) \
    (This)->lpVtbl->Release(This)
#define IVinilBuilder_CreateVariable(This, Type, Name, Variable) \
    (This)->lpVtbl->CreateVariable(This, Type, Name, Variable)
#define IVinilBuilder_CreateBlock(This, Block) \
    (This)->lpVtbl->CreateBlock(This, Block)
#define IVinilBuilder_SetInsertBlock(This, Block) \
    (This)->lpVtbl->SetInsertBlock(This, Block)
#define IVinilBuilder_BuildAdd(This, Dst, Src1, Src2) \
    (This)->lpVtbl->BuildAdd(This, Dst, Src1, Src2)
#define IVinilBuilder_BuildSub(This, Dst, Src1, Src2) \
    (This)->lpVtbl->BuildSub(This, Dst, Src1, Src2)
#define IVinilBuilder_BuildMul(This, Dst, Src1, Src2) \
    (This)->lpVtbl->BuildMul(This, Dst, Src1, Src2)
#define IVinilBuilder_BuildMad(This, Dst, Src1, Src2, Src3) \
    (This)->lpVtbl->BuildMad(This, Dst, Src1, Src2, Src3)
#define IVinilBuilder_BuildMov(This, Dst, Src) \
    (This)->lpVtbl->BuildMov(This, Dst, Src)
#define IVinilBuilder_BuildDp3(This, Dst, Src1, Src2) \
    (This)->lpVtbl->BuildDp3(This, Dst, Src1, Src2)
#define IVinilBuilder_BuildDp4(This, Dst, Src1, Src2) \
    (This)->lpVtbl->BuildDp4(This, Dst, Src1, Src2)
#define IVinilBuilder_BuildRet(This) \
    (This)->lpVtbl->BuildRet(This)
#define IVinilBuilder_Finalize(This, Program) \
    (This)->lpVtbl->Finalize(This, Program)

#endif /* COBJMACROS */

//
// Factory Function
//

/**
  Create a new IL program builder.

  @param[out]  Builder  Created builder interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreateBuilder (
    IVinilBuilder  **Builder
    );

