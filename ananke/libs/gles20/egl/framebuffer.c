/*++
    Module Name:

        framebuffer.c

    Abstract:

        Ananke framebuffer backend for Vincent ES 2.0.
        Integrates GLESv20 with the ananke framebuffer library.

--*/

#include <GLES/gl.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backends.h>
#include "config.h"
#include "platform/platform.h"
#include "gl/state.h"

typedef struct VinSurface * VinSurface;

typedef struct AnankeSurfaceWrapper {
    Surface                     surface;
    IFramebufferBackend         *backend;
    FRAMEBUFFER_DESC            desc;
    GLint                       refcount;
} AnankeSurfaceWrapper;

static void AddrefSurface(struct Surface * surface) {
    AnankeSurfaceWrapper * wrapper = (AnankeSurfaceWrapper *) surface;
    ++wrapper->refcount;
}

static void ReleaseSurface(struct Surface * surface) {
    AnankeSurfaceWrapper * wrapper = (AnankeSurfaceWrapper *) surface;

    if (--wrapper->refcount == 0) {
        if (wrapper->surface.depthBuffer) {
            GlesFree(wrapper->surface.depthBuffer);
        }

        if (wrapper->surface.stencilBuffer) {
            GlesFree(wrapper->surface.stencilBuffer);
        }

        if (wrapper->backend) {
            IUnknown_Release((IUnknown *)wrapper->backend);
        }

        GlesFree(wrapper);
    }
}

static void LockSurface(struct Surface * surface) {
    AnankeSurfaceWrapper * wrapper = (AnankeSurfaceWrapper *) surface;

    /* Set color buffer to framebuffer base */
    wrapper->surface.colorBuffer = (GLubyte *)(UINTN)wrapper->desc.PhysicalBase +
                                   wrapper->desc.Pitch * (wrapper->desc.Height - 1);

    wrapper->surface.viewport.x = 0;
    wrapper->surface.viewport.y = 0;
    wrapper->surface.viewport.width = wrapper->desc.Width;
    wrapper->surface.viewport.height = wrapper->desc.Height;
}

static void UnlockSurface(struct Surface * surface) {
    AnankeSurfaceWrapper * wrapper = (AnankeSurfaceWrapper *) surface;
    wrapper->surface.colorBuffer = NULL;
}

static SurfaceVtbl Vtbl = {
    &AddrefSurface,
    &ReleaseSurface,
    &LockSurface,
    &UnlockSurface
};

GL_API GLboolean GL_APIENTRY vinInitialize (void) {
    GlesInitState(GlesGetGlobalState());
    return GL_TRUE;
}

GL_API GLboolean GL_APIENTRY vinTerminate (void) {
    GlesDeInitState(GlesGetGlobalState());
    return GL_TRUE;
}

GL_API void (* GL_APIENTRY vinGetProcAddress (const char *procname))() {
    return NULL;
}

