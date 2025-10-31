/*
 * Markdown Viewer Widget Implementation
 *
 * Widget for displaying rendered markdown content.
 * Uses the markdown parser COM component to parse and display markdown.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_RENDERED_LINES 1024

typedef struct {
    CHAR8 Text[512];
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT32 Indent;
    BOOLEAN Bold;
    BOOLEAN Underline;
} RenderedLine;

typedef struct {
    ITuiMarkdownViewer Interface;
    WIDGET_STATE State;
    IMarkdownParser *Parser;
    CHAR8 *SourceMarkdown;
    RenderedLine Lines[MAX_RENDERED_LINES];
    UINT32 LineCount;
    UINT32 ScrollOffset;
    UINT32 ViewportHeight;
    UINT32 ViewportWidth;
    ITuiScrollBar *VScrollBar;
    ITuiScrollBar *HScrollBar;
    BOOLEAN ShowScrollBars;
} TuiMarkdownViewerImpl;

/* IUnknown methods */
static HRESULT ANXAPI MarkdownViewer_QueryInterface(
    ITuiMarkdownViewer *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI MarkdownViewer_AddRef(ITuiMarkdownViewer *This)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI MarkdownViewer_Release(ITuiMarkdownViewer *This)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Parser) impl->Parser->Vtbl->Release(impl->Parser);
        if (impl->SourceMarkdown) free(impl->SourceMarkdown);
        if (impl->VScrollBar) impl->VScrollBar->Vtbl->Release(impl->VScrollBar);
        if (impl->HScrollBar) impl->HScrollBar->Vtbl->Release(impl->HScrollBar);
        free(impl);
    }
    return refCount;
}

/* Helper: Render markdown node tree to lines */
static VOID RenderNode(
    TuiMarkdownViewerImpl *impl,
    MarkdownNode *node,
    UINT32 indent
)
{
    UINT32 i;
    RenderedLine *line;

    if (impl->LineCount >= MAX_RENDERED_LINES) return;

    switch (node->Type) {
        case MdNodeHeading:
            /* Render heading with appropriate styling */
            line = &impl->Lines[impl->LineCount++];
            line->Indent = 0;
            line->Bold = TRUE;
            line->Underline = (node->Level == 1);
            line->Foreground = TuiColorCyan;
            line->Background = TuiColorBlack;

            /* Add # prefix */
            for (i = 0; i < node->Level && i < 6; i++) {
                line->Text[i] = '#';
            }
            line->Text[i] = ' ';
            strncpy(line->Text + i + 1, node->Text, sizeof(line->Text) - i - 2);

            /* Add blank line after heading */
            if (impl->LineCount < MAX_RENDERED_LINES) {
                line = &impl->Lines[impl->LineCount++];
                line->Text[0] = '\0';
                line->Indent = 0;
                line->Foreground = TuiColorWhite;
                line->Background = TuiColorBlack;
            }
            break;

        case MdNodeParagraph:
            /* Render paragraph */
            line = &impl->Lines[impl->LineCount++];
            line->Indent = indent;
            line->Bold = FALSE;
            line->Underline = FALSE;
            line->Foreground = TuiColorWhite;
            line->Background = TuiColorBlack;
            strncpy(line->Text, node->Text, sizeof(line->Text) - 1);

            /* Add blank line after paragraph */
            if (impl->LineCount < MAX_RENDERED_LINES) {
                line = &impl->Lines[impl->LineCount++];
                line->Text[0] = '\0';
                line->Indent = 0;
                line->Foreground = TuiColorWhite;
                line->Background = TuiColorBlack;
            }
            break;

        case MdNodeCodeBlock:
            /* Render code block with background */
            line = &impl->Lines[impl->LineCount++];
            line->Indent = 2;
            line->Bold = FALSE;
            line->Underline = FALSE;
            line->Foreground = TuiColorGreen;
            line->Background = TuiColorBlack;
            strncpy(line->Text, node->Text, sizeof(line->Text) - 1);
            break;

        case MdNodeBlockquote:
            /* Render blockquote with > prefix */
            line = &impl->Lines[impl->LineCount++];
            line->Indent = indent + 2;
            line->Bold = FALSE;
            line->Underline = FALSE;
            line->Foreground = TuiColorBrightBlack;
            line->Background = TuiColorBlack;
            snprintf(line->Text, sizeof(line->Text), "> %s", node->Text);
            break;

        case MdNodeHorizontalRule:
            /* Render horizontal rule */
            line = &impl->Lines[impl->LineCount++];
            line->Indent = 0;
            line->Bold = FALSE;
            line->Underline = FALSE;
            line->Foreground = TuiColorBrightBlack;
            line->Background = TuiColorBlack;
            for (i = 0; i < 60 && i < sizeof(line->Text) - 1; i++) {
                line->Text[i] = '─';
            }
            line->Text[i] = '\0';
            break;

        case MdNodeList:
            /* Render list (children are list items) */
            for (i = 0; i < node->ChildCount; i++) {
                MarkdownNode *item = node->Children[i];
                if (impl->LineCount >= MAX_RENDERED_LINES) break;

                line = &impl->Lines[impl->LineCount++];
                line->Indent = indent + 2;
                line->Bold = FALSE;
                line->Underline = FALSE;
                line->Foreground = TuiColorWhite;
                line->Background = TuiColorBlack;

                if (node->Ordered) {
                    snprintf(line->Text, sizeof(line->Text), "%d. %s", i + 1, item->Text);
                } else {
                    snprintf(line->Text, sizeof(line->Text), "• %s", item->Text);
                }
            }

            /* Add blank line after list */
            if (impl->LineCount < MAX_RENDERED_LINES) {
                line = &impl->Lines[impl->LineCount++];
                line->Text[0] = '\0';
                line->Indent = 0;
                line->Foreground = TuiColorWhite;
                line->Background = TuiColorBlack;
            }
            break;

        default:
            break;
    }
}

