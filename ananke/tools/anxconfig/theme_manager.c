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
    TUI_BORDER_STYLE ButtonStyle;

    /* Visual properties */
    BOOLEAN WindowShadowEnabled;
    CHAR8 ShadowChar;
    BOOLEAN UseUnicode;

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

/* MS-DOS QBasic Theme - Classic QBasic IDE color scheme */
static CONST TUI_THEME_COLORS QBasicTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenu */             {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorCyan, TuiColorBlack, TuiAttrNormal},
    /* TuiThemeWindow */           {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowTitle */      {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeButtonFocused */    {TuiColorCyan, TuiColorBlack, TuiAttrBold},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeCheckbox */         {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorBlack, TuiColorCyan, TuiAttrBold},
    /* TuiThemeListBox */          {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeStatusBar */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDesktop */          {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorBlue, TuiAttrNormal}
};

/* Borland TurboVision Theme - Classic Turbo Pascal/C++ IDE colors */
static CONST TUI_THEME_COLORS TurboVisionTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenu */             {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorBlack, TuiColorWhite, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorWhite, TuiColorGreen, TuiAttrBold},
    /* TuiThemeWindow */           {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorCyan, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindowTitle */      {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorBlack, TuiColorCyan, TuiAttrBold},
    /* TuiThemeButtonFocused */    {TuiColorYellow, TuiColorGreen, TuiAttrBold},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorBlack, TuiColorWhite, TuiAttrBold},
    /* TuiThemeCheckbox */         {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorYellow, TuiColorGreen, TuiAttrBold},
    /* TuiThemeListBox */          {TuiColorYellow, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorWhite, TuiColorCyan, TuiAttrBold},
    /* TuiThemeStatusBar */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDesktop */          {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorYellow, TuiColorGreen, TuiAttrBold},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorBlue, TuiAttrNormal}
};

