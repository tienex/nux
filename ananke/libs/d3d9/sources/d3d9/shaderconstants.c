/*++
    Module Name:

        shaderconstants.c

    Abstract:

        Direct3D 9 shader constant management.
        Handles vertex and pixel shader constant storage and application.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <ananke/ntrtl.h>

/* D3D9 allows up to 256 float4 constants for vertex shaders
 * and 32 for pixel shaders in SM 2.0 */
#define D3D9_MAX_VS_CONSTS 256
#define D3D9_MAX_PS_CONSTS 32

/* --------------------------------------------------------------- */
/*  Shader Constants Storage                                       */
/* --------------------------------------------------------------- */

typedef struct _D3D9_SHADER_CONSTANTS {
    FLOAT VertexShaderConstantsF[D3D9_MAX_VS_CONSTS][4];
    FLOAT PixelShaderConstantsF[D3D9_MAX_PS_CONSTS][4];
    UINT32 VSDirtyStart;
    UINT32 VSDirtyEnd;
    UINT32 PSDirtyStart;
    UINT32 PSDirtyEnd;
} D3D9_SHADER_CONSTANTS;

/* --------------------------------------------------------------- */
/*  Create shader constants storage                                */
/* --------------------------------------------------------------- */

HRESULT
D3D9CreateShaderConstants(
    D3D9_SHADER_CONSTANTS **ppConstants)
{
    D3D9_SHADER_CONSTANTS *constants;

    if (!ppConstants) return E_POINTER;

    constants = (D3D9_SHADER_CONSTANTS*)RtlAllocateMemory(sizeof(D3D9_SHADER_CONSTANTS));
    if (!constants) return E_OUTOFMEMORY;

    RtlZeroMemory(constants, sizeof(D3D9_SHADER_CONSTANTS));

    /* Initialize dirty ranges to indicate no constants set */
    constants->VSDirtyStart = D3D9_MAX_VS_CONSTS;
    constants->VSDirtyEnd = 0;
    constants->PSDirtyStart = D3D9_MAX_PS_CONSTS;
    constants->PSDirtyEnd = 0;

    *ppConstants = constants;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Destroy shader constants storage                               */
/* --------------------------------------------------------------- */

VOID
D3D9DestroyShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants)
{
    if (pConstants) {
        RtlFreeMemory(pConstants);
    }
}

/* --------------------------------------------------------------- */
/*  Set vertex shader float constants                              */
/* --------------------------------------------------------------- */

