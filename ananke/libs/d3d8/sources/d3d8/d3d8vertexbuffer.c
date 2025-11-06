/*++
    Module Name:

        d3d8vertexbuffer.c

    Abstract:

        Direct3D 8 vertex buffer implementation wrapping OpenGL ES 2.0 buffers.
        Implements IDirect3DVertexBuffer8 interface with Lock/Unlock for data upload.

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

/* --------------------------------------------------------------- */
/*  D3D8 Vertex Buffer Object                                      */
/* --------------------------------------------------------------- */

typedef struct _D3D8_VERTEX_BUFFER {
    IDirect3DVertexBuffer8Vtbl *lpVtbl;
    UINT32                      RefCount;

    IGLDevice                  *GlDevice;
    IGLBuffer                  *GlBuffer;

    UINT32                      Length;
    DWORD                       Usage;
    DWORD                       FVF;
    D3DPOOL8                    Pool;

    /* CPU-side copy for Lock/Unlock (for D3DPOOL_MANAGED/SYSTEMMEM) */
    VOID                       *pData;
    BOOLEAN                     IsLocked;
    UINT32                      LockOffset;
    UINT32                      LockSize;
    DWORD                       LockFlags;
} D3D8_VERTEX_BUFFER;

/* --------------------------------------------------------------- */
/*  IDirect3DVertexBuffer8 Implementation                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_QueryInterface(
    IDirect3DVertexBuffer8 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DVertexBuffer8))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D8VertexBuffer_AddRef(IDirect3DVertexBuffer8 *This)
{
    D3D8_VERTEX_BUFFER *vb = (D3D8_VERTEX_BUFFER*)This;
    return ++vb->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D8VertexBuffer_Release(IDirect3DVertexBuffer8 *This)
{
    D3D8_VERTEX_BUFFER *vb = (D3D8_VERTEX_BUFFER*)This;
    UINT32 refCount = --vb->RefCount;

    if (refCount == 0) {
        if (vb->GlBuffer) {
            IUnknown_Release((IUnknown*)vb->GlBuffer);
        }
        if (vb->pData) {
            RtlFreeMemory(vb->pData);
        }
        RtlFreeMemory(vb);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_GetDevice(
    IDirect3DVertexBuffer8 *This,
    IDirect3DDevice8 **ppDevice)
{
    if (!ppDevice) return E_POINTER;
    *ppDevice = NULL;
    return E_NOTIMPL;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_SetPrivateData(
    IDirect3DVertexBuffer8 *This,
    REFGUID refguid,
    CONST void *pData,
    DWORD SizeOfData,
    DWORD Flags)
{
    return S_OK;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_GetPrivateData(
    IDirect3DVertexBuffer8 *This,
    REFGUID refguid,
    void *pData,
    DWORD *pSizeOfData)
{
    return E_NOTIMPL;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_FreePrivateData(
    IDirect3DVertexBuffer8 *This,
    REFGUID refguid)
{
    return S_OK;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D8VertexBuffer_SetPriority(
    IDirect3DVertexBuffer8 *This,
    DWORD PriorityNew)
{
    return 0;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D8VertexBuffer_GetPriority(IDirect3DVertexBuffer8 *This)
{
    return 0;  /* Stub */
}

static void STDMETHODCALLTYPE
D3D8VertexBuffer_PreLoad(IDirect3DVertexBuffer8 *This)
{
    /* Stub - buffers uploaded on Unlock */
}

