/*++
    Module Name:

        d3d8texture.c

    Abstract:

        Direct3D 8 texture implementation wrapping OpenGL ES 2.0 textures.
        Implements IDirect3DTexture8 interface with Lock/Unlock for data upload.

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
/*  D3D8 Texture Object                                            */
/* --------------------------------------------------------------- */

typedef struct _D3D8_TEXTURE {
    IDirect3DTexture8Vtbl *lpVtbl;
    UINT32                 RefCount;

    IGLDevice             *GlDevice;
    IGLTexture            *GlTexture;

    UINT32                 Width;
    UINT32                 Height;
    UINT32                 Levels;
    DWORD                  Usage;
    D3DFORMAT8             Format;
    D3DPOOL8               Pool;

    /* CPU-side copy for Lock/Unlock (for D3DPOOL_MANAGED/SYSTEMMEM) */
    VOID                  *pData;
    UINT32                 DataSize;
    UINT32                 Pitch;
    BOOLEAN                IsLocked;
    UINT32                 LockedLevel;
} D3D8_TEXTURE;

/* --------------------------------------------------------------- */
/*  Helper Functions                                               */
/* --------------------------------------------------------------- */

static UINT32
D3D8GetBytesPerPixel(D3DFORMAT8 format)
{
    switch (format) {
        case D3DFMT8_A8R8G8B8:
        case D3DFMT8_X8R8G8B8:
        case D3DFMT8_A8B8G8R8:
            return 4;

        case D3DFMT8_R5G6B5:
        case D3DFMT8_X1R5G5B5:
        case D3DFMT8_A1R5G5B5:
        case D3DFMT8_A4R4G4B4:
            return 2;

        case D3DFMT8_R8G8B8:
            return 3;

        case D3DFMT8_A8:
        case D3DFMT8_L8:
            return 1;

        default:
            return 4;  /* Default to 32-bit */
    }
}

static GLenum
D3D8FormatToGLFormat(D3DFORMAT8 format)
{
    switch (format) {
        case D3DFMT8_A8R8G8B8:
        case D3DFMT8_X8R8G8B8:
            return GL_BGRA_EXT;  /* If supported, else GL_RGBA */

        case D3DFMT8_A8B8G8R8:
            return GL_RGBA;

        case D3DFMT8_R8G8B8:
            return GL_RGB;

        case D3DFMT8_R5G6B5:
            return GL_RGB;

        case D3DFMT8_A8:
        case D3DFMT8_L8:
            return GL_ALPHA;

        default:
            return GL_RGBA;
    }
}

static GLenum
D3D8FormatToGLType(D3DFORMAT8 format)
{
    switch (format) {
        case D3DFMT8_A8R8G8B8:
        case D3DFMT8_X8R8G8B8:
        case D3DFMT8_A8B8G8R8:
        case D3DFMT8_R8G8B8:
        case D3DFMT8_A8:
        case D3DFMT8_L8:
            return GL_UNSIGNED_BYTE;

        case D3DFMT8_R5G6B5:
            return GL_UNSIGNED_SHORT_5_6_5;

        case D3DFMT8_X1R5G5B5:
        case D3DFMT8_A1R5G5B5:
            return GL_UNSIGNED_SHORT_5_5_5_1;

        case D3DFMT8_A4R4G4B4:
            return GL_UNSIGNED_SHORT_4_4_4_4;

        default:
            return GL_UNSIGNED_BYTE;
    }
}