HRESULT
D3D9SetVertexShaderConstantF(
    D3D9_SHADER_CONSTANTS *pConstants,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount)
{
    UINT32 i, j;
    UINT32 endRegister;

    if (!pConstants || !pConstantData) return E_POINTER;

    if (StartRegister + Vector4fCount > D3D9_MAX_VS_CONSTS) {
        return E_INVALIDARG;
    }

    endRegister = StartRegister + Vector4fCount;

    /* Copy constants */
    for (i = 0; i < Vector4fCount; i++) {
        for (j = 0; j < 4; j++) {
            pConstants->VertexShaderConstantsF[StartRegister + i][j] =
                pConstantData[i * 4 + j];
        }
    }

    /* Update dirty range */
    if (StartRegister < pConstants->VSDirtyStart) {
        pConstants->VSDirtyStart = StartRegister;
    }
    if (endRegister > pConstants->VSDirtyEnd) {
        pConstants->VSDirtyEnd = endRegister;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Set pixel shader float constants                               */
/* --------------------------------------------------------------- */

HRESULT
D3D9SetPixelShaderConstantF(
    D3D9_SHADER_CONSTANTS *pConstants,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount)
{
    UINT32 i, j;
    UINT32 endRegister;

    if (!pConstants || !pConstantData) return E_POINTER;

    if (StartRegister + Vector4fCount > D3D9_MAX_PS_CONSTS) {
        return E_INVALIDARG;
    }

    endRegister = StartRegister + Vector4fCount;

    /* Copy constants */
    for (i = 0; i < Vector4fCount; i++) {
        for (j = 0; j < 4; j++) {
            pConstants->PixelShaderConstantsF[StartRegister + i][j] =
                pConstantData[i * 4 + j];
        }
    }

    /* Update dirty range */
    if (StartRegister < pConstants->PSDirtyStart) {
        pConstants->PSDirtyStart = StartRegister;
    }
    if (endRegister > pConstants->PSDirtyEnd) {
        pConstants->PSDirtyEnd = endRegister;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Apply vertex shader constants to GL program                    */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyVertexShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants,
    IGLProgram *pProgram)
{
    UINT32 i;
    CHAR uniformName[32];
    GL_INT location;
    HRESULT hr;

    if (!pConstants || !pProgram) return E_POINTER;

    /* Apply all dirty constants */
    for (i = pConstants->VSDirtyStart; i < pConstants->VSDirtyEnd; i++) {
        /* Build uniform name: "c0", "c1", etc. */
        uniformName[0] = 'c';
        if (i < 10) {
            uniformName[1] = '0' + i;
            uniformName[2] = '\0';
        } else if (i < 100) {
            uniformName[1] = '0' + (i / 10);
            uniformName[2] = '0' + (i % 10);
            uniformName[3] = '\0';
        } else {
            uniformName[1] = '0' + (i / 100);
            uniformName[2] = '0' + ((i / 10) % 10);
            uniformName[3] = '0' + (i % 10);
            uniformName[4] = '\0';
        }

        /* Get uniform location */
        hr = IGLProgram_GetUniformLocation(pProgram, uniformName, &location);
        if (SUCCEEDED(hr) && location >= 0) {
            /* Set the uniform */
            IGLProgram_Uniform4f(pProgram, location,
                pConstants->VertexShaderConstantsF[i][0],
                pConstants->VertexShaderConstantsF[i][1],
                pConstants->VertexShaderConstantsF[i][2],
                pConstants->VertexShaderConstantsF[i][3]);
        }
    }

    /* Clear dirty range */
    pConstants->VSDirtyStart = D3D9_MAX_VS_CONSTS;
    pConstants->VSDirtyEnd = 0;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Apply pixel shader constants to GL program                     */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyPixelShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants,
    IGLProgram *pProgram)
{
    UINT32 i;
    CHAR uniformName[32];
    GL_INT location;
    HRESULT hr;

    if (!pConstants || !pProgram) return E_POINTER;

    /* Apply all dirty constants */
    for (i = pConstants->PSDirtyStart; i < pConstants->PSDirtyEnd; i++) {
        /* Build uniform name: "c0", "c1", etc. */
        uniformName[0] = 'c';
        if (i < 10) {
            uniformName[1] = '0' + i;
            uniformName[2] = '\0';
        } else if (i < 100) {
            uniformName[1] = '0' + (i / 10);
            uniformName[2] = '0' + (i % 10);
            uniformName[3] = '\0';
        } else {
            uniformName[1] = '0' + (i / 100);
            uniformName[2] = '0' + ((i / 10) % 10);
            uniformName[3] = '0' + (i % 10);
            uniformName[4] = '\0';
        }

        /* Get uniform location */
        hr = IGLProgram_GetUniformLocation(pProgram, uniformName, &location);
        if (SUCCEEDED(hr) && location >= 0) {
            /* Set the uniform */
            IGLProgram_Uniform4f(pProgram, location,
                pConstants->PixelShaderConstantsF[i][0],
                pConstants->PixelShaderConstantsF[i][1],
                pConstants->PixelShaderConstantsF[i][2],
                pConstants->PixelShaderConstantsF[i][3]);
        }
    }

    /* Clear dirty range */
    pConstants->PSDirtyStart = D3D9_MAX_PS_CONSTS;
    pConstants->PSDirtyEnd = 0;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Mark all constants as dirty (for shader change)                */
/* --------------------------------------------------------------- */

HRESULT
D3D9InvalidateShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants)
{
    if (!pConstants) return E_POINTER;

    /* Mark all constants as dirty */
    pConstants->VSDirtyStart = 0;
    pConstants->VSDirtyEnd = D3D9_MAX_VS_CONSTS;
    pConstants->PSDirtyStart = 0;
    pConstants->PSDirtyEnd = D3D9_MAX_PS_CONSTS;

    return S_OK;
}
