/*
 * Simple TUI Implementation (Stub)
 *
 * This would implement a full ncurses-like TUI.
 * For now, it's just a placeholder.
 */

#include <stdio.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

/* Run menu (stub) */
HRESULT ANXAPI AnxConfigRunMenu(
    IN IConfigDatabase *Database,
    IN CONST CHAR8 *Title
)
{
    printf("[TUI] Running menu: %s (stub)\n", Title);
    printf("[TUI] Full TUI will be implemented using portable terminal codes\n");
    printf("[TUI] Will support: VT100, ANSI, Windows Console, etc.\n");

    return S_OK;
}

/* TUI factory functions (stubs) */
HRESULT ANXAPI AnxTuiCreateScreen(OUT ITuiScreen **Screen)
{
    if (Screen == NULL) {
        return E_POINTER;
    }

    *Screen = NULL;
    return E_NOTIMPL;
}

HRESULT ANXAPI AnxTuiCreateWindow(
    IN  ITuiScreen *ParentScreen,
    OUT ITuiWindow **Window
)
{
    if (Window == NULL) {
        return E_POINTER;
    }

    *Window = NULL;
    return E_NOTIMPL;
}

HRESULT ANXAPI AnxTuiCreateMenu(
    IN  ITuiScreen *ParentScreen,
    IN  CONST CHAR8 *Title,
    OUT ITuiMenu **Menu
)
{
    if (Menu == NULL) {
        return E_POINTER;
    }

    *Menu = NULL;
    return E_NOTIMPL;
}