/* ITuiMarkdownViewer methods */
static HRESULT ANXAPI MarkdownViewer_SetMarkdown(
    ITuiMarkdownViewer *This,
    CONST CHAR8 *Markdown
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    HRESULT hr;
    MarkdownNode *root;
    UINT32 i;

    if (Markdown == NULL) return E_POINTER;

    /* Free old markdown */
    if (impl->SourceMarkdown) {
        free(impl->SourceMarkdown);
    }
    impl->SourceMarkdown = strdup(Markdown);

    /* Create parser if needed */
    if (impl->Parser == NULL) {
        hr = AnxCreateMarkdownParser(&impl->Parser);
        if (FAILED(hr)) return hr;
    }

    /* Parse markdown */
    hr = impl->Parser->Vtbl->Parse(impl->Parser, Markdown);
    if (FAILED(hr)) return hr;

    /* Get root node */
    hr = impl->Parser->Vtbl->GetRoot(impl->Parser, &root);
    if (FAILED(hr)) return hr;

    /* Render to lines */
    impl->LineCount = 0;
    if (root) {
        for (i = 0; i < root->ChildCount; i++) {
            RenderNode(impl, root->Children[i], 0);
        }
    }

    /* Update scrollbar */
    if (impl->VScrollBar) {
        impl->VScrollBar->Vtbl->SetRange(impl->VScrollBar, 0, impl->LineCount);
    }

    return S_OK;
}

static HRESULT ANXAPI MarkdownViewer_LoadFromFile(
    ITuiMarkdownViewer *This,
    CONST CHAR8 *FilePath
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    FILE *fp;
    CHAR8 *buffer;
    LONG fileSize;
    HRESULT hr;

    if (FilePath == NULL) return E_POINTER;

    /* Open file */
    fp = fopen(FilePath, "r");
    if (fp == NULL) return E_FAIL;

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Allocate buffer */
    buffer = (CHAR8 *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fclose(fp);
        return E_OUTOFMEMORY;
    }

    /* Read file */
    fread(buffer, 1, fileSize, fp);
    buffer[fileSize] = '\0';
    fclose(fp);

    /* Set markdown */
    hr = MarkdownViewer_SetMarkdown(This, buffer);
    free(buffer);

    return hr;
}

static HRESULT ANXAPI MarkdownViewer_SetViewportSize(
    ITuiMarkdownViewer *This,
    UINT32 Width,
    UINT32 Height
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    impl->ViewportWidth = Width;
    impl->ViewportHeight = Height;

    /* Update scrollbar page size */
    if (impl->VScrollBar) {
        impl->VScrollBar->Vtbl->SetPageSize(impl->VScrollBar, Height);
    }

    return S_OK;
}

static HRESULT ANXAPI MarkdownViewer_ScrollTo(
    ITuiMarkdownViewer *This,
    UINT32 Line
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;

    if (Line >= impl->LineCount) {
        Line = impl->LineCount > 0 ? impl->LineCount - 1 : 0;
    }

    impl->ScrollOffset = Line;

    /* Update scrollbar */
    if (impl->VScrollBar) {
        impl->VScrollBar->Vtbl->SetValue(impl->VScrollBar, Line);
    }

    return S_OK;
}

