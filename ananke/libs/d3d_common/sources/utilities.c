/*++
    Module Name:

        utilities.c

    Abstract:

        Common utility functions for all Direct3D implementations.
        Includes matrix operations and state conversions.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d_common.h>
#include <GLES/gl.h>

/* --------------------------------------------------------------- */
/*  Matrix Operations                                              */
/* --------------------------------------------------------------- */

VOID
D3DMatrixIdentity(
    D3D_MATRIX *pMatrix)
{
    UINT32 i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            pMatrix->m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

VOID
D3DMatrixMultiply(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM1,
    CONST D3D_MATRIX *pM2)
{
    D3D_MATRIX temp;
    UINT32 i, j, k;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp.m[i][j] = 0.0f;
            for (k = 0; k < 4; k++) {
                temp.m[i][j] += pM1->m[i][k] * pM2->m[k][j];
            }
        }
    }

    RtlCopyMemory(pOut, &temp, sizeof(D3D_MATRIX));
}

VOID
D3DMatrixTranspose(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM)
{
    D3D_MATRIX temp;
    UINT32 i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp.m[i][j] = pM->m[j][i];
        }
    }

    RtlCopyMemory(pOut, &temp, sizeof(D3D_MATRIX));
}

VOID
D3DMatrixInverse(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM)
{
    /* Simplified inversion for typical transform matrices */
    /* TODO: Implement full 4x4 matrix inversion */
    D3DMatrixIdentity(pOut);
}

/* --------------------------------------------------------------- */
/*  State Conversion Functions                                     */
/* --------------------------------------------------------------- */

GLenum
D3DBlendToGL(
    DWORD blend)
{
    switch (blend) {
    case 1:  /* D3DBLEND_ZERO */        return GL_ZERO;
    case 2:  /* D3DBLEND_ONE */         return GL_ONE;
    case 3:  /* D3DBLEND_SRCCOLOR */    return GL_SRC_COLOR;
    case 4:  /* D3DBLEND_INVSRCCOLOR */ return GL_ONE_MINUS_SRC_COLOR;
    case 5:  /* D3DBLEND_SRCALPHA */    return GL_SRC_ALPHA;
    case 6:  /* D3DBLEND_INVSRCALPHA */ return GL_ONE_MINUS_SRC_ALPHA;
    case 7:  /* D3DBLEND_DESTALPHA */   return GL_DST_ALPHA;
    case 8:  /* D3DBLEND_INVDESTALPHA*/ return GL_ONE_MINUS_DST_ALPHA;
    case 9:  /* D3DBLEND_DESTCOLOR */   return GL_DST_COLOR;
    case 10: /* D3DBLEND_INVDESTCOLOR*/ return GL_ONE_MINUS_DST_COLOR;
    case 11: /* D3DBLEND_SRCALPHASAT */ return GL_SRC_ALPHA_SATURATE;
    default:                             return GL_ONE;
    }
}

GLenum
D3DCmpFuncToGL(
    DWORD cmpFunc)
{
    switch (cmpFunc) {
    case 1:  /* D3DCMP_NEVER */        return GL_NEVER;
    case 2:  /* D3DCMP_LESS */         return GL_LESS;
    case 3:  /* D3DCMP_EQUAL */        return GL_EQUAL;
    case 4:  /* D3DCMP_LESSEQUAL */    return GL_LEQUAL;
    case 5:  /* D3DCMP_GREATER */      return GL_GREATER;
    case 6:  /* D3DCMP_NOTEQUAL */     return GL_NOTEQUAL;
    case 7:  /* D3DCMP_GREATEREQUAL */ return GL_GEQUAL;
    case 8:  /* D3DCMP_ALWAYS */       return GL_ALWAYS;
    default:                            return GL_ALWAYS;
    }
}

