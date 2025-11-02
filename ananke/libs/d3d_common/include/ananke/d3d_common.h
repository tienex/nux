/*++
    Module Name:

        d3d_common.h

    Abstract:

        Common infrastructure shared by all Direct3D implementations (D3D3-D3D9).
        Provides fixed-function pipeline emulation, state management, and utilities.

    Environment:

        C99 compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/gles20com.h>

#ifndef CHAR
typedef char  CHAR;
#endif
#ifndef FLOAT
typedef float FLOAT;
#endif
typedef UINT32 DWORD;

/* --------------------------------------------------------------- */
/*  Fixed-Function Pipeline State                                  */
/* --------------------------------------------------------------- */

#define D3D_MAX_LIGHTS              8
#define D3D_MAX_TEXTURE_STAGES      8
#define D3D_MAX_VERTEX_STREAMS      16

/* Transform types */
typedef enum _D3D_TRANSFORM_TYPE {
    D3D_TRANSFORM_WORLD = 0,
    D3D_TRANSFORM_VIEW  = 1,
    D3D_TRANSFORM_PROJECTION = 2,
    D3D_TRANSFORM_WORLD1 = 3,
    D3D_TRANSFORM_WORLD2 = 4,
    D3D_TRANSFORM_WORLD3 = 5,
    D3D_TRANSFORM_MAX    = 6,
} D3D_TRANSFORM_TYPE;

/* Matrix */
typedef struct _D3D_MATRIX {
    FLOAT m[4][4];
} D3D_MATRIX;

/* Vector3 */
typedef struct _D3D_VECTOR3 {
    FLOAT x, y, z;
} D3D_VECTOR3;

/* Vector4 */
typedef struct _D3D_VECTOR4 {
    FLOAT x, y, z, w;
} D3D_VECTOR4;

/* Color */
typedef struct _D3D_COLOR {
    FLOAT r, g, b, a;
} D3D_COLOR;

/* Material */
typedef struct _D3D_MATERIAL {
    D3D_COLOR diffuse;
    D3D_COLOR ambient;
    D3D_COLOR specular;
    D3D_COLOR emissive;
    FLOAT     power;
} D3D_MATERIAL;

/* Light type */
typedef enum _D3D_LIGHT_TYPE {
    D3D_LIGHT_POINT = 1,
    D3D_LIGHT_SPOT = 2,
    D3D_LIGHT_DIRECTIONAL = 3,
} D3D_LIGHT_TYPE;

/* Light */
typedef struct _D3D_LIGHT {
    D3D_LIGHT_TYPE type;
    D3D_COLOR      diffuse;
    D3D_COLOR      specular;
    D3D_COLOR      ambient;
    D3D_VECTOR3    position;
    D3D_VECTOR3    direction;
    FLOAT          range;
    FLOAT          falloff;
    FLOAT          attenuation0;
    FLOAT          attenuation1;
    FLOAT          attenuation2;
    FLOAT          theta;
    FLOAT          phi;
    BOOLEAN        enabled;
} D3D_LIGHT;

/* Texture operation */
typedef enum _D3D_TEXTURE_OP {
    D3D_TOP_DISABLE = 1,
    D3D_TOP_SELECTARG1 = 2,
    D3D_TOP_SELECTARG2 = 3,
    D3D_TOP_MODULATE = 4,
    D3D_TOP_MODULATE2X = 5,
    D3D_TOP_MODULATE4X = 6,
    D3D_TOP_ADD = 7,
    D3D_TOP_ADDSIGNED = 8,
    D3D_TOP_SUBTRACT = 10,
    D3D_TOP_BLENDDIFFUSEALPHA = 12,
    D3D_TOP_BLENDTEXTUREALPHA = 13,
} D3D_TEXTURE_OP;

/* Texture argument */
typedef enum _D3D_TEXTURE_ARG {
    D3D_TA_DIFFUSE = 0,
    D3D_TA_CURRENT = 1,
    D3D_TA_TEXTURE = 2,
    D3D_TA_TFACTOR = 3,
    D3D_TA_SPECULAR = 4,
} D3D_TEXTURE_ARG;

