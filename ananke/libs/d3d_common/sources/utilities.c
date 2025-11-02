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
    FLOAT inv[16], det;
    INT32 i;
    CONST FLOAT *m = (CONST FLOAT*)pM;

    /* Calculate the inverse using the adjugate method */
    inv[0] = m[5]  * m[10] * m[15] - m[5]  * m[11] * m[14] - m[9]  * m[6]  * m[15]
           + m[9]  * m[7]  * m[14] + m[13] * m[6]  * m[11] - m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + m[4]  * m[11] * m[14] + m[8]  * m[6]  * m[15]
           - m[8]  * m[7]  * m[14] - m[12] * m[6]  * m[11] + m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - m[4]  * m[11] * m[13] - m[8]  * m[5] * m[15]
           + m[8]  * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + m[4]  * m[10] * m[13] + m[8]  * m[5] * m[14]
            - m[8]  * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + m[1]  * m[11] * m[14] + m[9]  * m[2] * m[15]
           - m[9]  * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - m[0]  * m[11] * m[14] - m[8]  * m[2] * m[15]
           + m[8]  * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + m[0]  * m[11] * m[13] + m[8]  * m[1] * m[15]
           - m[8]  * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - m[0]  * m[10] * m[13] - m[8]  * m[1] * m[14]
            + m[8]  * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - m[1]  * m[7] * m[14] - m[5]  * m[2] * m[15]
           + m[5]  * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + m[0]  * m[7] * m[14] + m[4]  * m[2] * m[15]
           - m[4]  * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - m[0]  * m[7] * m[13] - m[4]  * m[1] * m[15]
            + m[4]  * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + m[0]  * m[6] * m[13] + m[4]  * m[1] * m[14]
            - m[4]  * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
           - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
           + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11]
            - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10]
            + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    /* Calculate determinant */
    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    /* If determinant is zero, matrix is not invertible - return identity */
    if (det == 0.0f) {
        D3DMatrixIdentity(pOut);
        return;
    }

    det = 1.0f / det;

    /* Multiply adjugate by 1/determinant */
    for (i = 0; i < 16; i++) {
        ((FLOAT*)pOut)[i] = inv[i] * det;
    }
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
    UINT32 i;

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

    /* Bind textures to texture units */
    for (i = 0; i < D3D_MAX_TEXTURE_STAGES; i++) {
        CONST D3D_TEXTURE_STAGE *stage = &pState->textureStages[i];

        /* Stop at first disabled stage */
        if (stage->colorOp == D3D_TOP_DISABLE) {
            break;
        }

        /* Activate texture unit */
        IGLContext_ActiveTexture(pContext, GL_TEXTURE0 + i);

        /* Bind texture if present */
        if (stage->texture) {
            IGLTexture_Bind(stage->texture, GL_TEXTURE_2D);
            IGLContext_Enable(pContext, GL_TEXTURE_2D);
        } else {
            IGLContext_Disable(pContext, GL_TEXTURE_2D);
        }
    }

    /* Reset to texture unit 0 */
    IGLContext_ActiveTexture(pContext, GL_TEXTURE0);

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Vertex Attribute Binding                                       */
/* --------------------------------------------------------------- */

