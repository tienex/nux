/**
 * @file internal.h
 * @brief DCL Shell - Internal implementation details
 */

#ifndef _DCL_INTERNAL_H_
#define _DCL_INTERNAL_H_

#include <dcl/dcl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum command line length
 */
#define DCL_MAX_COMMAND_LENGTH 1024

/**
 * Maximum number of tokens per command
 */
#define DCL_MAX_TOKENS 256

/**
 * Maximum number of registered syntax providers
 */
#define DCL_MAX_SYNTAX_PROVIDERS 64

/**
 * Maximum number of message handlers
 */
#define DCL_MAX_MESSAGE_HANDLERS 32

/**
 * DCL built-in keywords
 */
extern const CHAR8 *gDclKeywords[];
extern UINTN gDclKeywordCount;

/**
 * DCL built-in commands
 */
extern const CHAR8 *gDclCommands[];
extern UINTN gDclCommandCount;

/**
 * ANSI renderer implementation
 */
typedef struct _DCL_ANSI_RENDERER {
    IDclRenderer Interface;
    volatile ULONG RefCount;
    DCL_COLOR CurrentForeground;
    DCL_COLOR CurrentBackground;
} DCL_ANSI_RENDERER;

/**
 * Syntax provider implementation
 */
typedef struct _DCL_SYNTAX_ENTRY {
    CHAR8 Name[64];
    VOID *SyntaxData;
    BOOLEAN InUse;
} DCL_SYNTAX_ENTRY;

typedef struct _DCL_SYNTAX_PROVIDER {
    IDclSyntaxProvider Interface;
    volatile ULONG RefCount;
    DCL_SYNTAX_ENTRY Entries[DCL_MAX_SYNTAX_PROVIDERS];
    UINTN EntryCount;
} DCL_SYNTAX_PROVIDER;

/**
 * Parser implementation
 */
typedef struct _DCL_PARSER {
    IDclParser Interface;
    volatile ULONG RefCount;
    IDclSyntaxProvider *SyntaxProvider;
    DCL_SYNTAX_TOKEN TokenBuffer[DCL_MAX_TOKENS];
} DCL_PARSER;

/**
 * Message handler entry
 */
typedef struct _DCL_MESSAGE_HANDLER {
    DCL_MESSAGE_TYPE MessageType;
    HRESULT (*Handler)(const DCL_MESSAGE *, DCL_MESSAGE **);
    BOOLEAN InUse;
} DCL_MESSAGE_HANDLER;

/**
 * Message bus implementation
 */
typedef struct _DCL_MESSAGE_BUS {
    IDclMessageBus Interface;
    volatile ULONG RefCount;
    DCL_MESSAGE_HANDLER Handlers[DCL_MAX_MESSAGE_HANDLERS];
    UINTN HandlerCount;
} DCL_MESSAGE_BUS;

/**
 * Main DCL shell implementation
 */
typedef struct _DCL_SHELL {
    IDcl Interface;
    volatile ULONG RefCount;
    DCL_ANSI_RENDERER *Renderer;
    DCL_SYNTAX_PROVIDER *SyntaxProvider;
    DCL_PARSER *Parser;
    DCL_MESSAGE_BUS *MessageBus;
    BOOLEAN Running;
    CHAR8 CommandBuffer[DCL_MAX_COMMAND_LENGTH];
    UINTN CommandLength;
} DCL_SHELL;

/**
 * Component factory functions
 */
HRESULT DclCreateAnsiRenderer(IDclRenderer **Renderer);
HRESULT DclCreateSyntaxProvider(IDclSyntaxProvider **Provider);
HRESULT DclCreateParser(IDclParser **Parser);
HRESULT DclCreateMessageBus(IDclMessageBus **MessageBus);
HRESULT DclCreateShell(IDcl **Shell);

/**
 * Helper functions
 */
BOOLEAN DclIsKeyword(const CHAR8 *Text, UINTN Length);
BOOLEAN DclIsCommand(const CHAR8 *Text, UINTN Length);
BOOLEAN DclIsWhitespace(CHAR8 c);
BOOLEAN DclIsAlpha(CHAR8 c);
BOOLEAN DclIsDigit(CHAR8 c);
BOOLEAN DclIsAlphaNumeric(CHAR8 c);

/**
 * String comparison helper
 */
INT32 DclStrNCmp(const CHAR8 *s1, const CHAR8 *s2, UINTN n);
UINTN DclStrLen(const CHAR8 *s);
VOID DclStrCpy(CHAR8 *dest, const CHAR8 *src, UINTN max);

#ifdef __cplusplus
}
#endif

#endif /* _DCL_INTERNAL_H_ */
