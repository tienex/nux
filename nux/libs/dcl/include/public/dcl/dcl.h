/**
 * @file dcl.h
 * @brief Digital Command Language (DCL) Shell - Main COM Interface Header
 *
 * This file defines the COM interfaces for the DCL shell, which provides
 * syntax coloring and extensible command parsing through a client/server
 * architecture.
 */

#ifndef _DCL_H_
#define _DCL_H_

#include <ananke/ananke.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */
typedef struct _IDcl IDcl;
typedef struct _IDclRenderer IDclRenderer;
typedef struct _IDclSyntaxProvider IDclSyntaxProvider;
typedef struct _IDclParser IDclParser;
typedef struct _IDclMessageBus IDclMessageBus;

/**
 * Interface IDs
 */
/* {8F3A1C20-D4E5-4B9F-A1C3-8E2F9D6A7B4C} */
DEFINE_GUID(IID_IDcl,
    0x8F3A1C20, 0xD4E5, 0x4B9F, 0xA1, 0xC3, 0x8E, 0x2F, 0x9D, 0x6A, 0x7B, 0x4C);

/* {2A5B8C3D-E9F1-4A2B-8C3D-4E5F6A7B8C9D} */
DEFINE_GUID(IID_IDclRenderer,
    0x2A5B8C3D, 0xE9F1, 0x4A2B, 0x8C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D);

/* {3B6C9D4E-F1A2-5B3C-9D4E-5F6A7B8C9D0E} */
DEFINE_GUID(IID_IDclSyntaxProvider,
    0x3B6C9D4E, 0xF1A2, 0x5B3C, 0x9D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E);

/* {4C7D0E5F-A2B3-6C4D-0E5F-6A7B8C9D0E1F} */
DEFINE_GUID(IID_IDclParser,
    0x4C7D0E5F, 0xA2B3, 0x6C4D, 0x0E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F);

/* {5D8E1F6A-B3C4-7D5E-1F6A-7B8C9D0E1F2A} */
DEFINE_GUID(IID_IDclMessageBus,
    0x5D8E1F6A, 0xB3C4, 0x7D5E, 0x1F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A);

/**
 * DCL syntax element types for coloring
 */
typedef enum _DCL_SYNTAX_TYPE {
    DCL_SYNTAX_NORMAL = 0,
    DCL_SYNTAX_KEYWORD,
    DCL_SYNTAX_COMMAND,
    DCL_SYNTAX_STRING,
    DCL_SYNTAX_NUMBER,
    DCL_SYNTAX_COMMENT,
    DCL_SYNTAX_OPERATOR,
    DCL_SYNTAX_VARIABLE,
    DCL_SYNTAX_ERROR,
    DCL_SYNTAX_MAX
} DCL_SYNTAX_TYPE;

/**
 * DCL color codes (ANSI-compatible)
 */
typedef enum _DCL_COLOR {
    DCL_COLOR_BLACK = 0,
    DCL_COLOR_RED,
    DCL_COLOR_GREEN,
    DCL_COLOR_YELLOW,
    DCL_COLOR_BLUE,
    DCL_COLOR_MAGENTA,
    DCL_COLOR_CYAN,
    DCL_COLOR_WHITE,
    DCL_COLOR_DEFAULT = 9,
    DCL_COLOR_BRIGHT_BLACK = 60,
    DCL_COLOR_BRIGHT_RED = 61,
    DCL_COLOR_BRIGHT_GREEN = 62,
    DCL_COLOR_BRIGHT_YELLOW = 63,
    DCL_COLOR_BRIGHT_BLUE = 64,
    DCL_COLOR_BRIGHT_MAGENTA = 65,
    DCL_COLOR_BRIGHT_CYAN = 66,
    DCL_COLOR_BRIGHT_WHITE = 67
} DCL_COLOR;

/**
 * DCL syntax token
 */
typedef struct _DCL_SYNTAX_TOKEN {
    DCL_SYNTAX_TYPE Type;
    const CHAR8 *Text;
    UINTN Length;
    DCL_COLOR ForegroundColor;
    DCL_COLOR BackgroundColor;
    UINT32 Flags;
} DCL_SYNTAX_TOKEN;

/**
 * DCL message structure for client/server communication
 */
typedef struct _DCL_MESSAGE {
    UINT32 MessageId;
    UINT32 MessageType;
    UINTN DataSize;
    VOID *Data;
} DCL_MESSAGE;

/**
 * Message types
 */
typedef enum _DCL_MESSAGE_TYPE {
    DCL_MSG_REGISTER_SYNTAX = 1,
    DCL_MSG_UNREGISTER_SYNTAX,
    DCL_MSG_PARSE_REQUEST,
    DCL_MSG_PARSE_RESPONSE,
    DCL_MSG_RENDER_REQUEST,
    DCL_MSG_RENDER_RESPONSE
} DCL_MESSAGE_TYPE;

