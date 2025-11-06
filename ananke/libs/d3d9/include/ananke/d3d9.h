/*++
    Module Name:

        d3d9.h

    Abstract:

        Direct3D 9 emulation library using OpenGL ES 2.0 backend.
        Provides Microsoft DirectX 9 compatible interfaces for rendering.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  Basic type definitions for D3D9                                */
/* --------------------------------------------------------------- */

typedef char  CHAR;     /* 8-bit character */
typedef float FLOAT;    /* 32-bit float */

/* --------------------------------------------------------------- */
/*  Forward Declarations                                           */
/* --------------------------------------------------------------- */

typedef struct IDirect3D9 IDirect3D9;
typedef struct IDirect3DDevice9 IDirect3DDevice9;
typedef struct IDirect3DVertexBuffer9 IDirect3DVertexBuffer9;
typedef struct IDirect3DIndexBuffer9 IDirect3DIndexBuffer9;
typedef struct IDirect3DTexture9 IDirect3DTexture9;
typedef struct IDirect3DSurface9 IDirect3DSurface9;
typedef struct IDirect3DVertexShader9 IDirect3DVertexShader9;
typedef struct IDirect3DPixelShader9 IDirect3DPixelShader9;
typedef struct IDirect3DVertexDeclaration9 IDirect3DVertexDeclaration9;

/* --------------------------------------------------------------- */
/*  Direct3D 9 Constants                                           */
/* --------------------------------------------------------------- */

#define D3D_SDK_VERSION 32

/* Adapter */
#define D3DADAPTER_DEFAULT 0

/* Device types */
typedef enum _D3DDEVTYPE {
    D3DDEVTYPE_HAL         = 1,
    D3DDEVTYPE_REF         = 2,
    D3DDEVTYPE_SW          = 3,
} D3DDEVTYPE;

/* Multi-sample types */
typedef enum _D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE     = 0,
    D3DMULTISAMPLE_2_SAMPLES = 2,
    D3DMULTISAMPLE_4_SAMPLES = 4,
} D3DMULTISAMPLE_TYPE;

/* Formats */
typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN          = 0,
    D3DFMT_R8G8B8           = 20,
    D3DFMT_A8R8G8B8         = 21,
    D3DFMT_X8R8G8B8         = 22,
    D3DFMT_R5G6B5           = 23,
    D3DFMT_X1R5G5B5         = 24,
    D3DFMT_A1R5G5B5         = 25,
    D3DFMT_A4R4G4B4         = 26,
    D3DFMT_A8               = 28,
    D3DFMT_D16              = 80,
    D3DFMT_D24S8            = 75,
    D3DFMT_D24X8            = 77,
    D3DFMT_D32              = 71,
} D3DFORMAT;

/* Resource types */
typedef enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE        = 1,
    D3DRTYPE_VOLUME         = 2,
    D3DRTYPE_TEXTURE        = 3,
    D3DRTYPE_VOLUMETEXTURE  = 4,
    D3DRTYPE_CUBETEXTURE    = 5,
    D3DRTYPE_VERTEXBUFFER   = 6,
    D3DRTYPE_INDEXBUFFER    = 7,
} D3DRESOURCETYPE;

/* Pool types */
typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT         = 0,
    D3DPOOL_MANAGED         = 1,
    D3DPOOL_SYSTEMMEM       = 2,
} D3DPOOL;

/* Primitive types */
typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST         = 1,
    D3DPT_LINELIST          = 2,
    D3DPT_LINESTRIP         = 3,
    D3DPT_TRIANGLELIST      = 4,
    D3DPT_TRIANGLESTRIP     = 5,
    D3DPT_TRIANGLEFAN       = 6,
} D3DPRIMITIVETYPE;

/* Usage flags */
#define D3DUSAGE_RENDERTARGET       0x00000001L
#define D3DUSAGE_DEPTHSTENCIL       0x00000002L
#define D3DUSAGE_DYNAMIC            0x00000200L
#define D3DUSAGE_WRITEONLY          0x00000008L

/* Lock flags */
#define D3DLOCK_READONLY            0x00000010L
#define D3DLOCK_DISCARD             0x00002000L
#define D3DLOCK_NOOVERWRITE         0x00001000L

/* Clear flags */
#define D3DCLEAR_TARGET             0x00000001L
#define D3DCLEAR_ZBUFFER            0x00000002L
#define D3DCLEAR_STENCIL            0x00000004L

