/*++
    Module Name:

        d3d8indexbuffer.c

    Abstract:

        Direct3D 8 index buffer implementation wrapping OpenGL ES 2.0 buffers.
        Implements IDirect3DIndexBuffer8 interface with Lock/Unlock for data upload.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d8.h>
#include <ananke/gles20com.h>
#include <GLES/gl.h>

/* Index format constants (D3DFMT_INDEX16/INDEX32) */
#define D3DFMT8_INDEX16 101
#define D3DFMT8_INDEX32 102

/* --------------------------------------------------------------- */
/*  D3D8 Index Buffer Object                                       */
/* --------------------------------------------------------------- */

typedef struct _D3D8_INDEX_BUFFER {
    IDirect3DIndexBuffer8Vtbl *lpVtbl;
    UINT32                     RefCount;

    IGLDevice                 *GlDevice;
    IGLBuffer                 *GlBuffer;

    UINT32                     Length;
    DWORD                      Usage;
    DWORD                      Format;   /* D3DFMT_INDEX16 or D3DFMT_INDEX32 */

    /* CPU-side copy for Lock/Unlock */
    VOID                      *pData;
    BOOLEAN                    IsLocked;
    UINT32                     LockOffset;
    UINT32                     LockSize;
    DWORD                      LockFlags;
} D3D8_INDEX_BUFFER;

