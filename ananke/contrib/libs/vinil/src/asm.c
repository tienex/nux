/*++
    Module Name:

        asm.c

    Abstract:

        VINIL assembly language parser and assembler implementation.
        Converts text-based assembly into IL instructions.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#define COBJMACROS
#include <vinil/asm.h>
#include <vinil/disasm.h>
#include <vinil/il.h>
#include <vinil/types.h>
#include <vinil/memory.h>
#include "vinil_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* --------------------------------------------------------------- */
/*  Lexer                                                          */
/* --------------------------------------------------------------- */

typedef enum _TOKEN_TYPE {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_MINUS,
    TOKEN_NEWLINE,
    TOKEN_DIRECTIVE,
} TOKEN_TYPE;

typedef struct _TOKEN {
    TOKEN_TYPE  Type;
    CONST CHAR8 *Text;
    UINTN       Length;
    UINT32      Line;
    UINT32      Column;
} TOKEN;

typedef struct _LEXER {
    CONST CHAR8 *Source;
    UINTN       SourceSize;
    UINTN       Position;
    UINT32      Line;
    UINT32      Column;
} LEXER;

static VOID
LexerInit (
    LEXER       *Lexer,
    CONST CHAR8 *Source,
    UINTN       SourceSize
    )
{
    Lexer->Source = Source;
    Lexer->SourceSize = SourceSize;
    Lexer->Position = 0;
    Lexer->Line = 1;
    Lexer->Column = 1;
}

static CHAR8
LexerPeek (
    LEXER *Lexer
    )
{
    if (Lexer->Position >= Lexer->SourceSize) {
        return '\0';
    }
    return Lexer->Source[Lexer->Position];
}

static CHAR8
LexerAdvance (
    LEXER *Lexer
    )
{
    CHAR8 Ch;

    if (Lexer->Position >= Lexer->SourceSize) {
        return '\0';
    }

    Ch = Lexer->Source[Lexer->Position++];

    if (Ch == '\n') {
        Lexer->Line++;
        Lexer->Column = 1;
    } else {
        Lexer->Column++;
    }

    return Ch;
}

static VOID
LexerSkipWhitespace (
    LEXER *Lexer
    )
{
    CHAR8 Ch;

    while (TRUE) {
        Ch = LexerPeek(Lexer);

        /* Skip spaces and tabs */
        if (Ch == ' ' || Ch == '\t' || Ch == '\r') {
            LexerAdvance(Lexer);
            continue;
        }

        /* Skip comments */
        if (Ch == ';' || (Ch == '/' && Lexer->Position + 1 < Lexer->SourceSize &&
                          Lexer->Source[Lexer->Position + 1] == '/')) {
            /* Skip to end of line */
            while (LexerPeek(Lexer) != '\n' && LexerPeek(Lexer) != '\0') {
                LexerAdvance(Lexer);
            }
            continue;
        }

        break;
    }
}

static BOOLEAN
LexerNextToken (
    LEXER   *Lexer,
    TOKEN   *Token
    )
{
    CHAR8 Ch;

    LexerSkipWhitespace(Lexer);

    Token->Line = Lexer->Line;
    Token->Column = Lexer->Column;
    Token->Text = &Lexer->Source[Lexer->Position];
    Token->Length = 0;

    Ch = LexerPeek(Lexer);

    /* End of file */
    if (Ch == '\0') {
        Token->Type = TOKEN_EOF;
        return TRUE;
    }

    /* Newline */
    if (Ch == '\n') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_NEWLINE;
        Token->Length = 1;
        return TRUE;
    }

    /* Single-character tokens */
    if (Ch == ',') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_COMMA;
        Token->Length = 1;
        return TRUE;
    }

    if (Ch == '.') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_DOT;
        Token->Length = 1;
        return TRUE;
    }

    if (Ch == ':') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_COLON;
        Token->Length = 1;
        return TRUE;
    }

    if (Ch == '[') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_LBRACKET;
        Token->Length = 1;
        return TRUE;
    }

    if (Ch == ']') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_RBRACKET;
        Token->Length = 1;
        return TRUE;
    }

    if (Ch == '-') {
        LexerAdvance(Lexer);
        Token->Type = TOKEN_MINUS;
        Token->Length = 1;
        return TRUE;
    }

    /* Identifier or directive */
    if (isalpha(Ch) || Ch == '_') {
        while (isalnum(LexerPeek(Lexer)) || LexerPeek(Lexer) == '_') {
            LexerAdvance(Lexer);
            Token->Length++;
        }
        Token->Type = TOKEN_IDENTIFIER;
        return TRUE;
    }

    /* Number */
    if (isdigit(Ch)) {
        while (isdigit(LexerPeek(Lexer)) || LexerPeek(Lexer) == '.') {
            LexerAdvance(Lexer);
            Token->Length++;
        }
        Token->Type = TOKEN_NUMBER;
        return TRUE;
    }

    /* Unknown character */
    return FALSE;
}

