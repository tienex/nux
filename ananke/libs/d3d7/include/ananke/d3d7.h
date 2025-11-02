/*++
    Module Name:

        d3d7.h

    Abstract:

        Direct3D 7 emulation library using OpenGL ES 2.0 backend.
        Provides Microsoft DirectX 7 compatible interfaces for rendering.

        Note: D3D7 uses DirectDraw7 for surface management and does not
        use COM for the device interface (plain C vtable structs).

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  Basic type definitions for D3D7                                */
/* --------------------------------------------------------------- */

#ifndef CHAR
typedef char  CHAR;     /* 8-bit character */
#endif
#ifndef FLOAT
typedef float FLOAT;    /* 32-bit float */
#endif

typedef UINT32 DWORD;   /* 32-bit unsigned */

/* --------------------------------------------------------------- */
/*  Forward Declarations                                           */
/* --------------------------------------------------------------- */

typedef struct IDirect3D7 IDirect3D7;
typedef struct IDirect3DDevice7 IDirect3DDevice7;
typedef struct IDirect3DVertexBuffer7 IDirect3DVertexBuffer7;
typedef struct IDirect3DTexture2 IDirect3DTexture2;
typedef struct IDirectDrawSurface7 IDirectDrawSurface7;

/* --------------------------------------------------------------- */
/*  Direct3D 7 Constants                                           */
/* --------------------------------------------------------------- */

/* Device types */
typedef enum _D3DDEVTYPE7 {
    D3DDEVTYPE7_HAL         = 1,
    D3DDEVTYPE7_REF         = 2,
    D3DDEVTYPE7_SW          = 3,
} D3DDEVTYPE7;

/* Primitive types */
typedef enum _D3DPRIMITIVETYPE7 {
    D3DPT7_POINTLIST         = 1,
    D3DPT7_LINELIST          = 2,
    D3DPT7_LINESTRIP         = 3,
    D3DPT7_TRIANGLELIST      = 4,
    D3DPT7_TRIANGLESTRIP     = 5,
    D3DPT7_TRIANGLEFAN       = 6,
} D3DPRIMITIVETYPE7;

/* Flexible Vertex Format (FVF) flags */
#define D3DFVF_XYZ              0x002   /* Position */
#define D3DFVF_XYZRHW           0x004   /* Transformed position */
#define D3DFVF_XYZB1            0x006   /* Position + 1 blend weight */
#define D3DFVF_XYZB2            0x008   /* Position + 2 blend weights */
#define D3DFVF_XYZB3            0x00A   /* Position + 3 blend weights */
#define D3DFVF_XYZB4            0x00C   /* Position + 4 blend weights */

#define D3DFVF_NORMAL           0x010   /* Normal */
#define D3DFVF_PSIZE            0x020   /* Point size */
#define D3DFVF_DIFFUSE          0x040   /* Diffuse color */
#define D3DFVF_SPECULAR         0x080   /* Specular color */

#define D3DFVF_TEX0             0x000   /* 0 texture coordinates */
#define D3DFVF_TEX1             0x100   /* 1 texture coordinate */
#define D3DFVF_TEX2             0x200   /* 2 texture coordinates */
#define D3DFVF_TEX3             0x300   /* 3 texture coordinates */
#define D3DFVF_TEX4             0x400   /* 4 texture coordinates */
#define D3DFVF_TEX5             0x500   /* 5 texture coordinates */
#define D3DFVF_TEX6             0x600   /* 6 texture coordinates */
#define D3DFVF_TEX7             0x700   /* 7 texture coordinates */
#define D3DFVF_TEX8             0x800   /* 8 texture coordinates */

#define D3DFVF_TEXCOUNT_MASK    0xF00
#define D3DFVF_TEXCOUNT_SHIFT   8