/* Behavior flags */
#define D3DCREATE_HARDWARE_VERTEXPROCESSING  0x00000040L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING  0x00000020L
#define D3DCREATE_MIXED_VERTEXPROCESSING     0x00000080L

/* Render states */
typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE               = 7,
    D3DRS_FILLMODE              = 8,
    D3DRS_SHADEMODE             = 9,
    D3DRS_ZWRITEENABLE          = 14,
    D3DRS_ALPHATESTENABLE       = 15,
    D3DRS_SRCBLEND              = 19,
    D3DRS_DESTBLEND             = 20,
    D3DRS_CULLMODE              = 22,
    D3DRS_ZFUNC                 = 23,
    D3DRS_ALPHABLENDENABLE      = 27,
    D3DRS_FOGENABLE             = 28,
    D3DRS_SPECULARENABLE        = 29,
    D3DRS_FOGCOLOR              = 34,
    D3DRS_LIGHTING              = 137,
    D3DRS_AMBIENT               = 139,
    D3DRS_COLORVERTEX           = 141,
} D3DRENDERSTATETYPE;

/* Texture stage states */
typedef enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP              = 1,
    D3DTSS_COLORARG1            = 2,
    D3DTSS_COLORARG2            = 3,
    D3DTSS_ALPHAOP              = 4,
    D3DTSS_ALPHAARG1            = 5,
    D3DTSS_ALPHAARG2            = 6,
} D3DTEXTURESTAGESTATETYPE;

/* Sampler states */
typedef enum _D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU            = 1,
    D3DSAMP_ADDRESSV            = 2,
    D3DSAMP_ADDRESSW            = 3,
    D3DSAMP_MAGFILTER           = 5,
    D3DSAMP_MINFILTER           = 6,
    D3DSAMP_MIPFILTER           = 7,
} D3DSAMPLERSTATETYPE;

/* Blend modes */
typedef enum _D3DBLEND {
    D3DBLEND_ZERO               = 1,
    D3DBLEND_ONE                = 2,
    D3DBLEND_SRCCOLOR           = 3,
    D3DBLEND_INVSRCCOLOR        = 4,
    D3DBLEND_SRCALPHA           = 5,
    D3DBLEND_INVSRCALPHA        = 6,
    D3DBLEND_DESTALPHA          = 7,
    D3DBLEND_INVDESTALPHA       = 8,
    D3DBLEND_DESTCOLOR          = 9,
    D3DBLEND_INVDESTCOLOR       = 10,
} D3DBLEND;

/* Cull modes */
typedef enum _D3DCULL {
    D3DCULL_NONE                = 1,
    D3DCULL_CW                  = 2,
    D3DCULL_CCW                 = 3,
} D3DCULL;

/* Compare functions */
typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER                = 1,
    D3DCMP_LESS                 = 2,
    D3DCMP_EQUAL                = 3,
    D3DCMP_LESSEQUAL            = 4,
    D3DCMP_GREATER              = 5,
    D3DCMP_NOTEQUAL             = 6,
    D3DCMP_GREATEREQUAL         = 7,
    D3DCMP_ALWAYS               = 8,
} D3DCMPFUNC;

/* Vertex element declarations */
typedef enum _D3DDECLTYPE {
    D3DDECLTYPE_FLOAT1          = 0,
    D3DDECLTYPE_FLOAT2          = 1,
    D3DDECLTYPE_FLOAT3          = 2,
    D3DDECLTYPE_FLOAT4          = 3,
    D3DDECLTYPE_UBYTE4          = 5,
} D3DDECLTYPE;

typedef enum _D3DDECLUSAGE {
    D3DDECLUSAGE_POSITION       = 0,
    D3DDECLUSAGE_BLENDWEIGHT    = 1,
    D3DDECLUSAGE_NORMAL         = 3,
    D3DDECLUSAGE_TEXCOORD       = 5,
    D3DDECLUSAGE_COLOR          = 10,
} D3DDECLUSAGE;

#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_FLOAT1,0,0,0,0}

/* --------------------------------------------------------------- */
/*  Direct3D 9 Structures                                          */
/* --------------------------------------------------------------- */

