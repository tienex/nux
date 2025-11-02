/*++
    Module Name:

        d3d9indexbuffer.c

    Abstract:

        Direct3D 9 index buffer implementation wrapping OpenGL ES 2.0 buffers.
        Implements IDirect3DIndexBuffer9 interface with Lock/Unlock for data upload.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <GLES/gl.h>

/* Index format constants */
#define D3DFMT9_INDEX16 101
#define D3DFMT9_INDEX32 102

/* --------------------------------------------------------------- */
/*  D3D9 Index Buffer Object                                       */
/* --------------------------------------------------------------- */

typedef struct _D3D9_INDEX_BUFFER {
    IDirect3DIndexBuffer9Vtbl *lpVtbl;
    UINT32                     RefCount;

    IGLDevice                 *GlDevice;
    IGLBuffer                 *GlBuffer;

    UINT32                     Length;
    DWORD                      Usage;
    D3DFORMAT                  Format;   /* D3DFMT_INDEX16 or D3DFMT_INDEX32 */

    /* CPU-side copy for Lock/Unlock */
    VOID                      *pData;
    BOOLEAN                    IsLocked;
    UINT32                     LockOffset;
    UINT32                     LockSize;
    DWORD                      LockFlags;
} D3D9_INDEX_BUFFER;

/* --------------------------------------------------------------- */
/*  IDirect3DIndexBuffer9 Implementation                           */
/* --------------------------------------------------------------- */

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
        if (ib->pData) {
            RtlFreeMemory(ib->pData);
        }
        RtlFreeMemory(ib);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_GetDevice(
    IDirect3DIndexBuffer9 *This,
    IDirect3DDevice9 **ppDevice)
{
    if (!ppDevice) return E_POINTER;
    *ppDevice = NULL;
    return E_NOTIMPL;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_SetPrivateData(
    IDirect3DIndexBuffer9 *This,
    REFGUID refguid,
    CONST void *pData,
    DWORD SizeOfData,
    DWORD Flags)
{
    return S_OK;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_GetPrivateData(
    IDirect3DIndexBuffer9 *This,
    REFGUID refguid,
    void *pData,
    DWORD *pSizeOfData)
{
    return E_NOTIMPL;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_FreePrivateData(
    IDirect3DIndexBuffer9 *This,
    REFGUID refguid)
{
    return S_OK;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D9IndexBuffer_SetPriority(
    IDirect3DIndexBuffer9 *This,
    DWORD PriorityNew)
{
    return 0;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D9IndexBuffer_GetPriority(IDirect3DIndexBuffer9 *This)
{
    return 0;  /* Stub */
}

static void STDMETHODCALLTYPE
D3D9IndexBuffer_PreLoad(IDirect3DIndexBuffer9 *This)
{
    /* Stub - buffers uploaded on Unlock */
}

static D3DRESOURCETYPE STDMETHODCALLTYPE
D3D9IndexBuffer_GetType(IDirect3DIndexBuffer9 *This)
{
    return D3DRTYPE_INDEXBUFFER;
}

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_Lock(
    IDirect3DIndexBuffer9 *This,
    UINT OffsetToLock,
    UINT SizeToLock,
    void **ppbData,
    DWORD Flags)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;

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
D3D9IndexBuffer_Unlock(IDirect3DIndexBuffer9 *This)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;
    GLenum usage;

    if (!ib->IsLocked) return E_FAIL;
    if (!ib->pData) return E_FAIL;

    /* Determine GL buffer usage hint */
    if (ib->Usage & D3DUSAGE_DYNAMIC) {
        usage = GL_DYNAMIC_DRAW;
    } else if (ib->Usage & D3DUSAGE_WRITEONLY) {
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

static HRESULT STDMETHODCALLTYPE
D3D9IndexBuffer_GetDesc(
    IDirect3DIndexBuffer9 *This,
    D3DINDEXBUFFER_DESC *pDesc)
{
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)This;

    if (!pDesc) return E_POINTER;

    pDesc->Format = ib->Format;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = ib->Usage;
    pDesc->Pool = D3DPOOL_MANAGED;  /* Default pool */
    pDesc->Size = ib->Length;

    return S_OK;
}

static IDirect3DIndexBuffer9Vtbl D3D9IndexBufferVtbl = {
    .QueryInterface  = D3D9IndexBuffer_QueryInterface,
    .AddRef          = D3D9IndexBuffer_AddRef,
    .Release         = D3D9IndexBuffer_Release,
    .GetDevice       = D3D9IndexBuffer_GetDevice,
    .SetPrivateData  = D3D9IndexBuffer_SetPrivateData,
    .GetPrivateData  = D3D9IndexBuffer_GetPrivateData,
    .FreePrivateData = D3D9IndexBuffer_FreePrivateData,
    .SetPriority     = D3D9IndexBuffer_SetPriority,
    .GetPriority     = D3D9IndexBuffer_GetPriority,
    .PreLoad         = D3D9IndexBuffer_PreLoad,
    .GetType         = D3D9IndexBuffer_GetType,
    .Lock            = D3D9IndexBuffer_Lock,
    .Unlock          = D3D9IndexBuffer_Unlock,
    .GetDesc         = D3D9IndexBuffer_GetDesc,
};

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
D3D9CreateIndexBuffer(
    IGLDevice *pGLDevice,
    UINT Length,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DIndexBuffer9 **ppIndexBuffer,
    HANDLE *pSharedHandle)
{
    D3D9_INDEX_BUFFER *ib;
    HRESULT hr;

    if (!pGLDevice || !ppIndexBuffer) return E_POINTER;
    if (Length == 0) return E_INVALIDARG;

    /* Allocate index buffer object */
    ib = (D3D9_INDEX_BUFFER*)RtlAllocateMemory(sizeof(D3D9_INDEX_BUFFER));
    if (!ib) return E_OUTOFMEMORY;

    RtlZeroMemory(ib, sizeof(D3D9_INDEX_BUFFER));
    ib->lpVtbl = &D3D9IndexBufferVtbl;
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

    *ppIndexBuffer = (IDirect3DIndexBuffer9*)ib;
    if (pSharedHandle) *pSharedHandle = NULL;  /* Not supported */
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper - Get IGLBuffer from index buffer                       */
/* --------------------------------------------------------------- */

HRESULT
D3D9GetIndexBufferGLBuffer(
    IDirect3DIndexBuffer9 *pIndexBuffer,
    IGLBuffer **ppGLBuffer)
{
    D3D9_INDEX_BUFFER *ib;

    if (!pIndexBuffer || !ppGLBuffer) return E_POINTER;

    ib = (D3D9_INDEX_BUFFER*)pIndexBuffer;
    *ppGLBuffer = ib->GlBuffer;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper - Get index format (16-bit or 32-bit)                   */
/* --------------------------------------------------------------- */

HRESULT
D3D9GetIndexBufferFormat(
    IDirect3DIndexBuffer9 *pIndexBuffer,
    GLenum *pGLFormat)
{
    D3D9_INDEX_BUFFER *ib;

    if (!pIndexBuffer || !pGLFormat) return E_POINTER;

    ib = (D3D9_INDEX_BUFFER*)pIndexBuffer;

    /* Convert D3D format to GL type */
    if (ib->Format == D3DFMT9_INDEX16) {
        *pGLFormat = GL_UNSIGNED_SHORT;
    } else if (ib->Format == D3DFMT9_INDEX32) {
        *pGLFormat = GL_UNSIGNED_INT;
    } else {
        /* Default to 16-bit */
        *pGLFormat = GL_UNSIGNED_SHORT;
    }

    return S_OK;
}
