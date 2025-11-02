/*++
    Module Name:

        gles20com.h

    Abstract:

        COM wrapper interfaces for OpenGL ES 2.0 backend.
        Provides object-oriented access to the Vincent GLES20 library.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  GLES20 Type Definitions                                        */
/* --------------------------------------------------------------- */

typedef UINT32 GL_ENUM;
typedef UINT32 GL_BITFIELD;
typedef INT32  GL_INT;
typedef UINT32 GL_UINT;
typedef INT32  GL_SIZEI;
typedef FLOAT  GL_FLOAT;
typedef INT8   GL_BOOLEAN;
typedef VOID   GL_VOID;
typedef CHAR   GL_CHAR;

typedef UINT32 GL_BUFFER;
typedef UINT32 GL_TEXTURE;
typedef UINT32 GL_SHADER;
typedef UINT32 GL_PROGRAM;
typedef UINT32 GL_FRAMEBUFFER;
typedef UINT32 GL_RENDERBUFFER;

/* --------------------------------------------------------------- */
/*  IGLContext - Main GLES20 context interface                     */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLContext "D3D90001-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLContext,
    0xD3D90001, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLContext, IUnknown,
    IID_IGLContext, ANX_IID_IGLContext)

    /* Context Management */
    ANX_IFACE_METHOD(HRESULT, MakeCurrent, (VOID))

    /* Clear */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN GL_BITFIELD Mask))

    ANX_IFACE_METHOD(HRESULT, ClearColor, (
        IN GL_FLOAT Red,
        IN GL_FLOAT Green,
        IN GL_FLOAT Blue,
        IN GL_FLOAT Alpha))

    ANX_IFACE_METHOD(HRESULT, ClearDepth, (
        IN GL_FLOAT Depth))

    /* Viewport */
    ANX_IFACE_METHOD(HRESULT, Viewport, (
        IN GL_INT X,
        IN GL_INT Y,
        IN GL_SIZEI Width,
        IN GL_SIZEI Height))

    /* Drawing */
    ANX_IFACE_METHOD(HRESULT, DrawArrays, (
        IN GL_ENUM Mode,
        IN GL_INT First,
        IN GL_SIZEI Count))

    ANX_IFACE_METHOD(HRESULT, DrawElements, (
        IN GL_ENUM Mode,
        IN GL_SIZEI Count,
        IN GL_ENUM Type,
        IN CONST GL_VOID *Indices))

    /* State */
    ANX_IFACE_METHOD(HRESULT, Enable, (
        IN GL_ENUM Cap))

    ANX_IFACE_METHOD(HRESULT, Disable, (
        IN GL_ENUM Cap))

    ANX_IFACE_METHOD(HRESULT, BlendFunc, (
        IN GL_ENUM SFactor,
        IN GL_ENUM DFactor))

    ANX_IFACE_METHOD(HRESULT, DepthFunc, (
        IN GL_ENUM Func))

    ANX_IFACE_METHOD(HRESULT, CullFace, (
        IN GL_ENUM Mode))

    /* Swap buffers */
    ANX_IFACE_METHOD(HRESULT, SwapBuffers, (VOID))

    /* Vertex attributes */
    ANX_IFACE_METHOD(HRESULT, VertexAttribPointer, (
        IN GL_UINT Index,
        IN GL_INT Size,
        IN GL_ENUM Type,
        IN GL_BOOLEAN Normalized,
        IN GL_SIZEI Stride,
        IN CONST GL_VOID *Pointer))

    ANX_IFACE_METHOD(HRESULT, EnableVertexAttribArray, (
        IN GL_UINT Index))

    ANX_IFACE_METHOD(HRESULT, DisableVertexAttribArray, (
        IN GL_UINT Index))

    /* Multitexture */
    ANX_IFACE_METHOD(HRESULT, ActiveTexture, (
        IN GL_ENUM Texture))

ANX_END_INTERFACE(IGLContext)

