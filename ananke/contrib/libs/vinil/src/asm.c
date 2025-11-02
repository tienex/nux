/*++
    Module Name:

        asm.c

    Abstract:

        VINIL assembly language parser and assembler implementation.
        Converts text-based assembly into IL instructions.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/asm.h>
#include <vinil/disasm.h>
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
/*  Parser                                                         */
/* --------------------------------------------------------------- */

typedef struct _PARSER {
    LEXER           Lexer;
    TOKEN           CurrentToken;
    VINIL_ASM_ERROR *Error;
} PARSER;

static VOID
ParserInit (
    PARSER          *Parser,
    CONST CHAR8     *Source,
    UINTN           SourceSize,
    VINIL_ASM_ERROR *Error
    )
{
    LexerInit(&Parser->Lexer, Source, SourceSize);
    Parser->Error = Error;
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
    PARSER  Parser;
    BOOLEAN Success;

    if (Source == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* Initialize error info */
    if (Error != NULL) {
        Error->Line = 0;
        Error->Column = 0;
        Error->Message = NULL;
    }

    /* Initialize parser */
    ParserInit(&Parser, Source, SourceSize, Error);

    /* Parse assembly */
    Success = TRUE;
    while (Parser.CurrentToken.Type != TOKEN_EOF) {
        /* Skip empty lines */
        if (ParserMatch(&Parser, TOKEN_NEWLINE)) {
            continue;
        }

        /* Parse instruction or directive */
        if (Parser.CurrentToken.Type == TOKEN_IDENTIFIER) {
            /* TODO: Parse instruction */
            LexerNextToken(&Parser.Lexer, &Parser.CurrentToken);
        } else {
            ParserSetError(&Parser, (CONST CHAR8 *)"Expected instruction or directive");
            Success = FALSE;
            break;
        }
    }

    if (!Success) {
        return E_FAIL;
    }

    /* TODO: Build IL program from parsed instructions */
    *Program = NULL;

    (VOID)Flags;
    return E_NOTIMPL;
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
    VOID    *Program;
    HRESULT Hr;

    /* Try to assemble - if it succeeds, syntax is valid */
    Hr = VinilAssemble(Source, SourceSize, VinilAsmNone, &Program, Error);

    /* Free program if created */
    if (SUCCEEDED(Hr) && Program != NULL) {
        /* TODO: Free program */
    }

    return Hr;
}