typedef struct _D3DPRESENT_PARAMETERS {
    UINT32              BackBufferWidth;
    UINT32              BackBufferHeight;
    D3DFORMAT           BackBufferFormat;
    UINT32              BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    UINT32              MultiSampleQuality;
    UINT32              SwapEffect;
    VOID               *hDeviceWindow;
    INT32               Windowed;
    INT32               EnableAutoDepthStencil;
    D3DFORMAT           AutoDepthStencilFormat;
    UINT32              Flags;
    UINT32              FullScreen_RefreshRateInHz;
    UINT32              PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DVERTEXELEMENT9 {
    UINT16      Stream;
    UINT16      Offset;
    UINT8       Type;
    UINT8       Method;
    UINT8       Usage;
    UINT8       UsageIndex;
} D3DVERTEXELEMENT9;

typedef struct _D3DVIEWPORT9 {
    UINT32  X;
    UINT32  Y;
    UINT32  Width;
    UINT32  Height;
    FLOAT   MinZ;
    FLOAT   MaxZ;
} D3DVIEWPORT9;

typedef struct _D3DRECT {
    INT32   x1;
    INT32   y1;
    INT32   x2;
    INT32   y2;
} D3DRECT;

typedef struct _D3DLOCKED_RECT {
    INT32   Pitch;
    VOID   *pBits;
} D3DLOCKED_RECT;

typedef struct _D3DSURFACE_DESC {
    D3DFORMAT           Format;
    D3DRESOURCETYPE     Type;
    UINT32              Usage;
    D3DPOOL             Pool;
    UINT32              Width;
    UINT32              Height;
} D3DSURFACE_DESC;

typedef UINT32 D3DCOLOR;

#define D3DCOLOR_ARGB(a,r,g,b) \
    ((D3DCOLOR)((((a)&0xFF)<<24)|(((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF)))

#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)

/* --------------------------------------------------------------- */
/*  IDirect3D9 - Main D3D interface                                */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D9 "D3D90100-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3D9,
    0xD3D90100, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3D9, IUnknown,
    IID_IDirect3D9, ANX_IID_IDirect3D9)

    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN UINT32 Adapter,
        IN D3DDEVTYPE DeviceType,
        IN VOID *hFocusWindow,
        IN UINT32 BehaviorFlags,
        IN D3DPRESENT_PARAMETERS *pPresentationParameters,
        OUT IDirect3DDevice9 **ppReturnedDeviceInterface))

    ANX_IFACE_METHOD(HRESULT, GetAdapterCount, (
        OUT UINT32 *Count))