/* --------------------------------------------------------------- */
/*  IGLBuffer - Buffer object interface                            */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLBuffer "D3D90002-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLBuffer,
    0xD3D90002, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLBuffer, IUnknown,
    IID_IGLBuffer, ANX_IID_IGLBuffer)

    ANX_IFACE_METHOD(HRESULT, Bind, (
        IN GL_ENUM Target))

    ANX_IFACE_METHOD(HRESULT, BufferData, (
        IN GL_ENUM Target,
        IN GL_SIZEI Size,
        IN CONST GL_VOID *Data,
        IN GL_ENUM Usage))

    ANX_IFACE_METHOD(HRESULT, BufferSubData, (
        IN GL_ENUM Target,
        IN INT32 Offset,
        IN GL_SIZEI Size,
        IN CONST GL_VOID *Data))

    ANX_IFACE_METHOD(HRESULT, GetBufferHandle, (
        OUT GL_BUFFER *Handle))

ANX_END_INTERFACE(IGLBuffer)

/* --------------------------------------------------------------- */
/*  IGLTexture - Texture object interface                          */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLTexture "D3D90003-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLTexture,
    0xD3D90003, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLTexture, IUnknown,
    IID_IGLTexture, ANX_IID_IGLTexture)

    ANX_IFACE_METHOD(HRESULT, Bind, (
        IN GL_ENUM Target))

    ANX_IFACE_METHOD(HRESULT, TexImage2D, (
        IN GL_ENUM Target,
        IN GL_INT Level,
        IN GL_INT InternalFormat,
        IN GL_SIZEI Width,
        IN GL_SIZEI Height,
        IN GL_INT Border,
        IN GL_ENUM Format,
        IN GL_ENUM Type,
        IN CONST GL_VOID *Pixels))

    ANX_IFACE_METHOD(HRESULT, TexSubImage2D, (
        IN GL_ENUM Target,
        IN GL_INT Level,
        IN GL_INT XOffset,
        IN GL_INT YOffset,
        IN GL_SIZEI Width,
        IN GL_SIZEI Height,
        IN GL_ENUM Format,
        IN GL_ENUM Type,
        IN CONST GL_VOID *Pixels))

    ANX_IFACE_METHOD(HRESULT, TexParameteri, (
        IN GL_ENUM Target,
        IN GL_ENUM Pname,
        IN GL_INT Param))

    ANX_IFACE_METHOD(HRESULT, GetTextureHandle, (
        OUT GL_TEXTURE *Handle))

ANX_END_INTERFACE(IGLTexture)

/* --------------------------------------------------------------- */
/*  IGLShader - Shader object interface                            */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLShader "D3D90004-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLShader,
    0xD3D90004, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLShader, IUnknown,
    IID_IGLShader, ANX_IID_IGLShader)

    ANX_IFACE_METHOD(HRESULT, ShaderSource, (
        IN GL_SIZEI Count,
        IN CONST GL_CHAR **String,
        IN CONST GL_INT *Length))

    ANX_IFACE_METHOD(HRESULT, CompileShader, (VOID))

    ANX_IFACE_METHOD(HRESULT, GetCompileStatus, (
        OUT GL_BOOLEAN *Status))

    ANX_IFACE_METHOD(HRESULT, GetInfoLog, (
        IN GL_SIZEI BufSize,
        OUT GL_SIZEI *Length,
        OUT GL_CHAR *InfoLog))

    ANX_IFACE_METHOD(HRESULT, GetShaderHandle, (
        OUT GL_SHADER *Handle))

ANX_END_INTERFACE(IGLShader)