/* Transform state types */
typedef enum _D3DTRANSFORMSTATETYPE7 {
    D3DTS7_VIEW         = 2,
    D3DTS7_PROJECTION   = 3,
    D3DTS7_WORLD        = 256,
    D3DTS7_WORLD1       = 257,
    D3DTS7_WORLD2       = 258,
    D3DTS7_WORLD3       = 259,
} D3DTRANSFORMSTATETYPE7;

/* Render state types */
typedef enum _D3DRENDERSTATETYPE7 {
    D3DRS7_ZENABLE                  = 7,
    D3DRS7_FILLMODE                 = 8,
    D3DRS7_SHADEMODE                = 9,
    D3DRS7_ZWRITEENABLE             = 14,
    D3DRS7_ALPHATESTENABLE          = 15,
    D3DRS7_SRCBLEND                 = 19,
    D3DRS7_DESTBLEND                = 20,
    D3DRS7_CULLMODE                 = 22,
    D3DRS7_ZFUNC                    = 23,
    D3DRS7_ALPHAREF                 = 24,
    D3DRS7_ALPHAFUNC                = 25,
    D3DRS7_DITHERENABLE             = 26,
    D3DRS7_ALPHABLENDENABLE         = 27,
    D3DRS7_FOGENABLE                = 28,
    D3DRS7_SPECULARENABLE           = 29,
    D3DRS7_FOGCOLOR                 = 34,
    D3DRS7_FOGTABLEMODE             = 35,
    D3DRS7_FOGSTART                 = 36,
    D3DRS7_FOGEND                   = 37,
    D3DRS7_FOGDENSITY               = 38,
    D3DRS7_LIGHTING                 = 137,
    D3DRS7_AMBIENT                  = 139,
    D3DRS7_COLORVERTEX              = 141,
} D3DRENDERSTATETYPE7;

/* Texture stage state types */
typedef enum _D3DTEXTURESTAGESTATETYPE7 {
    D3DTSS7_COLOROP         = 1,
    D3DTSS7_COLORARG1       = 2,
    D3DTSS7_COLORARG2       = 3,
    D3DTSS7_ALPHAOP         = 4,
    D3DTSS7_ALPHAARG1       = 5,
    D3DTSS7_ALPHAARG2       = 6,
    D3DTSS7_BUMPENVMAT00    = 7,
    D3DTSS7_BUMPENVMAT01    = 8,
    D3DTSS7_BUMPENVMAT10    = 9,
    D3DTSS7_BUMPENVMAT11    = 10,
    D3DTSS7_TEXCOORDINDEX   = 11,
    D3DTSS7_ADDRESS         = 12,
    D3DTSS7_ADDRESSU        = 13,
    D3DTSS7_ADDRESSV        = 14,
    D3DTSS7_MAGFILTER       = 16,
    D3DTSS7_MINFILTER       = 17,
    D3DTSS7_MIPFILTER       = 18,
    D3DTSS7_MAXANISOTROPY   = 21,
} D3DTEXTURESTAGESTATETYPE7;

/* Texture operations */
typedef enum _D3DTEXTUREOP7 {
    D3DTOP7_DISABLE         = 1,
    D3DTOP7_SELECTARG1      = 2,
    D3DTOP7_SELECTARG2      = 3,
    D3DTOP7_MODULATE        = 4,
    D3DTOP7_MODULATE2X      = 5,
    D3DTOP7_MODULATE4X      = 6,
    D3DTOP7_ADD             = 7,
    D3DTOP7_ADDSIGNED       = 8,
    D3DTOP7_SUBTRACT        = 10,
    D3DTOP7_BLENDDIFFUSEALPHA = 12,
    D3DTOP7_BLENDTEXTUREALPHA = 13,
} D3DTEXTUREOP7;

/* Texture arguments */
#define D3DTA7_DIFFUSE      0x00000000
#define D3DTA7_CURRENT      0x00000001
#define D3DTA7_TEXTURE      0x00000002
#define D3DTA7_TFACTOR      0x00000003
#define D3DTA7_SPECULAR     0x00000004

