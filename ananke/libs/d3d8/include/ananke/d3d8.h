/*++
    Module Name:

        d3d8.h

    Abstract:

        Direct3D 8 emulation library using OpenGL ES 2.0 backend.
        Supports Shader Model 1.0-1.4 programmable pipeline.

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

typedef struct IDirect3D8 IDirect3D8;
typedef struct IDirect3DDevice8 IDirect3DDevice8;
typedef struct IDirect3DVertexBuffer8 IDirect3DVertexBuffer8;
typedef struct IDirect3DIndexBuffer8 IDirect3DIndexBuffer8;
typedef struct IDirect3DTexture8 IDirect3DTexture8;
typedef struct IDirect3DVertexShader8 IDirect3DVertexShader8;
typedef struct IDirect3DPixelShader8 IDirect3DPixelShader8;

/* --------------------------------------------------------------- */
/*  D3D8 Constants (same as D3D9 for most things)                  */
/* --------------------------------------------------------------- */

#define D3D8_SDK_VERSION 220

/* Use same enums as D3D9 for simplicity */
typedef enum _D3DFORMAT8 {
    D3DFMT8_A8R8G8B8 = 21,
    D3DFMT8_X8R8G8B8 = 22,
    D3DFMT8_R5G6B5   = 23,
} D3DFORMAT8;

typedef enum _D3DPRIMITIVETYPE8 {
    D3DPT8_TRIANGLELIST  = 4,
    D3DPT8_TRIANGLESTRIP = 5,
} D3DPRIMITIVETYPE8;

/* FVF flags (same as D3D7) */
#define D3DFVF8_XYZ      0x002
#define D3DFVF8_NORMAL   0x010
#define D3DFVF8_DIFFUSE  0x040
#define D3DFVF8_TEX1     0x100

/* Presentation parameters */
typedef struct _D3DPRESENT_PARAMETERS8 {
    UINT32       BackBufferWidth;
    UINT32       BackBufferHeight;
    D3DFORMAT8   BackBufferFormat;
    UINT32       BackBufferCount;
    BOOLEAN      Windowed;
    BOOLEAN      EnableAutoDepthStencil;
    D3DFORMAT8   AutoDepthStencilFormat;
} D3DPRESENT_PARAMETERS8;

/* --------------------------------------------------------------- */
/*  IDirect3D8 Interface                                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D8 "1DD9E8DA-1C77-4D40-B0CF-98FEFDFF9512"
ANX_DEFINE_GUID(IID_IDirect3D8,
    0x1DD9E8DA, 0x1C77, 0x4D40,
    0xB0, 0xCF, 0x98, 0xFE, 0xFD, 0xFF, 0x95, 0x12);

ANX_BEGIN_INTERFACE(IDirect3D8, IUnknown,
    IID_IDirect3D8, ANX_IID_IDirect3D8)

    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN UINT32 Adapter,
        IN UINT32 DeviceType,
        IN VOID *hFocusWindow,
        IN DWORD BehaviorFlags,
        IN D3DPRESENT_PARAMETERS8 *pPresentationParameters,
        OUT IDirect3DDevice8 **ppReturnedDeviceInterface))

ANX_END_INTERFACE(IDirect3D8)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice8 Interface                                     */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice8 "7385E5DF-8FE8-41D5-86B6-D7B48547B6CF"
ANX_DEFINE_GUID(IID_IDirect3DDevice8,
    0x7385E5DF, 0x8FE8, 0x41D5,
    0x86, 0xB6, 0xD7, 0xB4, 0x85, 0x47, 0xB6, 0xCF);

