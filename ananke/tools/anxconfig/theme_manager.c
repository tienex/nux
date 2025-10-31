/*
 * theme_manager.c - Theme Manager Implementation
 *
 * Provides theming support for all widgets.
 * Themes can be loaded from YAML files and applied globally.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_THEME_NAME 64

typedef struct {
    ITuiTheme Interface;
    UINTN RefCount;

    CHAR8 Name[MAX_THEME_NAME];
    TUI_BORDER_STYLE BorderStyle;

    /* Theme colors for each component */
    TUI_THEME_COLORS Colors[19];  /* Number of TUI_THEME_COMPONENT values */

} ThemeImpl;

/* Default theme presets */
static CONST TUI_THEME_COLORS DefaultDarkTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeMenu */             {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorBlack, TuiColorYellow, TuiAttrNormal},
    /* TuiThemeWindow */           {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowTitle */      {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeButtonFocused */    {TuiColorBlack, TuiColorYellow, TuiAttrNormal},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorBlack, TuiColorYellow, TuiAttrNormal},
    /* TuiThemeCheckbox */         {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorBlack, TuiColorYellow, TuiAttrNormal},
    /* TuiThemeListBox */          {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeStatusBar */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDesktop */          {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorBlack, TuiAttrNormal}
};

static CONST TUI_THEME_COLORS DefaultLightTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeMenu */             {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindow */           {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorBlue, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeWindowTitle */      {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeButtonFocused */    {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorBlack, TuiColorBrightWhite, TuiAttrNormal},
    /* TuiThemeCheckbox */         {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeListBox */          {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeStatusBar */        {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeDesktop */          {TuiColorBlack, TuiColorBrightWhite, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorWhite, TuiAttrNormal}
};

static CONST TUI_THEME_COLORS DefaultMonochromeTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorWhite, TuiColorBlack, TuiAttrReverse},
    /* TuiThemeMenu */             {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorWhite, TuiColorBlack, TuiAttrReverse | TuiAttrBold},
    /* TuiThemeWindow */           {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeWindowTitle */      {TuiColorWhite, TuiColorBlack, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorWhite, TuiColorBlack, TuiAttrReverse},
    /* TuiThemeButtonFocused */    {TuiColorWhite, TuiColorBlack, TuiAttrReverse | TuiAttrBold},
    /* TuiThemeInput */            {TuiColorWhite, TuiColorBlack, TuiAttrUnderline},
    /* TuiThemeInputFocused */     {TuiColorWhite, TuiColorBlack, TuiAttrUnderline | TuiAttrBold},
    /* TuiThemeCheckbox */         {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorWhite, TuiColorBlack, TuiAttrBold},
    /* TuiThemeListBox */          {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorWhite, TuiColorBlack, TuiAttrReverse},
    /* TuiThemeStatusBar */        {TuiColorWhite, TuiColorBlack, TuiAttrReverse},
    /* TuiThemeDesktop */          {TuiColorWhite, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorWhite, TuiColorBlack, TuiAttrReverse},
    /* TuiThemeDisabled */         {TuiColorWhite, TuiColorBlack, TuiAttrDim}
};

/* IUnknown methods */
static HRESULT ANXAPI Theme_QueryInterface(
    ITuiTheme *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiTheme)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Theme_AddRef(ITuiTheme *This)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI Theme_Release(ITuiTheme *This)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        free(impl);
    }

    return count;
}

/* Set colors for a component */
static HRESULT ANXAPI Theme_SetColors(
    ITuiTheme *This,
    TUI_THEME_COMPONENT Component,
    CONST TUI_THEME_COLORS *Colors
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Colors) return E_INVALIDARG;
    if (Component >= 19) return E_INVALIDARG;

    impl->Colors[Component] = *Colors;

    return S_OK;
}

/* Get colors for a component */
static HRESULT ANXAPI Theme_GetColors(
    ITuiTheme *This,
    TUI_THEME_COMPONENT Component,
    TUI_THEME_COLORS *Colors
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Colors) return E_INVALIDARG;
    if (Component >= 19) return E_INVALIDARG;

    *Colors = impl->Colors[Component];

    return S_OK;
}

/* Set border style */
static HRESULT ANXAPI Theme_SetBorderStyle(
    ITuiTheme *This,
    TUI_BORDER_STYLE Style
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    impl->BorderStyle = Style;
    return S_OK;
}

/* Get border style */
static HRESULT ANXAPI Theme_GetBorderStyle(
    ITuiTheme *This,
    TUI_BORDER_STYLE *Style
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Style) return E_INVALIDARG;

    *Style = impl->BorderStyle;
    return S_OK;
}

