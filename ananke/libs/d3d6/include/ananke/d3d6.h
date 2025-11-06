/*++
    Module Name:

        d3d6.h

    Abstract:

        Direct3D 6 emulation library using OpenGL ES 2.0 backend.
        Provides multitexture support (2-4 texture stages).

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

typedef struct IDirect3D3 IDirect3D3;
typedef struct IDirect3DDevice3 IDirect3DDevice3;
typedef struct IDirect3DTexture2 IDirect3DTexture2;

/* --------------------------------------------------------------- */
/*  D3D6 Types (COM-based, similar to D3D5)                        */
/* --------------------------------------------------------------- */

typedef enum _D3DPRIMITIVETYPE6 {
    D3DPT6_TRIANGLELIST  = 4,
    D3DPT6_TRIANGLESTRIP = 5,
} D3DPRIMITIVETYPE6;

/* FVF flags */
#define D3DFVF6_XYZ      0x002
#define D3DFVF6_NORMAL   0x010
#define D3DFVF6_DIFFUSE  0x040
#define D3DFVF6_TEX1     0x100
#define D3DFVF6_TEX2     0x200
#define D3DFVF6_TEX3     0x300
#define D3DFVF6_TEX4     0x400

/* Render states */
typedef enum _D3DRENDERSTATETYPE6 {
    D3DRS6_ZENABLE          = 7,
    D3DRS6_ZWRITEENABLE     = 14,
    D3DRS6_ALPHABLENDENABLE = 27,
    D3DRS6_LIGHTING         = 137,
} D3DRENDERSTATETYPE6;

/* Texture stage state types */
typedef enum _D3DTSS6 {
    D3DTSS6_COLOROP   = 1,
    D3DTSS6_COLORARG1 = 2,
    D3DTSS6_COLORARG2 = 3,
    D3DTSS6_ALPHAOP   = 4,
} D3DTSS6;

/* Texture operations */
typedef enum _D3DTOP6 {
    D3DTOP6_DISABLE    = 1,
    D3DTOP6_SELECTARG1 = 2,
    D3DTOP6_MODULATE   = 4,
    D3DTOP6_ADD        = 7,
} D3DTOP6;

/* Transform states */
typedef enum _D3DTS6 {
    D3DTS6_WORLD      = 1,
    D3DTS6_VIEW       = 2,
    D3DTS6_PROJECTION = 3,
} D3DTS6;

/* Matrix */
typedef struct _D3DMATRIX6 {
    FLOAT _11, _12, _13, _14;
    FLOAT _21, _22, _23, _24;
    FLOAT _31, _32, _33, _34;
    FLOAT _41, _42, _43, _44;
} D3DMATRIX6;

/* Viewport */
typedef struct _D3DVIEWPORT26 {
    DWORD dwX;
    DWORD dwY;
    DWORD dwWidth;
    DWORD dwHeight;
    FLOAT dvMinZ;
    FLOAT dvMaxZ;
} D3DVIEWPORT26;

/* --------------------------------------------------------------- */
/*  IDirect3D3 Interface                                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D3 "BB223240-E72B-11D0-A9B4-00AA00C0993E"
ANX_DEFINE_GUID(IID_IDirect3D3,
    0xBB223240, 0xE72B, 0x11D0,
    0xA9, 0xB4, 0x00, 0xAA, 0x00, 0xC0, 0x99, 0x3E);

ANX_BEGIN_INTERFACE(IDirect3D3, IUnknown,
    IID_IDirect3D3, ANX_IID_IDirect3D3)

    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN CONST GUID *rclsid,
        IN VOID *lpSurface,
        OUT IDirect3DDevice3 **lplpDirect3DDevice3,
        IN VOID *lpUnkOuter))

ANX_END_INTERFACE(IDirect3D3)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice3 Interface                                     */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice3 "BB223233-E72B-11D0-A9B4-00AA00C0993E"
ANX_DEFINE_GUID(IID_IDirect3DDevice3,
    0xBB223233, 0xE72B, 0x11D0,
    0xA9, 0xB4, 0x00, 0xAA, 0x00, 0xC0, 0x99, 0x3E);

ANX_BEGIN_INTERFACE(IDirect3DDevice3, IUnknown,
    IID_IDirect3DDevice3, ANX_IID_IDirect3DDevice3)

    /* Scene */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))

    /* Transform */
    ANX_IFACE_METHOD(HRESULT, SetTransform, (
        IN D3DTS6 dtstTransformStateType,
        IN D3DMATRIX6 *lpD3DMatrix))

    /* Viewport */
    ANX_IFACE_METHOD(HRESULT, SetViewport2, (
        IN D3DVIEWPORT26 *lpViewport))

    /* State */
    ANX_IFACE_METHOD(HRESULT, SetRenderState, (
        IN D3DRENDERSTATETYPE6 dwRenderStateType,
        IN DWORD dwRenderState))

    ANX_IFACE_METHOD(HRESULT, SetTextureStageState, (
        IN DWORD dwStage,
        IN D3DTSS6 dwState,
        IN DWORD dwValue))

    /* Drawing */
    ANX_IFACE_METHOD(HRESULT, DrawPrimitive, (
        IN D3DPRIMITIVETYPE6 dptPrimitiveType,
        IN DWORD dwVertexTypeDesc,
        IN VOID *lpvVertices,
        IN DWORD dwVertexCount,
        IN DWORD dwFlags))

ANX_END_INTERFACE(IDirect3DDevice3)

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                */
/* --------------------------------------------------------------- */

#define IDirect3DDevice3_BeginScene(p)          ((p)->lpVtbl->BeginScene(p))
#define IDirect3DDevice3_EndScene(p)            ((p)->lpVtbl->EndScene(p))
#define IDirect3DDevice3_SetTransform(p,a,b)    ((p)->lpVtbl->SetTransform(p,a,b))
#define IDirect3DDevice3_SetRenderState(p,a,b)  ((p)->lpVtbl->SetRenderState(p,a,b))
#define IDirect3DDevice3_DrawPrimitive(p,a,b,c,d,e) ((p)->lpVtbl->DrawPrimitive(p,a,b,c,d,e))

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate6(
    OUT IDirect3D3 **ppDirect3D3
);

#endif /* _ANANKE_D3D6_H_ */