ANX_BEGIN_INTERFACE(IDirect3DDevice8, IUnknown,
    IID_IDirect3DDevice8, ANX_IID_IDirect3DDevice8)

    /* Scene */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, Present, (
        IN VOID *pSourceRect,
        IN VOID *pDestRect,
        IN VOID *hDestWindowOverride,
        IN VOID *pDirtyRegion))

    /* Clear */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN DWORD Count,
        IN VOID *pRects,
        IN DWORD Flags,
        IN DWORD Color,
        IN FLOAT Z,
        IN DWORD Stencil))

    /* Resource creation (simplified) */
    ANX_IFACE_METHOD(HRESULT, CreateVertexBuffer, (
        IN UINT32 Length,
        IN DWORD Usage,
        IN DWORD FVF,
        IN DWORD Pool,
        OUT IDirect3DVertexBuffer8 **ppVertexBuffer))

    ANX_IFACE_METHOD(HRESULT, CreateIndexBuffer, (
        IN UINT32 Length,
        IN DWORD Usage,
        IN DWORD Format,
        IN DWORD Pool,
        OUT IDirect3DIndexBuffer8 **ppIndexBuffer))

    /* Shaders (Shader Model 1.x) */
    ANX_IFACE_METHOD(HRESULT, CreateVertexShader, (
        IN CONST DWORD *pDeclaration,
        IN CONST DWORD *pFunction,
        OUT DWORD *pHandle,
        IN DWORD Usage))

    ANX_IFACE_METHOD(HRESULT, CreatePixelShader, (
        IN CONST DWORD *pFunction,
        OUT DWORD *pHandle))

    ANX_IFACE_METHOD(HRESULT, SetVertexShader, (
        IN DWORD Handle))

    ANX_IFACE_METHOD(HRESULT, SetPixelShader, (
        IN DWORD Handle))

    /* State */
    ANX_IFACE_METHOD(HRESULT, SetRenderState, (
        IN DWORD State,
        IN DWORD Value))

    ANX_IFACE_METHOD(HRESULT, SetTexture, (
        IN DWORD Stage,
        IN IDirect3DTexture8 *pTexture))

    /* Drawing */
    ANX_IFACE_METHOD(HRESULT, SetStreamSource, (
        IN UINT32 StreamNumber,
        IN IDirect3DVertexBuffer8 *pStreamData,
        IN UINT32 Stride))

    ANX_IFACE_METHOD(HRESULT, SetIndices, (
        IN IDirect3DIndexBuffer8 *pIndexData,
        IN UINT32 BaseVertexIndex))

    ANX_IFACE_METHOD(HRESULT, DrawPrimitive, (
        IN D3DPRIMITIVETYPE8 PrimitiveType,
        IN UINT32 StartVertex,
        IN UINT32 PrimitiveCount))

    ANX_IFACE_METHOD(HRESULT, DrawIndexedPrimitive, (
        IN D3DPRIMITIVETYPE8 PrimitiveType,
        IN UINT32 MinVertexIndex,
        IN UINT32 NumVertices,
        IN UINT32 StartIndex,
        IN UINT32 PrimitiveCount))

ANX_END_INTERFACE(IDirect3DDevice8)

/* --------------------------------------------------------------- */
/*  IDirect3DVertexBuffer8 Interface                               */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DVertexBuffer8 "8AEEEAC7-05F9-44D4-B591-000E0F012C0A"
ANX_DEFINE_GUID(IID_IDirect3DVertexBuffer8,
    0x8AEEEAC7, 0x05F9, 0x44D4,
    0xB5, 0x91, 0x00, 0x0E, 0x0F, 0x01, 0x2C, 0x0A);

ANX_BEGIN_INTERFACE(IDirect3DVertexBuffer8, IUnknown,
    IID_IDirect3DVertexBuffer8, ANX_IID_IDirect3DVertexBuffer8)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN UINT32 OffsetToLock,
        IN UINT32 SizeToLock,
        OUT UINT8 **ppbData,
        IN DWORD Flags))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

ANX_END_INTERFACE(IDirect3DVertexBuffer8)

/* --------------------------------------------------------------- */
/*  IDirect3DIndexBuffer8 Interface                                */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DIndexBuffer8 "0E689C9A-053D-44A0-9D92-DB0E3D750F86"
ANX_DEFINE_GUID(IID_IDirect3DIndexBuffer8,
    0x0E689C9A, 0x053D, 0x44A0,
    0x9D, 0x92, 0xDB, 0x0E, 0x3D, 0x75, 0x0F, 0x86);

ANX_BEGIN_INTERFACE(IDirect3DIndexBuffer8, IUnknown,
    IID_IDirect3DIndexBuffer8, ANX_IID_IDirect3DIndexBuffer8)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN UINT32 OffsetToLock,
        IN UINT32 SizeToLock,
        OUT UINT8 **ppbData,
        IN DWORD Flags))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

ANX_END_INTERFACE(IDirect3DIndexBuffer8)

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                */
/* --------------------------------------------------------------- */

#define IDirect3DDevice8_BeginScene(p)           ((p)->lpVtbl->BeginScene(p))
#define IDirect3DDevice8_EndScene(p)             ((p)->lpVtbl->EndScene(p))
#define IDirect3DDevice8_Clear(p,a,b,c,d,e,f)   ((p)->lpVtbl->Clear(p,a,b,c,d,e,f))
#define IDirect3DDevice8_Present(p,a,b,c,d)     ((p)->lpVtbl->Present(p,a,b,c,d))
#define IDirect3DDevice8_SetVertexShader(p,a)   ((p)->lpVtbl->SetVertexShader(p,a))
#define IDirect3DDevice8_SetPixelShader(p,a)    ((p)->lpVtbl->SetPixelShader(p,a))
#define IDirect3DDevice8_SetStreamSource(p,a,b,c) ((p)->lpVtbl->SetStreamSource(p,a,b,c))
#define IDirect3DDevice8_DrawPrimitive(p,a,b,c) ((p)->lpVtbl->DrawPrimitive(p,a,b,c))

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

IDirect3D8*
Direct3DCreate8(
    UINT32 SDKVersion
);

#endif /* _ANANKE_D3D8_H_ */
