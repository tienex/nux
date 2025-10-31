/**
 * @file parser.c
 * @brief DCL command parser with syntax coloring support
 */

#include <dcl/internal.h>

/**
 * Forward declarations
 */
static HRESULT STDMETHODCALLTYPE Parser_QueryInterface(
    IDclParser *This,
    IID *riid,
    VOID **ppvObject);

static ULONG STDMETHODCALLTYPE Parser_AddRef(IDclParser *This);

static ULONG STDMETHODCALLTYPE Parser_Release(IDclParser *This);

static HRESULT STDMETHODCALLTYPE Parser_ParseLine(
    IDclParser *This,
    const CHAR8 *Line,
    UINTN Length,
    DCL_SYNTAX_TOKEN **Tokens,
    UINTN *TokenCount);

static HRESULT STDMETHODCALLTYPE Parser_FreeTokens(
    IDclParser *This,
    DCL_SYNTAX_TOKEN *Tokens,
    UINTN TokenCount);

static HRESULT STDMETHODCALLTYPE Parser_SetSyntaxProvider(
    IDclParser *This,
    IDclSyntaxProvider *Provider);

/**
 * VTable for parser
 */
static IDclParserVtbl gParserVtbl = {
    Parser_QueryInterface,
    Parser_AddRef,
    Parser_Release,
    Parser_ParseLine,
    Parser_FreeTokens,
    Parser_SetSyntaxProvider
};

/**
 * Get color for syntax type
 */
static DCL_COLOR Parser_GetColorForType(DCL_SYNTAX_TYPE Type)
{
    switch (Type) {
        case DCL_SYNTAX_KEYWORD:
            return DCL_COLOR_BRIGHT_CYAN;
        case DCL_SYNTAX_COMMAND:
            return DCL_COLOR_BRIGHT_GREEN;
        case DCL_SYNTAX_STRING:
            return DCL_COLOR_BRIGHT_YELLOW;
        case DCL_SYNTAX_NUMBER:
            return DCL_COLOR_BRIGHT_MAGENTA;
        case DCL_SYNTAX_COMMENT:
            return DCL_COLOR_BRIGHT_BLACK;
        case DCL_SYNTAX_OPERATOR:
            return DCL_COLOR_BRIGHT_RED;
        case DCL_SYNTAX_VARIABLE:
            return DCL_COLOR_BRIGHT_BLUE;
        case DCL_SYNTAX_ERROR:
            return DCL_COLOR_RED;
        default:
            return DCL_COLOR_DEFAULT;
    }
}

/**
 * Identify token type
 */
static DCL_SYNTAX_TYPE Parser_IdentifyToken(
    const CHAR8 *Text,
    UINTN Length,
    BOOLEAN IsFirstToken)
{
    /* Check for comment */
    if (Length > 0 && Text[0] == '!') {
        return DCL_SYNTAX_COMMENT;
    }

    /* Check for string */
    if (Length > 0 && Text[0] == '"') {
        return DCL_SYNTAX_STRING;
    }

    /* Check for number */
    if (Length > 0 && DclIsDigit(Text[0])) {
        UINTN i;
        for (i = 1; i < Length; i++) {
            if (!DclIsDigit(Text[i])) {
                break;
            }
        }
        if (i == Length) {
            return DCL_SYNTAX_NUMBER;
        }
    }

    /* Check for variable ($name) */
    if (Length > 0 && Text[0] == '$') {
        return DCL_SYNTAX_VARIABLE;
    }

    /* Check for operators */
    if (Length == 1) {
        switch (Text[0]) {
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case ':':
            case ',':
            case '(':
            case ')':
            case '[':
            case ']':
                return DCL_SYNTAX_OPERATOR;
        }
    }

    /* Check for command (first token) */
    if (IsFirstToken && DclIsCommand(Text, Length)) {
        return DCL_SYNTAX_COMMAND;
    }

    /* Check for keyword */
    if (DclIsKeyword(Text, Length)) {
        return DCL_SYNTAX_KEYWORD;
    }

    return DCL_SYNTAX_NORMAL;
}

/**
 * QueryInterface implementation
 */
static HRESULT STDMETHODCALLTYPE Parser_QueryInterface(
    IDclParser *This,
    IID *riid,
    VOID **ppvObject)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDclParser)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

/**
 * AddRef implementation
 */
static ULONG STDMETHODCALLTYPE Parser_AddRef(IDclParser *This)
{
    DCL_PARSER *pParser = (DCL_PARSER *)This;
    return AtomicIncrement(&pParser->RefCount);
}

/**
 * Release implementation
 */
static ULONG STDMETHODCALLTYPE Parser_Release(IDclParser *This)
{
    DCL_PARSER *pParser = (DCL_PARSER *)This;
    ULONG refCount = AtomicDecrement(&pParser->RefCount);

    if (refCount == 0) {
        /* Free the parser - would use kernel allocator in full implementation */
        /* For now, this is a static/global object */
    }

    return refCount;
}

/**
 * ParseLine implementation
 */