/* Blend modes */
typedef enum _D3DBLEND7 {
    D3DBLEND7_ZERO              = 1,
    D3DBLEND7_ONE               = 2,
    D3DBLEND7_SRCCOLOR          = 3,
    D3DBLEND7_INVSRCCOLOR       = 4,
    D3DBLEND7_SRCALPHA          = 5,
    D3DBLEND7_INVSRCALPHA       = 6,
    D3DBLEND7_DESTALPHA         = 7,
    D3DBLEND7_INVDESTALPHA      = 8,
    D3DBLEND7_DESTCOLOR         = 9,
    D3DBLEND7_INVDESTCOLOR      = 10,
    D3DBLEND7_SRCALPHASAT       = 11,
} D3DBLEND7;

/* Cull modes */
typedef enum _D3DCULL7 {
    D3DCULL7_NONE       = 1,
    D3DCULL7_CW         = 2,
    D3DCULL7_CCW        = 3,
} D3DCULL7;

/* Compare functions */
typedef enum _D3DCMPFUNC7 {
    D3DCMP7_NEVER           = 1,
    D3DCMP7_LESS            = 2,
    D3DCMP7_EQUAL           = 3,
    D3DCMP7_LESSEQUAL       = 4,
    D3DCMP7_GREATER         = 5,
    D3DCMP7_NOTEQUAL        = 6,
    D3DCMP7_GREATEREQUAL    = 7,
    D3DCMP7_ALWAYS          = 8,
} D3DCMPFUNC7;

/* Light types */
typedef enum _D3DLIGHTTYPE7 {
    D3DLIGHT7_POINT         = 1,
    D3DLIGHT7_SPOT          = 2,
    D3DLIGHT7_DIRECTIONAL   = 3,
} D3DLIGHTTYPE7;

/* --------------------------------------------------------------- */
/*  Direct3D 7 Structures                                          */
/* --------------------------------------------------------------- */

/* Vector3 */
typedef struct _D3DVECTOR7 {
    FLOAT x;
    FLOAT y;
    FLOAT z;
} D3DVECTOR7;

/* Matrix */
typedef struct _D3DMATRIX7 {
    FLOAT _11, _12, _13, _14;
    FLOAT _21, _22, _23, _24;
    FLOAT _31, _32, _33, _34;
    FLOAT _41, _42, _43, _44;
} D3DMATRIX7;

/* Color value */
typedef struct _D3DCOLORVALUE7 {
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
} D3DCOLORVALUE7;

/* Material */
typedef struct _D3DMATERIAL7 {
    D3DCOLORVALUE7 dcvDiffuse;
    D3DCOLORVALUE7 dcvAmbient;
    D3DCOLORVALUE7 dcvSpecular;
    D3DCOLORVALUE7 dcvEmissive;
    FLOAT          dvPower;
} D3DMATERIAL7;

/* Light */
typedef struct _D3DLIGHT7 {
    D3DLIGHTTYPE7   dltType;
    D3DCOLORVALUE7  dcvDiffuse;
    D3DCOLORVALUE7  dcvSpecular;
    D3DCOLORVALUE7  dcvAmbient;
    D3DVECTOR7      dvPosition;
    D3DVECTOR7      dvDirection;
    FLOAT           dvRange;
    FLOAT           dvFalloff;
    FLOAT           dvAttenuation0;
    FLOAT           dvAttenuation1;
    FLOAT           dvAttenuation2;
    FLOAT           dvTheta;
    FLOAT           dvPhi;
} D3DLIGHT7;

/* Viewport */
typedef struct _D3DVIEWPORT7 {
    DWORD dwX;
    DWORD dwY;
    DWORD dwWidth;
    DWORD dwHeight;
    FLOAT dvMinZ;
    FLOAT dvMaxZ;
} D3DVIEWPORT7;

/* Vertex buffer description */
typedef struct _D3DVERTEXBUFFERDESC7 {
    DWORD dwSize;
    DWORD dwCaps;
    DWORD dwFVF;
    DWORD dwNumVertices;
} D3DVERTEXBUFFERDESC7;

