/*
 * YAML UI Composition Parser
 *
 * Allows defining TUI layouts in YAML format for easy UI design.
 * Supports all TUI widgets, layouts, and styling.
 *
 * Example YAML:
 *
 * window:
 *   title: "Configuration"
 *   width: 80
 *   height: 25
 *   border: single
 *   resizable: true
 *   draggable: true
 *   children:
 *     - type: groupbox
 *       title: "General Settings"
 *       x: 2
 *       y: 2
 *       width: 35
 *       height: 10
 *       border: sunken
 *       children:
 *         - type: checkbox
 *           label: "Enable feature"
 *           x: 2
 *           y: 1
 *           id: checkbox_enable
 *         - type: input
 *           label: "Path:"
 *           x: 2
 *           y: 3
 *           width: 25
 *           id: input_path
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

/* Widget registry entry */
typedef struct _UI_WIDGET_ENTRY {
    CHAR8 Id[64];
    VOID *Widget;
    struct _UI_WIDGET_ENTRY *Next;
} UI_WIDGET_ENTRY;

/* UI Composer context */
typedef struct {
    ITuiScreen *Screen;
    UI_WIDGET_ENTRY *WidgetRegistry;
    CHAR8 *YamlContent;
} UI_COMPOSER_CONTEXT;

/**
  Parse widget type from string.
**/
static INT32 ParseWidgetType(CONST CHAR8 *TypeStr)
{
    if (strcmp(TypeStr, "window") == 0) return 0;
    if (strcmp(TypeStr, "groupbox") == 0) return 1;
    if (strcmp(TypeStr, "checkbox") == 0) return 2;
    if (strcmp(TypeStr, "input") == 0) return 3;
    if (strcmp(TypeStr, "button") == 0) return 4;
    if (strcmp(TypeStr, "radio") == 0) return 5;
    if (strcmp(TypeStr, "listbox") == 0) return 6;
    if (strcmp(TypeStr, "combobox") == 0) return 7;
    if (strcmp(TypeStr, "dropdown") == 0) return 8;
    if (strcmp(TypeStr, "tabcontrol") == 0) return 9;
    if (strcmp(TypeStr, "progressbar") == 0) return 10;
    if (strcmp(TypeStr, "label") == 0) return 11;
    return -1;
}

/**
  Parse border style from string.
**/
static TUI_BORDER_STYLE ParseBorderStyle(CONST CHAR8 *StyleStr)
{
    if (strcmp(StyleStr, "none") == 0) return TuiBorderNone;
    if (strcmp(StyleStr, "single") == 0) return TuiBorderSingle;
    if (strcmp(StyleStr, "double") == 0) return TuiBorderDouble;
    if (strcmp(StyleStr, "rounded") == 0) return TuiBorderRounded;
    if (strcmp(StyleStr, "ascii") == 0) return TuiBorderAscii;
    if (strcmp(StyleStr, "single-double") == 0) return TuiBorderSingleDouble;
    if (strcmp(StyleStr, "double-single") == 0) return TuiBorderDoubleSingle;
    if (strcmp(StyleStr, "flat") == 0) return TuiBorderFlat;
    if (strcmp(StyleStr, "sunken") == 0) return TuiBorderSunken;
    if (strcmp(StyleStr, "risen") == 0) return TuiBorderRisen;
    if (strcmp(StyleStr, "3d") == 0) return TuiBorder3D;
    if (strcmp(StyleStr, "etched") == 0) return TuiBorderEtched;
    if (strcmp(StyleStr, "ridge") == 0) return TuiBorderRidge;
    if (strcmp(StyleStr, "dashed") == 0) return TuiBorderDashed;
    if (strcmp(StyleStr, "dotted") == 0) return TuiBorderDotted;
    if (strcmp(StyleStr, "thick") == 0) return TuiBorderThick;
    if (strcmp(StyleStr, "block") == 0) return TuiBorderBlock;
    return TuiBorderSingle;  /* Default */
}

/**
  Register a widget in the registry for later lookup.
**/
static HRESULT RegisterWidget(
    UI_COMPOSER_CONTEXT *Context,
    CONST CHAR8 *Id,
    VOID *Widget
)
{
    UI_WIDGET_ENTRY *entry;

    if (Context == NULL || Id == NULL || Widget == NULL) {
        return E_POINTER;
    }

    entry = (UI_WIDGET_ENTRY *)malloc(sizeof(UI_WIDGET_ENTRY));
    if (entry == NULL) {
        return E_OUTOFMEMORY;
    }

    strncpy(entry->Id, Id, sizeof(entry->Id) - 1);
    entry->Id[sizeof(entry->Id) - 1] = '\0';
    entry->Widget = Widget;
    entry->Next = Context->WidgetRegistry;
    Context->WidgetRegistry = entry;

    return S_OK;
}

/**
  Find a widget by ID.
**/
static VOID *FindWidget(
    UI_COMPOSER_CONTEXT *Context,
    CONST CHAR8 *Id
)
{
    UI_WIDGET_ENTRY *entry;

    if (Context == NULL || Id == NULL) {
        return NULL;
    }

    for (entry = Context->WidgetRegistry; entry != NULL; entry = entry->Next) {
        if (strcmp(entry->Id, Id) == 0) {
            return entry->Widget;
        }
    }

    return NULL;
}