/* --------------------------------------------------------------- */
/*  IDirect3DIndexBuffer8 Implementation                           */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D8IndexBuffer_QueryInterface(
    IDirect3DIndexBuffer8 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DIndexBuffer8))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D8IndexBuffer_AddRef(IDirect3DIndexBuffer8 *This)
{
    D3D8_INDEX_BUFFER *ib = (D3D8_INDEX_BUFFER*)This;
    return ++ib->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D8IndexBuffer_Release(IDirect3DIndexBuffer8 *This)
{
    D3D8_INDEX_BUFFER *ib = (D3D8_INDEX_BUFFER*)This;
    UINT32 refCount = --ib->RefCount;

    if (refCount == 0) {
        if (ib->GlBuffer) {
            IUnknown_Release((IUnknown*)ib->GlBuffer);
        }
        if (ib->pData) {
            RtlFreeMemory(ib->pData);
        }
        RtlFreeMemory(ib);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D8IndexBuffer_Lock(
    IDirect3DIndexBuffer8 *This,
    UINT OffsetToLock,
    UINT SizeToLock,
    BYTE **ppbData,
    DWORD Flags)
{
    D3D8_INDEX_BUFFER *ib = (D3D8_INDEX_BUFFER*)This;

    if (!ppbData) return E_POINTER;
    if (ib->IsLocked) return E_FAIL;

    /* If SizeToLock is 0, lock entire buffer */
    if (SizeToLock == 0) {
        SizeToLock = ib->Length - OffsetToLock;
    }

    /* Validate range */
    if (OffsetToLock + SizeToLock > ib->Length) {
        return E_INVALIDARG;
    }

    /* Ensure CPU-side buffer exists */
    if (!ib->pData) {
        ib->pData = RtlAllocateMemory(ib->Length);
        if (!ib->pData) {
            return E_OUTOFMEMORY;
        }
        RtlZeroMemory(ib->pData, ib->Length);
    }

    /* Return pointer to locked region */
    *ppbData = (BYTE*)ib->pData + OffsetToLock;

    ib->IsLocked = TRUE;
    ib->LockOffset = OffsetToLock;
    ib->LockSize = SizeToLock;
    ib->LockFlags = Flags;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8IndexBuffer_Unlock(IDirect3DIndexBuffer8 *This)
{
    D3D8_INDEX_BUFFER *ib = (D3D8_INDEX_BUFFER*)This;
    GLenum usage;

    if (!ib->IsLocked) return E_FAIL;
    if (!ib->pData) return E_FAIL;

    /* Determine GL buffer usage hint (same as vertex buffers) */
    if (ib->Usage & 0x00000200L /* D3DUSAGE_DYNAMIC */) {
        usage = GL_DYNAMIC_DRAW;
    } else if (ib->Usage & 0x00000008L /* D3DUSAGE_WRITEONLY */) {
        usage = GL_STREAM_DRAW;
    } else {
        usage = GL_STATIC_DRAW;
    }

    /* Upload buffer data to GPU via GL_ELEMENT_ARRAY_BUFFER */
    IGLBuffer_Bind(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER);

    if (ib->LockOffset == 0 && ib->LockSize == ib->Length) {
        /* Upload entire buffer */
        IGLBuffer_BufferData(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER,
                             ib->Length, ib->pData, usage);
    } else {
        /* Partial update - use BufferSubData */
        IGLBuffer_BufferSubData(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER,
                                ib->LockOffset, ib->LockSize,
                                (BYTE*)ib->pData + ib->LockOffset);
    }

    ib->IsLocked = FALSE;

    return S_OK;
}

static IDirect3DIndexBuffer8Vtbl D3D8IndexBufferVtbl = {
    .QueryInterface  = D3D8IndexBuffer_QueryInterface,
    .AddRef          = D3D8IndexBuffer_AddRef,
    .Release         = D3D8IndexBuffer_Release,
    .Lock            = D3D8IndexBuffer_Lock,
    .Unlock          = D3D8IndexBuffer_Unlock,
};

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
D3D8CreateIndexBuffer(
    IGLDevice *pGLDevice,
    UINT Length,
    DWORD Usage,
    DWORD Format,
    IDirect3DIndexBuffer8 **ppIndexBuffer)
{
    D3D8_INDEX_BUFFER *ib;
    HRESULT hr;

    if (!pGLDevice || !ppIndexBuffer) return E_POINTER;
    if (Length == 0) return E_INVALIDARG;

    /* Allocate index buffer object */
    ib = (D3D8_INDEX_BUFFER*)RtlAllocateMemory(sizeof(D3D8_INDEX_BUFFER));
    if (!ib) return E_OUTOFMEMORY;

    RtlZeroMemory(ib, sizeof(D3D8_INDEX_BUFFER));
    ib->lpVtbl = &D3D8IndexBufferVtbl;
    ib->RefCount = 1;
    ib->GlDevice = pGLDevice;
    ib->Length = Length;
    ib->Usage = Usage;
    ib->Format = Format;

    /* Create OpenGL buffer object */
    hr = IGLDevice_CreateBuffer(pGLDevice, &ib->GlBuffer);
    if (FAILED(hr)) {
        RtlFreeMemory(ib);
        return hr;
    }

    *ppIndexBuffer = (IDirect3DIndexBuffer8*)ib;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper - Get IGLBuffer from index buffer                       */
/* --------------------------------------------------------------- */

HRESULT
D3D8GetIndexBufferGLBuffer(
    IDirect3DIndexBuffer8 *pIndexBuffer,
    IGLBuffer **ppGLBuffer)
{
    D3D8_INDEX_BUFFER *ib;

    if (!pIndexBuffer || !ppGLBuffer) return E_POINTER;

    ib = (D3D8_INDEX_BUFFER*)pIndexBuffer;
    *ppGLBuffer = ib->GlBuffer;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper - Get index format (16-bit or 32-bit)                   */
/* --------------------------------------------------------------- */

HRESULT
D3D8GetIndexBufferFormat(
    IDirect3DIndexBuffer8 *pIndexBuffer,
    GLenum *pGLFormat)
{
    D3D8_INDEX_BUFFER *ib;

    if (!pIndexBuffer || !pGLFormat) return E_POINTER;

    ib = (D3D8_INDEX_BUFFER*)pIndexBuffer;

    /* Convert D3D format to GL type */
    if (ib->Format == D3DFMT8_INDEX16) {
        *pGLFormat = GL_UNSIGNED_SHORT;
    } else if (ib->Format == D3DFMT8_INDEX32) {
        *pGLFormat = GL_UNSIGNED_INT;
    } else {
        /* Default to 16-bit */
        *pGLFormat = GL_UNSIGNED_SHORT;
    }

    return S_OK;
}