/* Device description */
typedef struct _D3DDEVICEDESC7 {
    DWORD dwDevCaps;
    DWORD dwPrimitiveMiscCaps;
    DWORD dwRasterCaps;
    DWORD dwZCmpCaps;
    DWORD dwSrcBlendCaps;
    DWORD dwDestBlendCaps;
    DWORD dwAlphaCmpCaps;
    DWORD dwShadeCaps;
    DWORD dwTextureCaps;
    DWORD dwTextureFilterCaps;
    DWORD dwTextureBlendCaps;
    DWORD dwTextureAddressCaps;
    DWORD dwMaxTextureWidth;
    DWORD dwMaxTextureHeight;
    DWORD dwMaxTextureRepeat;
    DWORD dwMaxTextureAspectRatio;
    DWORD dwMaxAnisotropy;
    DWORD dwMaxVertexW;
    FLOAT dvGuardBandLeft;
    FLOAT dvGuardBandTop;
    FLOAT dvGuardBandRight;
    FLOAT dvGuardBandBottom;
    FLOAT dvExtentsAdjust;
    DWORD dwStencilCaps;
    DWORD dwFVFCaps;
    DWORD dwTextureOpCaps;
    WORD  wMaxTextureBlendStages;
    WORD  wMaxSimultaneousTextures;
    DWORD dwMaxActiveLights;
    FLOAT dvMaxVertexBlendMatrices;
    DWORD dwMaxUserClipPlanes;
    DWORD dwVertexProcessingCaps;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DWORD dwReserved4;
} D3DDEVICEDESC7;

/* --------------------------------------------------------------- */
/*  IDirect3D7 - Main D3D7 interface                               */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D7 "F5049E77-4861-11D2-A407-00A0C90629A8"
ANX_DEFINE_GUID(IID_IDirect3D7,
    0xF5049E77, 0x4861, 0x11D2,
    0xA4, 0x07, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8);

ANX_BEGIN_INTERFACE(IDirect3D7, IUnknown,
    IID_IDirect3D7, ANX_IID_IDirect3D7)

    /* Enumerate devices */
    ANX_IFACE_METHOD(HRESULT, EnumDevices, (
        IN VOID *lpEnumCallback,
        IN VOID *lpUserArg))

    /* Create device */
    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN CONST GUID *rclsid,
        IN IDirectDrawSurface7 *lpDDS,
        OUT IDirect3DDevice7 **lplpD3DDevice))

    /* Create vertex buffer */
    ANX_IFACE_METHOD(HRESULT, CreateVertexBuffer, (
        IN D3DVERTEXBUFFERDESC7 *lpVBDesc,
        OUT IDirect3DVertexBuffer7 **lplpD3DVertexBuffer,
        IN DWORD dwFlags))

ANX_END_INTERFACE(IDirect3D7)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice7 - D3D7 Device interface                       */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice7 "F5049E79-4861-11D2-A407-00A0C90629A8"
ANX_DEFINE_GUID(IID_IDirect3DDevice7,
    0xF5049E79, 0x4861, 0x11D2,
    0xA4, 0x07, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8);