/* --------------------------------------------------------------- */
/*  Parser and Assembler Context                                  */
/* --------------------------------------------------------------- */

#define MAX_VARIABLES 256

typedef struct _VARIABLE_ENTRY {
    CHAR8             Name[64];
    IVinilVariable    *Variable;
} VARIABLE_ENTRY;

typedef struct _ASSEMBLER_CONTEXT {
    IVinilMemoryPool  *MemoryPool;
    IVinilProgram     *Program;
    VARIABLE_ENTRY    Variables[MAX_VARIABLES];
    UINT32            VariableCount;
    UINT32            NextVariableId;
} ASSEMBLER_CONTEXT;

typedef struct _PARSER {
    LEXER               Lexer;
    TOKEN               CurrentToken;
    VINIL_ASM_ERROR     *Error;
    ASSEMBLER_CONTEXT   *Context;
} PARSER;

/* Helper: Find or create variable by name */
static IVinilVariable *
ContextGetVariable (
    ASSEMBLER_CONTEXT *Context,
    CONST CHAR8       *Name,
    UINTN             NameLength
    )
{
    UINT32            i;
    IVinilType        *Type;
    IVinilVariable    *Variable;
    HRESULT           Hr;

    /* Search existing variables */
    for (i = 0; i < Context->VariableCount; i++) {
        if (strncmp((const char *)Context->Variables[i].Name, (const char *)Name, NameLength) == 0 &&
            Context->Variables[i].Name[NameLength] == '\0') {
            return Context->Variables[i].Variable;
        }
    }

    /* Create new variable if not found */
    if (Context->VariableCount >= MAX_VARIABLES) {
        return NULL;
    }

    /* Create float4 type for now */
    Hr = VinilGetBasicType(VINIL_TYPE_FLOAT_VEC4, VinilPrecisionHigh, &Type);
    if (FAILED(Hr)) {
        return NULL;
    }

    /* Create variable */
    Hr = VinilVariableCreate(Type, Name, Context->NextVariableId++, &Variable);
    IVinilType_Release(Type);

    if (FAILED(Hr)) {
        return NULL;
    }

    /* Store in table */
    strncpy((char *)Context->Variables[Context->VariableCount].Name, (const char *)Name, NameLength);
    Context->Variables[Context->VariableCount].Name[NameLength] = '\0';
    Context->Variables[Context->VariableCount].Variable = Variable;
    Context->VariableCount++;

    return Variable;
}

static VOID
ParserInit (
    PARSER            *Parser,
    CONST CHAR8       *Source,
    UINTN             SourceSize,
    VINIL_ASM_ERROR   *Error,
    ASSEMBLER_CONTEXT *Context
    )
{
    LexerInit(&Parser->Lexer, Source, SourceSize);
    Parser->Error = Error;
    Parser->Context = Context;
    LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);
}

static VOID
ParserSetError (
    PARSER      *Parser,
    CONST CHAR8 *Message
    )
{
    if (Parser->Error != NULL) {
        Parser->Error->Line = Parser->CurrentToken.Line;
        Parser->Error->Column = Parser->CurrentToken.Column;
        Parser->Error->Message = Message;
    }
}

static BOOLEAN
__attribute__((unused))
ParserExpect (
    PARSER      *Parser,
    TOKEN_TYPE  Type
    )
{
    if (Parser->CurrentToken.Type != Type) {
        ParserSetError(Parser, (CONST CHAR8 *)"Unexpected token");
        return FALSE;
    }

    LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);
    return TRUE;
}

static BOOLEAN
ParserMatch (
    PARSER      *Parser,
    TOKEN_TYPE  Type
    )
{
    if (Parser->CurrentToken.Type == Type) {
        LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);
        return TRUE;
    }
    return FALSE;
}