/* Norton Utilities Theme - Classic Norton Commander/Utilities look */
static CONST TUI_THEME_COLORS NortonUtilitiesTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenu */             {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindow */           {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeWindowBorder */     {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindowTitle */      {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeButtonFocused */    {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorBlack, TuiColorWhite, TuiAttrBold},
    /* TuiThemeCheckbox */         {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeCheckboxFocused */  {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeListBox */          {TuiColorWhite, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeListBoxSelected */  {TuiColorYellow, TuiColorCyan, TuiAttrBold},
    /* TuiThemeStatusBar */        {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeDesktop */          {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorBlue, TuiAttrNormal}
};

/* Central Point (PC Tools) Theme - Classic PC Tools Desktop look */
static CONST TUI_THEME_COLORS CentralPointTheme[] = {
    /* TuiThemeMenuBar */         {TuiColorWhite, TuiColorMagenta, TuiAttrBold},
    /* TuiThemeMenu */             {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItem */         {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeMenuItemSelected */ {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindow */           {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindowBorder */     {TuiColorWhite, TuiColorBlue, TuiAttrBold},
    /* TuiThemeWindowTitle */      {TuiColorWhite, TuiColorMagenta, TuiAttrBold},
    /* TuiThemeButton */           {TuiColorBlack, TuiColorCyan, TuiAttrBold},
    /* TuiThemeButtonFocused */    {TuiColorYellow, TuiColorRed, TuiAttrBold},
    /* TuiThemeInput */            {TuiColorBlack, TuiColorCyan, TuiAttrNormal},
    /* TuiThemeInputFocused */     {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeCheckbox */         {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeCheckboxFocused */  {TuiColorWhite, TuiColorRed, TuiAttrBold},
    /* TuiThemeListBox */          {TuiColorYellow, TuiColorBlue, TuiAttrBold},
    /* TuiThemeListBoxSelected */  {TuiColorWhite, TuiColorCyan, TuiAttrBold},
    /* TuiThemeStatusBar */        {TuiColorWhite, TuiColorMagenta, TuiAttrBold},
    /* TuiThemeDesktop */          {TuiColorCyan, TuiColorBlue, TuiAttrNormal},
    /* TuiThemeSelection */        {TuiColorWhite, TuiColorRed, TuiAttrBold},
    /* TuiThemeDisabled */         {TuiColorBrightBlack, TuiColorBlue, TuiAttrNormal}
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

/* Set window shadow */
static HRESULT ANXAPI Theme_SetWindowShadow(
    ITuiTheme *This,
    BOOLEAN Enabled,
    CHAR8 ShadowChar
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    impl->WindowShadowEnabled = Enabled;
    impl->ShadowChar = ShadowChar;
    return S_OK;
}

/* Get window shadow */
static HRESULT ANXAPI Theme_GetWindowShadow(
    ITuiTheme *This,
    BOOLEAN *Enabled,
    CHAR8 *ShadowChar
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Enabled || !ShadowChar) return E_INVALIDARG;

    *Enabled = impl->WindowShadowEnabled;
    *ShadowChar = impl->ShadowChar;

    return S_OK;
}

/* Set button style */
static HRESULT ANXAPI Theme_SetButtonStyle(
    ITuiTheme *This,
    TUI_BORDER_STYLE Style
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    impl->ButtonStyle = Style;
    return S_OK;
}

/* Get button style */
static HRESULT ANXAPI Theme_GetButtonStyle(
    ITuiTheme *This,
    TUI_BORDER_STYLE *Style
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!Style) return E_INVALIDARG;

    *Style = impl->ButtonStyle;
    return S_OK;
}

/* Set use unicode */
static HRESULT ANXAPI Theme_SetUseUnicode(
    ITuiTheme *This,
    BOOLEAN UseUnicode
)
{
    ThemeImpl *impl = (ThemeImpl *)This;
    impl->UseUnicode = UseUnicode;
    return S_OK;
}

/* Get use unicode */
static HRESULT ANXAPI Theme_GetUseUnicode(
    ITuiTheme *This,
    BOOLEAN *UseUnicode
)
{
    ThemeImpl *impl = (ThemeImpl *)This;

    if (!UseUnicode) return E_INVALIDARG;

    *UseUnicode = impl->UseUnicode;
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
    Theme_SetWindowShadow,
    Theme_GetWindowShadow,
    Theme_SetButtonStyle,
    Theme_GetButtonStyle,
    Theme_SetUseUnicode,
    Theme_GetUseUnicode,
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
    impl->ButtonStyle = TuiBorderSingle;
    impl->WindowShadowEnabled = FALSE;
    impl->ShadowChar = 0xB1;  /* ░ light shade */
    impl->UseUnicode = TRUE;

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

/* Create MS-DOS QBasic theme */
HRESULT AnxTuiCreateQBasicTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("MS-DOS QBasic", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, QBasicTheme, sizeof(impl->Colors));

        /* QBasic visual style */
        impl->BorderStyle = TuiBorderDouble;        /* Double-line windows */
        impl->ButtonStyle = TuiBorderSingle;        /* Simple buttons */
        impl->WindowShadowEnabled = FALSE;          /* No shadows in QBasic */
        impl->ShadowChar = 0;
        impl->UseUnicode = TRUE;                    /* Use box drawing chars */
    }
    return hr;
}

/* Create Borland TurboVision theme */
HRESULT AnxTuiCreateTurboVisionTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("Borland TurboVision", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, TurboVisionTheme, sizeof(impl->Colors));

        /* TurboVision visual style */
        impl->BorderStyle = TuiBorderDouble;        /* Double-line windows */
        impl->ButtonStyle = TuiBorderSunken;        /* 3D sunken buttons */
        impl->WindowShadowEnabled = TRUE;           /* Windows cast shadows */
        impl->ShadowChar = 0xB0;                    /* ░ medium shade */
        impl->UseUnicode = TRUE;                    /* Use box drawing chars */
    }
    return hr;
}

/* Create Norton Utilities theme */
HRESULT AnxTuiCreateNortonUtilitiesTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("Norton Utilities", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, NortonUtilitiesTheme, sizeof(impl->Colors));

        /* Norton Utilities visual style */
        impl->BorderStyle = TuiBorderSingle;        /* Single-line borders (Norton Commander style) */
        impl->ButtonStyle = TuiBorder3D;            /* 3D effect buttons */
        impl->WindowShadowEnabled = TRUE;           /* Classic Norton shadows */
        impl->ShadowChar = 0xB1;                    /* ░ light shade (Norton style) */
        impl->UseUnicode = TRUE;                    /* Use box drawing chars */
    }
    return hr;
}

/* Create Central Point (PC Tools) theme */
HRESULT AnxTuiCreateCentralPointTheme(ITuiTheme **OutTheme)
{
    HRESULT hr = AnxTuiCreateTheme("Central Point PC Tools", OutTheme);
    if (SUCCEEDED(hr)) {
        ThemeImpl *impl = (ThemeImpl *)*OutTheme;
        memcpy(impl->Colors, CentralPointTheme, sizeof(impl->Colors));

        /* PC Tools visual style */
        impl->BorderStyle = TuiBorderDouble;        /* Double-line borders (PC Tools style) */
        impl->ButtonStyle = TuiBorderRisen;         /* Raised 3D buttons */
        impl->WindowShadowEnabled = TRUE;           /* PC Tools had shadows */
        impl->ShadowChar = 0xB2;                    /* ▒ dark shade (PC Tools style) */
        impl->UseUnicode = TRUE;                    /* Use box drawing chars */
    }
    return hr;
}
