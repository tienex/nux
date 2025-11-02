/*++
    Module Name:

        d3d5.h

    Abstract:

        Direct3D 5 emulation library using OpenGL ES 2.0 backend.
        First "modern" D3D API with DrawPrimitive (single texture).

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

#ifndef CHAR
typedef char  CHAR;
#endif
#ifndef FLOAT
typedef float FLOAT;
#endif
typedef UINT32 DWORD;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                           */
/* --------------------------------------------------------------- */

typedef struct IDirect3D2 IDirect3D2;
typedef struct IDirect3DDevice2 IDirect3DDevice2;
typedef struct IDirect3DTexture IDirect3DTexture;

/* --------------------------------------------------------------- */
/*  D3D5 Types                                                     */
/* --------------------------------------------------------------- */

typedef enum _D3DPRIMITIVETYPE5 {
    D3DPT5_TRIANGLELIST  = 4,
    D3DPT5_TRIANGLESTRIP = 5,
} D3DPRIMITIVETYPE5;

/* FVF flags (simplified) */
#define D3DFVF5_XYZ      0x002
#define D3DFVF5_NORMAL   0x010
#define D3DFVF5_DIFFUSE  0x040
#define D3DFVF5_TEX1     0x100

/* Render states */
typedef enum _D3DRS5 {
    D3DRS5_ZENABLE          = 7,
    D3DRS5_ZWRITEENABLE     = 14,
    D3DRS5_ALPHABLENDENABLE = 27,
} D3DRS5;

/* Transform states */
typedef enum _D3DTS5 {
    D3DTS5_WORLD      = 1,
    D3DTS5_VIEW       = 2,
    D3DTS5_PROJECTION = 3,
} D3DTS5;

/* Matrix */
typedef struct _D3DMATRIX5 {
    FLOAT _11, _12, _13, _14;
    FLOAT _21, _22, _23, _24;
    FLOAT _31, _32, _33, _34;
    FLOAT _41, _42, _43, _44;
} D3DMATRIX5;

/* --------------------------------------------------------------- */
/*  IDirect3D2 Interface                                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D2 "6AE2A420-22B4-11CF-B60E-00AA006C3E32"
ANX_DEFINE_GUID(IID_IDirect3D2,
    0x6AE2A420, 0x22B4, 0x11CF,
    0xB6, 0x0E, 0x00, 0xAA, 0x00, 0x6C, 0x3E, 0x32);

ANX_BEGIN_INTERFACE(IDirect3D2, IUnknown,
    IID_IDirect3D2, ANX_IID_IDirect3D2)

    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN CONST GUID *rclsid,
        IN VOID *lpSurface,
        OUT IDirect3DDevice2 **lplpDirect3DDevice2))

ANX_END_INTERFACE(IDirect3D2)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice2 Interface                                     */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice2 "93281500-8CF8-11D0-89AB-00A0C9054129"
ANX_DEFINE_GUID(IID_IDirect3DDevice2,
    0x93281500, 0x8CF8, 0x11D0,
    0x89, 0xAB, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29);

ANX_BEGIN_INTERFACE(IDirect3DDevice2, IUnknown,
    IID_IDirect3DDevice2, ANX_IID_IDirect3DDevice2)

    /* Scene */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))

    /* Transform */
    ANX_IFACE_METHOD(HRESULT, SetTransform, (
        IN D3DTS5 dtstTransformStateType,
        IN D3DMATRIX5 *lpD3DMatrix))

    /* State */
    ANX_IFACE_METHOD(HRESULT, SetRenderState, (
        IN D3DRS5 dwRenderStateType,
        IN DWORD dwRenderState))

    /* Drawing - The new DrawPrimitive API! */
    ANX_IFACE_METHOD(HRESULT, DrawPrimitive, (
        IN D3DPRIMITIVETYPE5 dptPrimitiveType,
        IN DWORD dwVertexTypeDesc,
        IN VOID *lpvVertices,
        IN DWORD dwVertexCount,
        IN DWORD dwFlags))

ANX_END_INTERFACE(IDirect3DDevice2)

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                */
/* --------------------------------------------------------------- */

#define IDirect3DDevice2_BeginScene(p)          ((p)->lpVtbl->BeginScene(p))
#define IDirect3DDevice2_EndScene(p)            ((p)->lpVtbl->EndScene(p))
#define IDirect3DDevice2_SetTransform(p,a,b)    ((p)->lpVtbl->SetTransform(p,a,b))
#define IDirect3DDevice2_SetRenderState(p,a,b)  ((p)->lpVtbl->SetRenderState(p,a,b))
#define IDirect3DDevice2_DrawPrimitive(p,a,b,c,d,e) ((p)->lpVtbl->DrawPrimitive(p,a,b,c,d,e))

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate5(
    OUT IDirect3D2 **ppDirect3D2
);

#endif /* _ANANKE_D3D5_H_ */
