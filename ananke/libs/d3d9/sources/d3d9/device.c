/*++
    Module Name:

        device.c

    Abstract:

        Direct3D 9 device implementation.
        Maps D3D9 rendering calls to OpenGL ES 2.0 backend.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <ananke/ntrtl.h>
#include <GLES/gl.h>

/* --------------------------------------------------------------- */
/*  Internal Structures                                            */
/* --------------------------------------------------------------- */

typedef struct _D3D9_DEVICE {
    IDirect3DDevice9Vtbl *lpVtbl;
    UINT32                RefCount;
    IGLDevice            *GlDevice;
    IGLContext           *GlContext;

    /* Current state */
    IDirect3DVertexBuffer9       *CurrentVertexBuffer;
    IDirect3DIndexBuffer9        *CurrentIndexBuffer;
    IDirect3DVertexShader9       *CurrentVertexShader;
    IDirect3DPixelShader9        *CurrentPixelShader;
    IDirect3DVertexDeclaration9  *CurrentVertexDeclaration;
    IDirect3DTexture9            *CurrentTextures[8];

    UINT32                CurrentStride;
    UINT32                CurrentOffset;

    /* Presentation parameters */
    D3DPRESENT_PARAMETERS PresentParams;
} D3D9_DEVICE;

typedef struct _D3D9_MAIN {
    IDirect3D9Vtbl *lpVtbl;
    UINT32          RefCount;
} D3D9_MAIN;

/* --------------------------------------------------------------- */
/*  Vertex Buffer Implementation                                   */
/* --------------------------------------------------------------- */

typedef struct _D3D9_VERTEX_BUFFER {
    IDirect3DVertexBuffer9Vtbl *lpVtbl;
    UINT32                      RefCount;
    IGLBuffer                  *GlBuffer;
    UINT32                      Length;
    VOID                       *LocalCopy;
} D3D9_VERTEX_BUFFER;

