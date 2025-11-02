/*++
    Module Name:

        fvf_parser.c

    Abstract:

        Flexible Vertex Format (FVF) parsing and utilities.
        Used by all D3D versions that support FVF (D3D3-D3D8).

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d_common.h>

/* --------------------------------------------------------------- */
/*  Parse FVF flags into descriptor                                */
/* --------------------------------------------------------------- */

HRESULT
D3DParseFVF(
    DWORD fvf,
    D3D_FVF_DESCRIPTOR *pDescriptor)
{
    UINT32 offset = 0;
    UINT32 i;

    if (!pDescriptor) return E_POINTER;

    RtlZeroMemory(pDescriptor, sizeof(D3D_FVF_DESCRIPTOR));

    /* Position */
    if (fvf & D3D_FVF_XYZRHW) {
        pDescriptor->hasPosition = TRUE;
        pDescriptor->hasRHW = TRUE;
        pDescriptor->positionOffset = offset;
        offset += 16;  /* 4 floats (x, y, z, rhw) */
    } else if (fvf & D3D_FVF_XYZ) {
        pDescriptor->hasPosition = TRUE;
        pDescriptor->hasRHW = FALSE;
        pDescriptor->positionOffset = offset;
        offset += 12;  /* 3 floats (x, y, z) */
    }

    /* Normal */
    if (fvf & D3D_FVF_NORMAL) {
        pDescriptor->hasNormal = TRUE;
        pDescriptor->normalOffset = offset;
        offset += 12;  /* 3 floats (nx, ny, nz) */
    }

    /* Diffuse color */
    if (fvf & D3D_FVF_DIFFUSE) {
        pDescriptor->hasDiffuse = TRUE;
        pDescriptor->diffuseOffset = offset;
        offset += 4;  /* 1 DWORD (ARGB) */
    }

    /* Specular color */
    if (fvf & D3D_FVF_SPECULAR) {
        pDescriptor->hasSpecular = TRUE;
        pDescriptor->specularOffset = offset;
        offset += 4;  /* 1 DWORD (ARGB) */
    }

    /* Texture coordinates */
    pDescriptor->texCoordCount = D3D_FVF_TEX_COUNT(fvf);
    for (i = 0; i < pDescriptor->texCoordCount; i++) {
        pDescriptor->texCoordOffset[i] = offset;
        offset += 8;  /* 2 floats (u, v) */
    }

    pDescriptor->vertexSize = offset;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Get vertex size from FVF                                       */
/* --------------------------------------------------------------- */

UINT32
D3DGetFVFVertexSize(
    DWORD fvf)
{
    D3D_FVF_DESCRIPTOR desc;
    if (FAILED(D3DParseFVF(fvf, &desc))) {
        return 0;
    }
    return desc.vertexSize;
}