static D3DRESOURCETYPE8 STDMETHODCALLTYPE
D3D8VertexBuffer_GetType(IDirect3DVertexBuffer8 *This)
{
    return D3DRTYPE8_VERTEXBUFFER;
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_Lock(
    IDirect3DVertexBuffer8 *This,
    UINT OffsetToLock,
    UINT SizeToLock,
    BYTE **ppbData,
    DWORD Flags)
{
    D3D8_VERTEX_BUFFER *vb = (D3D8_VERTEX_BUFFER*)This;

    if (!ppbData) return E_POINTER;
    if (vb->IsLocked) return E_FAIL;

    /* If SizeToLock is 0, lock entire buffer */
    if (SizeToLock == 0) {
        SizeToLock = vb->Length - OffsetToLock;
    }

    /* Validate range */
    if (OffsetToLock + SizeToLock > vb->Length) {
        return E_INVALIDARG;
    }

    /* Ensure CPU-side buffer exists */
    if (!vb->pData) {
        vb->pData = RtlAllocateMemory(vb->Length);
        if (!vb->pData) {
            return E_OUTOFMEMORY;
        }
        RtlZeroMemory(vb->pData, vb->Length);
    }

    /* Return pointer to locked region */
    *ppbData = (BYTE*)vb->pData + OffsetToLock;

    vb->IsLocked = TRUE;
    vb->LockOffset = OffsetToLock;
    vb->LockSize = SizeToLock;
    vb->LockFlags = Flags;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_Unlock(IDirect3DVertexBuffer8 *This)
{
    D3D8_VERTEX_BUFFER *vb = (D3D8_VERTEX_BUFFER*)This;
    GLenum usage;

    if (!vb->IsLocked) return E_FAIL;
    if (!vb->pData) return E_FAIL;

    /* Determine GL buffer usage hint */
    if (vb->Usage & D3DUSAGE8_DYNAMIC) {
        usage = GL_DYNAMIC_DRAW;
    } else if (vb->Usage & D3DUSAGE8_WRITEONLY) {
        usage = GL_STREAM_DRAW;
    } else {
        usage = GL_STATIC_DRAW;
    }

    /* Upload buffer data to GPU */
    IGLBuffer_Bind(vb->GlBuffer, GL_ARRAY_BUFFER);

    if (vb->LockOffset == 0 && vb->LockSize == vb->Length) {
        /* Upload entire buffer */
        IGLBuffer_BufferData(vb->GlBuffer, GL_ARRAY_BUFFER,
                             vb->Length, vb->pData, usage);
    } else {
        /* Partial update - use BufferSubData */
        IGLBuffer_BufferSubData(vb->GlBuffer, GL_ARRAY_BUFFER,
                                vb->LockOffset, vb->LockSize,
                                (BYTE*)vb->pData + vb->LockOffset);
    }

    vb->IsLocked = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8VertexBuffer_GetDesc(
    IDirect3DVertexBuffer8 *This,
    D3DVERTEXBUFFER_DESC8 *pDesc)
{
    D3D8_VERTEX_BUFFER *vb = (D3D8_VERTEX_BUFFER*)This;

    if (!pDesc) return E_POINTER;

    pDesc->Format = D3DFMT8_VERTEXDATA;
    pDesc->Type = D3DRTYPE8_VERTEXBUFFER;
    pDesc->Usage = vb->Usage;
    pDesc->Pool = vb->Pool;
    pDesc->Size = vb->Length;
    pDesc->FVF = vb->FVF;

    return S_OK;
}

static IDirect3DVertexBuffer8Vtbl D3D8VertexBufferVtbl = {
    .QueryInterface  = D3D8VertexBuffer_QueryInterface,
    .AddRef          = D3D8VertexBuffer_AddRef,
    .Release         = D3D8VertexBuffer_Release,
    .GetDevice       = D3D8VertexBuffer_GetDevice,
    .SetPrivateData  = D3D8VertexBuffer_SetPrivateData,
    .GetPrivateData  = D3D8VertexBuffer_GetPrivateData,
    .FreePrivateData = D3D8VertexBuffer_FreePrivateData,
    .SetPriority     = D3D8VertexBuffer_SetPriority,
    .GetPriority     = D3D8VertexBuffer_GetPriority,
    .PreLoad         = D3D8VertexBuffer_PreLoad,
    .GetType         = D3D8VertexBuffer_GetType,
    .Lock            = D3D8VertexBuffer_Lock,
    .Unlock          = D3D8VertexBuffer_Unlock,
    .GetDesc         = D3D8VertexBuffer_GetDesc,
};

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
D3D8CreateVertexBuffer(
    IGLDevice *pGLDevice,
    UINT Length,
    DWORD Usage,
    DWORD FVF,
    D3DPOOL8 Pool,
    IDirect3DVertexBuffer8 **ppVertexBuffer)
{
    D3D8_VERTEX_BUFFER *vb;
    HRESULT hr;

    if (!pGLDevice || !ppVertexBuffer) return E_POINTER;
    if (Length == 0) return E_INVALIDARG;

    /* Allocate vertex buffer object */
    vb = (D3D8_VERTEX_BUFFER*)RtlAllocateMemory(sizeof(D3D8_VERTEX_BUFFER));
    if (!vb) return E_OUTOFMEMORY;

    RtlZeroMemory(vb, sizeof(D3D8_VERTEX_BUFFER));
    vb->lpVtbl = &D3D8VertexBufferVtbl;
    vb->RefCount = 1;
    vb->GlDevice = pGLDevice;
    vb->Length = Length;
    vb->Usage = Usage;
    vb->FVF = FVF;
    vb->Pool = Pool;

    /* Create OpenGL buffer object */
    hr = IGLDevice_CreateBuffer(pGLDevice, &vb->GlBuffer);
    if (FAILED(hr)) {
        RtlFreeMemory(vb);
        return hr;
    }

    *ppVertexBuffer = (IDirect3DVertexBuffer8*)vb;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper - Get IGLBuffer from vertex buffer                      */
/* --------------------------------------------------------------- */

HRESULT
D3D8GetVertexBufferGLBuffer(
    IDirect3DVertexBuffer8 *pVertexBuffer,
    IGLBuffer **ppGLBuffer)
{
    D3D8_VERTEX_BUFFER *vb;

    if (!pVertexBuffer || !ppGLBuffer) return E_POINTER;

    vb = (D3D8_VERTEX_BUFFER*)pVertexBuffer;
    *ppGLBuffer = vb->GlBuffer;

    return S_OK;
}