/* Helper: Lookup opcode by name */
static BOOLEAN
LookupOpcode (
    CONST CHAR8     *Name,
    UINTN           NameLength,
    VINIL_OPCODE    *Opcode
    )
{
    UINT32  i;

    for (i = 0; i < VINIL_OP_COUNT; i++) {
        if (gVinilOpcodeTable[i].Name == NULL) {
            continue;
        }

        if (strncmp((const char *)gVinilOpcodeTable[i].Name, (const char *)Name, NameLength) == 0 &&
            gVinilOpcodeTable[i].Name[NameLength] == '\0') {
            *Opcode = (VINIL_OPCODE)i;
            return TRUE;
        }
    }

    return FALSE;
}

/* Parse instruction and add to program */
static BOOLEAN
ParseInstruction (
    PARSER *Parser
    )
{
    VINIL_OPCODE            Opcode;
    CONST VINIL_OPCODE_INFO *Info;
    IVinilVariable          *Dst = NULL;
    IVinilVariable          *Src[3] = {NULL, NULL, NULL};
    UINT32                  SrcIdx;
    HRESULT                 Hr;

    /* Get opcode name */
    if (Parser->CurrentToken.Type != TOKEN_IDENTIFIER) {
        ParserSetError(Parser, (CONST CHAR8 *)"Expected opcode");
        return FALSE;
    }

    /* Lookup opcode */
    if (!LookupOpcode(Parser->CurrentToken.Text, Parser->CurrentToken.Length, &Opcode)) {
        ParserSetError(Parser, (CONST CHAR8 *)"Unknown opcode");
        return FALSE;
    }

    Info = VinilGetOpcodeInfo(Opcode);
    if (Info == NULL) {
        ParserSetError(Parser, (CONST CHAR8 *)"Invalid opcode");
        return FALSE;
    }

    LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);

    /* Parse destination if instruction has one */
    if (Info->HasDestination) {
        if (Parser->CurrentToken.Type != TOKEN_IDENTIFIER) {
            ParserSetError(Parser, (CONST CHAR8 *)"Expected destination operand");
            return FALSE;
        }

        Dst = ContextGetVariable(Parser->Context, Parser->CurrentToken.Text, Parser->CurrentToken.Length);
        if (Dst == NULL) {
            ParserSetError(Parser, (CONST CHAR8 *)"Failed to create variable");
            return FALSE;
        }

        LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);

        /* Expect comma after destination if there are source operands */
        if (Info->NumSources > 0) {
            if (!ParserMatch(Parser, TOKEN_COMMA)) {
                ParserSetError(Parser, (CONST CHAR8 *)"Expected comma");
                return FALSE;
            }
        }
    }

    /* Parse source operands */
    for (SrcIdx = 0; SrcIdx < Info->NumSources && SrcIdx < 3; SrcIdx++) {
        if (Parser->CurrentToken.Type != TOKEN_IDENTIFIER) {
            ParserSetError(Parser, (CONST CHAR8 *)"Expected source operand");
            return FALSE;
        }

        Src[SrcIdx] = ContextGetVariable(Parser->Context, Parser->CurrentToken.Text, Parser->CurrentToken.Length);
        if (Src[SrcIdx] == NULL) {
            ParserSetError(Parser, (CONST CHAR8 *)"Failed to create variable");
            return FALSE;
        }

        LexerNextToken(&Parser->Lexer, &Parser->CurrentToken);

        /* Expect comma if not last operand */
        if (SrcIdx < Info->NumSources - 1) {
            if (!ParserMatch(Parser, TOKEN_COMMA)) {
                ParserSetError(Parser, (CONST CHAR8 *)"Expected comma");
                return FALSE;
            }
        }
    }

    /* Add instruction to program */
    Hr = VinilProgramAddInstruction(Parser->Context->Program, Opcode, Dst, Src[0], Src[1], Src[2]);
    if (FAILED(Hr)) {
        ParserSetError(Parser, (CONST CHAR8 *)"Failed to add instruction");
        return FALSE;
    }

    return TRUE;
}

/* --------------------------------------------------------------- */
/*  Assembler Implementation                                       */
/* --------------------------------------------------------------- */