ANX_END_INTERFACE(IDirect3D9)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice9 - Device interface                            */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice9 "D3D90101-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DDevice9,
    0xD3D90101, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DDevice9, IUnknown,
    IID_IDirect3DDevice9, ANX_IID_IDirect3DDevice9)

    /* Device state */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN UINT32 Count,
        IN CONST D3DRECT *pRects,
        IN UINT32 Flags,
        IN D3DCOLOR Color,
        IN FLOAT Z,
        IN UINT32 Stencil))
    ANX_IFACE_METHOD(HRESULT, Present, (
        IN CONST D3DRECT *pSourceRect,
        IN CONST D3DRECT *pDestRect,
        IN VOID *hDestWindowOverride,
        IN CONST VOID *pDirtyRegion))

    /* Resource creation */
    ANX_IFACE_METHOD(HRESULT, CreateVertexBuffer, (
        IN UINT32 Length,
        IN UINT32 Usage,
        IN UINT32 FVF,
        IN D3DPOOL Pool,
        OUT IDirect3DVertexBuffer9 **ppVertexBuffer,
        IN VOID *pSharedHandle))

    ANX_IFACE_METHOD(HRESULT, CreateIndexBuffer, (
        IN UINT32 Length,
        IN UINT32 Usage,
        IN D3DFORMAT Format,
        IN D3DPOOL Pool,
        OUT IDirect3DIndexBuffer9 **ppIndexBuffer,
        IN VOID *pSharedHandle))

    ANX_IFACE_METHOD(HRESULT, CreateTexture, (
        IN UINT32 Width,
        IN UINT32 Height,
        IN UINT32 Levels,
        IN UINT32 Usage,
        IN D3DFORMAT Format,
        IN D3DPOOL Pool,
        OUT IDirect3DTexture9 **ppTexture,
        IN VOID *pSharedHandle))

    ANX_IFACE_METHOD(HRESULT, CreateVertexShader, (
        IN CONST UINT32 *pFunction,
        OUT IDirect3DVertexShader9 **ppShader))

    ANX_IFACE_METHOD(HRESULT, CreatePixelShader, (
        IN CONST UINT32 *pFunction,
        OUT IDirect3DPixelShader9 **ppShader))

    ANX_IFACE_METHOD(HRESULT, CreateVertexDeclaration, (
        IN CONST D3DVERTEXELEMENT9 *pVertexElements,
        OUT IDirect3DVertexDeclaration9 **ppDecl))

    /* Rendering */
    ANX_IFACE_METHOD(HRESULT, DrawPrimitive, (
        IN D3DPRIMITIVETYPE PrimitiveType,
        IN UINT32 StartVertex,
        IN UINT32 PrimitiveCount))

    ANX_IFACE_METHOD(HRESULT, DrawIndexedPrimitive, (
        IN D3DPRIMITIVETYPE PrimitiveType,
        IN INT32 BaseVertexIndex,
        IN UINT32 MinVertexIndex,
        IN UINT32 NumVertices,
        IN UINT32 StartIndex,
        IN UINT32 PrimitiveCount))

    /* State management */
    ANX_IFACE_METHOD(HRESULT, SetRenderState, (
        IN D3DRENDERSTATETYPE State,
        IN UINT32 Value))

    ANX_IFACE_METHOD(HRESULT, SetTexture, (
        IN UINT32 Stage,
        IN IDirect3DTexture9 *pTexture))

    ANX_IFACE_METHOD(HRESULT, SetStreamSource, (
        IN UINT32 StreamNumber,
        IN IDirect3DVertexBuffer9 *pStreamData,
        IN UINT32 OffsetInBytes,
        IN UINT32 Stride))

    ANX_IFACE_METHOD(HRESULT, SetIndices, (
        IN IDirect3DIndexBuffer9 *pIndexData))

    ANX_IFACE_METHOD(HRESULT, SetVertexShader, (
        IN IDirect3DVertexShader9 *pShader))

    ANX_IFACE_METHOD(HRESULT, SetPixelShader, (
        IN IDirect3DPixelShader9 *pShader))

    ANX_IFACE_METHOD(HRESULT, SetVertexDeclaration, (
        IN IDirect3DVertexDeclaration9 *pDecl))

    ANX_IFACE_METHOD(HRESULT, SetViewport, (
        IN CONST D3DVIEWPORT9 *pViewport))

    /* Shader constants */
    ANX_IFACE_METHOD(HRESULT, SetVertexShaderConstantF, (
        IN UINT32 StartRegister,
        IN CONST FLOAT *pConstantData,
        IN UINT32 Vector4fCount))

    ANX_IFACE_METHOD(HRESULT, SetPixelShaderConstantF, (
        IN UINT32 StartRegister,
        IN CONST FLOAT *pConstantData,
        IN UINT32 Vector4fCount))

    /* State getters */
    ANX_IFACE_METHOD(HRESULT, GetRenderState, (
        IN D3DRENDERSTATETYPE State,
        OUT UINT32 *pValue))

    /* Texture stage states */
    ANX_IFACE_METHOD(HRESULT, SetTextureStageState, (
        IN UINT32 Stage,
        IN D3DTEXTURESTAGESTATETYPE Type,
        IN UINT32 Value))

    /* Sampler states */
    ANX_IFACE_METHOD(HRESULT, SetSamplerState, (
        IN UINT32 Sampler,
        IN D3DSAMPLERSTATETYPE Type,
        IN UINT32 Value))

ANX_END_INTERFACE(IDirect3DDevice9)

/* --------------------------------------------------------------- */
/*  IDirect3DVertexBuffer9                                         */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DVertexBuffer9 "D3D90102-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DVertexBuffer9,
    0xD3D90102, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DVertexBuffer9, IUnknown,
    IID_IDirect3DVertexBuffer9, ANX_IID_IDirect3DVertexBuffer9)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN UINT32 OffsetToLock,
        IN UINT32 SizeToLock,
        OUT VOID **ppbData,
        IN UINT32 Flags))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

ANX_END_INTERFACE(IDirect3DVertexBuffer9)

/* --------------------------------------------------------------- */
/*  IDirect3DIndexBuffer9                                          */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DIndexBuffer9 "D3D90103-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DIndexBuffer9,
    0xD3D90103, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DIndexBuffer9, IUnknown,
    IID_IDirect3DIndexBuffer9, ANX_IID_IDirect3DIndexBuffer9)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN UINT32 OffsetToLock,
        IN UINT32 SizeToLock,
        OUT VOID **ppbData,
        IN UINT32 Flags))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

