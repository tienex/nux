/*++
    Module Name:

        shader.c

    Abstract:

        Direct3D 9 shader implementation.
        Handles vertex and pixel shader objects.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <ananke/ntrtl.h>
#include "d3d9_internal.h"

/* --------------------------------------------------------------- */
/*  Vertex Shader Implementation                                   */
/* --------------------------------------------------------------- */

typedef struct _D3D9_VERTEX_SHADER {
    IDirect3DVertexShader9Vtbl *lpVtbl;
    UINT32                      RefCount;
    IGLShader                  *GlShader;
    IGLProgram                 *GlProgram;
} D3D9_VERTEX_SHADER;

static HRESULT STDMETHODCALLTYPE
D3D9VertexShader_QueryInterface(
    IDirect3DVertexShader9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DVertexShader9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexShader_AddRef(IDirect3DVertexShader9 *This)
{
    D3D9_VERTEX_SHADER *shader = (D3D9_VERTEX_SHADER*)This;
    return ++shader->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexShader_Release(IDirect3DVertexShader9 *This)
{
    D3D9_VERTEX_SHADER *shader = (D3D9_VERTEX_SHADER*)This;
    UINT32 refCount = --shader->RefCount;

    if (refCount == 0) {
        if (shader->GlShader) {
            IUnknown_Release((IUnknown*)shader->GlShader);
        }
        if (shader->GlProgram) {
            IUnknown_Release((IUnknown*)shader->GlProgram);
        }
        RtlFreeMemory(shader);
    }

    return refCount;
}

static IDirect3DVertexShader9Vtbl D3D9VertexShaderVtbl = {
    .QueryInterface = D3D9VertexShader_QueryInterface,
    .AddRef         = D3D9VertexShader_AddRef,
    .Release        = D3D9VertexShader_Release,
};

/* Internal function to create vertex shader */
HRESULT
D3D9CreateVertexShader(
    IGLDevice *GlDevice,
    CONST UINT32 *pFunction,
    IDirect3DVertexShader9 **ppShader)
{
    D3D9_VERTEX_SHADER *shader;
    HRESULT hr;

    if (!ppShader) return E_POINTER;

    shader = (D3D9_VERTEX_SHADER*)RtlAllocateMemory(sizeof(D3D9_VERTEX_SHADER));
    if (!shader) return E_OUTOFMEMORY;

    RtlZeroMemory(shader, sizeof(D3D9_VERTEX_SHADER));
    shader->lpVtbl = &D3D9VertexShaderVtbl;
    shader->RefCount = 1;

    /* Translate and compile the shader */
    hr = D3D9CreateGLShader(
        GlDevice,
        pFunction,
        TRUE,  /* IsVertexShader */
        &shader->GlShader,
        &shader->GlProgram);

    if (FAILED(hr)) {
        RtlFreeMemory(shader);
        return hr;
    }

    *ppShader = (IDirect3DVertexShader9*)shader;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Pixel Shader Implementation                                    */
/* --------------------------------------------------------------- */

typedef struct _D3D9_PIXEL_SHADER {
    IDirect3DPixelShader9Vtbl *lpVtbl;
    UINT32                     RefCount;
    IGLShader                 *GlShader;
    IGLProgram                *GlProgram;
} D3D9_PIXEL_SHADER;

static HRESULT STDMETHODCALLTYPE
D3D9PixelShader_QueryInterface(
    IDirect3DPixelShader9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DPixelShader9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9PixelShader_AddRef(IDirect3DPixelShader9 *This)
{
    D3D9_PIXEL_SHADER *shader = (D3D9_PIXEL_SHADER*)This;
    return ++shader->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9PixelShader_Release(IDirect3DPixelShader9 *This)
{
    D3D9_PIXEL_SHADER *shader = (D3D9_PIXEL_SHADER*)This;
    UINT32 refCount = --shader->RefCount;

    if (refCount == 0) {
        if (shader->GlShader) {
            IUnknown_Release((IUnknown*)shader->GlShader);
        }
        if (shader->GlProgram) {
            IUnknown_Release((IUnknown*)shader->GlProgram);
        }
        RtlFreeMemory(shader);
    }

    return refCount;
}

static IDirect3DPixelShader9Vtbl D3D9PixelShaderVtbl = {
    .QueryInterface = D3D9PixelShader_QueryInterface,
    .AddRef         = D3D9PixelShader_AddRef,
    .Release        = D3D9PixelShader_Release,
};

/* Internal function to create pixel shader */
HRESULT
D3D9CreatePixelShader(
    IGLDevice *GlDevice,
    CONST UINT32 *pFunction,
    IDirect3DPixelShader9 **ppShader)
{
    D3D9_PIXEL_SHADER *shader;
    HRESULT hr;

    if (!ppShader) return E_POINTER;

    shader = (D3D9_PIXEL_SHADER*)RtlAllocateMemory(sizeof(D3D9_PIXEL_SHADER));
    if (!shader) return E_OUTOFMEMORY;

    RtlZeroMemory(shader, sizeof(D3D9_PIXEL_SHADER));
    shader->lpVtbl = &D3D9PixelShaderVtbl;
    shader->RefCount = 1;

    /* Translate and compile the shader */
    hr = D3D9CreateGLShader(
        GlDevice,
        pFunction,
        FALSE,  /* IsVertexShader */
        &shader->GlShader,
        &shader->GlProgram);

    if (FAILED(hr)) {
        RtlFreeMemory(shader);
        return hr;
    }

    *ppShader = (IDirect3DPixelShader9*)shader;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Vertex Declaration Implementation                              */
/* --------------------------------------------------------------- */

typedef struct _D3D9_VERTEX_DECLARATION {
    IDirect3DVertexDeclaration9Vtbl *lpVtbl;
    UINT32                           RefCount;
    D3DVERTEXELEMENT9               *Elements;
    UINT32                           ElementCount;
} D3D9_VERTEX_DECLARATION;

static HRESULT STDMETHODCALLTYPE
D3D9VertexDeclaration_QueryInterface(
    IDirect3DVertexDeclaration9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DVertexDeclaration9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexDeclaration_AddRef(IDirect3DVertexDeclaration9 *This)
{
    D3D9_VERTEX_DECLARATION *decl = (D3D9_VERTEX_DECLARATION*)This;
    return ++decl->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexDeclaration_Release(IDirect3DVertexDeclaration9 *This)
{
    D3D9_VERTEX_DECLARATION *decl = (D3D9_VERTEX_DECLARATION*)This;
    UINT32 refCount = --decl->RefCount;

    if (refCount == 0) {
        if (decl->Elements) {
            RtlFreeMemory(decl->Elements);
        }
        RtlFreeMemory(decl);
    }

    return refCount;
}

static IDirect3DVertexDeclaration9Vtbl D3D9VertexDeclarationVtbl = {
    .QueryInterface = D3D9VertexDeclaration_QueryInterface,
    .AddRef         = D3D9VertexDeclaration_AddRef,
    .Release        = D3D9VertexDeclaration_Release,
};

/* Internal function to create vertex declaration */
HRESULT
D3D9CreateVertexDeclaration(
    CONST D3DVERTEXELEMENT9 *pVertexElements,
    IDirect3DVertexDeclaration9 **ppDecl)
{
    D3D9_VERTEX_DECLARATION *decl;
    UINT32 count = 0;

    if (!ppDecl) return E_POINTER;

    /* Count elements */
    while (pVertexElements[count].Stream != 0xFF) {
        count++;
    }
    count++; /* Include terminator */

    decl = (D3D9_VERTEX_DECLARATION*)RtlAllocateMemory(sizeof(D3D9_VERTEX_DECLARATION));
    if (!decl) return E_OUTOFMEMORY;

    decl->Elements = (D3DVERTEXELEMENT9*)RtlAllocateMemory(count * sizeof(D3DVERTEXELEMENT9));
    if (!decl->Elements) {
        RtlFreeMemory(decl);
        return E_OUTOFMEMORY;
    }

    RtlZeroMemory(decl, sizeof(D3D9_VERTEX_DECLARATION));
    decl->lpVtbl = &D3D9VertexDeclarationVtbl;
    decl->RefCount = 1;
    decl->ElementCount = count;

    RtlCopyMemory(decl->Elements, pVertexElements, count * sizeof(D3DVERTEXELEMENT9));

    *ppDecl = (IDirect3DVertexDeclaration9*)decl;
    return S_OK;
}