HRESULT
D3DBindVertexAttributes(
    IGLContext *pContext,
    IGLProgram *pProgram,
    CONST D3D_FVF_DESCRIPTOR *pFVF,
    CONST VOID *pVertexData)
{
    GL_INT location;
    CONST UINT8 *vertexBytes = (CONST UINT8*)pVertexData;
    UINT32 i;

    if (!pContext || !pProgram || !pFVF || !pVertexData) {
        return E_POINTER;
    }

    /* Position attribute */
    if (pFVF->hasPosition) {
        IGLProgram_GetAttribLocation(pProgram, "aPosition", &location);
        if (location >= 0) {
            IGLContext_VertexAttribPointer(
                pContext,
                location,
                pFVF->hasRHW ? 4 : 3,
                GL_FLOAT,
                GL_FALSE,
                pFVF->vertexSize,
                vertexBytes + pFVF->positionOffset
            );
            IGLContext_EnableVertexAttribArray(pContext, location);
        }
    }

    /* Normal attribute */
    if (pFVF->hasNormal) {
        IGLProgram_GetAttribLocation(pProgram, "aNormal", &location);
        if (location >= 0) {
            IGLContext_VertexAttribPointer(
                pContext,
                location,
                3,
                GL_FLOAT,
                GL_FALSE,
                pFVF->vertexSize,
                vertexBytes + pFVF->normalOffset
            );
            IGLContext_EnableVertexAttribArray(pContext, location);
        }
    }

    /* Diffuse color attribute */
    if (pFVF->hasDiffuse) {
        IGLProgram_GetAttribLocation(pProgram, "aDiffuse", &location);
        if (location >= 0) {
            IGLContext_VertexAttribPointer(
                pContext,
                location,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                pFVF->vertexSize,
                vertexBytes + pFVF->diffuseOffset
            );
            IGLContext_EnableVertexAttribArray(pContext, location);
        }
    }

    /* Specular color attribute */
    if (pFVF->hasSpecular) {
        IGLProgram_GetAttribLocation(pProgram, "aSpecular", &location);
        if (location >= 0) {
            IGLContext_VertexAttribPointer(
                pContext,
                location,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                pFVF->vertexSize,
                vertexBytes + pFVF->specularOffset
            );
            IGLContext_EnableVertexAttribArray(pContext, location);
        }
    }

    /* Texture coordinates */
    for (i = 0; i < pFVF->texCoordCount && i < 8; i++) {
        CHAR attrName[32];
        /* Simple manual formatting for aTexCoord0-7 */
        attrName[0] = 'a';
        attrName[1] = 'T';
        attrName[2] = 'e';
        attrName[3] = 'x';
        attrName[4] = 'C';
        attrName[5] = 'o';
        attrName[6] = 'o';
        attrName[7] = 'r';
        attrName[8] = 'd';
        attrName[9] = '0' + (CHAR)i;
        attrName[10] = '\0';

        IGLProgram_GetAttribLocation(pProgram, attrName, &location);
        if (location >= 0) {
            IGLContext_VertexAttribPointer(
                pContext,
                location,
                2,
                GL_FLOAT,
                GL_FALSE,
                pFVF->vertexSize,
                vertexBytes + pFVF->texCoordOffset[i]
            );
            IGLContext_EnableVertexAttribArray(pContext, location);
        }
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  FFP Uniform Updates                                            */
/* --------------------------------------------------------------- */

HRESULT
D3DUpdateFFPUniforms(
    IGLProgram *pProgram,
    CONST D3D_FFP_STATE *pState)
{
    GL_INT location;
    D3D_MATRIX mvpMatrix, modelView;
    UINT32 i;

    if (!pProgram || !pState) {
        return E_POINTER;
    }

    /* Calculate ModelViewProjection matrix */
    D3DMatrixMultiply(&modelView,
                      &pState->transforms[D3D_TRANSFORM_WORLD],
                      &pState->transforms[D3D_TRANSFORM_VIEW]);
    D3DMatrixMultiply(&mvpMatrix,
                      &modelView,
                      &pState->transforms[D3D_TRANSFORM_PROJECTION]);

    /* Set MVP matrix */
    IGLProgram_GetUniformLocation(pProgram, "uMVPMatrix", &location);
    if (location >= 0) {
        IGLProgram_UniformMatrix4fv(pProgram, location, 1, GL_FALSE,
                                     (CONST GL_FLOAT*)&mvpMatrix);
    }

    /* Set Model matrix */
    IGLProgram_GetUniformLocation(pProgram, "uModelMatrix", &location);
    if (location >= 0) {
        IGLProgram_UniformMatrix4fv(pProgram, location, 1, GL_FALSE,
                                     (CONST GL_FLOAT*)&pState->transforms[D3D_TRANSFORM_WORLD]);
    }

    /* Set View matrix */
    IGLProgram_GetUniformLocation(pProgram, "uViewMatrix", &location);
    if (location >= 0) {
        IGLProgram_UniformMatrix4fv(pProgram, location, 1, GL_FALSE,
                                     (CONST GL_FLOAT*)&pState->transforms[D3D_TRANSFORM_VIEW]);
    }

    /* Set Material properties */
    if (pState->lightingEnabled) {
        IGLProgram_GetUniformLocation(pProgram, "uMaterialDiffuse", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->material.diffuse.r,
                                pState->material.diffuse.g,
                                pState->material.diffuse.b,
                                pState->material.diffuse.a);
        }

        IGLProgram_GetUniformLocation(pProgram, "uMaterialAmbient", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->material.ambient.r,
                                pState->material.ambient.g,
                                pState->material.ambient.b,
                                pState->material.ambient.a);
        }

        IGLProgram_GetUniformLocation(pProgram, "uMaterialSpecular", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->material.specular.r,
                                pState->material.specular.g,
                                pState->material.specular.b,
                                pState->material.specular.a);
        }

        IGLProgram_GetUniformLocation(pProgram, "uMaterialEmissive", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->material.emissive.r,
                                pState->material.emissive.g,
                                pState->material.emissive.b,
                                pState->material.emissive.a);
        }

        IGLProgram_GetUniformLocation(pProgram, "uMaterialPower", &location);
        if (location >= 0) {
            IGLProgram_Uniform1f(pProgram, location, pState->material.power);
        }

        /* Set lights (up to 8 lights) */
        for (i = 0; i < D3D_MAX_LIGHTS; i++) {
            if (!pState->lights[i].enabled) continue;

            CHAR uniformName[64];

            /* Light position - uLights[i].position */
            uniformName[0] = 'u'; uniformName[1] = 'L'; uniformName[2] = 'i';
            uniformName[3] = 'g'; uniformName[4] = 'h'; uniformName[5] = 't';
            uniformName[6] = 's'; uniformName[7] = '['; uniformName[8] = '0' + (CHAR)i;
            uniformName[9] = ']'; uniformName[10] = '.'; uniformName[11] = 'p';
            uniformName[12] = 'o'; uniformName[13] = 's'; uniformName[14] = 'i';
            uniformName[15] = 't'; uniformName[16] = 'i'; uniformName[17] = 'o';
            uniformName[18] = 'n'; uniformName[19] = '\0';

            IGLProgram_GetUniformLocation(pProgram, uniformName, &location);
            if (location >= 0) {
                IGLProgram_Uniform3f(pProgram, location,
                                    pState->lights[i].position.x,
                                    pState->lights[i].position.y,
                                    pState->lights[i].position.z);
            }

            /* Light diffuse - uLights[i].diffuse */
            uniformName[11] = 'd'; uniformName[12] = 'i'; uniformName[13] = 'f';
            uniformName[14] = 'f'; uniformName[15] = 'u'; uniformName[16] = 's';
            uniformName[17] = 'e'; uniformName[18] = '\0';

            IGLProgram_GetUniformLocation(pProgram, uniformName, &location);
            if (location >= 0) {
                IGLProgram_Uniform4f(pProgram, location,
                                    pState->lights[i].diffuse.r,
                                    pState->lights[i].diffuse.g,
                                    pState->lights[i].diffuse.b,
                                    pState->lights[i].diffuse.a);
            }
        }

        /* Set ambient light */
        IGLProgram_GetUniformLocation(pProgram, "uAmbientLight", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->ambientLight.r,
                                pState->ambientLight.g,
                                pState->ambientLight.b,
                                pState->ambientLight.a);
        }
    }

    /* Set fog parameters */
    if (pState->fogEnabled) {
        IGLProgram_GetUniformLocation(pProgram, "uFogColor", &location);
        if (location >= 0) {
            IGLProgram_Uniform4f(pProgram, location,
                                pState->fogColor.r,
                                pState->fogColor.g,
                                pState->fogColor.b,
                                pState->fogColor.a);
        }

        IGLProgram_GetUniformLocation(pProgram, "uFogStart", &location);
        if (location >= 0) {
            IGLProgram_Uniform1f(pProgram, location, pState->fogStart);
        }

        IGLProgram_GetUniformLocation(pProgram, "uFogEnd", &location);
        if (location >= 0) {
            IGLProgram_Uniform1f(pProgram, location, pState->fogEnd);
        }
    }

    /* Set texture samplers */
    for (i = 0; i < D3D_MAX_TEXTURE_STAGES; i++) {
        if (pState->textureStages[i].colorOp == D3D_TOP_DISABLE) break;

        CHAR samplerName[32];
        samplerName[0] = 'u'; samplerName[1] = 'T'; samplerName[2] = 'e';
        samplerName[3] = 'x'; samplerName[4] = 't'; samplerName[5] = 'u';
        samplerName[6] = 'r'; samplerName[7] = 'e'; samplerName[8] = '0' + (CHAR)i;
        samplerName[9] = '\0';

        IGLProgram_GetUniformLocation(pProgram, samplerName, &location);
        if (location >= 0) {
            IGLProgram_Uniform1f(pProgram, location, (GL_FLOAT)i);
        }
    }

    return S_OK;
}