static HRESULT STDMETHODCALLTYPE
D3D9VertexBuffer_QueryInterface(
    IDirect3DVertexBuffer9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DVertexBuffer9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexBuffer_AddRef(IDirect3DVertexBuffer9 *This)
{
    D3D9_VERTEX_BUFFER *vb = (D3D9_VERTEX_BUFFER*)This;
    return ++vb->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9VertexBuffer_Release(IDirect3DVertexBuffer9 *This)
{
    D3D9_VERTEX_BUFFER *vb = (D3D9_VERTEX_BUFFER*)This;
    UINT32 refCount = --vb->RefCount;

    if (refCount == 0) {
        if (vb->GlBuffer) {
            IUnknown_Release((IUnknown*)vb->GlBuffer);
        }
        if (vb->LocalCopy) {
            RtlFreeMemory(vb->LocalCopy);
        }
        RtlFreeMemory(vb);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9VertexBuffer_Lock(
    IDirect3DVertexBuffer9 *This,
    UINT32 OffsetToLock,
    UINT32 SizeToLock,
    VOID **ppbData,
    UINT32 Flags)
{
    D3D9_VERTEX_BUFFER *vb = (D3D9_VERTEX_BUFFER*)This;

    if (!ppbData) return E_POINTER;

    if (SizeToLock == 0) {
        SizeToLock = vb->Length - OffsetToLock;
    }

    *ppbData = (UINT8*)vb->LocalCopy + OffsetToLock;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9VertexBuffer_Unlock(IDirect3DVertexBuffer9 *This)
{
    D3D9_VERTEX_BUFFER *vb = (D3D9_VERTEX_BUFFER*)This;

    /* Upload to GL buffer */
    IGLBuffer_Bind(vb->GlBuffer, GL_ARRAY_BUFFER);
    IGLBuffer_BufferData(vb->GlBuffer, GL_ARRAY_BUFFER, vb->Length, vb->LocalCopy, GL_STATIC_DRAW);

    return S_OK;
}

static IDirect3DVertexBuffer9Vtbl D3D9VertexBufferVtbl = {
    .QueryInterface = D3D9VertexBuffer_QueryInterface,
    .AddRef         = D3D9VertexBuffer_AddRef,
    .Release        = D3D9VertexBuffer_Release,
    .Lock           = D3D9VertexBuffer_Lock,
    .Unlock         = D3D9VertexBuffer_Unlock,
};

/* --------------------------------------------------------------- */
/*  Index Buffer Implementation                                    */
/* --------------------------------------------------------------- */

typedef struct _D3D9_INDEX_BUFFER {
    IDirect3DIndexBuffer9Vtbl *lpVtbl;
    UINT32                     RefCount;
    IGLBuffer                 *GlBuffer;
    UINT32                     Length;
    VOID                      *LocalCopy;
} D3D9_INDEX_BUFFER;

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_QueryInterface(
    IDirect3DIndexBuffer9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DIndexBuffer9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9IndexBuffer_AddRef(IDirect3DIndexBuffer9 *This)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;
    return ++ib->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9IndexBuffer_Release(IDirect3DIndexBuffer9 *This)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;
    UINT32 refCount = --ib->RefCount;

    if (refCount == 0) {
        if (ib->GlBuffer) {
            IUnknown_Release((IUnknown*)ib->GlBuffer);
        }
        if (ib->LocalCopy) {
            RtlFreeMemory(ib->LocalCopy);
        }
        RtlFreeMemory(ib);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_Lock(
    IDirect3DIndexBuffer9 *This,
    UINT32 OffsetToLock,
    UINT32 SizeToLock,
    VOID **ppbData,
    UINT32 Flags)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;

    if (!ppbData) return E_POINTER;

    if (SizeToLock == 0) {
        SizeToLock = ib->Length - OffsetToLock;
    }

    *ppbData = (UINT8*)ib->LocalCopy + OffsetToLock;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_Unlock(IDirect3DIndexBuffer9 *This)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;

    /* Upload to GL buffer */
    IGLBuffer_Bind(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER);
    IGLBuffer_BufferData(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER, ib->Length, ib->LocalCopy, GL_STATIC_DRAW);

    return S_OK;
}

static IDirect3DIndexBuffer9Vtbl D3D9IndexBufferVtbl = {
    .QueryInterface = D3D9IndexBuffer_QueryInterface,
    .AddRef         = D3D9IndexBuffer_AddRef,
    .Release        = D3D9IndexBuffer_Release,
    .Lock           = D3D9IndexBuffer_Lock,
    .Unlock         = D3D9IndexBuffer_Unlock,
};

/* --------------------------------------------------------------- */
/*  Texture Implementation                                         */
/* --------------------------------------------------------------- */

typedef struct _D3D9_TEXTURE {
    IDirect3DTexture9Vtbl *lpVtbl;
    UINT32                 RefCount;
    IGLTexture            *GlTexture;
    UINT32                 Width;
    UINT32                 Height;
    D3DFORMAT              Format;
    VOID                  *LocalCopy;
} D3D9_TEXTURE;

static HRESULT STDMETHODCALLTYPE
D3D9Texture_QueryInterface(
    IDirect3DTexture9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DTexture9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9Texture_AddRef(IDirect3DTexture9 *This)
{
    D3D9_TEXTURE *tex = (D3D9_TEXTURE*)This;
    return ++tex->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9Texture_Release(IDirect3DTexture9 *This)
{
    D3D9_TEXTURE *tex = (D3D9_TEXTURE*)This;
    UINT32 refCount = --tex->RefCount;

    if (refCount == 0) {
        if (tex->GlTexture) {
            IUnknown_Release((IUnknown*)tex->GlTexture);
        }
        if (tex->LocalCopy) {
            RtlFreeMemory(tex->LocalCopy);
        }
        RtlFreeMemory(tex);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9Texture_LockRect(
    IDirect3DTexture9 *This,
    UINT32 Level,
    D3DLOCKED_RECT *pLockedRect,
    CONST D3DRECT *pRect,
    UINT32 Flags)
{
    D3D9_TEXTURE *tex = (D3D9_TEXTURE*)This;

    if (!pLockedRect) return E_POINTER;
    if (Level != 0) return E_NOTIMPL; /* Only level 0 supported for now */

    /* Calculate pitch based on format */
    UINT32 bpp = 4; /* Default to 32-bit */
    switch (tex->Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bpp = 4;
            break;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            bpp = 2;
            break;
        case D3DFMT_R8G8B8:
            bpp = 3;
            break;
    }

    pLockedRect->Pitch = tex->Width * bpp;
    pLockedRect->pBits = tex->LocalCopy;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Texture_UnlockRect(
    IDirect3DTexture9 *This,
    UINT32 Level)
{
    D3D9_TEXTURE *tex = (D3D9_TEXTURE*)This;
    GLenum glFormat = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;

    if (Level != 0) return E_NOTIMPL;

    /* Map D3D format to GL format */
    switch (tex->Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            glFormat = GL_RGBA;
            glType = GL_UNSIGNED_BYTE;
            break;
        case D3DFMT_R5G6B5:
            glFormat = GL_RGB;
            glType = GL_UNSIGNED_SHORT_5_6_5;
            break;
        case D3DFMT_R8G8B8:
            glFormat = GL_RGB;
            glType = GL_UNSIGNED_BYTE;
            break;
    }

    /* Upload to GL texture */
    IGLTexture_Bind(tex->GlTexture, GL_TEXTURE_2D);
    IGLTexture_TexImage2D(tex->GlTexture, GL_TEXTURE_2D, 0, glFormat,
                          tex->Width, tex->Height, 0, glFormat, glType,
                          tex->LocalCopy);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Texture_GetLevelDesc(
    IDirect3DTexture9 *This,
    UINT32 Level,
    D3DSURFACE_DESC *pDesc)
{
    D3D9_TEXTURE *tex = (D3D9_TEXTURE*)This;

    if (!pDesc) return E_POINTER;
    if (Level != 0) return E_NOTIMPL;

    pDesc->Format = tex->Format;
    pDesc->Type = D3DRTYPE_TEXTURE;
    pDesc->Usage = 0;
    pDesc->Pool = D3DPOOL_MANAGED;
    pDesc->Width = tex->Width;
    pDesc->Height = tex->Height;

    return S_OK;
}

static IDirect3DTexture9Vtbl D3D9TextureVtbl = {
    .QueryInterface = D3D9Texture_QueryInterface,
    .AddRef         = D3D9Texture_AddRef,
    .Release        = D3D9Texture_Release,
    .LockRect       = D3D9Texture_LockRect,
    .UnlockRect     = D3D9Texture_UnlockRect,
    .GetLevelDesc   = D3D9Texture_GetLevelDesc,
};

/* Continued in next part... */
