/**
 * @file helpers.c
 * @brief Helper functions for DCL shell
 */

#include <dcl/internal.h>

/**
 * DCL built-in keywords (VMS DCL style)
 */
const CHAR8 *gDclKeywords[] = {
    "IF",
    "THEN",
    "ELSE",
    "ENDIF",
    "GOTO",
    "GOSUB",
    "RETURN",
    "EXIT",
    "ON",
    "CALL",
    "READ",
    "WRITE",
    "INQUIRE",
    "OPEN",
    "CLOSE",
    NULL
};

UINTN gDclKeywordCount = 15;

/**
 * DCL built-in commands (VMS DCL style)
 */
const CHAR8 *gDclCommands[] = {
    "HELP",
    "DIR",
    "DIRECTORY",
    "SHOW",
    "SET",
    "CREATE",
    "DELETE",
    "COPY",
    "RENAME",
    "TYPE",
    "PRINT",
    "RUN",
    "SPAWN",
    "LOGOUT",
    "DEFINE",
    "ASSIGN",
    "DEASSIGN",
    "MOUNT",
    "DISMOUNT",
    "ALLOCATE",
    "DEALLOCATE",
    "SUBMIT",
    "PURGE",
    "SEARCH",
    "EDIT",
    "DUMP",
    "ANALYZE",
    "BACKUP",
    "RESTORE",
    NULL
};

UINTN gDclCommandCount = 29;

/**
 * Check if text is a keyword
 */
BOOLEAN DclIsKeyword(const CHAR8 *Text, UINTN Length)
{
    UINTN i;

    if (Text == NULL || Length == 0) {
        return FALSE;
    }

    for (i = 0; gDclKeywords[i] != NULL; i++) {
        UINTN keywordLen = DclStrLen(gDclKeywords[i]);
        if (keywordLen == Length) {
            UINTN j;
            BOOLEAN match = TRUE;

            /* Case-insensitive comparison */
            for (j = 0; j < Length; j++) {
                CHAR8 c1 = Text[j];
                CHAR8 c2 = gDclKeywords[i][j];

                /* Convert to uppercase for comparison */
                if (c1 >= 'a' && c1 <= 'z') {
                    c1 = c1 - 'a' + 'A';
                }
                if (c2 >= 'a' && c2 <= 'z') {
                    c2 = c2 - 'a' + 'A';
                }

                if (c1 != c2) {
                    match = FALSE;
                    break;
                }
            }

            if (match) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Check if text is a command
 */
BOOLEAN DclIsCommand(const CHAR8 *Text, UINTN Length)
{
    UINTN i;

    if (Text == NULL || Length == 0) {
        return FALSE;
    }

    for (i = 0; gDclCommands[i] != NULL; i++) {
        UINTN cmdLen = DclStrLen(gDclCommands[i]);
        if (cmdLen == Length) {
            UINTN j;
            BOOLEAN match = TRUE;

            /* Case-insensitive comparison */
            for (j = 0; j < Length; j++) {
                CHAR8 c1 = Text[j];
                CHAR8 c2 = gDclCommands[i][j];

                /* Convert to uppercase for comparison */
                if (c1 >= 'a' && c1 <= 'z') {
                    c1 = c1 - 'a' + 'A';
                }
                if (c2 >= 'a' && c2 <= 'z') {
                    c2 = c2 - 'a' + 'A';
                }

                if (c1 != c2) {
                    match = FALSE;
                    break;
                }
            }

            if (match) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Check if character is whitespace
 */
BOOLEAN DclIsWhitespace(CHAR8 c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

/**
 * Check if character is alphabetic
 */
BOOLEAN DclIsAlpha(CHAR8 c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/**
 * Check if character is a digit
 */
BOOLEAN DclIsDigit(CHAR8 c)
{
    return (c >= '0' && c <= '9');
}

/**
 * Check if character is alphanumeric
 */
BOOLEAN DclIsAlphaNumeric(CHAR8 c)
{
    return DclIsAlpha(c) || DclIsDigit(c);
}

/**
 * String comparison (case-sensitive)
 */
INT32 DclStrNCmp(const CHAR8 *s1, const CHAR8 *s2, UINTN n)
{
    UINTN i;

    if (s1 == NULL || s2 == NULL) {
        return 0;
    }

    for (i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (INT32)(s1[i] - s2[i]);
        }
        if (s1[i] == '\0') {
            break;
        }
    }

    return 0;
}

/**
 * String length
 */
UINTN DclStrLen(const CHAR8 *s)
{
    UINTN len = 0;

    if (s == NULL) {
        return 0;
    }

    while (s[len] != '\0') {
        len++;
    }

    return len;
}

/**
 * String copy
 */
VOID DclStrCpy(CHAR8 *dest, const CHAR8 *src, UINTN max)
{
    UINTN i;

    if (dest == NULL || src == NULL || max == 0) {
        return;
    }

    for (i = 0; i < max - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    dest[i] = '\0';
}
