/*++
    Module Name:

        ffp_generator.c

    Abstract:

        Fixed-Function Pipeline shader generator for Direct3D 3-7.
        Generates GLSL ES shaders that emulate D3D fixed-function pipeline.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d_common.h>

/* --------------------------------------------------------------- */
/*  String Builder Helper                                          */
/* --------------------------------------------------------------- */

typedef struct _STRING_BUILDER {
    CHAR *buffer;
    UINT32 size;
    UINT32 pos;
} STRING_BUILDER;

static HRESULT
SBInit(STRING_BUILDER *sb, UINT32 initialSize)
{
    sb->buffer = (CHAR*)RtlAllocateMemory(initialSize);
    if (!sb->buffer) return E_OUTOFMEMORY;

    sb->size = initialSize;
    sb->pos = 0;
    sb->buffer[0] = '\0';
    return S_OK;
}

static VOID
SBFree(STRING_BUILDER *sb)
{
    if (sb->buffer) {
        RtlFreeMemory(sb->buffer);
        sb->buffer = NULL;
    }
}

static HRESULT
SBAppend(STRING_BUILDER *sb, CONST CHAR *str)
{
    UINT32 len = 0;
    CONST CHAR *p = str;

    while (*p++) len++;

    /* Expand buffer if needed */
    if (sb->pos + len + 1 >= sb->size) {
        UINT32 newSize = sb->size * 2;
        CHAR *newBuffer = (CHAR*)RtlAllocateMemory(newSize);
        if (!newBuffer) return E_OUTOFMEMORY;

        RtlCopyMemory(newBuffer, sb->buffer, sb->pos);
        RtlFreeMemory(sb->buffer);
        sb->buffer = newBuffer;
        sb->size = newSize;
    }

    RtlCopyMemory(sb->buffer + sb->pos, str, len);
    sb->pos += len;
    sb->buffer[sb->pos] = '\0';

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Generate Vertex Shader                                         */
/* --------------------------------------------------------------- */

HRESULT
D3DGenerateVertexShader(
    CONST D3D_FFP_SHADER_KEY *pKey,
    CONST D3D_FVF_DESCRIPTOR *pFVF,
    CHAR **ppSource)
{
    STRING_BUILDER sb;
    HRESULT hr;
    UINT32 i;

    if (!pKey || !pFVF || !ppSource) return E_POINTER;

    hr = SBInit(&sb, 4096);
    if (FAILED(hr)) return hr;

    /* Header */
    SBAppend(&sb, "// Generated FFP Vertex Shader\n");
    SBAppend(&sb, "precision mediump float;\n\n");

    /* Attributes */
    if (pFVF->hasPosition) {
        SBAppend(&sb, "attribute vec3 a_position;\n");
    }
    if (pFVF->hasNormal) {
        SBAppend(&sb, "attribute vec3 a_normal;\n");
    }
    if (pFVF->hasDiffuse) {
        SBAppend(&sb, "attribute vec4 a_diffuse;\n");
    }
    if (pFVF->hasSpecular) {
        SBAppend(&sb, "attribute vec4 a_specular;\n");
    }
    for (i = 0; i < pFVF->texCoordCount; i++) {
        SBAppend(&sb, "attribute vec2 a_texcoord");
        /* Append number */
        CHAR num[2] = {'0' + (CHAR)i, '\0'};
        SBAppend(&sb, num);
        SBAppend(&sb, ";\n");
    }
    SBAppend(&sb, "\n");

    /* Uniforms */
    SBAppend(&sb, "uniform mat4 u_worldMatrix;\n");
    SBAppend(&sb, "uniform mat4 u_viewMatrix;\n");
    SBAppend(&sb, "uniform mat4 u_projMatrix;\n");

    if (pKey->lightingEnabled) {
        SBAppend(&sb, "uniform mat3 u_normalMatrix;\n");
        SBAppend(&sb, "uniform vec4 u_materialAmbient;\n");
        SBAppend(&sb, "uniform vec4 u_materialDiffuse;\n");
        SBAppend(&sb, "uniform vec4 u_materialSpecular;\n");
        SBAppend(&sb, "uniform float u_materialPower;\n");
        SBAppend(&sb, "uniform vec4 u_globalAmbient;\n");

        /* Light uniforms */
        for (i = 0; i < D3D_MAX_LIGHTS; i++) {
            if (pKey->activeLightMask & (1 << i)) {
                SBAppend(&sb, "uniform int u_lightType");
                CHAR num[2] = {'0' + (CHAR)i, '\0'};
                SBAppend(&sb, num);
                SBAppend(&sb, ";\n");

                SBAppend(&sb, "uniform vec3 u_lightPos");
                SBAppend(&sb, num);
                SBAppend(&sb, ";\n");

                SBAppend(&sb, "uniform vec3 u_lightDir");
                SBAppend(&sb, num);
                SBAppend(&sb, ";\n");

                SBAppend(&sb, "uniform vec4 u_lightDiffuse");
                SBAppend(&sb, num);
                SBAppend(&sb, ";\n");

                SBAppend(&sb, "uniform vec4 u_lightSpecular");
                SBAppend(&sb, num);
                SBAppend(&sb, ";\n");
            }
        }
    }

    SBAppend(&sb, "\n");

    /* Varyings */
    SBAppend(&sb, "varying vec4 v_color;\n");
    if (pFVF->hasSpecular) {
        SBAppend(&sb, "varying vec4 v_specular;\n");
    }
    for (i = 0; i < pFVF->texCoordCount; i++) {
        SBAppend(&sb, "varying vec2 v_texcoord");
        CHAR num[2] = {'0' + (CHAR)i, '\0'};
        SBAppend(&sb, num);
        SBAppend(&sb, ";\n");
    }
    SBAppend(&sb, "\n");

    /* Lighting function */
    if (pKey->lightingEnabled) {
        SBAppend(&sb, "vec4 calculateLighting(vec3 worldPos, vec3 worldNormal) {\n");
        SBAppend(&sb, "    vec4 color = u_globalAmbient * u_materialAmbient;\n\n");

        for (i = 0; i < D3D_MAX_LIGHTS; i++) {
            if (pKey->activeLightMask & (1 << i)) {
                CHAR num[2] = {'0' + (CHAR)i, '\0'};

                SBAppend(&sb, "    // Light ");
                SBAppend(&sb, num);
                SBAppend(&sb, "\n");
                SBAppend(&sb, "    {\n");

                /* Directional light */
                SBAppend(&sb, "        if (u_lightType");
                SBAppend(&sb, num);
                SBAppend(&sb, " == 3) {\n");
                SBAppend(&sb, "            vec3 lightDir = normalize(-u_lightDir");
                SBAppend(&sb, num);
                SBAppend(&sb, ");\n");
                SBAppend(&sb, "            float ndotl = max(dot(worldNormal, lightDir), 0.0);\n");
                SBAppend(&sb, "            color += u_lightDiffuse");
                SBAppend(&sb, num);
                SBAppend(&sb, " * u_materialDiffuse * ndotl;\n");
                SBAppend(&sb, "        }\n");

                /* Point light */
                SBAppend(&sb, "        else if (u_lightType");
                SBAppend(&sb, num);
                SBAppend(&sb, " == 1) {\n");
                SBAppend(&sb, "            vec3 lightDir = normalize(u_lightPos");
                SBAppend(&sb, num);
                SBAppend(&sb, " - worldPos);\n");
                SBAppend(&sb, "            float ndotl = max(dot(worldNormal, lightDir), 0.0);\n");
                SBAppend(&sb, "            color += u_lightDiffuse");
                SBAppend(&sb, num);
                SBAppend(&sb, " * u_materialDiffuse * ndotl;\n");
                SBAppend(&sb, "        }\n");

                SBAppend(&sb, "    }\n\n");
            }
        }

        SBAppend(&sb, "    return clamp(color, 0.0, 1.0);\n");
        SBAppend(&sb, "}\n\n");
    }

    /* Main function */
    SBAppend(&sb, "void main() {\n");

    /* Transform position */
    if (pFVF->hasRHW) {
        /* Pre-transformed vertices */
        SBAppend(&sb, "    gl_Position = vec4(a_position, 1.0);\n");
    } else {
        SBAppend(&sb, "    vec4 worldPos = u_worldMatrix * vec4(a_position, 1.0);\n");
        SBAppend(&sb, "    vec4 viewPos = u_viewMatrix * worldPos;\n");
        SBAppend(&sb, "    gl_Position = u_projMatrix * viewPos;\n");
    }

    /* Calculate lighting or pass through color */
    if (pKey->lightingEnabled && pFVF->hasNormal) {
        SBAppend(&sb, "    vec3 worldNormal = u_normalMatrix * a_normal;\n");
        SBAppend(&sb, "    worldNormal = normalize(worldNormal);\n");
        SBAppend(&sb, "    v_color = calculateLighting(worldPos.xyz, worldNormal);\n");
    } else if (pFVF->hasDiffuse) {
        SBAppend(&sb, "    v_color = a_diffuse;\n");
    } else {
        SBAppend(&sb, "    v_color = vec4(1.0, 1.0, 1.0, 1.0);\n");
    }

    /* Specular */
    if (pFVF->hasSpecular) {
        SBAppend(&sb, "    v_specular = a_specular;\n");
    }

    /* Texture coordinates */
    for (i = 0; i < pFVF->texCoordCount; i++) {
        SBAppend(&sb, "    v_texcoord");
        CHAR num[2] = {'0' + (CHAR)i, '\0'};
        SBAppend(&sb, num);
        SBAppend(&sb, " = a_texcoord");
        SBAppend(&sb, num);
        SBAppend(&sb, ";\n");
    }

    SBAppend(&sb, "}\n");

    *ppSource = sb.buffer;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Generate Pixel Shader                                          */
/* --------------------------------------------------------------- */

HRESULT
D3DGeneratePixelShader(
    CONST D3D_FFP_SHADER_KEY *pKey,
    CHAR **ppSource)
{
    STRING_BUILDER sb;
    HRESULT hr;
    UINT32 stage;

    if (!pKey || !ppSource) return E_POINTER;

    hr = SBInit(&sb, 2048);
    if (FAILED(hr)) return hr;

    /* Header */
    SBAppend(&sb, "// Generated FFP Pixel Shader\n");
    SBAppend(&sb, "#ifdef GL_ES\n");
    SBAppend(&sb, "precision mediump float;\n");
    SBAppend(&sb, "#endif\n\n");

    /* Varyings */
    SBAppend(&sb, "varying vec4 v_color;\n");

    /* Texture samplers and coordinates */
    for (stage = 0; stage < pKey->activeTextureStages; stage++) {
        if (pKey->stageOps[stage] != D3D_TOP_DISABLE) {
            SBAppend(&sb, "uniform sampler2D u_texture");
            CHAR num[2] = {'0' + (CHAR)stage, '\0'};
            SBAppend(&sb, num);
            SBAppend(&sb, ";\n");

            SBAppend(&sb, "varying vec2 v_texcoord");
            SBAppend(&sb, num);
            SBAppend(&sb, ";\n");
        }
    }
    SBAppend(&sb, "\n");

    /* Main function */
    SBAppend(&sb, "void main() {\n");
    SBAppend(&sb, "    vec4 color = v_color;\n\n");

    /* Process texture stages */
    for (stage = 0; stage < pKey->activeTextureStages; stage++) {
        D3D_TEXTURE_OP op = pKey->stageOps[stage];

        if (op == D3D_TOP_DISABLE) break;

        CHAR numStr[2] = {'0' + (CHAR)stage, '\0'};

        SBAppend(&sb, "    // Stage ");
        SBAppend(&sb, numStr);
        SBAppend(&sb, "\n");

        /* Sample texture */
        SBAppend(&sb, "    vec4 texColor");
        SBAppend(&sb, numStr);
        SBAppend(&sb, " = texture2D(u_texture");
        SBAppend(&sb, numStr);
        SBAppend(&sb, ", v_texcoord");
        SBAppend(&sb, numStr);
        SBAppend(&sb, ");\n");

        /* Apply operation */
        switch (op) {
        case D3D_TOP_SELECTARG1:
            /* Determine arg1 */
            if (pKey->stageArgs1[stage] == D3D_TA_TEXTURE) {
                SBAppend(&sb, "    color = texColor");
                SBAppend(&sb, numStr);
                SBAppend(&sb, ";\n");
            }
            break;

        case D3D_TOP_SELECTARG2:
            /* Determine arg2 */
            if (pKey->stageArgs2[stage] == D3D_TA_TEXTURE) {
                SBAppend(&sb, "    color = texColor");
                SBAppend(&sb, numStr);
                SBAppend(&sb, ";\n");
            }
            break;

        case D3D_TOP_MODULATE:
            SBAppend(&sb, "    color = color * texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ";\n");
            break;

        case D3D_TOP_MODULATE2X:
            SBAppend(&sb, "    color = color * texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, " * 2.0;\n");
            break;

        case D3D_TOP_MODULATE4X:
            SBAppend(&sb, "    color = color * texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, " * 4.0;\n");
            break;

        case D3D_TOP_ADD:
            SBAppend(&sb, "    color = color + texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ";\n");
            break;

        case D3D_TOP_ADDSIGNED:
            SBAppend(&sb, "    color = color + texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, " - 0.5;\n");
            break;

        case D3D_TOP_SUBTRACT:
            SBAppend(&sb, "    color = color - texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ";\n");
            break;

        case D3D_TOP_BLENDTEXTUREALPHA:
            SBAppend(&sb, "    color = mix(color, texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ", texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ".a);\n");
            break;

        default:
            /* Default to modulate */
            SBAppend(&sb, "    color = color * texColor");
            SBAppend(&sb, numStr);
            SBAppend(&sb, ";\n");
            break;
        }

        SBAppend(&sb, "\n");
    }

    SBAppend(&sb, "    gl_FragColor = clamp(color, 0.0, 1.0);\n");
    SBAppend(&sb, "}\n");

    *ppSource = sb.buffer;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Generate Complete FFP Shader Program                           */
/* --------------------------------------------------------------- */

HRESULT
D3DGenerateFFPShader(
    IGLDevice *pDevice,
    CONST D3D_FFP_SHADER_KEY *pKey,
    CONST D3D_FVF_DESCRIPTOR *pFVF,
    IGLProgram **ppProgram)
{
    CHAR *vertexSource = NULL;
    CHAR *pixelSource = NULL;
    IGLShader *vertexShader = NULL;
    IGLShader *pixelShader = NULL;
    IGLProgram *program = NULL;
    GL_BOOLEAN compileStatus;
    HRESULT hr;

    if (!pDevice || !pKey || !pFVF || !ppProgram) {
        return E_POINTER;
    }

    /* Generate vertex shader source */
    hr = D3DGenerateVertexShader(pKey, pFVF, &vertexSource);
    if (FAILED(hr)) goto cleanup;

    /* Generate pixel shader source */
    hr = D3DGeneratePixelShader(pKey, &pixelSource);
    if (FAILED(hr)) goto cleanup;

    /* Create and compile vertex shader */
    hr = IGLDevice_CreateShader(pDevice, 0x8B31 /* GL_VERTEX_SHADER */, &vertexShader);
    if (FAILED(hr)) goto cleanup;

    CONST CHAR *vsSources[1] = { vertexSource };
    IGLShader_ShaderSource(vertexShader, 1, vsSources, NULL);
    IGLShader_CompileShader(vertexShader);
    IGLShader_GetCompileStatus(vertexShader, &compileStatus);
    if (!compileStatus) {
        hr = E_FAIL;
        goto cleanup;
    }

    /* Create and compile pixel shader */
    hr = IGLDevice_CreateShader(pDevice, 0x8B30 /* GL_FRAGMENT_SHADER */, &pixelShader);
    if (FAILED(hr)) goto cleanup;

    CONST CHAR *psSources[1] = { pixelSource };
    IGLShader_ShaderSource(pixelShader, 1, psSources, NULL);
    IGLShader_CompileShader(pixelShader);
    IGLShader_GetCompileStatus(pixelShader, &compileStatus);
    if (!compileStatus) {
        hr = E_FAIL;
        goto cleanup;
    }

    /* Create program */
    hr = IGLDevice_CreateProgram(pDevice, &program);
    if (FAILED(hr)) goto cleanup;

    /* Attach shaders */
    IGLProgram_AttachShader(program, vertexShader);
    IGLProgram_AttachShader(program, pixelShader);

    /* Link program */
    IGLProgram_LinkProgram(program);
    IGLProgram_GetLinkStatus(program, &compileStatus);
    if (!compileStatus) {
        hr = E_FAIL;
        goto cleanup;
    }

    *ppProgram = program;
    hr = S_OK;

cleanup:
    if (vertexSource) RtlFreeMemory(vertexSource);
    if (pixelSource) RtlFreeMemory(pixelSource);
    if (vertexShader) IUnknown_Release((IUnknown*)vertexShader);
    if (pixelShader) IUnknown_Release((IUnknown*)pixelShader);
    if (FAILED(hr) && program) IUnknown_Release((IUnknown*)program);

    return hr;
}
