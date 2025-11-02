/*++
    Module Name:

        vertexbinding.c

    Abstract:

        Direct3D 9 vertex declaration to OpenGL attribute binding.
        Handles vertex format translation and attribute setup.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <GLES/gl.h>
#include "d3d9_internal.h"

/* --------------------------------------------------------------- */
/*  Convert D3D declaration type to GL type and size               */
/* --------------------------------------------------------------- */

static HRESULT
D3DDeclTypeToGL(
    D3DDECLTYPE DeclType,
    GLint *OutSize,
    GLenum *OutType,
    GLboolean *OutNormalized)
{
    switch (DeclType) {
        case D3DDECLTYPE_FLOAT1:
            *OutSize = 1;
            *OutType = GL_FLOAT;
            *OutNormalized = GL_FALSE;
            break;

        case D3DDECLTYPE_FLOAT2:
            *OutSize = 2;
            *OutType = GL_FLOAT;
            *OutNormalized = GL_FALSE;
            break;

        case D3DDECLTYPE_FLOAT3:
            *OutSize = 3;
            *OutType = GL_FLOAT;
            *OutNormalized = GL_FALSE;
            break;

        case D3DDECLTYPE_FLOAT4:
            *OutSize = 4;
            *OutType = GL_FLOAT;
            *OutNormalized = GL_FALSE;
            break;

        case D3DDECLTYPE_UBYTE4:
            *OutSize = 4;
            *OutType = GL_UNSIGNED_BYTE;
            *OutNormalized = GL_TRUE;
            break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Get attribute location for D3D usage                           */
/* --------------------------------------------------------------- */

static GLint
D3DUsageToAttributeIndex(D3DDECLUSAGE Usage, UINT8 UsageIndex)
{
    /* Map D3D semantics to fixed attribute indices
     * These can be overridden by shader attribute queries
     */
    switch (Usage) {
        case D3DDECLUSAGE_POSITION:
            return 0;  /* Position is typically attribute 0 */

        case D3DDECLUSAGE_NORMAL:
            return 1;  /* Normal */

        case D3DDECLUSAGE_TEXCOORD:
            return 2 + UsageIndex;  /* TexCoord0-7 */

        case D3DDECLUSAGE_COLOR:
            return 10 + UsageIndex;  /* Color0-1 */

        case D3DDECLUSAGE_BLENDWEIGHT:
            return 12;

        default:
            return -1;  /* Unsupported */
    }
}

/* --------------------------------------------------------------- */
/*  Apply vertex declaration to OpenGL state                       */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyVertexDeclaration(
    IDirect3DVertexDeclaration9 *pDecl,
    UINT32 Stride,
    UINTN BaseOffset)
{
    D3D9_VERTEX_DECLARATION *decl = (D3D9_VERTEX_DECLARATION*)pDecl;
    UINT32 i;
    HRESULT hr;

    if (!decl || !decl->Elements) {
        return E_POINTER;
    }

    /* Iterate through all elements and set up attributes */
    for (i = 0; i < decl->ElementCount; i++) {
        D3DVERTEXELEMENT9 *elem = &decl->Elements[i];
        GLint attrIndex;
        GLint size;
        GLenum type;
        GLboolean normalized;

        /* End marker */
        if (elem->Stream == 0xFF) {
            break;
        }

        /* Only handle stream 0 for now */
        if (elem->Stream != 0) {
            continue;
        }

        /* Get attribute index */
        attrIndex = D3DUsageToAttributeIndex(elem->Usage, elem->UsageIndex);
        if (attrIndex < 0) {
            continue;  /* Skip unsupported attributes */
        }

        /* Convert type */
        hr = D3DDeclTypeToGL(elem->Type, &size, &type, &normalized);
        if (FAILED(hr)) {
            continue;
        }

        /* Setup vertex attribute pointer */
        glEnableVertexAttribArray(attrIndex);
        glVertexAttribPointer(
            attrIndex,
            size,
            type,
            normalized,
            Stride,
            (const void*)(BaseOffset + elem->Offset)
        );
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Disable all vertex attributes                                  */
/* --------------------------------------------------------------- */

HRESULT
D3D9DisableVertexAttributes(VOID)
{
    /* Disable common attribute locations */
    UINT32 i;
    for (i = 0; i < 16; i++) {
        glDisableVertexAttribArray(i);
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Get size of vertex element type in bytes                       */
/* --------------------------------------------------------------- */

UINT32
D3D9GetDeclTypeSize(D3DDECLTYPE Type)
{
    switch (Type) {
        case D3DDECLTYPE_FLOAT1: return 4;
        case D3DDECLTYPE_FLOAT2: return 8;
        case D3DDECLTYPE_FLOAT3: return 12;
        case D3DDECLTYPE_FLOAT4: return 16;
        case D3DDECLTYPE_UBYTE4: return 4;
        default: return 0;
    }
}

/* --------------------------------------------------------------- */
/*  Calculate stride from vertex declaration                       */
/* --------------------------------------------------------------- */

HRESULT
D3D9CalculateVertexStride(
    IDirect3DVertexDeclaration9 *pDecl,
    UINT32 *OutStride)
{
    D3D9_VERTEX_DECLARATION *decl = (D3D9_VERTEX_DECLARATION*)pDecl;
    UINT32 maxOffset = 0;
    UINT32 lastSize = 0;
    UINT32 i;

    if (!decl || !OutStride) {
        return E_POINTER;
    }

    /* Find the element with the largest offset */
    for (i = 0; i < decl->ElementCount; i++) {
        D3DVERTEXELEMENT9 *elem = &decl->Elements[i];

        if (elem->Stream == 0xFF) {
            break;
        }

        if (elem->Offset >= maxOffset) {
            maxOffset = elem->Offset;
            lastSize = D3D9GetDeclTypeSize(elem->Type);
        }
    }

    *OutStride = maxOffset + lastSize;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Bind vertex declaration with shader program                    */
/* --------------------------------------------------------------- */

HRESULT
D3D9BindVertexDeclToShader(
    IDirect3DVertexDeclaration9 *pDecl,
    IGLProgram *pProgram,
    UINT32 Stride,
    UINTN BaseOffset)
{
    D3D9_VERTEX_DECLARATION *decl = (D3D9_VERTEX_DECLARATION*)pDecl;
    UINT32 i;
    HRESULT hr;

    if (!decl || !decl->Elements || !pProgram) {
        return E_POINTER;
    }

    /* Iterate through all elements */
    for (i = 0; i < decl->ElementCount; i++) {
        D3DVERTEXELEMENT9 *elem = &decl->Elements[i];
        GLint attrLocation = -1;
        GLint size;
        GLenum type;
        GLboolean normalized;
        CHAR attrName[64];

        /* End marker */
        if (elem->Stream == 0xFF) {
            break;
        }

        /* Only handle stream 0 */
        if (elem->Stream != 0) {
            continue;
        }

        /* Build attribute name from usage */
        switch (elem->Usage) {
            case D3DDECLUSAGE_POSITION:
                RtlCopyMemory(attrName, "a_position", 11);
                break;
            case D3DDECLUSAGE_NORMAL:
                RtlCopyMemory(attrName, "a_normal", 9);
                break;
            case D3DDECLUSAGE_TEXCOORD:
                if (elem->UsageIndex == 0) {
                    RtlCopyMemory(attrName, "a_texcoord0", 12);
                } else {
                    RtlCopyMemory(attrName, "a_texcoord1", 12);
                }
                break;
            case D3DDECLUSAGE_COLOR:
                RtlCopyMemory(attrName, "a_color", 8);
                break;
            default:
                continue;
        }

        /* Get attribute location from shader */
        hr = IGLProgram_GetAttribLocation(pProgram, attrName, &attrLocation);
        if (FAILED(hr) || attrLocation < 0) {
            /* Attribute not found in shader, use default index */
            attrLocation = D3DUsageToAttributeIndex(elem->Usage, elem->UsageIndex);
            if (attrLocation < 0) {
                continue;
            }
        }

        /* Convert type */
        hr = D3DDeclTypeToGL(elem->Type, &size, &type, &normalized);
        if (FAILED(hr)) {
            continue;
        }

        /* Setup vertex attribute pointer */
        glEnableVertexAttribArray(attrLocation);
        glVertexAttribPointer(
            attrLocation,
            size,
            type,
            normalized,
            Stride,
            (const void*)(BaseOffset + elem->Offset)
        );
    }

    return S_OK;
}
