/*++
    Module Name:

        shadertranslator.c

    Abstract:

        HLSL to GLSL ES translator for Direct3D 9 shaders.
        Converts D3D9 shader bytecode to GLSL ES 1.0 source.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <GLES/gl.h>
#include "d3d9_internal.h"

/* D3D9 shader bytecode version tokens */
#define D3DVS_VERSION(major, minor) (0xFFFE0000 | ((major) << 8) | (minor))
#define D3DPS_VERSION(major, minor) (0xFFFF0000 | ((major) << 8) | (minor))

/* Shader instruction opcodes (simplified subset) */
#define D3DSIO_NOP      0
#define D3DSIO_MOV      1
#define D3DSIO_ADD      2
#define D3DSIO_SUB      3
#define D3DSIO_MUL      4
#define D3DSIO_MAD      5
#define D3DSIO_DP3      8
#define D3DSIO_DP4      9
#define D3DSIO_END      0xFFFF

/* --------------------------------------------------------------- */
/*  Shader Translator Context                                      */
/* --------------------------------------------------------------- */

typedef struct _SHADER_TRANSLATOR {
    CHAR *OutputBuffer;
    UINT32 OutputSize;
    UINT32 OutputPos;
    BOOLEAN IsVertexShader;
    UINT32 MajorVersion;
    UINT32 MinorVersion;
} SHADER_TRANSLATOR;

/* --------------------------------------------------------------- */
/*  Helper: Append string to output                                */
/* --------------------------------------------------------------- */