ANX_END_INTERFACE(IDirect3DIndexBuffer9)

/* --------------------------------------------------------------- */
/*  IDirect3DTexture9                                              */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DTexture9 "D3D90104-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DTexture9,
    0xD3D90104, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DTexture9, IUnknown,
    IID_IDirect3DTexture9, ANX_IID_IDirect3DTexture9)

    ANX_IFACE_METHOD(HRESULT, LockRect, (
        IN UINT32 Level,
        OUT D3DLOCKED_RECT *pLockedRect,
        IN CONST D3DRECT *pRect,
        IN UINT32 Flags))

    ANX_IFACE_METHOD(HRESULT, UnlockRect, (
        IN UINT32 Level))

    ANX_IFACE_METHOD(HRESULT, GetLevelDesc, (
        IN UINT32 Level,
        OUT D3DSURFACE_DESC *pDesc))

ANX_END_INTERFACE(IDirect3DTexture9)

/* --------------------------------------------------------------- */
/*  IDirect3DVertexShader9                                         */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DVertexShader9 "D3D90105-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DVertexShader9,
    0xD3D90105, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DVertexShader9, IUnknown,
    IID_IDirect3DVertexShader9, ANX_IID_IDirect3DVertexShader9)

    /* Placeholder - shader is opaque */

ANX_END_INTERFACE(IDirect3DVertexShader9)

/* --------------------------------------------------------------- */
/*  IDirect3DPixelShader9                                          */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DPixelShader9 "D3D90106-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DPixelShader9,
    0xD3D90106, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DPixelShader9, IUnknown,
    IID_IDirect3DPixelShader9, ANX_IID_IDirect3DPixelShader9)

    /* Placeholder - shader is opaque */

ANX_END_INTERFACE(IDirect3DPixelShader9)

/* --------------------------------------------------------------- */
/*  IDirect3DVertexDeclaration9                                    */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DVertexDeclaration9 "D3D90107-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IDirect3DVertexDeclaration9,
    0xD3D90107, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IDirect3DVertexDeclaration9, IUnknown,
    IID_IDirect3DVertexDeclaration9, ANX_IID_IDirect3DVertexDeclaration9)

    /* Placeholder - declaration is opaque */

ANX_END_INTERFACE(IDirect3DVertexDeclaration9)

/* --------------------------------------------------------------- */
/*  Factory function                                                */
/* --------------------------------------------------------------- */

IDirect3D9*
Direct3DCreate9(
    UINT32 SDKVersion
);

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                 */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IDirect3D9_CreateDevice(This, Adapter, DevType, hWnd, Flags, pPP, ppDev) \
    ((This)->lpVtbl->CreateDevice(This, Adapter, DevType, hWnd, Flags, pPP, ppDev))

#define IDirect3DDevice9_BeginScene(This) \
    ((This)->lpVtbl->BeginScene(This))
#define IDirect3DDevice9_EndScene(This) \
    ((This)->lpVtbl->EndScene(This))
#define IDirect3DDevice9_Clear(This, Count, pRects, Flags, Color, Z, Stencil) \
    ((This)->lpVtbl->Clear(This, Count, pRects, Flags, Color, Z, Stencil))
#define IDirect3DDevice9_Present(This, pSrc, pDst, hWnd, pDirty) \
    ((This)->lpVtbl->Present(This, pSrc, pDst, hWnd, pDirty))
#define IDirect3DDevice9_CreateVertexBuffer(This, Len, Usage, FVF, Pool, ppVB, pHandle) \
    ((This)->lpVtbl->CreateVertexBuffer(This, Len, Usage, FVF, Pool, ppVB, pHandle))
#define IDirect3DDevice9_CreateIndexBuffer(This, Len, Usage, Fmt, Pool, ppIB, pHandle) \
    ((This)->lpVtbl->CreateIndexBuffer(This, Len, Usage, Fmt, Pool, ppIB, pHandle))
#define IDirect3DDevice9_CreateTexture(This, W, H, Lvls, Usage, Fmt, Pool, ppTex, pHandle) \
    ((This)->lpVtbl->CreateTexture(This, W, H, Lvls, Usage, Fmt, Pool, ppTex, pHandle))