GL_API VinSurface GL_APIENTRY vinCreateFramebufferSurface (
    IFramebufferBackend *backend,
    GLenum depthFormat,
    GLenum stencilFormat
    )
{
    AnankeSurfaceWrapper * wrapper = NULL;
    GLuint depthBits;
    GLuint stencilBits;
    GLuint redBits, greenBits, blueBits, alphaBits;
    GLenum colorFormat = GL_INVALID_VALUE;
    GLenum colorReadFormat = GL_INVALID_VALUE;
    GLenum colorReadType = GL_INVALID_VALUE;
    GLint depthPitch, stencilPitch;
    FRAMEBUFFER_DESC desc;
    HRESULT hr;

    if (backend == NULL) {
        return NULL;
    }

    /* Get framebuffer descriptor */
    hr = IFramebufferBackend_GetDescriptor(backend, &desc);
    if (FAILED(hr)) {
        return NULL;
    }

    /* Setup depth buffer */
    switch (depthFormat) {
    case GL_DEPTH_COMPONENT16:  depthBits = 16; break;
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:  depthBits = 32; break;
    default:
        return NULL;
    }

    depthPitch = depthBits * desc.Width;

    /* Setup stencil buffer */
    switch (stencilFormat) {
    case GL_STENCIL_INDEX1_OES: stencilBits = 1; break;
    case GL_STENCIL_INDEX4_OES: stencilBits = 4; break;
    case GL_STENCIL_INDEX8_OES: stencilBits = 8; break;
    default:
        stencilBits = 0;
        break;
    }

    stencilPitch = stencilBits * desc.Width;

    /* Determine color format based on framebuffer pixel format */
    switch (desc.PixelFormat) {
        case FbPixelFormatRgb888:
            redBits = greenBits = blueBits = 8;
            alphaBits = 0;
            colorFormat = GL_RGBA;
            colorReadFormat = GL_RGB;
            colorReadType = GL_UNSIGNED_BYTE;
            break;

        case FbPixelFormatRgb565:
            redBits = blueBits = 5;
            greenBits = 6;
            alphaBits = 0;
            colorFormat = GL_RGB;
            colorReadFormat = GL_RGB;
            colorReadType = GL_UNSIGNED_SHORT_5_6_5;
            break;

        case FbPixelFormatRgb555:
            redBits = greenBits = blueBits = 5;
            alphaBits = 0;
            colorFormat = GL_RGBA;
            colorReadFormat = GL_RGBA;
            colorReadType = GL_UNSIGNED_SHORT_5_5_5_1;
            break;

        default:
            return NULL;
    }

    /* Allocate wrapper */
    wrapper = (AnankeSurfaceWrapper *) GlesCalloc(1, sizeof(AnankeSurfaceWrapper));

    if (!wrapper) {
        return NULL;
    }

    /* Allocate depth buffer */
    wrapper->surface.depthBuffer = GlesCalloc(desc.Height, depthPitch / 8);

    if (!wrapper->surface.depthBuffer) {
        GlesFree(wrapper);
        return NULL;
    }

    /* Allocate stencil buffer if needed */
    if (stencilBits) {
        wrapper->surface.stencilBuffer = GlesCalloc(desc.Height, stencilPitch / 8);

        if (!wrapper->surface.stencilBuffer) {
            GlesFree(wrapper->surface.depthBuffer);
            GlesFree(wrapper);
            return NULL;
        }
    }

    /* Initialize surface structure */
    wrapper->surface.vtbl = &Vtbl;
    wrapper->surface.width = desc.Width;
    wrapper->surface.height = desc.Height;
    wrapper->surface.pitch = -(GLint)desc.Pitch;  /* Negative for bottom-up */
    wrapper->surface.bitsRed = redBits;
    wrapper->surface.bitsGreen = greenBits;
    wrapper->surface.bitsBlue = blueBits;
    wrapper->surface.bitsAlpha = alphaBits;
    wrapper->surface.bitsDepth = depthBits;
    wrapper->surface.bitsStencil = stencilBits;
    wrapper->surface.depthPitch = depthPitch;
    wrapper->surface.stencilPitch = stencilPitch;
    wrapper->surface.colorFormat = colorFormat;
    wrapper->surface.colorReadFormat = colorReadFormat;
    wrapper->surface.colorReadType = colorReadType;
    wrapper->refcount = 1;
    wrapper->backend = backend;
    wrapper->desc = desc;

    /* AddRef the backend */
    IUnknown_AddRef((IUnknown *)backend);

    return (VinSurface) wrapper;
}

GL_API GLboolean GL_APIENTRY vinMakeCurrent (VinSurface surface) {
    State *state = GlesGetGlobalState();

    if (!state) {
        return GL_FALSE;
    }

    if (surface) {
        GlesSelectSurface(state, (Surface *)surface);
        return GL_TRUE;
    }

    return GL_FALSE;
}

GL_API void GL_APIENTRY vinDestroySurface (VinSurface surface) {
    if (surface) {
        struct Surface *surf = (struct Surface *)surface;
        surf->vtbl->release(surf);
    }
}