GLenum
D3DCullModeToGL(
    DWORD cullMode)
{
    switch (cullMode) {
    case 1:  /* D3DCULL_NONE */ return 0;  /* Special: disable culling */
    case 2:  /* D3DCULL_CW */   return GL_FRONT;
    case 3:  /* D3DCULL_CCW */  return GL_BACK;
    default:                     return GL_BACK;
    }
}

GLenum
D3DPrimitiveTypeToGL(
    DWORD primitiveType)
{
    switch (primitiveType) {
    case 1:  /* D3DPT_POINTLIST */     return GL_POINTS;
    case 2:  /* D3DPT_LINELIST */      return GL_LINES;
    case 3:  /* D3DPT_LINESTRIP */     return GL_LINE_STRIP;
    case 4:  /* D3DPT_TRIANGLELIST */  return GL_TRIANGLES;
    case 5:  /* D3DPT_TRIANGLESTRIP */ return GL_TRIANGLE_STRIP;
    case 6:  /* D3DPT_TRIANGLEFAN */   return GL_TRIANGLE_FAN;
    default:                            return GL_TRIANGLES;
    }
}

/* --------------------------------------------------------------- */
/*  FFP State Management                                           */
/* --------------------------------------------------------------- */

HRESULT
D3DInitializeFFPState(
    D3D_FFP_STATE *pState)
{
    UINT32 i;

    if (!pState) return E_POINTER;

    RtlZeroMemory(pState, sizeof(D3D_FFP_STATE));

    /* Initialize transforms to identity */
    for (i = 0; i < D3D_TRANSFORM_MAX; i++) {
        D3DMatrixIdentity(&pState->transforms[i]);
        pState->transformDirty[i] = TRUE;
    }

    /* Initialize lighting */
    pState->lightingEnabled = FALSE;
    pState->material.diffuse.r = 1.0f;
    pState->material.diffuse.g = 1.0f;
    pState->material.diffuse.b = 1.0f;
    pState->material.diffuse.a = 1.0f;
    pState->material.ambient.r = 0.2f;
    pState->material.ambient.g = 0.2f;
    pState->material.ambient.b = 0.2f;
    pState->material.ambient.a = 1.0f;
    pState->material.power = 0.0f;

    pState->ambientLight.r = 0.2f;
    pState->ambientLight.g = 0.2f;
    pState->ambientLight.b = 0.2f;
    pState->ambientLight.a = 1.0f;

    /* Initialize texture stages */
    for (i = 0; i < D3D_MAX_TEXTURE_STAGES; i++) {
        pState->textureStages[i].colorOp = (i == 0) ? D3D_TOP_MODULATE : D3D_TOP_DISABLE;
        pState->textureStages[i].colorArg1 = D3D_TA_TEXTURE;
        pState->textureStages[i].colorArg2 = D3D_TA_CURRENT;
        pState->textureStages[i].alphaOp = (i == 0) ? D3D_TOP_SELECTARG1 : D3D_TOP_DISABLE;
        pState->textureStages[i].alphaArg1 = D3D_TA_TEXTURE;
        pState->textureStages[i].alphaArg2 = D3D_TA_CURRENT;
        pState->textureStages[i].texCoordIndex = i;
        pState->textureStages[i].texture = NULL;
    }

    /* Initialize render states */
    pState->depthTestEnable = TRUE;
    pState->depthWriteEnable = TRUE;
    pState->alphaBlendEnable = FALSE;
    pState->alphaTestEnable = FALSE;

    pState->currentProgram = NULL;
    pState->stateHash = 0;

    return S_OK;
}

HRESULT
D3DSetFFPTransform(
    D3D_FFP_STATE *pState,
    D3D_TRANSFORM_TYPE transformType,
    CONST D3D_MATRIX *pMatrix)
{
    if (!pState || !pMatrix) return E_POINTER;
    if (transformType >= D3D_TRANSFORM_MAX) return E_INVALIDARG;

    RtlCopyMemory(&pState->transforms[transformType], pMatrix, sizeof(D3D_MATRIX));
    pState->transformDirty[transformType] = TRUE;

    return S_OK;
}