/* --------------------------------------------------------------- */
/*  IGLProgram - Shader program interface                          */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLProgram "D3D90005-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLProgram,
    0xD3D90005, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLProgram, IUnknown,
    IID_IGLProgram, ANX_IID_IGLProgram)

    ANX_IFACE_METHOD(HRESULT, AttachShader, (
        IN IGLShader *Shader))

    ANX_IFACE_METHOD(HRESULT, LinkProgram, (VOID))

    ANX_IFACE_METHOD(HRESULT, UseProgram, (VOID))

    ANX_IFACE_METHOD(HRESULT, GetLinkStatus, (
        OUT GL_BOOLEAN *Status))

    ANX_IFACE_METHOD(HRESULT, GetInfoLog, (
        IN GL_SIZEI BufSize,
        OUT GL_SIZEI *Length,
        OUT GL_CHAR *InfoLog))

    ANX_IFACE_METHOD(HRESULT, GetUniformLocation, (
        IN CONST GL_CHAR *Name,
        OUT GL_INT *Location))

    ANX_IFACE_METHOD(HRESULT, GetAttribLocation, (
        IN CONST GL_CHAR *Name,
        OUT GL_INT *Location))

    ANX_IFACE_METHOD(HRESULT, Uniform1f, (
        IN GL_INT Location,
        IN GL_FLOAT V0))

    ANX_IFACE_METHOD(HRESULT, Uniform2f, (
        IN GL_INT Location,
        IN GL_FLOAT V0,
        IN GL_FLOAT V1))

    ANX_IFACE_METHOD(HRESULT, Uniform3f, (
        IN GL_INT Location,
        IN GL_FLOAT V0,
        IN GL_FLOAT V1,
        IN GL_FLOAT V2))

    ANX_IFACE_METHOD(HRESULT, Uniform4f, (
        IN GL_INT Location,
        IN GL_FLOAT V0,
        IN GL_FLOAT V1,
        IN GL_FLOAT V2,
        IN GL_FLOAT V3))

    ANX_IFACE_METHOD(HRESULT, UniformMatrix4fv, (
        IN GL_INT Location,
        IN GL_SIZEI Count,
        IN GL_BOOLEAN Transpose,
        IN CONST GL_FLOAT *Value))

    ANX_IFACE_METHOD(HRESULT, GetProgramHandle, (
        OUT GL_PROGRAM *Handle))

ANX_END_INTERFACE(IGLProgram)

/* --------------------------------------------------------------- */
/*  IGLDevice - Device factory interface                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IGLDevice "D3D90006-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IGLDevice,
    0xD3D90006, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IGLDevice, IUnknown,
    IID_IGLDevice, ANX_IID_IGLDevice)

    /* Context */
    ANX_IFACE_METHOD(HRESULT, GetContext, (
        OUT IGLContext **Context))

    /* Buffer objects */
    ANX_IFACE_METHOD(HRESULT, CreateBuffer, (
        OUT IGLBuffer **Buffer))

    /* Texture objects */
    ANX_IFACE_METHOD(HRESULT, CreateTexture, (
        OUT IGLTexture **Texture))

    /* Shader objects */
    ANX_IFACE_METHOD(HRESULT, CreateShader, (
        IN GL_ENUM Type,
        OUT IGLShader **Shader))

    /* Program objects */
    ANX_IFACE_METHOD(HRESULT, CreateProgram, (
        OUT IGLProgram **Program))

ANX_END_INTERFACE(IGLDevice)

/* --------------------------------------------------------------- */
/*  Factory function                                                */
/* --------------------------------------------------------------- */

HRESULT
AnxCreateGLDevice(
    OUT IGLDevice **Device
);

#ifndef __cplusplus

/* C Helper Macros */
#define IGLContext_MakeCurrent(This) \
    ((This)->lpVtbl->MakeCurrent(This))
#define IGLContext_Clear(This, Mask) \
    ((This)->lpVtbl->Clear(This, Mask))
#define IGLContext_ClearColor(This, R, G, B, A) \
    ((This)->lpVtbl->ClearColor(This, R, G, B, A))
#define IGLContext_Viewport(This, X, Y, W, H) \
    ((This)->lpVtbl->Viewport(This, X, Y, W, H))
#define IGLContext_DrawArrays(This, Mode, First, Count) \
    ((This)->lpVtbl->DrawArrays(This, Mode, First, Count))
#define IGLContext_DrawElements(This, Mode, Count, Type, Indices) \
    ((This)->lpVtbl->DrawElements(This, Mode, Count, Type, Indices))

#define IGLDevice_GetContext(This, Context) \
    ((This)->lpVtbl->GetContext(This, Context))
#define IGLDevice_CreateBuffer(This, Buffer) \
    ((This)->lpVtbl->CreateBuffer(This, Buffer))
#define IGLDevice_CreateTexture(This, Texture) \
    ((This)->lpVtbl->CreateTexture(This, Texture))
#define IGLDevice_CreateShader(This, Type, Shader) \
    ((This)->lpVtbl->CreateShader(This, Type, Shader))
#define IGLDevice_CreateProgram(This, Program) \
    ((This)->lpVtbl->CreateProgram(This, Program))

#endif /* !__cplusplus */