static HRESULT
AppendString(
    SHADER_TRANSLATOR *Translator,
    CONST CHAR *String)
{
    UINT32 len = 0;
    CONST CHAR *p = String;

    while (*p++) len++;

    if (Translator->OutputPos + len >= Translator->OutputSize) {
        return E_OUTOFMEMORY;
    }

    RtlCopyMemory(Translator->OutputBuffer + Translator->OutputPos, String, len);
    Translator->OutputPos += len;
    Translator->OutputBuffer[Translator->OutputPos] = '\0';

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Generate default vertex shader (pass-through)                  */
/* --------------------------------------------------------------- */

static HRESULT
GenerateDefaultVertexShader(
    SHADER_TRANSLATOR *Translator)
{
    HRESULT hr;

    hr = AppendString(Translator, "// Default vertex shader\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "attribute vec3 a_position;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "attribute vec4 a_color;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "attribute vec2 a_texcoord0;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "varying vec4 v_color;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "varying vec2 v_texcoord0;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "uniform mat4 u_mvpMatrix;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "void main() {\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "    gl_Position = u_mvpMatrix * vec4(a_position, 1.0);\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "    v_color = a_color;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "    v_texcoord0 = a_texcoord0;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "}\n");
    if (FAILED(hr)) return hr;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Generate default pixel shader (simple texture + color)         */
/* --------------------------------------------------------------- */

static HRESULT
GenerateDefaultPixelShader(
    SHADER_TRANSLATOR *Translator)
{
    HRESULT hr;

    hr = AppendString(Translator, "// Default pixel shader\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "#ifdef GL_ES\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "precision mediump float;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "#endif\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "varying vec4 v_color;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "varying vec2 v_texcoord0;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "uniform sampler2D u_texture0;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "void main() {\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "    vec4 texColor = texture2D(u_texture0, v_texcoord0);\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "    gl_FragColor = texColor * v_color;\n");
    if (FAILED(hr)) return hr;

    hr = AppendString(Translator, "}\n");
    if (FAILED(hr)) return hr;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Parse shader bytecode version                                  */
/* --------------------------------------------------------------- */

static HRESULT
ParseShaderVersion(
    CONST UINT32 *pFunction,
    SHADER_TRANSLATOR *Translator)
{
    UINT32 versionToken;

    if (!pFunction) {
        /* No bytecode provided, generate default */
        return S_OK;
    }

    versionToken = pFunction[0];

    /* Check if vertex shader */
    if ((versionToken & 0xFFFF0000) == 0xFFFE0000) {
        Translator->IsVertexShader = TRUE;
        Translator->MajorVersion = (versionToken >> 8) & 0xFF;
        Translator->MinorVersion = versionToken & 0xFF;
    }
    /* Check if pixel shader */
    else if ((versionToken & 0xFFFF0000) == 0xFFFF0000) {
        Translator->IsVertexShader = FALSE;
        Translator->MajorVersion = (versionToken >> 8) & 0xFF;
        Translator->MinorVersion = versionToken & 0xFF;
    }
    else {
        return E_INVALIDARG;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Translate D3D9 shader bytecode to GLSL                         */
/* --------------------------------------------------------------- */

HRESULT
D3D9TranslateShader(
    CONST UINT32 *pFunction,
    BOOLEAN IsVertexShader,
    CHAR **ppGLSLSource)
{
    SHADER_TRANSLATOR translator;
    HRESULT hr;

    if (!ppGLSLSource) return E_POINTER;

    /* Allocate output buffer */
    translator.OutputSize = 4096;  /* 4KB should be enough for basic shaders */
    translator.OutputBuffer = (CHAR*)RtlAllocateMemory(translator.OutputSize);
    if (!translator.OutputBuffer) {
        return E_OUTOFMEMORY;
    }

    translator.OutputPos = 0;
    translator.OutputBuffer[0] = '\0';
    translator.IsVertexShader = IsVertexShader;
    translator.MajorVersion = 2;
    translator.MinorVersion = 0;

    /* Parse shader bytecode if provided */
    if (pFunction) {
        hr = ParseShaderVersion(pFunction, &translator);
        if (FAILED(hr)) {
            RtlFreeMemory(translator.OutputBuffer);
            return hr;
        }
    }

    /* For now, generate default shaders
     * TODO: Implement full bytecode translation
     */
    if (translator.IsVertexShader) {
        hr = GenerateDefaultVertexShader(&translator);
    } else {
        hr = GenerateDefaultPixelShader(&translator);
    }

    if (FAILED(hr)) {
        RtlFreeMemory(translator.OutputBuffer);
        return hr;
    }

    *ppGLSLSource = translator.OutputBuffer;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Free translated shader source                                  */
/* --------------------------------------------------------------- */

VOID
D3D9FreeShaderSource(
    CHAR *pGLSLSource)
{
    if (pGLSLSource) {
        RtlFreeMemory(pGLSLSource);
    }
}

/* --------------------------------------------------------------- */
/*  Create and compile GL shader from D3D bytecode                 */
/* --------------------------------------------------------------- */

HRESULT
D3D9CreateGLShader(
    IGLDevice *pGLDevice,
    CONST UINT32 *pFunction,
    BOOLEAN IsVertexShader,
    IGLShader **ppGLShader,
    IGLProgram **ppGLProgram)
{
    CHAR *glslSource = NULL;
    IGLShader *glShader = NULL;
    IGLProgram *glProgram = NULL;
    GL_ENUM shaderType;
    GL_BOOLEAN compileStatus;
    CHAR infoLog[512];
    GL_SIZEI logLength;
    HRESULT hr;

    if (!pGLDevice || !ppGLShader || !ppGLProgram) {
        return E_POINTER;
    }

    /* Translate shader */
    hr = D3D9TranslateShader(pFunction, IsVertexShader, &glslSource);
    if (FAILED(hr)) {
        return hr;
    }

    /* Create GL shader */
    shaderType = IsVertexShader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    hr = IGLDevice_CreateShader(pGLDevice, shaderType, &glShader);
    if (FAILED(hr)) {
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Set shader source */
    CONST GL_CHAR *sources[1] = { glslSource };
    hr = IGLShader_ShaderSource(glShader, 1, sources, NULL);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Compile shader */
    hr = IGLShader_CompileShader(glShader);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Check compile status */
    hr = IGLShader_GetCompileStatus(glShader, &compileStatus);
    if (FAILED(hr) || !compileStatus) {
        IGLShader_GetInfoLog(glShader, sizeof(infoLog), &logLength, infoLog);
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return E_FAIL;
    }

    /* Create program */
    hr = IGLDevice_CreateProgram(pGLDevice, &glProgram);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Attach shader to program */
    hr = IGLProgram_AttachShader(glProgram, glShader);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)glProgram);
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Link program */
    hr = IGLProgram_LinkProgram(glProgram);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)glProgram);
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return hr;
    }

    /* Check link status */
    hr = IGLProgram_GetLinkStatus(glProgram, &compileStatus);
    if (FAILED(hr) || !compileStatus) {
        IGLProgram_GetInfoLog(glProgram, sizeof(infoLog), &logLength, infoLog);
        IUnknown_Release((IUnknown*)glProgram);
        IUnknown_Release((IUnknown*)glShader);
        D3D9FreeShaderSource(glslSource);
        return E_FAIL;
    }

    D3D9FreeShaderSource(glslSource);

    *ppGLShader = glShader;
    *ppGLProgram = glProgram;

    return S_OK;
}