ANX_BEGIN_INTERFACE(IDirect3DDevice7, IUnknown,
    IID_IDirect3DDevice7, ANX_IID_IDirect3DDevice7)

    /* Capability queries */
    ANX_IFACE_METHOD(HRESULT, GetCaps, (
        OUT D3DDEVICEDESC7 *lpD3DDevDesc))

    /* Scene */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))

    /* Clearing */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN DWORD dwCount,
        IN VOID *lpRects,
        IN DWORD dwFlags,
        IN DWORD dwColor,
        IN FLOAT dvZ,
        IN DWORD dwStencil))

    /* Transform */
    ANX_IFACE_METHOD(HRESULT, SetTransform, (
        IN D3DTRANSFORMSTATETYPE7 dtstTransformStateType,
        IN CONST D3DMATRIX7 *lpD3DMatrix))

    ANX_IFACE_METHOD(HRESULT, GetTransform, (
        IN D3DTRANSFORMSTATETYPE7 dtstTransformStateType,
        OUT D3DMATRIX7 *lpD3DMatrix))

    /* Viewport */
    ANX_IFACE_METHOD(HRESULT, SetViewport, (
        IN D3DVIEWPORT7 *lpViewport))

    /* Materials and lighting */
    ANX_IFACE_METHOD(HRESULT, SetMaterial, (
        IN D3DMATERIAL7 *lpMaterial))

    ANX_IFACE_METHOD(HRESULT, SetLight, (
        IN DWORD dwLightIndex,
        IN D3DLIGHT7 *lpLight))

    ANX_IFACE_METHOD(HRESULT, LightEnable, (
        IN DWORD dwLightIndex,
        IN BOOLEAN bEnable))

    /* Render states */
    ANX_IFACE_METHOD(HRESULT, SetRenderState, (
        IN D3DRENDERSTATETYPE7 dwRenderStateType,
        IN DWORD dwRenderState))

    ANX_IFACE_METHOD(HRESULT, GetRenderState, (
        IN D3DRENDERSTATETYPE7 dwRenderStateType,
        OUT DWORD *lpdwRenderState))

    /* Texture stage states */
    ANX_IFACE_METHOD(HRESULT, SetTextureStageState, (
        IN DWORD dwStage,
        IN D3DTEXTURESTAGESTATETYPE7 dwState,
        IN DWORD dwValue))

    /* Texture */
    ANX_IFACE_METHOD(HRESULT, SetTexture, (
        IN DWORD dwStage,
        IN IDirectDrawSurface7 *lpTexture))

    /* Drawing */
    ANX_IFACE_METHOD(HRESULT, DrawPrimitive, (
        IN D3DPRIMITIVETYPE7 dptPrimitiveType,
        IN DWORD dwVertexTypeDesc,
        IN VOID *lpvVertices,
        IN DWORD dwVertexCount,
        IN DWORD dwFlags))

    ANX_IFACE_METHOD(HRESULT, DrawIndexedPrimitive, (
        IN D3DPRIMITIVETYPE7 d3dptPrimitiveType,
        IN DWORD dwVertexTypeDesc,
        IN VOID *lpvVertices,
        IN DWORD dwVertexCount,
        IN UINT16 *lpwIndices,
        IN DWORD dwIndexCount,
        IN DWORD dwFlags))

    ANX_IFACE_METHOD(HRESULT, DrawPrimitiveVB, (
        IN D3DPRIMITIVETYPE7 d3dptPrimitiveType,
        IN IDirect3DVertexBuffer7 *lpd3dVertexBuffer,
        IN DWORD dwStartVertex,
        IN DWORD dwNumVertices,
        IN DWORD dwFlags))

    ANX_IFACE_METHOD(HRESULT, DrawIndexedPrimitiveVB, (
        IN D3DPRIMITIVETYPE7 d3dptPrimitiveType,
        IN IDirect3DVertexBuffer7 *lpd3dVertexBuffer,
        IN DWORD dwStartVertex,
        IN DWORD dwNumVertices,
        IN UINT16 *lpwIndices,
        IN DWORD dwIndexCount,
        IN DWORD dwFlags))

ANX_END_INTERFACE(IDirect3DDevice7)

/* --------------------------------------------------------------- */
/*  IDirect3DVertexBuffer7 - Vertex buffer interface               */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DVertexBuffer7 "F5049E7D-4861-11D2-A407-00A0C90629A8"
ANX_DEFINE_GUID(IID_IDirect3DVertexBuffer7,
    0xF5049E7D, 0x4861, 0x11D2,
    0xA4, 0x07, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8);