static HRESULT STDMETHODCALLTYPE Parser_ParseLine(
    IDclParser *This,
    const CHAR8 *Line,
    UINTN Length,
    DCL_SYNTAX_TOKEN **Tokens,
    UINTN *TokenCount)
{
    DCL_PARSER *pParser = (DCL_PARSER *)This;
    UINTN i, tokenStart, tokenLen;
    UINTN count = 0;
    BOOLEAN inString = FALSE;
    BOOLEAN inComment = FALSE;
    BOOLEAN isFirstToken = TRUE;

    if (Line == NULL || Tokens == NULL || TokenCount == NULL) {
        return E_POINTER;
    }

    /* Parse line into tokens */
    i = 0;
    while (i < Length && count < DCL_MAX_TOKENS) {
        /* Skip whitespace */
        while (i < Length && DclIsWhitespace(Line[i])) {
            i++;
        }

        if (i >= Length) {
            break;
        }

        tokenStart = i;

        /* Handle comment (rest of line) */
        if (Line[i] == '!') {
            inComment = TRUE;
            while (i < Length) {
                i++;
            }
            tokenLen = i - tokenStart;

            pParser->TokenBuffer[count].Type = DCL_SYNTAX_COMMENT;
            pParser->TokenBuffer[count].Text = &Line[tokenStart];
            pParser->TokenBuffer[count].Length = tokenLen;
            pParser->TokenBuffer[count].ForegroundColor = Parser_GetColorForType(DCL_SYNTAX_COMMENT);
            pParser->TokenBuffer[count].BackgroundColor = DCL_COLOR_DEFAULT;
            pParser->TokenBuffer[count].Flags = 0;
            count++;
            break;
        }

        /* Handle string */
        if (Line[i] == '"') {
            inString = TRUE;
            i++; /* Skip opening quote */
            while (i < Length && Line[i] != '"') {
                i++;
            }
            if (i < Length) {
                i++; /* Skip closing quote */
            }
            tokenLen = i - tokenStart;

            pParser->TokenBuffer[count].Type = DCL_SYNTAX_STRING;
            pParser->TokenBuffer[count].Text = &Line[tokenStart];
            pParser->TokenBuffer[count].Length = tokenLen;
            pParser->TokenBuffer[count].ForegroundColor = Parser_GetColorForType(DCL_SYNTAX_STRING);
            pParser->TokenBuffer[count].BackgroundColor = DCL_COLOR_DEFAULT;
            pParser->TokenBuffer[count].Flags = 0;
            count++;
            isFirstToken = FALSE;
            continue;
        }

        /* Handle regular token */
        while (i < Length && !DclIsWhitespace(Line[i]) &&
               Line[i] != '"' && Line[i] != '!' &&
               Line[i] != '=' && Line[i] != '(' && Line[i] != ')' &&
               Line[i] != '[' && Line[i] != ']' && Line[i] != ',') {
            i++;
        }

        tokenLen = i - tokenStart;

        if (tokenLen > 0) {
            DCL_SYNTAX_TYPE tokenType = Parser_IdentifyToken(
                &Line[tokenStart],
                tokenLen,
                isFirstToken);

            pParser->TokenBuffer[count].Type = tokenType;
            pParser->TokenBuffer[count].Text = &Line[tokenStart];
            pParser->TokenBuffer[count].Length = tokenLen;
            pParser->TokenBuffer[count].ForegroundColor = Parser_GetColorForType(tokenType);
            pParser->TokenBuffer[count].BackgroundColor = DCL_COLOR_DEFAULT;
            pParser->TokenBuffer[count].Flags = 0;
            count++;
            isFirstToken = FALSE;
        }

        /* Handle operators as separate tokens */
        if (i < Length) {
            switch (Line[i]) {
                case '=':
                case '(':
                case ')':
                case '[':
                case ']':
                case ',':
                    if (count < DCL_MAX_TOKENS) {
                        pParser->TokenBuffer[count].Type = DCL_SYNTAX_OPERATOR;
                        pParser->TokenBuffer[count].Text = &Line[i];
                        pParser->TokenBuffer[count].Length = 1;
                        pParser->TokenBuffer[count].ForegroundColor = Parser_GetColorForType(DCL_SYNTAX_OPERATOR);
                        pParser->TokenBuffer[count].BackgroundColor = DCL_COLOR_DEFAULT;
                        pParser->TokenBuffer[count].Flags = 0;
                        count++;
                    }
                    i++;
                    break;
            }
        }
    }

    *Tokens = pParser->TokenBuffer;
    *TokenCount = count;

    return S_OK;
}

/**
 * FreeTokens implementation
 */
static HRESULT STDMETHODCALLTYPE Parser_FreeTokens(
    IDclParser *This,
    DCL_SYNTAX_TOKEN *Tokens,
    UINTN TokenCount)
{
    /* Tokens use internal buffer, no need to free */
    return S_OK;
}

/**
 * SetSyntaxProvider implementation
 */
static HRESULT STDMETHODCALLTYPE Parser_SetSyntaxProvider(
    IDclParser *This,
    IDclSyntaxProvider *Provider)
{
    DCL_PARSER *pParser = (DCL_PARSER *)This;

    if (Provider == NULL) {
        return E_POINTER;
    }

    /* Release old provider if any */
    if (pParser->SyntaxProvider != NULL) {
        pParser->SyntaxProvider->lpVtbl->Release((IUnknown *)pParser->SyntaxProvider);
    }

    /* Set new provider */
    pParser->SyntaxProvider = Provider;
    Provider->lpVtbl->AddRef((IUnknown *)Provider);

    return S_OK;
}

/**
 * Factory function to create parser
 */
HRESULT DclCreateParser(IDclParser **Parser)
{
    static DCL_PARSER gParser;

    if (Parser == NULL) {
        return E_POINTER;
    }

    /* Initialize parser on first use */
    if (gParser.Interface.lpVtbl == NULL) {
        gParser.Interface.lpVtbl = &gParserVtbl;
        gParser.RefCount = 1;
        gParser.SyntaxProvider = NULL;
    }

    *Parser = &gParser.Interface;
    gParser.Interface.lpVtbl->AddRef(*Parser);

    return S_OK;
}
