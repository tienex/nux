/*++
    Module Name:

        statemanager.c

    Abstract:

        Direct3D 9 state management.
        Handles render states, texture stage states, and sampler states.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <GLES/gl.h>

/* --------------------------------------------------------------- */
/*  Blend factor conversion                                        */
/* --------------------------------------------------------------- */

static GLenum
D3DBlendToGL(D3DBLEND Blend)
{
    switch (Blend) {
        case D3DBLEND_ZERO:           return GL_ZERO;
        case D3DBLEND_ONE:            return GL_ONE;
        case D3DBLEND_SRCCOLOR:       return GL_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR:    return GL_ONE_MINUS_SRC_COLOR;
        case D3DBLEND_SRCALPHA:       return GL_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA:    return GL_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA:      return GL_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA:   return GL_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR:      return GL_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR:   return GL_ONE_MINUS_DST_COLOR;
        default:                      return GL_ONE;
    }
}

/* --------------------------------------------------------------- */
/*  Compare function conversion                                    */
/* --------------------------------------------------------------- */

static GLenum
D3DCmpFuncToGL(D3DCMPFUNC Func)
{
    switch (Func) {
        case D3DCMP_NEVER:        return GL_NEVER;
        case D3DCMP_LESS:         return GL_LESS;
        case D3DCMP_EQUAL:        return GL_EQUAL;
        case D3DCMP_LESSEQUAL:    return GL_LEQUAL;
        case D3DCMP_GREATER:      return GL_GREATER;
        case D3DCMP_NOTEQUAL:     return GL_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
        case D3DCMP_ALWAYS:       return GL_ALWAYS;
        default:                  return GL_LESS;
    }
}

/* --------------------------------------------------------------- */
/*  Apply render state to GL context                               */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyRenderState(
    IGLContext *GlContext,
    D3DRENDERSTATETYPE State,
    UINT32 Value)
{
    switch (State) {
        case D3DRS_ZENABLE:
            if (Value) {
                IGLContext_Enable(GlContext, GL_DEPTH_TEST);
            } else {
                IGLContext_Disable(GlContext, GL_DEPTH_TEST);
            }
            break;

        case D3DRS_ZWRITEENABLE:
            glDepthMask(Value ? GL_TRUE : GL_FALSE);
            break;

        case D3DRS_ZFUNC:
            IGLContext_DepthFunc(GlContext, D3DCmpFuncToGL((D3DCMPFUNC)Value));
            break;

        case D3DRS_ALPHABLENDENABLE:
            if (Value) {
                IGLContext_Enable(GlContext, GL_BLEND);
            } else {
                IGLContext_Disable(GlContext, GL_BLEND);
            }
            break;

        case D3DRS_SRCBLEND:
            /* Store for later application with DESTBLEND */
            break;

        case D3DRS_DESTBLEND:
            /* Apply blend function - need to track SRCBLEND separately */
            break;

        case D3DRS_CULLMODE:
            if (Value == D3DCULL_NONE) {
                IGLContext_Disable(GlContext, GL_CULL_FACE);
            } else {
                IGLContext_Enable(GlContext, GL_CULL_FACE);
                if (Value == D3DCULL_CW) {
                    glFrontFace(GL_CW);
                } else {
                    glFrontFace(GL_CCW);
                }
            }
            break;

        case D3DRS_ALPHATESTENABLE:
            /* GL ES 2.0 doesn't have alpha test - needs shader emulation */
            break;

        case D3DRS_FOGENABLE:
            /* Fog needs shader emulation in GLES20 */
            break;

        case D3DRS_LIGHTING:
            /* Lighting needs shader implementation */
            break;

        case D3DRS_SPECULARENABLE:
            /* Specular needs shader implementation */
            break;

        case D3DRS_SHADEMODE:
            /* Flat/Gouraud shading - GL ES 2.0 is always Gouraud */
            break;

        case D3DRS_FILLMODE:
            /* Wireframe mode not supported in GL ES 2.0 */
            break;

        case D3DRS_AMBIENT:
            /* Ambient lighting - needs shader implementation */
            break;

        case D3DRS_COLORVERTEX:
            /* Color vertex - needs shader implementation */
            break;

        case D3DRS_FOGCOLOR:
            /* Fog color - needs shader implementation */
            break;

        default:
            /* Unsupported or unrecognized state */
            break;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Apply texture stage state                                      */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyTextureStageState(
    IGLContext *GlContext,
    UINT32 Stage,
    D3DTEXTURESTAGESTATETYPE Type,
    UINT32 Value)
{
    /* Texture stage states in D3D9 control texture combiners.
     * In GLES20, this is handled by shaders.
     * For basic operation, we'll ignore these and rely on shaders.
     */

    switch (Type) {
        case D3DTSS_COLOROP:
        case D3DTSS_COLORARG1:
        case D3DTSS_COLORARG2:
        case D3DTSS_ALPHAOP:
        case D3DTSS_ALPHAARG1:
        case D3DTSS_ALPHAARG2:
            /* These require shader emulation */
            break;

        default:
            break;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Apply sampler state                                            */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplySamplerState(
    IGLContext *GlContext,
    UINT32 Sampler,
    D3DSAMPLERSTATETYPE Type,
    UINT32 Value)
{
    GLenum target = GL_TEXTURE_2D;

    /* Activate texture unit */
    glActiveTexture(GL_TEXTURE0 + Sampler);

    switch (Type) {
        case D3DSAMP_ADDRESSU:
            glTexParameteri(target, GL_TEXTURE_WRAP_S,
                          (Value == 1) ? GL_REPEAT :     /* D3DTADDRESS_WRAP */
                          (Value == 2) ? GL_MIRRORED_REPEAT :  /* D3DTADDRESS_MIRROR */
                          GL_CLAMP_TO_EDGE);              /* D3DTADDRESS_CLAMP */
            break;

        case D3DSAMP_ADDRESSV:
            glTexParameteri(target, GL_TEXTURE_WRAP_T,
                          (Value == 1) ? GL_REPEAT :
                          (Value == 2) ? GL_MIRRORED_REPEAT :
                          GL_CLAMP_TO_EDGE);
            break;

        case D3DSAMP_MAGFILTER:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
                          (Value == 1) ? GL_NEAREST :    /* D3DTEXF_POINT */
                          GL_LINEAR);                     /* D3DTEXF_LINEAR */
            break;

        case D3DSAMP_MINFILTER:
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                          (Value == 1) ? GL_NEAREST :
                          GL_LINEAR);
            break;

        case D3DSAMP_MIPFILTER:
            /* Mipmap filtering */
            if (Value == 1) {  /* D3DTEXF_POINT */
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            } else if (Value == 2) {  /* D3DTEXF_LINEAR */
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            }
            break;

        default:
            break;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Apply blending state                                           */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyBlendState(
    IGLContext *GlContext,
    D3DBLEND SrcBlend,
    D3DBLEND DestBlend)
{
    GLenum glSrc = D3DBlendToGL(SrcBlend);
    GLenum glDst = D3DBlendToGL(DestBlend);

    IGLContext_BlendFunc(GlContext, glSrc, glDst);

    return S_OK;
}