ANX_BEGIN_INTERFACE(IDirect3DVertexBuffer7, IUnknown,
    IID_IDirect3DVertexBuffer7, ANX_IID_IDirect3DVertexBuffer7)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN DWORD dwFlags,
        OUT VOID **lplpData,
        IN DWORD *lpdwSize))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

    ANX_IFACE_METHOD(HRESULT, GetVertexBufferDesc, (
        OUT D3DVERTEXBUFFERDESC7 *lpVBDesc))

ANX_END_INTERFACE(IDirect3DVertexBuffer7)

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                */
/* --------------------------------------------------------------- */

#define IDirect3D7_CreateDevice(This, rclsid, lpDDS, lplpD3DDevice) \
    ((This)->lpVtbl->CreateDevice(This, rclsid, lpDDS, lplpD3DDevice))
#define IDirect3D7_CreateVertexBuffer(This, lpVBDesc, lplpD3DVertexBuffer, dwFlags) \
    ((This)->lpVtbl->CreateVertexBuffer(This, lpVBDesc, lplpD3DVertexBuffer, dwFlags))

#define IDirect3DDevice7_BeginScene(This) \
    ((This)->lpVtbl->BeginScene(This))
#define IDirect3DDevice7_EndScene(This) \
    ((This)->lpVtbl->EndScene(This))
#define IDirect3DDevice7_Clear(This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil) \
    ((This)->lpVtbl->Clear(This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil))
#define IDirect3DDevice7_SetTransform(This, dtstTransformStateType, lpD3DMatrix) \
    ((This)->lpVtbl->SetTransform(This, dtstTransformStateType, lpD3DMatrix))
#define IDirect3DDevice7_SetViewport(This, lpViewport) \
    ((This)->lpVtbl->SetViewport(This, lpViewport))
#define IDirect3DDevice7_SetMaterial(This, lpMaterial) \
    ((This)->lpVtbl->SetMaterial(This, lpMaterial))
#define IDirect3DDevice7_SetLight(This, dwLightIndex, lpLight) \
    ((This)->lpVtbl->SetLight(This, dwLightIndex, lpLight))
#define IDirect3DDevice7_LightEnable(This, dwLightIndex, bEnable) \
    ((This)->lpVtbl->LightEnable(This, dwLightIndex, bEnable))
#define IDirect3DDevice7_SetRenderState(This, dwRenderStateType, dwRenderState) \
    ((This)->lpVtbl->SetRenderState(This, dwRenderStateType, dwRenderState))
#define IDirect3DDevice7_SetTextureStageState(This, dwStage, dwState, dwValue) \
    ((This)->lpVtbl->SetTextureStageState(This, dwStage, dwState, dwValue))
#define IDirect3DDevice7_SetTexture(This, dwStage, lpTexture) \
    ((This)->lpVtbl->SetTexture(This, dwStage, lpTexture))
#define IDirect3DDevice7_DrawPrimitive(This, dptPrimitiveType, dwVertexTypeDesc, lpvVertices, dwVertexCount, dwFlags) \
    ((This)->lpVtbl->DrawPrimitive(This, dptPrimitiveType, dwVertexTypeDesc, lpvVertices, dwVertexCount, dwFlags))
#define IDirect3DDevice7_DrawIndexedPrimitive(This, d3dptPrimitiveType, dwVertexTypeDesc, lpvVertices, dwVertexCount, lpwIndices, dwIndexCount, dwFlags) \
    ((This)->lpVtbl->DrawIndexedPrimitive(This, d3dptPrimitiveType, dwVertexTypeDesc, lpvVertices, dwVertexCount, lpwIndices, dwIndexCount, dwFlags))

#define IDirect3DVertexBuffer7_Lock(This, dwFlags, lplpData, lpdwSize) \
    ((This)->lpVtbl->Lock(This, dwFlags, lplpData, lpdwSize))
#define IDirect3DVertexBuffer7_Unlock(This) \
    ((This)->lpVtbl->Unlock(This))

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate7(
    OUT IDirect3D7 **ppDirect3D7
);

#endif /* _ANANKE_D3D7_H_ */