/* Texture stage state */
typedef struct _D3D_TEXTURE_STAGE {
    D3D_TEXTURE_OP  colorOp;
    D3D_TEXTURE_ARG colorArg1;
    D3D_TEXTURE_ARG colorArg2;
    D3D_TEXTURE_OP  alphaOp;
    D3D_TEXTURE_ARG alphaArg1;
    D3D_TEXTURE_ARG alphaArg2;
    DWORD           texCoordIndex;
    IGLTexture     *texture;
} D3D_TEXTURE_STAGE;

/* Flexible Vertex Format (FVF) flags */
#define D3D_FVF_XYZ         0x002
#define D3D_FVF_XYZRHW      0x004
#define D3D_FVF_NORMAL      0x010
#define D3D_FVF_DIFFUSE     0x040
#define D3D_FVF_SPECULAR    0x080
#define D3D_FVF_TEX1        0x100
#define D3D_FVF_TEX2        0x200
#define D3D_FVF_TEX3        0x300
#define D3D_FVF_TEX4        0x400
#define D3D_FVF_TEX_COUNT(fvf)  (((fvf) >> 8) & 0xF)

/* FVF vertex descriptor */
typedef struct _D3D_FVF_DESCRIPTOR {
    BOOLEAN hasPosition;
    BOOLEAN hasRHW;          /* Pre-transformed */
    BOOLEAN hasNormal;
    BOOLEAN hasDiffuse;
    BOOLEAN hasSpecular;
    UINT32  texCoordCount;
    UINT32  vertexSize;      /* Total size in bytes */
    UINT32  positionOffset;
    UINT32  normalOffset;
    UINT32  diffuseOffset;
    UINT32  specularOffset;
    UINT32  texCoordOffset[8];
} D3D_FVF_DESCRIPTOR;

/* --------------------------------------------------------------- */
/*  Fixed-Function Pipeline State Container                        */
/* --------------------------------------------------------------- */

typedef struct _D3D_FFP_STATE {
    /* Transforms */
    D3D_MATRIX transforms[D3D_TRANSFORM_MAX];
    BOOLEAN    transformDirty[D3D_TRANSFORM_MAX];

    /* Lighting */
    BOOLEAN      lightingEnabled;
    D3D_MATERIAL material;
    D3D_LIGHT    lights[D3D_MAX_LIGHTS];
    D3D_COLOR    ambientLight;
    BOOLEAN      specularEnabled;

    /* Texture stages */
    D3D_TEXTURE_STAGE textureStages[D3D_MAX_TEXTURE_STAGES];

    /* Fog */
    BOOLEAN fogEnabled;
    D3D_COLOR fogColor;
    FLOAT     fogStart;
    FLOAT     fogEnd;
    FLOAT     fogDensity;

    /* Render states */
    BOOLEAN alphaBlendEnable;
    BOOLEAN alphaTestEnable;
    BOOLEAN depthTestEnable;
    BOOLEAN depthWriteEnable;

    /* Current shader program (generated from state) */
    IGLProgram *currentProgram;
    UINT32      stateHash;      /* Hash of current state for caching */
} D3D_FFP_STATE;

/* --------------------------------------------------------------- */
/*  FVF Parsing                                                    */
/* --------------------------------------------------------------- */

HRESULT
D3DParseFVF(
    DWORD fvf,
    D3D_FVF_DESCRIPTOR *pDescriptor
);

UINT32
D3DGetFVFVertexSize(
    DWORD fvf
);

/* --------------------------------------------------------------- */
/*  Fixed-Function Pipeline Shader Generator                       */
/* --------------------------------------------------------------- */

typedef struct _D3D_FFP_SHADER_KEY {
    /* Vertex shader key */
    BOOLEAN lightingEnabled;
    UINT32  activeLightMask;    /* Bitmask of enabled lights */
    BOOLEAN fogEnabled;
    BOOLEAN normalizeNormals;
    UINT32  texCoordCount;

    /* Pixel shader key */
    UINT32 activeTextureStages; /* How many stages are active */
    D3D_TEXTURE_OP stageOps[D3D_MAX_TEXTURE_STAGES];
    D3D_TEXTURE_ARG stageArgs1[D3D_MAX_TEXTURE_STAGES];
    D3D_TEXTURE_ARG stageArgs2[D3D_MAX_TEXTURE_STAGES];
} D3D_FFP_SHADER_KEY;

