/*++
    Module Name:

        d3d3.h

    Abstract:

        Direct3D 1-3 emulation library using OpenGL ES 2.0 backend.
        Immediate mode rendering and execute buffer emulation.

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

typedef struct IDirect3D IDirect3D;
typedef struct IDirect3DDevice IDirect3DDevice;
typedef struct IDirect3DExecuteBuffer IDirect3DExecuteBuffer;

/* --------------------------------------------------------------- */
/*  D3D3 Types (Oldest API)                                        */
/* --------------------------------------------------------------- */

/* Primitive types */
typedef enum _D3DPT3 {
    D3DPT3_TRIANGLELIST = 4,
} D3DPT3;

/* Transform states */
typedef enum _D3DTS3 {
    D3DTS3_WORLD      = 1,
    D3DTS3_VIEW       = 2,
    D3DTS3_PROJECTION = 3,
} D3DTS3;

/* Matrix */
typedef struct _D3DMATRIX3 {
    FLOAT _11, _12, _13, _14;
    FLOAT _21, _22, _23, _24;
    FLOAT _31, _32, _33, _34;
    FLOAT _41, _42, _43, _44;
} D3DMATRIX3;

/* Vertex (for immediate mode) */
typedef struct _D3DVERTEX3 {
    FLOAT x, y, z;
    FLOAT nx, ny, nz;
    FLOAT tu, tv;
} D3DVERTEX3;

/* Execute buffer description */
typedef struct _D3DEXECUTEBUFFERDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwCaps;
    DWORD dwBufferSize;
    VOID *lpData;
} D3DEXECUTEBUFFERDESC;

/* --------------------------------------------------------------- */
/*  IDirect3D Interface                                            */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3D "3BBA0080-2421-11CF-A31A-00AA00B93356"
ANX_DEFINE_GUID(IID_IDirect3D,
    0x3BBA0080, 0x2421, 0x11CF,
    0xA3, 0x1A, 0x00, 0xAA, 0x00, 0xB9, 0x33, 0x56);

ANX_BEGIN_INTERFACE(IDirect3D, IUnknown,
    IID_IDirect3D, ANX_IID_IDirect3D)

    ANX_IFACE_METHOD(HRESULT, CreateDevice, (
        IN CONST GUID *rclsid,
        IN VOID *lpSurface,
        OUT IDirect3DDevice **lplpDirect3DDevice))

ANX_END_INTERFACE(IDirect3D)

/* --------------------------------------------------------------- */
/*  IDirect3DDevice Interface                                      */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DDevice "64108800-957D-11D0-89AB-00A0C9054129"
ANX_DEFINE_GUID(IID_IDirect3DDevice,
    0x64108800, 0x957D, 0x11D0,
    0x89, 0xAB, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29);

ANX_BEGIN_INTERFACE(IDirect3DDevice, IUnknown,
    IID_IDirect3DDevice, ANX_IID_IDirect3DDevice)

    /* Scene */
    ANX_IFACE_METHOD(HRESULT, BeginScene, (VOID))
    ANX_IFACE_METHOD(HRESULT, EndScene, (VOID))

    /* Transform */
    ANX_IFACE_METHOD(HRESULT, SetTransform, (
        IN D3DTS3 dtstTransformStateType,
        IN D3DMATRIX3 *lpD3DMatrix))

    /* Execute buffer (legacy) */
    ANX_IFACE_METHOD(HRESULT, CreateExecuteBuffer, (
        IN D3DEXECUTEBUFFERDESC *lpDesc,
        OUT IDirect3DExecuteBuffer **lplpExecuteBuffer,
        IN VOID *pUnkOuter))

    ANX_IFACE_METHOD(HRESULT, Execute, (
        IN IDirect3DExecuteBuffer *lpExecuteBuffer,
        IN VOID *lpViewport,
        IN DWORD dwFlags))

    /* Immediate mode (simplified) */
    ANX_IFACE_METHOD(HRESULT, DrawPrimitiveImmediate, (
        IN D3DPT3 primitiveType,
        IN D3DVERTEX3 *vertices,
        IN DWORD vertexCount))

ANX_END_INTERFACE(IDirect3DDevice)

/* --------------------------------------------------------------- */
/*  IDirect3DExecuteBuffer Interface                               */
/* --------------------------------------------------------------- */

#define ANX_IID_IDirect3DExecuteBuffer "4FD05BE0-FA3E-11D0-9B6D-0000C0781BC3"
ANX_DEFINE_GUID(IID_IDirect3DExecuteBuffer,
    0x4FD05BE0, 0xFA3E, 0x11D0,
    0x9B, 0x6D, 0x00, 0x00, 0xC0, 0x78, 0x1B, 0xC3);

ANX_BEGIN_INTERFACE(IDirect3DExecuteBuffer, IUnknown,
    IID_IDirect3DExecuteBuffer, ANX_IID_IDirect3DExecuteBuffer)

    ANX_IFACE_METHOD(HRESULT, Lock, (
        IN D3DEXECUTEBUFFERDESC *lpDesc))

    ANX_IFACE_METHOD(HRESULT, Unlock, (VOID))

ANX_END_INTERFACE(IDirect3DExecuteBuffer)

/* --------------------------------------------------------------- */
/*  C Helper Macros                                                */
/* --------------------------------------------------------------- */

#define IDirect3DDevice_BeginScene(p)       ((p)->lpVtbl->BeginScene(p))
#define IDirect3DDevice_EndScene(p)         ((p)->lpVtbl->EndScene(p))
#define IDirect3DDevice_SetTransform(p,a,b) ((p)->lpVtbl->SetTransform(p,a,b))
#define IDirect3DDevice_DrawPrimitiveImmediate(p,a,b,c) ((p)->lpVtbl->DrawPrimitiveImmediate(p,a,b,c))

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate3(
    OUT IDirect3D **ppDirect3D
);

#endif /* _ANANKE_D3D3_H_ */