HRESULT
D3DSetFFPMaterial(
    D3D_FFP_STATE *pState,
    CONST D3D_MATERIAL *pMaterial)
{
    if (!pState || !pMaterial) return E_POINTER;

    RtlCopyMemory(&pState->material, pMaterial, sizeof(D3D_MATERIAL));
    pState->stateHash++;  /* Mark state as changed */

    return S_OK;
}

HRESULT
D3DSetFFPLight(
    D3D_FFP_STATE *pState,
    UINT32 index,
    CONST D3D_LIGHT *pLight)
{
    if (!pState || !pLight) return E_POINTER;
    if (index >= D3D_MAX_LIGHTS) return E_INVALIDARG;

    RtlCopyMemory(&pState->lights[index], pLight, sizeof(D3D_LIGHT));
    pState->stateHash++;  /* Mark state as changed */

    return S_OK;
}

HRESULT
D3DEnableFFPLight(
    D3D_FFP_STATE *pState,
    UINT32 index,
    BOOLEAN enable)
{
    if (!pState) return E_POINTER;
    if (index >= D3D_MAX_LIGHTS) return E_INVALIDARG;

    pState->lights[index].enabled = enable;
    pState->stateHash++;  /* Mark state as changed */

    return S_OK;
}

HRESULT
D3DSetFFPTextureStage(
    D3D_FFP_STATE *pState,
    UINT32 stage,
    CONST D3D_TEXTURE_STAGE *pStage)
{
    if (!pState || !pStage) return E_POINTER;
    if (stage >= D3D_MAX_TEXTURE_STAGES) return E_INVALIDARG;

    RtlCopyMemory(&pState->textureStages[stage], pStage, sizeof(D3D_TEXTURE_STAGE));
    pState->stateHash++;  /* Mark state as changed */

    return S_OK;
}

HRESULT
D3DUpdateFFPShaderProgram(
    IGLDevice *pDevice,
    D3D_FFP_STATE *pState,
    CONST D3D_FVF_DESCRIPTOR *pFVF)
{
    D3D_FFP_SHADER_KEY key;
    UINT32 i;
    HRESULT hr;

    if (!pDevice || !pState || !pFVF) return E_POINTER;

    /* Build shader key from current state */
    RtlZeroMemory(&key, sizeof(key));

    key.lightingEnabled = pState->lightingEnabled;
    key.fogEnabled = pState->fogEnabled;
    key.normalizeNormals = TRUE;
    key.texCoordCount = pFVF->texCoordCount;

    /* Build light mask */
    key.activeLightMask = 0;
    for (i = 0; i < D3D_MAX_LIGHTS; i++) {
        if (pState->lights[i].enabled) {
            key.activeLightMask |= (1 << i);
        }
    }

    /* Build texture stage info */
    key.activeTextureStages = 0;
    for (i = 0; i < D3D_MAX_TEXTURE_STAGES; i++) {
        key.stageOps[i] = pState->textureStages[i].colorOp;
        key.stageArgs1[i] = pState->textureStages[i].colorArg1;
        key.stageArgs2[i] = pState->textureStages[i].colorArg2;

        if (key.stageOps[i] != D3D_TOP_DISABLE) {
            key.activeTextureStages = i + 1;
        }
    }

    /* Generate shader program */
    hr = D3DGenerateFFPShader(pDevice, &key, pFVF, &pState->currentProgram);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT
D3DApplyFFPState(
    IGLContext *pContext,
    CONST D3D_FFP_STATE *pState)
{
    if (!pContext || !pState) return E_POINTER;

    /* Apply render states */
    if (pState->depthTestEnable) {
        IGLContext_Enable(pContext, GL_DEPTH_TEST);
    } else {
        IGLContext_Disable(pContext, GL_DEPTH_TEST);
    }

    if (pState->alphaBlendEnable) {
        IGLContext_Enable(pContext, GL_BLEND);
    } else {
        IGLContext_Disable(pContext, GL_BLEND);
    }

    return S_OK;
}