#define IDirect3DDevice9_CreateVertexShader(This, pFunc, ppShader) \
    ((This)->lpVtbl->CreateVertexShader(This, pFunc, ppShader))
#define IDirect3DDevice9_CreatePixelShader(This, pFunc, ppShader) \
    ((This)->lpVtbl->CreatePixelShader(This, pFunc, ppShader))
#define IDirect3DDevice9_CreateVertexDeclaration(This, pElems, ppDecl) \
    ((This)->lpVtbl->CreateVertexDeclaration(This, pElems, ppDecl))
#define IDirect3DDevice9_DrawPrimitive(This, PrimType, StartVtx, PrimCount) \
    ((This)->lpVtbl->DrawPrimitive(This, PrimType, StartVtx, PrimCount))
#define IDirect3DDevice9_DrawIndexedPrimitive(This, PrimType, BaseVtx, MinIdx, NumVtx, StartIdx, PrimCount) \
    ((This)->lpVtbl->DrawIndexedPrimitive(This, PrimType, BaseVtx, MinIdx, NumVtx, StartIdx, PrimCount))
#define IDirect3DDevice9_SetRenderState(This, State, Value) \
    ((This)->lpVtbl->SetRenderState(This, State, Value))
#define IDirect3DDevice9_SetTexture(This, Stage, pTex) \
    ((This)->lpVtbl->SetTexture(This, Stage, pTex))
#define IDirect3DDevice9_SetStreamSource(This, Stream, pData, Offset, Stride) \
    ((This)->lpVtbl->SetStreamSource(This, Stream, pData, Offset, Stride))
#define IDirect3DDevice9_SetIndices(This, pData) \
    ((This)->lpVtbl->SetIndices(This, pData))
#define IDirect3DDevice9_SetVertexShader(This, pShader) \
    ((This)->lpVtbl->SetVertexShader(This, pShader))
#define IDirect3DDevice9_SetPixelShader(This, pShader) \
    ((This)->lpVtbl->SetPixelShader(This, pShader))
#define IDirect3DDevice9_SetVertexDeclaration(This, pDecl) \
    ((This)->lpVtbl->SetVertexDeclaration(This, pDecl))
#define IDirect3DDevice9_SetViewport(This, pVP) \
    ((This)->lpVtbl->SetViewport(This, pVP))
#define IDirect3DDevice9_SetVertexShaderConstantF(This, StartReg, pData, Count) \
    ((This)->lpVtbl->SetVertexShaderConstantF(This, StartReg, pData, Count))
#define IDirect3DDevice9_SetPixelShaderConstantF(This, StartReg, pData, Count) \
    ((This)->lpVtbl->SetPixelShaderConstantF(This, StartReg, pData, Count))
#define IDirect3DDevice9_GetRenderState(This, State, pValue) \
    ((This)->lpVtbl->GetRenderState(This, State, pValue))
#define IDirect3DDevice9_SetTextureStageState(This, Stage, Type, Value) \
    ((This)->lpVtbl->SetTextureStageState(This, Stage, Type, Value))
#define IDirect3DDevice9_SetSamplerState(This, Sampler, Type, Value) \
    ((This)->lpVtbl->SetSamplerState(This, Sampler, Type, Value))

#define IDirect3DVertexBuffer9_Lock(This, Offset, Size, ppData, Flags) \
    ((This)->lpVtbl->Lock(This, Offset, Size, ppData, Flags))
#define IDirect3DVertexBuffer9_Unlock(This) \
    ((This)->lpVtbl->Unlock(This))

#define IDirect3DIndexBuffer9_Lock(This, Offset, Size, ppData, Flags) \
    ((This)->lpVtbl->Lock(This, Offset, Size, ppData, Flags))
#define IDirect3DIndexBuffer9_Unlock(This) \
    ((This)->lpVtbl->Unlock(This))

#define IDirect3DTexture9_LockRect(This, Level, pLocked, pRect, Flags) \
    ((This)->lpVtbl->LockRect(This, Level, pLocked, pRect, Flags))
#define IDirect3DTexture9_UnlockRect(This, Level) \
    ((This)->lpVtbl->UnlockRect(This, Level))
#define IDirect3DTexture9_GetLevelDesc(This, Level, pDesc) \
    ((This)->lpVtbl->GetLevelDesc(This, Level, pDesc))

#endif /* !__cplusplus */