HRESULT
D3DGenerateFFPShader(
    IGLDevice *pDevice,
    CONST D3D_FFP_SHADER_KEY *pKey,
    CONST D3D_FVF_DESCRIPTOR *pFVF,
    IGLProgram **ppProgram
);

HRESULT
D3DGenerateVertexShader(
    CONST D3D_FFP_SHADER_KEY *pKey,
    CONST D3D_FVF_DESCRIPTOR *pFVF,
    CHAR **ppSource
);

HRESULT
D3DGeneratePixelShader(
    CONST D3D_FFP_SHADER_KEY *pKey,
    CHAR **ppSource
);

/* --------------------------------------------------------------- */
/*  FFP State Management                                           */
/* --------------------------------------------------------------- */

HRESULT
D3DInitializeFFPState(
    D3D_FFP_STATE *pState
);

HRESULT
D3DSetFFPTransform(
    D3D_FFP_STATE *pState,
    D3D_TRANSFORM_TYPE transformType,
    CONST D3D_MATRIX *pMatrix
);

HRESULT
D3DSetFFPMaterial(
    D3D_FFP_STATE *pState,
    CONST D3D_MATERIAL *pMaterial
);

HRESULT
D3DSetFFPLight(
    D3D_FFP_STATE *pState,
    UINT32 index,
    CONST D3D_LIGHT *pLight
);

HRESULT
D3DEnableFFPLight(
    D3D_FFP_STATE *pState,
    UINT32 index,
    BOOLEAN enable
);

HRESULT
D3DSetFFPTextureStage(
    D3D_FFP_STATE *pState,
    UINT32 stage,
    CONST D3D_TEXTURE_STAGE *pStage
);

HRESULT
D3DUpdateFFPShaderProgram(
    IGLDevice *pDevice,
    D3D_FFP_STATE *pState,
    CONST D3D_FVF_DESCRIPTOR *pFVF
);

HRESULT
D3DApplyFFPState(
    IGLContext *pContext,
    CONST D3D_FFP_STATE *pState
);

/* --------------------------------------------------------------- */
/*  Matrix Utilities                                               */
/* --------------------------------------------------------------- */

VOID
D3DMatrixIdentity(
    D3D_MATRIX *pMatrix
);

VOID
D3DMatrixMultiply(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM1,
    CONST D3D_MATRIX *pM2
);

VOID
D3DMatrixTranspose(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM
);

VOID
D3DMatrixInverse(
    D3D_MATRIX *pOut,
    CONST D3D_MATRIX *pM
);

/* --------------------------------------------------------------- */
/*  State Conversion Utilities                                     */
/* --------------------------------------------------------------- */

GLenum
D3DBlendToGL(
    DWORD blend
);

GLenum
D3DCmpFuncToGL(
    DWORD cmpFunc
);

GLenum
D3DCullModeToGL(
    DWORD cullMode
);

GLenum
D3DPrimitiveTypeToGL(
    DWORD primitiveType
);

/* --------------------------------------------------------------- */
/*  Shader Cache                                                   */
/* --------------------------------------------------------------- */

typedef struct _D3D_SHADER_CACHE D3D_SHADER_CACHE;

HRESULT
D3DCreateShaderCache(
    D3D_SHADER_CACHE **ppCache
);

VOID
D3DDestroyShaderCache(
    D3D_SHADER_CACHE *pCache
);

HRESULT
D3DShaderCacheLookup(
    D3D_SHADER_CACHE *pCache,
    CONST D3D_FFP_SHADER_KEY *pKey,
    IGLProgram **ppProgram
);

HRESULT
D3DShaderCacheInsert(
    D3D_SHADER_CACHE *pCache,
    CONST D3D_FFP_SHADER_KEY *pKey,
    IGLProgram *pProgram
);

#endif /* _ANANKE_D3D_COMMON_H_ */
