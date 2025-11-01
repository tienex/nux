/*++
    Module Name:

        kms_renderer.c

    Abstract:

        KMS renderer with GLESv20 integration.
        Provides double-buffered rendering using OpenGL ES 2.0.

--*/

#include <ananke/kms.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backends.h>
#include <string.h>

/* Forward declare Vincent ES 2.0 functions */
extern void* vinInitialize(void);
extern void vinTerminate(void);
extern void* vinCreateFramebufferSurface(IFramebufferBackend* backend, int depthFormat, int stencilFormat);
extern void vinMakeCurrent(void* surface);
extern void vinDestroySurface(void* surface);

/* --------------------------------------------------------------- */
/*  KMS Renderer Implementation                                     */
/* --------------------------------------------------------------- */

#define NUM_BUFFERS 2  /* Double buffering */

typedef struct {
    /* COM interface */
    const struct IKmsRendererVtbl* lpVtbl;
    UINT32 RefCount;

    /* KMS device */
    IKmsDevice* KmsDevice;

    /* Dimensions */
    UINT32 Width;
    UINT32 Height;

    /* Framebuffers for double buffering */
    IFramebufferBackend* Buffers[NUM_BUFFERS];
    UINT32 FbIds[NUM_BUFFERS];
    UINT32 CurrentBuffer;

    /* GLESv20 surface */
    void* GlSurface;

    /* Initialized */
    BOOLEAN Initialized;
} KmsRenderer;

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE KmsRenderer_QueryInterface(
    IKmsRenderer* This,
    REFIID riid,
    VOID** ppvObject
)
{
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IKmsRenderer)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE KmsRenderer_AddRef(IKmsRenderer* This)
{
    KmsRenderer* renderer = (KmsRenderer*)This;
    return ++renderer->RefCount;
}

static UINT32 STDMETHODCALLTYPE KmsRenderer_Release(IKmsRenderer* This)
{
    KmsRenderer* renderer = (KmsRenderer*)This;
    UINT32 refCount = --renderer->RefCount;

    if (refCount == 0) {
        /* Cleanup GLESv20 */
        if (renderer->GlSurface) {
            vinDestroySurface(renderer->GlSurface);
        }
        if (renderer->Initialized) {
            vinTerminate();
        }

        /* Cleanup framebuffers */
        for (UINT32 i = 0; i < NUM_BUFFERS; i++) {
            if (renderer->Buffers[i]) {
                IUnknown_Release((IUnknown*)renderer->Buffers[i]);
            }
            if (renderer->FbIds[i] && renderer->KmsDevice) {
                IKmsDevice_RemoveFramebuffer(renderer->KmsDevice, renderer->FbIds[i]);
            }
        }

        if (renderer->KmsDevice) {
            IUnknown_Release((IUnknown*)renderer->KmsDevice);
        }

        free(renderer);
    }

    return refCount;
}

/* --------------------------------------------------------------- */
/*  IKmsRenderer Implementation                                     */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE KmsRenderer_Initialize(
    IKmsRenderer* This,
    IKmsDevice* pKmsDevice,
    UINT32 Width,
    UINT32 Height
)
{
    KmsRenderer* renderer = (KmsRenderer*)This;
    if (!pKmsDevice) return E_POINTER;

    renderer->KmsDevice = pKmsDevice;
    IUnknown_AddRef((IUnknown*)pKmsDevice);

    renderer->Width = Width;
    renderer->Height = Height;

    /* Create double-buffered framebuffers */
    for (UINT32 i = 0; i < NUM_BUFFERS; i++) {
        /* Create framebuffer backend */
        renderer->Buffers[i] = FbCreateGenericBackend();
        if (!renderer->Buffers[i]) {
            return E_OUTOFMEMORY;
        }

        /* Initialize framebuffer descriptor */
        FRAMEBUFFER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.PixelFormat = FbPixelFormatRgb888;
        desc.Width = Width;
        desc.Height = Height;
        desc.Pitch = Width * 4;
        desc.PhysicalBase = 0;  /* Will be allocated by backend */
        desc.Size = Width * Height * 4;

        HRESULT hr = IFramebufferBackend_Initialize(renderer->Buffers[i], &desc);
        if (FAILED(hr)) {
            return hr;
        }

        /* Register with KMS */
        hr = IKmsDevice_AddFramebuffer(
            pKmsDevice,
            Width,
            Height,
            desc.Pitch,
            32,
            desc.PhysicalBase,
            &renderer->FbIds[i]
        );

        if (FAILED(hr)) {
            return hr;
        }
    }

    /* Initialize Vincent ES 2.0 */
    vinInitialize();

    /* Create GL surface on first buffer */
    renderer->GlSurface = vinCreateFramebufferSurface(
        renderer->Buffers[0],
        0x81A6,  /* GL_DEPTH_COMPONENT16 */
        0x8D48   /* GL_STENCIL_INDEX8_OES */
    );

    if (!renderer->GlSurface) {
        return E_FAIL;
    }

    vinMakeCurrent(renderer->GlSurface);

    renderer->CurrentBuffer = 0;
    renderer->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsRenderer_GetGLSurface(
    IKmsRenderer* This,
    VOID** ppSurface
)
{
    KmsRenderer* renderer = (KmsRenderer*)This;
    if (!ppSurface) return E_POINTER;

    *ppSurface = renderer->GlSurface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsRenderer_GetFramebuffer(
    IKmsRenderer* This,
    IFramebufferBackend** ppBackend
)
{
    KmsRenderer* renderer = (KmsRenderer*)This;
    if (!ppBackend) return E_POINTER;

    *ppBackend = renderer->Buffers[renderer->CurrentBuffer];
    if (*ppBackend) {
        IUnknown_AddRef((IUnknown*)*ppBackend);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE KmsRenderer_BeginFrame(IKmsRenderer* This)
{
    KmsRenderer* renderer = (KmsRenderer*)This;

    /* Make current buffer's GL context active */
    if (renderer->GlSurface) {
        vinMakeCurrent(renderer->GlSurface);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsRenderer_EndFrame(IKmsRenderer* This)
{
    /* GL rendering is complete, ready to swap */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsRenderer_SwapBuffers(IKmsRenderer* This)
{
    KmsRenderer* renderer = (KmsRenderer*)This;

    /* Page flip to current buffer */
    HRESULT hr = IKmsDevice_PageFlip(
        renderer->KmsDevice,
        0,  /* CRTC 0 */
        renderer->FbIds[renderer->CurrentBuffer],
        0   /* Wait for VBlank */
    );

    if (SUCCEEDED(hr)) {
        /* Switch to next buffer */
        renderer->CurrentBuffer = (renderer->CurrentBuffer + 1) % NUM_BUFFERS;
    }

    return hr;
}

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static const struct IKmsRendererVtbl KmsRendererVtbl = {
    KmsRenderer_QueryInterface,
    KmsRenderer_AddRef,
    KmsRenderer_Release,
    KmsRenderer_Initialize,
    KmsRenderer_GetGLSurface,
    KmsRenderer_GetFramebuffer,
    KmsRenderer_BeginFrame,
    KmsRenderer_EndFrame,
    KmsRenderer_SwapBuffers,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

IKmsRenderer* KmsCreateRenderer(VOID)
{
    KmsRenderer* renderer = (KmsRenderer*)malloc(sizeof(KmsRenderer));
    if (!renderer) return NULL;

    memset(renderer, 0, sizeof(KmsRenderer));
    renderer->lpVtbl = &KmsRendererVtbl;
    renderer->RefCount = 1;

    return (IKmsRenderer*)renderer;
}