/**
  Create a window from YAML properties (stub).
**/
static HRESULT CreateWindowFromYAML(
    UI_COMPOSER_CONTEXT *Context,
    CONST CHAR8 *Title,
    INT32 Width,
    INT32 Height,
    TUI_BORDER_STYLE BorderStyle,
    BOOLEAN Resizable,
    BOOLEAN Draggable,
    ITuiWindow **Window
)
{
    HRESULT hr;
    TUI_RECT rect;
    UINT32 flags = 0;

    hr = AnxTuiCreateWindow(Context->Screen, Window);
    if (FAILED(hr)) {
        return hr;
    }

    /* Set window properties */
    rect.X = 0;
    rect.Y = 0;
    rect.Width = Width;
    rect.Height = Height;

    (*Window)->Vtbl->SetBounds(*Window, &rect);
    (*Window)->Vtbl->SetTitle(*Window, Title);
    (*Window)->Vtbl->SetBorderStyle(*Window, BorderStyle);

    if (Resizable) flags |= TuiWindowResizable;
    if (Draggable) flags |= TuiWindowDraggable;

    (*Window)->Vtbl->SetFlags(*Window, flags);

    return S_OK;
}

/**
  Create a group box from YAML properties (stub).
**/
static HRESULT CreateGroupBoxFromYAML(
    UI_COMPOSER_CONTEXT *Context,
    CONST CHAR8 *Title,
    TUI_BORDER_STYLE BorderStyle,
    ITuiGroupBox **GroupBox
)
{
    HRESULT hr;

    hr = AnxTuiCreateGroupBox(Title, GroupBox);
    if (FAILED(hr)) {
        return hr;
    }

    (*GroupBox)->Vtbl->SetBorderStyle(*GroupBox, BorderStyle);

    return S_OK;
}

/**
  Create a checkbox from YAML properties (stub).
**/
static HRESULT CreateCheckboxFromYAML(
    UI_COMPOSER_CONTEXT *Context,
    CONST CHAR8 *Label,
    BOOLEAN Checked,
    ITuiCheckbox **Checkbox
)
{
    HRESULT hr;

    hr = AnxTuiCreateCheckbox(Checkbox);
    if (FAILED(hr)) {
        return hr;
    }

    (*Checkbox)->Vtbl->SetLabel(*Checkbox, Label);
    (*Checkbox)->Vtbl->SetChecked(*Checkbox, Checked);

    return S_OK;
}

/**
  Parse YAML and compose UI (stub implementation).

  Full implementation would use a YAML parser library (libyaml, etc.)
  to parse the YAML and create widgets accordingly.
**/
HRESULT ANXAPI AnxTuiComposeFromYAML(
    IN  ITuiScreen *Screen,
    IN  CONST CHAR8 *YamlFilePath,
    OUT ITuiWindow **RootWindow
)
{
    UI_COMPOSER_CONTEXT context;
    FILE *file;
    long fileSize;
    HRESULT hr;

    if (Screen == NULL || YamlFilePath == NULL || RootWindow == NULL) {
        return E_POINTER;
    }

    /* Initialize context */
    memset(&context, 0, sizeof(UI_COMPOSER_CONTEXT));
    context.Screen = Screen;
    context.WidgetRegistry = NULL;

    /* Read YAML file */
    file = fopen(YamlFilePath, "r");
    if (file == NULL) {
        return E_FAIL;
    }

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    context.YamlContent = (CHAR8 *)malloc(fileSize + 1);
    if (context.YamlContent == NULL) {
        fclose(file);
        return E_OUTOFMEMORY;
    }

    fread(context.YamlContent, 1, fileSize, file);
    context.YamlContent[fileSize] = '\0';
    fclose(file);

    /*
     * TODO: Parse YAML using libyaml or similar library
     * For now, this is a stub that creates a simple window
     */

    /* Create a simple default window as placeholder */
    hr = CreateWindowFromYAML(
        &context,
        "Default Window",
        80,
        25,
        TuiBorderSingle,
        TRUE,
        TRUE,
        RootWindow
    );

    /* Cleanup */
    free(context.YamlContent);

    /* Free widget registry */
    while (context.WidgetRegistry != NULL) {
        UI_WIDGET_ENTRY *next = context.WidgetRegistry->Next;
        free(context.WidgetRegistry);
        context.WidgetRegistry = next;
    }

    return hr;
}

/**
  Save UI composition to YAML file (stub).
**/
HRESULT ANXAPI AnxTuiSaveToYAML(
    IN ITuiWindow *RootWindow,
    IN CONST CHAR8 *YamlFilePath
)
{
    FILE *file;

    if (RootWindow == NULL || YamlFilePath == NULL) {
        return E_POINTER;
    }

    file = fopen(YamlFilePath, "w");
    if (file == NULL) {
        return E_FAIL;
    }

    /* TODO: Serialize window hierarchy to YAML */
    fprintf(file, "# UI Composition\n");
    fprintf(file, "window:\n");
    fprintf(file, "  title: \"Window\"\n");
    fprintf(file, "  width: 80\n");
    fprintf(file, "  height: 25\n");

    fclose(file);
    return S_OK;
}