/* Load theme from YAML file */
static HRESULT ANXAPI Theme_LoadFromFile(
    ITuiTheme *This,
    CONST CHAR8 *FilePath
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    FILE *file = NULL;
    CHAR8 line[256];

    if (!FilePath) return E_INVALIDARG;

    file = fopen(FilePath, "r");
    if (!file) return E_FAIL;

    /* Simple YAML parser - production would use proper YAML library */
    while (fgets(line, sizeof(line), file)) {
        /* Parse theme properties */
        if (strstr(line, "name:")) {
            CHAR8 *value = strchr(line, ':');
            if (value) {
                value++;
                while (*value == ' ') value++;
                /* Remove newline */
                CHAR8 *newline = strchr(value, '\n');
                if (newline) *newline = '\0';
                strncpy(impl->Name, value, sizeof(impl->Name) - 1);
            }
        }
        /* Add more parsing for colors, border style, etc. */
    }

    fclose(file);
    return S_OK;
}

/* Save theme to YAML file */
static HRESULT ANXAPI Theme_SaveToFile(
    ITuiTheme *This,
    CONST CHAR8 *FilePath
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    FILE *file = NULL;

    if (!FilePath) return E_INVALIDARG;

    file = fopen(FilePath, "w");
    if (!file) return E_FAIL;

    fprintf(file, "# ANXCONFIG Theme\n");
    fprintf(file, "name: %s\n", impl->Name);
    fprintf(file, "border_style: %d\n", impl->BorderStyle);
    fprintf(file, "\ncolors:\n");

    CONST CHAR8 *componentNames[] = {
        "menu_bar", "menu", "menu_item", "menu_item_selected",
        "window", "window_border", "window_title",
        "button", "button_focused",
        "input", "input_focused",
        "checkbox", "checkbox_focused",
        "list_box", "list_box_selected",
        "status_bar", "desktop", "selection", "disabled"
    };

    for (INT32 i = 0; i < 19; i++) {
        fprintf(file, "  %s:\n", componentNames[i]);
        fprintf(file, "    foreground: %d\n", impl->Colors[i].Foreground);
        fprintf(file, "    background: %d\n", impl->Colors[i].Background);
        fprintf(file, "    attributes: %d\n", impl->Colors[i].Attributes);
    }

    fclose(file);
    return S_OK;
}

/* Set theme name */
static HRESULT ANXAPI Theme_SetName(
    ITuiTheme *This,
    CONST CHAR8 *Name
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Name) return E_INVALIDARG;

    strncpy(impl->Name, Name, sizeof(impl->Name) - 1);
    impl->Name[sizeof(impl->Name) - 1] = '\0';

    return S_OK;
}

/* Get theme name */
static HRESULT ANXAPI Theme_GetName(
    ITuiTheme *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Buffer || BufferSize == 0) return E_INVALIDARG;

    strncpy(Buffer, impl->Name, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

/* VTable */
static ITuiTheme_Vtbl ThemeVtbl = {
    Theme_QueryInterface,
    Theme_AddRef,
    Theme_Release,
    Theme_SetColors,
    Theme_GetColors,
    Theme_SetBorderStyle,
    Theme_GetBorderStyle,
    Theme_LoadFromFile,
    Theme_SaveToFile,
    Theme_SetName,
    Theme_GetName
};

/* Factory function - Create default dark theme */
HRESULT AnxTuiCreateTheme(
    CONST CHAR8 *ThemeName,
    ITuiTheme **OutTheme
)
{
    ThemeImpl *impl;

    if (!OutTheme) return E_INVALIDARG;

    impl = (ThemeImpl *)calloc(1, sizeof(ThemeImpl));
    if (!impl) return E_OUTOFMEMORY;

    impl->Interface.Vtbl = &ThemeVtbl;
    impl->RefCount = 1;

    /* Set theme name */
    if (ThemeName) {
        strncpy(impl->Name, ThemeName, sizeof(impl->Name) - 1);
    } else {
        strcpy(impl->Name, "Default Dark");
    }

    /* Copy default colors */
    memcpy(impl->Colors, DefaultDarkTheme, sizeof(impl->Colors));

    impl->BorderStyle = TuiBorderSingle;

    *OutTheme = &impl->Interface;
    return S_OK;
}

/* Create light theme */
HRESULT AnxTuiCreateLightTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("Default Light", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, DefaultLightTheme, sizeof(impl->Colors));
    }
    return hr;
}

/* Create monochrome theme */
HRESULT AnxTuiCreateMonochromeTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("Monochrome", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, DefaultMonochromeTheme, sizeof(impl->Colors));
        impl->BorderStyle = TuiBorderAscii;
    }
    return hr;
}