static HRESULT ANXAPI MarkdownViewer_ScrollBy(
    ITuiMarkdownViewer *This,
    INT32 Lines
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    INT32 newOffset = (INT32)impl->ScrollOffset + Lines;

    if (newOffset < 0) newOffset = 0;
    if ((UINT32)newOffset >= impl->LineCount) {
        newOffset = impl->LineCount > 0 ? impl->LineCount - 1 : 0;
    }

    return MarkdownViewer_ScrollTo(This, newOffset);
}

static HRESULT ANXAPI MarkdownViewer_Render(
    ITuiMarkdownViewer *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;
    UINT32 i, line;
    CHAR8 display[1024];

    if (!impl->State.Visible) return S_OK;

    /* Clear viewport */
    ClearRect(Screen, X, Y, Width, Height, TuiColorBlack);

    /* Render visible lines */
    for (i = 0; i < Height && (impl->ScrollOffset + i) < impl->LineCount; i++) {
        line = impl->ScrollOffset + i;
        RenderedLine *rline = &impl->Lines[line];

        /* Apply indentation */
        UINT32 indent = rline->Indent;
        if (indent > Width - 2) indent = Width - 2;

        /* Format with indentation */
        for (UINT32 j = 0; j < indent; j++) {
            display[j] = ' ';
        }
        strncpy(display + indent, rline->Text, Width - indent - 1);
        display[Width - 1] = '\0';

        /* Render line */
        Screen->Vtbl->WriteText(Screen, X, Y + i, display,
                                rline->Foreground, rline->Background);

        /* Apply underline for headings if needed */
        if (rline->Underline && i + 1 < Height) {
            UINT32 len = strlen(rline->Text);
            for (UINT32 j = 0; j < len && j < Width - indent; j++) {
                display[j] = '─';
            }
            display[len] = '\0';
            Screen->Vtbl->WriteText(Screen, X + indent, Y + i + 1, display,
                                    rline->Foreground, rline->Background);
            i++;  /* Skip next line */
        }
    }

    /* Render scrollbar */
    if (impl->ShowScrollBars && impl->VScrollBar && impl->LineCount > Height) {
        impl->VScrollBar->Vtbl->Render(impl->VScrollBar, Screen,
                                        X + Width - 1, Y, 1, Height);
    }

    return S_OK;
}

static HRESULT ANXAPI MarkdownViewer_HandleKey(
    ITuiMarkdownViewer *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiMarkdownViewerImpl *impl = (TuiMarkdownViewerImpl *)This;

    switch (Key) {
        case TuiKeyUp:
            MarkdownViewer_ScrollBy(This, -1);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
            MarkdownViewer_ScrollBy(This, 1);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            MarkdownViewer_ScrollBy(This, -(INT32)impl->ViewportHeight);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            MarkdownViewer_ScrollBy(This, (INT32)impl->ViewportHeight);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            MarkdownViewer_ScrollTo(This, 0);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            MarkdownViewer_ScrollTo(This, impl->LineCount);
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiMarkdownViewer_Vtbl MarkdownViewerVtbl = {
    MarkdownViewer_QueryInterface,
    MarkdownViewer_AddRef,
    MarkdownViewer_Release,
    MarkdownViewer_SetMarkdown,
    MarkdownViewer_LoadFromFile,
    MarkdownViewer_SetViewportSize,
    MarkdownViewer_ScrollTo,
    MarkdownViewer_ScrollBy,
    MarkdownViewer_Render,
    MarkdownViewer_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateMarkdownViewer(OUT ITuiMarkdownViewer **Viewer)
{
    TuiMarkdownViewerImpl *impl;
    HRESULT hr;

    if (Viewer == NULL) return E_POINTER;

    impl = (TuiMarkdownViewerImpl *)calloc(1, sizeof(TuiMarkdownViewerImpl));
    if (impl == NULL) {
        *Viewer = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &MarkdownViewerVtbl;
    InitWidgetState(&impl->State);

    impl->Parser = NULL;
    impl->SourceMarkdown = NULL;
    impl->LineCount = 0;
    impl->ScrollOffset = 0;
    impl->ViewportHeight = 25;
    impl->ViewportWidth = 80;
    impl->ShowScrollBars = TRUE;

    /* Create scrollbar */
    hr = AnxTuiCreateScrollBar(TRUE, &impl->VScrollBar);
    if (FAILED(hr)) {
        free(impl);
        *Viewer = NULL;
        return hr;
    }

    impl->VScrollBar->Vtbl->SetRange(impl->VScrollBar, 0, 100);
    impl->VScrollBar->Vtbl->SetPageSize(impl->VScrollBar, 25);

    impl->HScrollBar = NULL;

    *Viewer = &impl->Interface;
    return S_OK;
}