/**
 * IDclRenderer - Abstract rendering backend interface
 */
#undef INTERFACE
#define INTERFACE IDclRenderer

DECLARE_INTERFACE_(IDclRenderer, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ IID *riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDclRenderer methods */
    STDMETHOD(Initialize)(THIS) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
    STDMETHOD(SetColor)(THIS_ DCL_COLOR Foreground, DCL_COLOR Background) PURE;
    STDMETHOD(ResetColor)(THIS) PURE;
    STDMETHOD(WriteText)(THIS_ const CHAR8 *Text, UINTN Length) PURE;
    STDMETHOD(WriteToken)(THIS_ const DCL_SYNTAX_TOKEN *Token) PURE;
    STDMETHOD(NewLine)(THIS) PURE;
    STDMETHOD(ClearScreen)(THIS) PURE;
    STDMETHOD(MoveCursor)(THIS_ UINTN X, UINTN Y) PURE;
};

/**
 * IDclSyntaxProvider - Syntax provider interface for registering custom syntaxes
 */
#undef INTERFACE
#define INTERFACE IDclSyntaxProvider

DECLARE_INTERFACE_(IDclSyntaxProvider, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ IID *riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDclSyntaxProvider methods */
    STDMETHOD(RegisterSyntax)(THIS_ const CHAR8 *Name, VOID *SyntaxData) PURE;
    STDMETHOD(UnregisterSyntax)(THIS_ const CHAR8 *Name) PURE;
    STDMETHOD(GetSyntax)(THIS_ const CHAR8 *Name, VOID **SyntaxData) PURE;
    STDMETHOD(EnumerateSyntaxes)(THIS_ UINTN *Count, CHAR8 ***Names) PURE;
};

/**
 * IDclParser - Parser interface for DCL command parsing
 */
#undef INTERFACE
#define INTERFACE IDclParser

DECLARE_INTERFACE_(IDclParser, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ IID *riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDclParser methods */
    STDMETHOD(ParseLine)(THIS_ const CHAR8 *Line, UINTN Length,
                         DCL_SYNTAX_TOKEN **Tokens, UINTN *TokenCount) PURE;
    STDMETHOD(FreeTokens)(THIS_ DCL_SYNTAX_TOKEN *Tokens, UINTN TokenCount) PURE;
    STDMETHOD(SetSyntaxProvider)(THIS_ IDclSyntaxProvider *Provider) PURE;
};

/**
 * IDclMessageBus - Message bus for client/server communication
 */
#undef INTERFACE
#define INTERFACE IDclMessageBus

DECLARE_INTERFACE_(IDclMessageBus, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ IID *riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDclMessageBus methods */
    STDMETHOD(Initialize)(THIS) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
    STDMETHOD(SendMessage)(THIS_ const DCL_MESSAGE *Message, DCL_MESSAGE **Response) PURE;
    STDMETHOD(RegisterHandler)(THIS_ DCL_MESSAGE_TYPE MessageType,
                               HRESULT (*Handler)(const DCL_MESSAGE *, DCL_MESSAGE **)) PURE;
    STDMETHOD(UnregisterHandler)(THIS_ DCL_MESSAGE_TYPE MessageType) PURE;
};

/**
 * IDcl - Main DCL shell interface (aggregator)
 */
#undef INTERFACE
#define INTERFACE IDcl

DECLARE_INTERFACE_(IDcl, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ IID *riid, VOID **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDcl methods */
    STDMETHOD(Initialize)(THIS) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
    STDMETHOD(Run)(THIS) PURE;
    STDMETHOD(GetRendererInterface)(THIS_ IDclRenderer **Renderer) PURE;
    STDMETHOD(GetSyntaxProviderInterface)(THIS_ IDclSyntaxProvider **Provider) PURE;
    STDMETHOD(GetParserInterface)(THIS_ IDclParser **Parser) PURE;
    STDMETHOD(GetMessageBusInterface)(THIS_ IDclMessageBus **MessageBus) PURE;
    STDMETHOD(ExecuteCommand)(THIS_ const CHAR8 *Command) PURE;
};

/**
 * Global DCL shell instance
 */
extern IDcl *gpDcl;

/**
 * Legacy function wrappers for backward compatibility
 */
HRESULT dcl_initialize(VOID);
HRESULT dcl_shutdown(VOID);
HRESULT dcl_run(VOID);
HRESULT dcl_execute_command(const CHAR8 *Command);

#ifdef __cplusplus
}
#endif

#endif /* _DCL_H_ */