/* --------------------------------------------------------------- */
/*  IDirect3DTexture8 Implementation                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D8Texture_QueryInterface(
    IDirect3DTexture8 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DTexture8))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D8Texture_AddRef(IDirect3DTexture8 *This)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;
    return ++texture->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D8Texture_Release(IDirect3DTexture8 *This)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;
    UINT32 refCount = --texture->RefCount;

    if (refCount == 0) {
        if (texture->GlTexture) {
            IUnknown_Release((IUnknown*)texture->GlTexture);
        }
        if (texture->pData) {
            RtlFreeMemory(texture->pData);
        }
        RtlFreeMemory(texture);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_GetDevice(
    IDirect3DTexture8 *This,
    IDirect3DDevice8 **ppDevice)
{
    /* Not implemented - stub */
    if (!ppDevice) return E_POINTER;
    *ppDevice = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_SetPrivateData(
    IDirect3DTexture8 *This,
    REFGUID refguid,
    CONST void *pData,
    DWORD SizeOfData,
    DWORD Flags)
{
    return S_OK;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_GetPrivateData(
    IDirect3DTexture8 *This,
    REFGUID refguid,
    void *pData,
    DWORD *pSizeOfData)
{
    return E_NOTIMPL;  /* Stub */
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_FreePrivateData(
    IDirect3DTexture8 *This,
    REFGUID refguid)
{
    return S_OK;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D8Texture_SetPriority(
    IDirect3DTexture8 *This,
    DWORD PriorityNew)
{
    return 0;  /* Stub */
}

static DWORD STDMETHODCALLTYPE
D3D8Texture_GetPriority(IDirect3DTexture8 *This)
{
    return 0;  /* Stub */
}

static void STDMETHODCALLTYPE
D3D8Texture_PreLoad(IDirect3DTexture8 *This)
{
    /* Stub - textures are uploaded immediately on Unlock */
}

static D3DRESOURCETYPE8 STDMETHODCALLTYPE
D3D8Texture_GetType(IDirect3DTexture8 *This)
{
    return D3DRTYPE8_TEXTURE;
}

static DWORD STDMETHODCALLTYPE
D3D8Texture_SetLOD(
    IDirect3DTexture8 *This,
    DWORD LODNew)
{
    return 0;  /* Stub - no mipmap LOD control */
}

static DWORD STDMETHODCALLTYPE
D3D8Texture_GetLOD(IDirect3DTexture8 *This)
{
    return 0;  /* Always use highest detail */
}

static DWORD STDMETHODCALLTYPE
D3D8Texture_GetLevelCount(IDirect3DTexture8 *This)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;
    return texture->Levels;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_GetLevelDesc(
    IDirect3DTexture8 *This,
    UINT Level,
    D3DSURFACE_DESC8 *pDesc)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;

    if (!pDesc) return E_POINTER;
    if (Level >= texture->Levels) return E_INVALIDARG;

    pDesc->Format = texture->Format;
    pDesc->Type = D3DRTYPE8_SURFACE;
    pDesc->Usage = texture->Usage;
    pDesc->Pool = texture->Pool;
    pDesc->Size = texture->DataSize >> Level;  /* Approximate */
    pDesc->MultiSampleType = D3DMULTISAMPLE8_NONE;
    pDesc->Width = texture->Width >> Level;
    pDesc->Height = texture->Height >> Level;
    if (pDesc->Width == 0) pDesc->Width = 1;
    if (pDesc->Height == 0) pDesc->Height = 1;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_GetSurfaceLevel(
    IDirect3DTexture8 *This,
    UINT Level,
    IDirect3DSurface8 **ppSurfaceLevel)
{
    /* Not implemented - would need IDirect3DSurface8 implementation */
    if (!ppSurfaceLevel) return E_POINTER;
    *ppSurfaceLevel = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_LockRect(
    IDirect3DTexture8 *This,
    UINT Level,
    D3DLOCKED_RECT8 *pLockedRect,
    CONST RECT *pRect,
    DWORD Flags)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;
    UINT32 levelWidth, levelHeight;

    if (!pLockedRect) return E_POINTER;
    if (Level >= texture->Levels) return E_INVALIDARG;
    if (texture->IsLocked) return E_FAIL;

    /* Calculate dimensions for this mip level */
    levelWidth = texture->Width >> Level;
    levelHeight = texture->Height >> Level;
    if (levelWidth == 0) levelWidth = 1;
    if (levelHeight == 0) levelHeight = 1;

    /* Only support locking entire surface for now */
    if (pRect != NULL) {
        return E_NOTIMPL;  /* Partial locks not implemented */
    }

    /* Ensure CPU-side buffer exists */
    if (!texture->pData) {
        UINT32 bpp = D3D8GetBytesPerPixel(texture->Format);
        texture->Pitch = levelWidth * bpp;
        texture->DataSize = texture->Pitch * levelHeight;
        texture->pData = RtlAllocateMemory(texture->DataSize);
        if (!texture->pData) {
            return E_OUTOFMEMORY;
        }
        RtlZeroMemory(texture->pData, texture->DataSize);
    }

    pLockedRect->Pitch = texture->Pitch;
    pLockedRect->pBits = texture->pData;

    texture->IsLocked = TRUE;
    texture->LockedLevel = Level;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_UnlockRect(
    IDirect3DTexture8 *This,
    UINT Level)
{
    D3D8_TEXTURE *texture = (D3D8_TEXTURE*)This;
    GLenum glFormat, glType;
    UINT32 levelWidth, levelHeight;

    if (!texture->IsLocked) return E_FAIL;
    if (Level != texture->LockedLevel) return E_INVALIDARG;
    if (!texture->pData) return E_FAIL;

    /* Calculate dimensions for this mip level */
    levelWidth = texture->Width >> Level;
    levelHeight = texture->Height >> Level;
    if (levelWidth == 0) levelWidth = 1;
    if (levelHeight == 0) levelHeight = 1;

    /* Upload texture data to GPU */
    glFormat = D3D8FormatToGLFormat(texture->Format);
    glType = D3D8FormatToGLType(texture->Format);

    IGLTexture_Bind(texture->GlTexture, GL_TEXTURE_2D);
    IGLTexture_TexImage2D(texture->GlTexture,
                          GL_TEXTURE_2D,
                          Level,
                          glFormat,  /* Internal format */
                          levelWidth,
                          levelHeight,
                          0,         /* Border */
                          glFormat,  /* Format */
                          glType,
                          texture->pData);

    /* Set default texture parameters */
    if (Level == 0) {
        IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D,
                                  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D,
                                  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D,
                                  GL_TEXTURE_WRAP_S, GL_REPEAT);
        IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D,
                                  GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    texture->IsLocked = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Texture_AddDirtyRect(
    IDirect3DTexture8 *This,
    CONST RECT *pDirtyRect)
{
    return S_OK;  /* Stub - we always upload on Unlock */
}

static IDirect3DTexture8Vtbl D3D8TextureVtbl = {
    .QueryInterface    = D3D8Texture_QueryInterface,
    .AddRef            = D3D8Texture_AddRef,
    .Release           = D3D8Texture_Release,
    .GetDevice         = D3D8Texture_GetDevice,
    .SetPrivateData    = D3D8Texture_SetPrivateData,
    .GetPrivateData    = D3D8Texture_GetPrivateData,
    .FreePrivateData   = D3D8Texture_FreePrivateData,
    .SetPriority       = D3D8Texture_SetPriority,
    .GetPriority       = D3D8Texture_GetPriority,
    .PreLoad           = D3D8Texture_PreLoad,
    .GetType           = D3D8Texture_GetType,
    .SetLOD            = D3D8Texture_SetLOD,
    .GetLOD            = D3D8Texture_GetLOD,
    .GetLevelCount     = D3D8Texture_GetLevelCount,
    .GetLevelDesc      = D3D8Texture_GetLevelDesc,
    .GetSurfaceLevel   = D3D8Texture_GetSurfaceLevel,
    .LockRect          = D3D8Texture_LockRect,
    .UnlockRect        = D3D8Texture_UnlockRect,
    .AddDirtyRect      = D3D8Texture_AddDirtyRect,
};

/* --------------------------------------------------------------- */
/*  Creation Function                                              */
/* --------------------------------------------------------------- */

HRESULT
D3D8CreateTexture(
    IGLDevice *pGLDevice,
    UINT Width,
    UINT Height,
    UINT Levels,
    DWORD Usage,
    D3DFORMAT8 Format,
    D3DPOOL8 Pool,
    IDirect3DTexture8 **ppTexture)
{
    D3D8_TEXTURE *texture;
    HRESULT hr;

    if (!pGLDevice || !ppTexture) return E_POINTER;
    if (Width == 0 || Height == 0) return E_INVALIDARG;

    /* Allocate texture object */
    texture = (D3D8_TEXTURE*)RtlAllocateMemory(sizeof(D3D8_TEXTURE));
    if (!texture) return E_OUTOFMEMORY;

    RtlZeroMemory(texture, sizeof(D3D8_TEXTURE));
    texture->lpVtbl = &D3D8TextureVtbl;
    texture->RefCount = 1;
    texture->GlDevice = pGLDevice;
    texture->Width = Width;
    texture->Height = Height;
    texture->Levels = (Levels == 0) ? 1 : Levels;  /* Default to 1 level */
    texture->Usage = Usage;
    texture->Format = Format;
    texture->Pool = Pool;

    /* Create OpenGL texture object */
    hr = IGLDevice_CreateTexture(pGLDevice, &texture->GlTexture);
    if (FAILED(hr)) {
        RtlFreeMemory(texture);
        return hr;
    }

    *ppTexture = (IDirect3DTexture8*)texture;
    return S_OK;
}