HRESULT
VinilAssemble (
    CONST CHAR8         *Source,
    UINTN               SourceSize,
    VINIL_ASM_FLAGS     Flags,
    VOID                **Program,
    VINIL_ASM_ERROR     *Error
    )
{
    ASSEMBLER_CONTEXT   Context;
    PARSER              Parser;
    IVinilMemoryPool    *MemoryPool = NULL;
    BOOLEAN             Success;
    HRESULT             Hr;
    UINT32              i;

    if (Source == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* Initialize error info */
    if (Error != NULL) {
        Error->Line = 0;
        Error->Column = 0;
        Error->Message = NULL;
    }

    /* Create memory pool */
    Hr = VinilCreateMemoryPool(VINIL_DEFAULT_PAGE_SIZE, &MemoryPool);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Initialize assembler context */
    memset(&Context, 0, sizeof(Context));
    Context.MemoryPool = MemoryPool;
    Context.VariableCount = 0;
    Context.NextVariableId = 0;

    /* Create program */
    Hr = VinilProgramCreate(VinilExecutionModeCompute, MemoryPool, &Context.Program);
    if (FAILED(Hr)) {
        IVinilMemoryPool_Release(MemoryPool);
        return Hr;
    }

    /* Initialize parser */
    ParserInit(&Parser, Source, SourceSize, Error, &Context);

    /* Parse assembly */
    Success = TRUE;
    while (Parser.CurrentToken.Type != TOKEN_EOF) {
        /* Skip empty lines and comments */
        if (ParserMatch(&Parser, TOKEN_NEWLINE)) {
            continue;
        }

        /* Parse instruction */
        if (Parser.CurrentToken.Type == TOKEN_IDENTIFIER) {
            if (!ParseInstruction(&Parser)) {
                Success = FALSE;
                break;
            }

            /* Expect newline or EOF after instruction */
            if (Parser.CurrentToken.Type != TOKEN_NEWLINE && Parser.CurrentToken.Type != TOKEN_EOF) {
                ParserSetError(&Parser, (CONST CHAR8 *)"Expected newline after instruction");
                Success = FALSE;
                break;
            }

            if (Parser.CurrentToken.Type == TOKEN_NEWLINE) {
                LexerNextToken(&Parser.Lexer, &Parser.CurrentToken);
            }
        } else {
            ParserSetError(&Parser, (CONST CHAR8 *)"Expected instruction");
            Success = FALSE;
            break;
        }
    }

    /* Clean up variables */
    for (i = 0; i < Context.VariableCount; i++) {
        if (Context.Variables[i].Variable != NULL) {
            IVinilVariable_Release(Context.Variables[i].Variable);
        }
    }

    if (!Success) {
        IVinilProgram_Release(Context.Program);
        IVinilMemoryPool_Release(MemoryPool);
        return E_FAIL;
    }

    /* Return program */
    *Program = Context.Program;

    (VOID)Flags;
    return S_OK;
}

HRESULT
VinilAssembleFile (
    CONST CHAR8         *FilePath,
    VINIL_ASM_FLAGS     Flags,
    VOID                **Program,
    VINIL_ASM_ERROR     *Error
    )
{
    CHAR8   Buffer[65536];
    FILE    *File;
    UINTN   BytesRead;
    HRESULT Hr;

    if (FilePath == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* Open file for reading */
    File = fopen((const char *)FilePath, "r");
    if (File == NULL) {
        return E_FAIL;
    }

    /* Read file into buffer */
    BytesRead = fread(Buffer, 1, sizeof(Buffer) - 1, File);
    fclose(File);

    if (BytesRead == 0) {
        return E_FAIL;
    }

    Buffer[BytesRead] = '\0';

    /* Assemble from buffer */
    Hr = VinilAssemble((CONST CHAR8 *)Buffer, BytesRead, Flags, Program, Error);
    return Hr;
}

HRESULT
VinilValidateAsm (
    CONST CHAR8         *Source,
    UINTN               SourceSize,
    VINIL_ASM_ERROR     *Error
    )
{
    IVinilProgram   *Program;
    HRESULT         Hr;

    /* Try to assemble - if it succeeds, syntax is valid */
    Hr = VinilAssemble(Source, SourceSize, VinilAsmNone, (VOID **)&Program, Error);

    /* Free program if created */
    if (SUCCEEDED(Hr) && Program != NULL) {
        IVinilProgram_Release(Program);
    }

    return Hr;
}
