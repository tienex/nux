/*++
    Module Name:

        gldevice.c

    Abstract:

        COM wrapper implementation for OpenGL ES 2.0 backend.
        Wraps the Vincent GLES20 C API into COM interfaces.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/gles20com.h>
#include <ananke/ntrtl.h>
#include <GLES/gl.h>

/* --------------------------------------------------------------- */
/*  Forward Declarations                                           */
/* --------------------------------------------------------------- */

typedef struct _GL_CONTEXT GL_CONTEXT;
typedef struct _GL_BUFFER GL_BUFFER;
typedef struct _GL_TEXTURE GL_TEXTURE;
typedef struct _GL_SHADER GL_SHADER;
typedef struct _GL_PROGRAM GL_PROGRAM;
typedef struct _GL_DEVICE GL_DEVICE;

/* --------------------------------------------------------------- */
/*  GL Context Implementation                                      */
/* --------------------------------------------------------------- */

struct _GL_CONTEXT {
    IGLContextVtbl *lpVtbl;
    UINT32          RefCount;
    GL_DEVICE      *Device;
};

static HRESULT STDMETHODCALLTYPE
GlContext_QueryInterface(
    IGLContext *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLContext))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlContext_AddRef(IGLContext *This)
{
    GL_CONTEXT *ctx = (GL_CONTEXT*)This;
    return ++ctx->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlContext_Release(IGLContext *This)
{
    GL_CONTEXT *ctx = (GL_CONTEXT*)This;
    UINT32 refCount = --ctx->RefCount;

    if (refCount == 0) {
        RtlFreeMemory(ctx);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlContext_MakeCurrent(IGLContext *This)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_Clear(IGLContext *This, GL_BITFIELD Mask)
{
    glClear(Mask);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_ClearColor(
    IGLContext *This,
    GL_FLOAT Red,
    GL_FLOAT Green,
    GL_FLOAT Blue,
    GL_FLOAT Alpha)
{
    glClearColor(Red, Green, Blue, Alpha);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_ClearDepth(IGLContext *This, GL_FLOAT Depth)
{
    glClearDepthf(Depth);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_Viewport(
    IGLContext *This,
    GL_INT X,
    GL_INT Y,
    GL_SIZEI Width,
    GL_SIZEI Height)
{
    glViewport(X, Y, Width, Height);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_DrawArrays(
    IGLContext *This,
    GL_ENUM Mode,
    GL_INT First,
    GL_SIZEI Count)
{
    glDrawArrays(Mode, First, Count);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_DrawElements(
    IGLContext *This,
    GL_ENUM Mode,
    GL_SIZEI Count,
    GL_ENUM Type,
    CONST GL_VOID *Indices)
{
    glDrawElements(Mode, Count, Type, Indices);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_Enable(IGLContext *This, GL_ENUM Cap)
{
    glEnable(Cap);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_Disable(IGLContext *This, GL_ENUM Cap)
{
    glDisable(Cap);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_BlendFunc(
    IGLContext *This,
    GL_ENUM SFactor,
    GL_ENUM DFactor)
{
    glBlendFunc(SFactor, DFactor);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_DepthFunc(IGLContext *This, GL_ENUM Func)
{
    glDepthFunc(Func);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_CullFace(IGLContext *This, GL_ENUM Mode)
{
    glCullFace(Mode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_SwapBuffers(IGLContext *This)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_VertexAttribPointer(
    IGLContext *This,
    GL_UINT Index,
    GL_INT Size,
    GL_ENUM Type,
    GL_BOOLEAN Normalized,
    GL_SIZEI Stride,
    CONST GL_VOID *Pointer)
{
    glVertexAttribPointer(Index, Size, Type, Normalized, Stride, Pointer);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_EnableVertexAttribArray(IGLContext *This, GL_UINT Index)
{
    glEnableVertexAttribArray(Index);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_DisableVertexAttribArray(IGLContext *This, GL_UINT Index)
{
    glDisableVertexAttribArray(Index);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlContext_ActiveTexture(IGLContext *This, GL_ENUM Texture)
{
    glActiveTexture(Texture);
    return S_OK;
}

static IGLContextVtbl GlContextVtbl = {
    .QueryInterface          = GlContext_QueryInterface,
    .AddRef                  = GlContext_AddRef,
    .Release                 = GlContext_Release,
    .MakeCurrent             = GlContext_MakeCurrent,
    .Clear                   = GlContext_Clear,
    .ClearColor              = GlContext_ClearColor,
    .ClearDepth              = GlContext_ClearDepth,
    .Viewport                = GlContext_Viewport,
    .DrawArrays              = GlContext_DrawArrays,
    .DrawElements            = GlContext_DrawElements,
    .Enable                  = GlContext_Enable,
    .Disable                 = GlContext_Disable,
    .BlendFunc               = GlContext_BlendFunc,
    .DepthFunc               = GlContext_DepthFunc,
    .CullFace                = GlContext_CullFace,
    .SwapBuffers             = GlContext_SwapBuffers,
    .VertexAttribPointer     = GlContext_VertexAttribPointer,
    .EnableVertexAttribArray = GlContext_EnableVertexAttribArray,
    .DisableVertexAttribArray= GlContext_DisableVertexAttribArray,
    .ActiveTexture           = GlContext_ActiveTexture,
};

/* --------------------------------------------------------------- */
/*  GL Buffer Implementation                                       */
/* --------------------------------------------------------------- */

struct _GL_BUFFER {
    IGLBufferVtbl *lpVtbl;
    UINT32         RefCount;
    GLuint         Handle;
};

static HRESULT STDMETHODCALLTYPE
GlBuffer_QueryInterface(
    IGLBuffer *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLBuffer))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlBuffer_AddRef(IGLBuffer *This)
{
    GL_BUFFER *buf = (GL_BUFFER*)This;
    return ++buf->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlBuffer_Release(IGLBuffer *This)
{
    GL_BUFFER *buf = (GL_BUFFER*)This;
    UINT32 refCount = --buf->RefCount;

    if (refCount == 0) {
        if (buf->Handle) {
            glDeleteBuffers(1, &buf->Handle);
        }
        RtlFreeMemory(buf);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlBuffer_Bind(IGLBuffer *This, GL_ENUM Target)
{
    GL_BUFFER *buf = (GL_BUFFER*)This;
    glBindBuffer(Target, buf->Handle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlBuffer_BufferData(
    IGLBuffer *This,
    GL_ENUM Target,
    GL_SIZEI Size,
    CONST GL_VOID *Data,
    GL_ENUM Usage)
{
    glBufferData(Target, Size, Data, Usage);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlBuffer_BufferSubData(
    IGLBuffer *This,
    GL_ENUM Target,
    INT32 Offset,
    GL_SIZEI Size,
    CONST GL_VOID *Data)
{
    glBufferSubData(Target, Offset, Size, Data);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlBuffer_GetBufferHandle(
    IGLBuffer *This,
    GL_BUFFER *Handle)
{
    GL_BUFFER *buf = (GL_BUFFER*)This;
    if (!Handle) return E_POINTER;
    *Handle = buf->Handle;
    return S_OK;
}

static IGLBufferVtbl GlBufferVtbl = {
    .QueryInterface  = GlBuffer_QueryInterface,
    .AddRef          = GlBuffer_AddRef,
    .Release         = GlBuffer_Release,
    .Bind            = GlBuffer_Bind,
    .BufferData      = GlBuffer_BufferData,
    .BufferSubData   = GlBuffer_BufferSubData,
    .GetBufferHandle = GlBuffer_GetBufferHandle,
};

/* --------------------------------------------------------------- */
/*  GL Texture Implementation                                      */
/* --------------------------------------------------------------- */

struct _GL_TEXTURE {
    IGLTextureVtbl *lpVtbl;
    UINT32          RefCount;
    GLuint          Handle;
};

static HRESULT STDMETHODCALLTYPE
GlTexture_QueryInterface(
    IGLTexture *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLTexture))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlTexture_AddRef(IGLTexture *This)
{
    GL_TEXTURE *tex = (GL_TEXTURE*)This;
    return ++tex->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlTexture_Release(IGLTexture *This)
{
    GL_TEXTURE *tex = (GL_TEXTURE*)This;
    UINT32 refCount = --tex->RefCount;

    if (refCount == 0) {
        if (tex->Handle) {
            glDeleteTextures(1, &tex->Handle);
        }
        RtlFreeMemory(tex);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlTexture_Bind(IGLTexture *This, GL_ENUM Target)
{
    GL_TEXTURE *tex = (GL_TEXTURE*)This;
    glBindTexture(Target, tex->Handle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlTexture_TexImage2D(
    IGLTexture *This,
    GL_ENUM Target,
    GL_INT Level,
    GL_INT InternalFormat,
    GL_SIZEI Width,
    GL_SIZEI Height,
    GL_INT Border,
    GL_ENUM Format,
    GL_ENUM Type,
    CONST GL_VOID *Pixels)
{
    glTexImage2D(Target, Level, InternalFormat, Width, Height, Border, Format, Type, Pixels);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlTexture_TexSubImage2D(
    IGLTexture *This,
    GL_ENUM Target,
    GL_INT Level,
    GL_INT XOffset,
    GL_INT YOffset,
    GL_SIZEI Width,
    GL_SIZEI Height,
    GL_ENUM Format,
    GL_ENUM Type,
    CONST GL_VOID *Pixels)
{
    glTexSubImage2D(Target, Level, XOffset, YOffset, Width, Height, Format, Type, Pixels);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlTexture_TexParameteri(
    IGLTexture *This,
    GL_ENUM Target,
    GL_ENUM Pname,
    GL_INT Param)
{
    glTexParameteri(Target, Pname, Param);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlTexture_GetTextureHandle(
    IGLTexture *This,
    GL_TEXTURE *Handle)
{
    GL_TEXTURE *tex = (GL_TEXTURE*)This;
    if (!Handle) return E_POINTER;
    *Handle = tex->Handle;
    return S_OK;
}

static IGLTextureVtbl GlTextureVtbl = {
    .QueryInterface     = GlTexture_QueryInterface,
    .AddRef             = GlTexture_AddRef,
    .Release            = GlTexture_Release,
    .Bind               = GlTexture_Bind,
    .TexImage2D         = GlTexture_TexImage2D,
    .TexSubImage2D      = GlTexture_TexSubImage2D,
    .TexParameteri      = GlTexture_TexParameteri,
    .GetTextureHandle   = GlTexture_GetTextureHandle,
};

/* --------------------------------------------------------------- */
/*  GL Shader Implementation                                       */
/* --------------------------------------------------------------- */

struct _GL_SHADER {
    IGLShaderVtbl *lpVtbl;
    UINT32         RefCount;
    GLuint         Handle;
};

static HRESULT STDMETHODCALLTYPE
GlShader_QueryInterface(
    IGLShader *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLShader))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlShader_AddRef(IGLShader *This)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    return ++shader->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlShader_Release(IGLShader *This)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    UINT32 refCount = --shader->RefCount;

    if (refCount == 0) {
        if (shader->Handle) {
            glDeleteShader(shader->Handle);
        }
        RtlFreeMemory(shader);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlShader_ShaderSource(
    IGLShader *This,
    GL_SIZEI Count,
    CONST GL_CHAR **String,
    CONST GL_INT *Length)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    glShaderSource(shader->Handle, Count, String, Length);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlShader_CompileShader(IGLShader *This)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    glCompileShader(shader->Handle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlShader_GetCompileStatus(
    IGLShader *This,
    GL_BOOLEAN *Status)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    GLint status = 0;

    if (!Status) return E_POINTER;

    glGetShaderiv(shader->Handle, GL_COMPILE_STATUS, &status);
    *Status = (GL_BOOLEAN)status;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlShader_GetInfoLog(
    IGLShader *This,
    GL_SIZEI BufSize,
    GL_SIZEI *Length,
    GL_CHAR *InfoLog)
{
    GL_SHADER *shader = (GL_SHADER*)This;

    if (!InfoLog) return E_POINTER;

    glGetShaderInfoLog(shader->Handle, BufSize, Length, InfoLog);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlShader_GetShaderHandle(
    IGLShader *This,
    GL_SHADER *Handle)
{
    GL_SHADER *shader = (GL_SHADER*)This;
    if (!Handle) return E_POINTER;
    *Handle = shader->Handle;
    return S_OK;
}

static IGLShaderVtbl GlShaderVtbl = {
    .QueryInterface    = GlShader_QueryInterface,
    .AddRef            = GlShader_AddRef,
    .Release           = GlShader_Release,
    .ShaderSource      = GlShader_ShaderSource,
    .CompileShader     = GlShader_CompileShader,
    .GetCompileStatus  = GlShader_GetCompileStatus,
    .GetInfoLog        = GlShader_GetInfoLog,
    .GetShaderHandle   = GlShader_GetShaderHandle,
};

/* --------------------------------------------------------------- */
/*  GL Program Implementation                                      */
/* --------------------------------------------------------------- */

struct _GL_PROGRAM {
    IGLProgramVtbl *lpVtbl;
    UINT32          RefCount;
    GLuint          Handle;
};

static HRESULT STDMETHODCALLTYPE
GlProgram_QueryInterface(
    IGLProgram *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLProgram))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlProgram_AddRef(IGLProgram *This)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    return ++program->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlProgram_Release(IGLProgram *This)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    UINT32 refCount = --program->RefCount;

    if (refCount == 0) {
        if (program->Handle) {
            glDeleteProgram(program->Handle);
        }
        RtlFreeMemory(program);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_AttachShader(
    IGLProgram *This,
    IGLShader *Shader)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    GL_SHADER shaderHandle = 0;
    HRESULT hr;

    if (!Shader) return E_POINTER;

    hr = IGLShader_GetShaderHandle(Shader, &shaderHandle);
    if (FAILED(hr)) return hr;

    glAttachShader(program->Handle, shaderHandle);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_LinkProgram(IGLProgram *This)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    glLinkProgram(program->Handle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_UseProgram(IGLProgram *This)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    glUseProgram(program->Handle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_GetLinkStatus(
    IGLProgram *This,
    GL_BOOLEAN *Status)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    GLint status = 0;

    if (!Status) return E_POINTER;

    glGetProgramiv(program->Handle, GL_LINK_STATUS, &status);
    *Status = (GL_BOOLEAN)status;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_GetInfoLog(
    IGLProgram *This,
    GL_SIZEI BufSize,
    GL_SIZEI *Length,
    GL_CHAR *InfoLog)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;

    if (!InfoLog) return E_POINTER;

    glGetProgramInfoLog(program->Handle, BufSize, Length, InfoLog);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_GetUniformLocation(
    IGLProgram *This,
    CONST GL_CHAR *Name,
    GL_INT *Location)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;

    if (!Name || !Location) return E_POINTER;

    *Location = glGetUniformLocation(program->Handle, Name);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_GetAttribLocation(
    IGLProgram *This,
    CONST GL_CHAR *Name,
    GL_INT *Location)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;

    if (!Name || !Location) return E_POINTER;

    *Location = glGetAttribLocation(program->Handle, Name);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_Uniform1f(
    IGLProgram *This,
    GL_INT Location,
    GL_FLOAT V0)
{
    glUniform1f(Location, V0);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_Uniform2f(
    IGLProgram *This,
    GL_INT Location,
    GL_FLOAT V0,
    GL_FLOAT V1)
{
    glUniform2f(Location, V0, V1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_Uniform3f(
    IGLProgram *This,
    GL_INT Location,
    GL_FLOAT V0,
    GL_FLOAT V1,
    GL_FLOAT V2)
{
    glUniform3f(Location, V0, V1, V2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_Uniform4f(
    IGLProgram *This,
    GL_INT Location,
    GL_FLOAT V0,
    GL_FLOAT V1,
    GL_FLOAT V2,
    GL_FLOAT V3)
{
    glUniform4f(Location, V0, V1, V2, V3);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_UniformMatrix4fv(
    IGLProgram *This,
    GL_INT Location,
    GL_SIZEI Count,
    GL_BOOLEAN Transpose,
    CONST GL_FLOAT *Value)
{
    glUniformMatrix4fv(Location, Count, Transpose, Value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlProgram_GetProgramHandle(
    IGLProgram *This,
    GL_PROGRAM *Handle)
{
    GL_PROGRAM *program = (GL_PROGRAM*)This;
    if (!Handle) return E_POINTER;
    *Handle = program->Handle;
    return S_OK;
}

static IGLProgramVtbl GlProgramVtbl = {
    .QueryInterface      = GlProgram_QueryInterface,
    .AddRef              = GlProgram_AddRef,
    .Release             = GlProgram_Release,
    .AttachShader        = GlProgram_AttachShader,
    .LinkProgram         = GlProgram_LinkProgram,
    .UseProgram          = GlProgram_UseProgram,
    .GetLinkStatus       = GlProgram_GetLinkStatus,
    .GetInfoLog          = GlProgram_GetInfoLog,
    .GetUniformLocation  = GlProgram_GetUniformLocation,
    .GetAttribLocation   = GlProgram_GetAttribLocation,
    .Uniform1f           = GlProgram_Uniform1f,
    .Uniform2f           = GlProgram_Uniform2f,
    .Uniform3f           = GlProgram_Uniform3f,
    .Uniform4f           = GlProgram_Uniform4f,
    .UniformMatrix4fv    = GlProgram_UniformMatrix4fv,
    .GetProgramHandle    = GlProgram_GetProgramHandle,
};

/* --------------------------------------------------------------- */
/*  GL Device Implementation                                       */
/* --------------------------------------------------------------- */

struct _GL_DEVICE {
    IGLDeviceVtbl *lpVtbl;
    UINT32         RefCount;
    GL_CONTEXT    *Context;
};

static HRESULT STDMETHODCALLTYPE
GlDevice_QueryInterface(
    IGLDevice *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IGLDevice))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GlDevice_AddRef(IGLDevice *This)
{
    GL_DEVICE *device = (GL_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
GlDevice_Release(IGLDevice *This)
{
    GL_DEVICE *device = (GL_DEVICE*)This;
    UINT32 refCount = --device->RefCount;

    if (refCount == 0) {
        if (device->Context) {
            IUnknown_Release((IUnknown*)device->Context);
        }
        RtlFreeMemory(device);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
GlDevice_GetContext(
    IGLDevice *This,
    IGLContext **Context)
{
    GL_DEVICE *device = (GL_DEVICE*)This;

    if (!Context) return E_POINTER;

    *Context = (IGLContext*)device->Context;
    if (*Context) {
        IUnknown_AddRef((IUnknown*)*Context);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlDevice_CreateBuffer(
    IGLDevice *This,
    IGLBuffer **Buffer)
{
    GL_BUFFER *buffer;

    if (!Buffer) return E_POINTER;

    buffer = (GL_BUFFER*)RtlAllocateMemory(sizeof(GL_BUFFER));
    if (!buffer) return E_OUTOFMEMORY;

    RtlZeroMemory(buffer, sizeof(GL_BUFFER));
    buffer->lpVtbl = &GlBufferVtbl;
    buffer->RefCount = 1;

    glGenBuffers(1, &buffer->Handle);

    *Buffer = (IGLBuffer*)buffer;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlDevice_CreateTexture(
    IGLDevice *This,
    IGLTexture **Texture)
{
    GL_TEXTURE *texture;

    if (!Texture) return E_POINTER;

    texture = (GL_TEXTURE*)RtlAllocateMemory(sizeof(GL_TEXTURE));
    if (!texture) return E_OUTOFMEMORY;

    RtlZeroMemory(texture, sizeof(GL_TEXTURE));
    texture->lpVtbl = &GlTextureVtbl;
    texture->RefCount = 1;

    glGenTextures(1, &texture->Handle);

    *Texture = (IGLTexture*)texture;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlDevice_CreateShader(
    IGLDevice *This,
    GL_ENUM Type,
    IGLShader **Shader)
{
    GL_SHADER *shader;

    if (!Shader) return E_POINTER;

    shader = (GL_SHADER*)RtlAllocateMemory(sizeof(GL_SHADER));
    if (!shader) return E_OUTOFMEMORY;

    RtlZeroMemory(shader, sizeof(GL_SHADER));
    shader->lpVtbl = &GlShaderVtbl;
    shader->RefCount = 1;
    shader->Handle = glCreateShader(Type);

    *Shader = (IGLShader*)shader;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GlDevice_CreateProgram(
    IGLDevice *This,
    IGLProgram **Program)
{
    GL_PROGRAM *program;

    if (!Program) return E_POINTER;

    program = (GL_PROGRAM*)RtlAllocateMemory(sizeof(GL_PROGRAM));
    if (!program) return E_OUTOFMEMORY;

    RtlZeroMemory(program, sizeof(GL_PROGRAM));
    program->lpVtbl = &GlProgramVtbl;
    program->RefCount = 1;
    program->Handle = glCreateProgram();

    *Program = (IGLProgram*)program;
    return S_OK;
}

static IGLDeviceVtbl GlDeviceVtbl = {
    .QueryInterface = GlDevice_QueryInterface,
    .AddRef         = GlDevice_AddRef,
    .Release        = GlDevice_Release,
    .GetContext     = GlDevice_GetContext,
    .CreateBuffer   = GlDevice_CreateBuffer,
    .CreateTexture  = GlDevice_CreateTexture,
    .CreateShader   = GlDevice_CreateShader,
    .CreateProgram  = GlDevice_CreateProgram,
};

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

HRESULT
AnxCreateGLDevice(
    IGLDevice **Device)
{
    GL_DEVICE *device;
    GL_CONTEXT *context;

    if (!Device) return E_POINTER;

    device = (GL_DEVICE*)RtlAllocateMemory(sizeof(GL_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    context = (GL_CONTEXT*)RtlAllocateMemory(sizeof(GL_CONTEXT));
    if (!context) {
        RtlFreeMemory(device);
        return E_OUTOFMEMORY;
    }

    RtlZeroMemory(device, sizeof(GL_DEVICE));
    RtlZeroMemory(context, sizeof(GL_CONTEXT));

    device->lpVtbl = &GlDeviceVtbl;
    device->RefCount = 1;
    device->Context = context;

    context->lpVtbl = &GlContextVtbl;
    context->RefCount = 1;
    context->Device = device;

    *Device = (IGLDevice*)device;
    return S_OK;
}
