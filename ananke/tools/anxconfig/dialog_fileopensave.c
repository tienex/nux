/*
 * File Open/Save Dialog Implementation
 *
 * Common dialog for file selection with directory navigation,
 * filtering, and preview support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_PATH_LENGTH 512
#define MAX_FILTER_LENGTH 64
#define MAX_FILES 256

typedef enum {
    FileDialogOpen,
    FileDialogSave,
    FileDialogDirectory
} FileDialogType;

typedef struct {
    CHAR8 Name[256];
    BOOLEAN IsDirectory;
    UINT64 Size;
} FileEntry;

typedef struct {
    ITuiFileDialog Interface;
    WIDGET_STATE State;
    FileDialogType Type;
    CHAR8 Title[128];
    CHAR8 CurrentPath[MAX_PATH_LENGTH];
    CHAR8 SelectedFile[MAX_PATH_LENGTH];
    CHAR8 Filter[MAX_FILTER_LENGTH];
    FileEntry Files[MAX_FILES];
    UINT32 FileCount;
    INT32 SelectedIndex;
    UINT32 ScrollOffset;
    BOOLEAN DialogResult;      /* TRUE if user clicked OK */
    ITuiListBox *FileList;
    ITuiInput *FileNameInput;
    ITuiInput *PathInput;
    ITuiButton *OkButton;
    ITuiButton *CancelButton;
} TuiFileDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI FileDialog_QueryInterface(
    ITuiFileDialog *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI FileDialog_AddRef(ITuiFileDialog *This)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FileDialog_Release(ITuiFileDialog *This)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        /* Release child widgets */
        if (impl->FileList) impl->FileList->Vtbl->Release(impl->FileList);
        if (impl->FileNameInput) impl->FileNameInput->Vtbl->Release(impl->FileNameInput);
        if (impl->PathInput) impl->PathInput->Vtbl->Release(impl->PathInput);
        if (impl->OkButton) impl->OkButton->Vtbl->Release(impl->OkButton);
        if (impl->CancelButton) impl->CancelButton->Vtbl->Release(impl->CancelButton);
        free(impl);
    }
    return refCount;
}

/* Helper: Scan directory and populate file list */
static HRESULT ScanDirectory(TuiFileDialogImpl *impl)
{
    /* Simplified implementation - would use platform-specific directory APIs */
    /* For now, add some mock entries */

    impl->FileCount = 0;

    /* Add parent directory entry */
    strcpy(impl->Files[impl->FileCount].Name, "..");
    impl->Files[impl->FileCount].IsDirectory = TRUE;
    impl->Files[impl->FileCount].Size = 0;
    impl->FileCount++;

    /* Would call platform-specific directory enumeration here */
    /* Example: WIN32 FindFirstFile/FindNextFile, POSIX opendir/readdir */

    /* Mock entries for demonstration */
    strcpy(impl->Files[impl->FileCount].Name, "config");
    impl->Files[impl->FileCount].IsDirectory = TRUE;
    impl->Files[impl->FileCount].Size = 0;
    impl->FileCount++;

    strcpy(impl->Files[impl->FileCount].Name, "config.yaml");
    impl->Files[impl->FileCount].IsDirectory = FALSE;
    impl->Files[impl->FileCount].Size = 1234;
    impl->FileCount++;

    strcpy(impl->Files[impl->FileCount].Name, "settings.yaml");
    impl->Files[impl->FileCount].IsDirectory = FALSE;
    impl->Files[impl->FileCount].Size = 5678;
    impl->FileCount++;

    /* Update list box */
    if (impl->FileList) {
        UINT32 i;
        CHAR8 display[300];

        impl->FileList->Vtbl->Clear(impl->FileList);

        for (i = 0; i < impl->FileCount; i++) {
            if (impl->Files[i].IsDirectory) {
                snprintf(display, sizeof(display), "[%s]", impl->Files[i].Name);
            } else {
                snprintf(display, sizeof(display), " %s (%llu bytes)",
                         impl->Files[i].Name,
                         (unsigned long long)impl->Files[i].Size);
            }
            impl->FileList->Vtbl->AddItem(impl->FileList, display, NULL);
        }
    }

    return S_OK;
}

/* ITuiFileDialog methods */
static HRESULT ANXAPI FileDialog_SetType(
    ITuiFileDialog *This,
    UINT32 Type
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    impl->Type = (FileDialogType)Type;

    /* Update title based on type */
    switch (impl->Type) {
        case FileDialogOpen:
            strcpy(impl->Title, "Open File");
            break;
        case FileDialogSave:
            strcpy(impl->Title, "Save File");
            break;
        case FileDialogDirectory:
            strcpy(impl->Title, "Select Directory");
            break;
    }

    return S_OK;
}

static HRESULT ANXAPI FileDialog_SetTitle(
    ITuiFileDialog *This,
    CONST CHAR8 *Title
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    if (Title == NULL) return E_POINTER;

    strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI FileDialog_SetInitialPath(
    ITuiFileDialog *This,
    CONST CHAR8 *Path
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    if (Path == NULL) return E_POINTER;

    strncpy(impl->CurrentPath, Path, sizeof(impl->CurrentPath) - 1);
    impl->CurrentPath[sizeof(impl->CurrentPath) - 1] = '\0';

    /* Update path input */
    if (impl->PathInput) {
        impl->PathInput->Vtbl->SetValue(impl->PathInput, impl->CurrentPath);
    }

    /* Rescan directory */
    ScanDirectory(impl);

    return S_OK;
}

static HRESULT ANXAPI FileDialog_SetFilter(
    ITuiFileDialog *This,
    CONST CHAR8 *Filter
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    if (Filter == NULL) return E_POINTER;

    strncpy(impl->Filter, Filter, sizeof(impl->Filter) - 1);
    impl->Filter[sizeof(impl->Filter) - 1] = '\0';

    /* Rescan directory with new filter */
    ScanDirectory(impl);

    return S_OK;
}

static HRESULT ANXAPI FileDialog_Show(
    ITuiFileDialog *This,
    ITuiScreen *Screen
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;

    /* Initialize dialog */
    impl->DialogResult = FALSE;
    impl->State.Visible = TRUE;

    /* Scan initial directory */
    ScanDirectory(impl);

    /* Enter modal loop (simplified) */
    /* Actual implementation would need proper event loop */

    return S_OK;
}

static HRESULT ANXAPI FileDialog_GetResult(
    ITuiFileDialog *This,
    BOOLEAN *Accepted
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    if (Accepted == NULL) return E_POINTER;
    *Accepted = impl->DialogResult;
    return S_OK;
}

static HRESULT ANXAPI FileDialog_GetSelectedPath(
    ITuiFileDialog *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->SelectedFile, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI FileDialog_Render(
    ITuiFileDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;
    UINT32 i;

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow */
    for (i = 0; i < Height + 1; i++) {
        ClearRect(Screen, X + 2, Y + i + 1, Width, 1, TuiColorBlack);
    }

    /* Draw dialog box */
    DrawBoxSingle(Screen, X, Y, Width, Height,
                  TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    CHAR8 titleBar[256];
    snprintf(titleBar, sizeof(titleBar), " %s ", impl->Title);
    Screen->Vtbl->WriteText(Screen, X + 2, Y, titleBar,
                            TuiColorWhite, TuiColorBlue);

    /* Draw path label */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 2, "Path:",
                            TuiColorBlack, TuiColorWhite);

    /* Render path input */
    if (impl->PathInput) {
        impl->PathInput->Vtbl->Render(impl->PathInput, Screen,
                                       X + 8, Y + 2, FALSE);
    }

    /* Draw file list label */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 4, "Files:",
                            TuiColorBlack, TuiColorWhite);

    /* Render file list */
    if (impl->FileList) {
        impl->FileList->Vtbl->Render(impl->FileList, Screen,
                                      X + 2, Y + 5, Width - 4, Height - 12);
    }

    /* Draw filename label (for save mode) */
    if (impl->Type == FileDialogSave) {
        Screen->Vtbl->WriteText(Screen, X + 2, Y + Height - 5, "File name:",
                                TuiColorBlack, TuiColorWhite);

        /* Render filename input */
        if (impl->FileNameInput) {
            impl->FileNameInput->Vtbl->Render(impl->FileNameInput, Screen,
                                               X + 13, Y + Height - 5, FALSE);
        }
    }

    /* Render buttons */
    if (impl->OkButton) {
        impl->OkButton->Vtbl->Render(impl->OkButton, Screen,
                                      X + Width - 24, Y + Height - 2, FALSE);
    }

    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen,
                                          X + Width - 12, Y + Height - 2, FALSE);
    }

    return S_OK;
}

static HRESULT ANXAPI FileDialog_HandleKey(
    ITuiFileDialog *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiFileDialogImpl *impl = (TuiFileDialogImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Route to active widget */
    if (impl->FileList) {
        impl->FileList->Vtbl->HandleKey(impl->FileList, Key, Handled);
        if (*Handled) return S_OK;
    }

    /* Handle dialog-level keys */
    if (Key == TuiKeyEsc) {
        impl->DialogResult = FALSE;
        impl->State.Visible = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    if (Key == TuiKeyEnter) {
        /* Get selected file */
        INT32 selectedIndex;
        impl->FileList->Vtbl->GetSelectedIndex(impl->FileList, &selectedIndex);

        if (selectedIndex >= 0 && (UINT32)selectedIndex < impl->FileCount) {
            if (impl->Files[selectedIndex].IsDirectory) {
                /* Navigate into directory */
                /* Would update CurrentPath and rescan */
            } else {
                /* Select file */
                snprintf(impl->SelectedFile, sizeof(impl->SelectedFile),
                         "%s/%s", impl->CurrentPath,
                         impl->Files[selectedIndex].Name);
                impl->DialogResult = TRUE;
                impl->State.Visible = FALSE;
            }
        }

        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiFileDialog_Vtbl FileDialogVtbl = {
    FileDialog_QueryInterface,
    FileDialog_AddRef,
    FileDialog_Release,
    FileDialog_SetType,
    FileDialog_SetTitle,
    FileDialog_SetInitialPath,
    FileDialog_SetFilter,
    FileDialog_Show,
    FileDialog_GetResult,
    FileDialog_GetSelectedPath,
    FileDialog_Render,
    FileDialog_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateFileDialog(
    IN  UINT32 Type,
    OUT ITuiFileDialog **FileDialog
)
{
    TuiFileDialogImpl *impl;
    HRESULT hr;

    if (FileDialog == NULL) return E_POINTER;

    impl = (TuiFileDialogImpl *)calloc(1, sizeof(TuiFileDialogImpl));
    if (impl == NULL) {
        *FileDialog = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &FileDialogVtbl;
    InitWidgetState(&impl->State);

    impl->Type = (FileDialogType)Type;
    strcpy(impl->CurrentPath, ".");
    impl->SelectedFile[0] = '\0';
    impl->Filter[0] = '\0';
    impl->FileCount = 0;
    impl->SelectedIndex = -1;
    impl->DialogResult = FALSE;

    /* Set initial title */
    FileDialog_SetType(&impl->Interface, Type);

    /* Create child widgets */
    hr = AnxTuiCreateListBox(15, &impl->FileList);
    if (FAILED(hr)) {
        free(impl);
        *FileDialog = NULL;
        return hr;
    }

    hr = AnxTuiCreateInput(NULL, &impl->PathInput);
    if (FAILED(hr)) {
        impl->FileList->Vtbl->Release(impl->FileList);
        free(impl);
        *FileDialog = NULL;
        return hr;
    }
    impl->PathInput->Vtbl->SetWidth(impl->PathInput, 50);

    hr = AnxTuiCreateInput(NULL, &impl->FileNameInput);
    if (FAILED(hr)) {
        impl->FileList->Vtbl->Release(impl->FileList);
        impl->PathInput->Vtbl->Release(impl->PathInput);
        free(impl);
        *FileDialog = NULL;
        return hr;
    }
    impl->FileNameInput->Vtbl->SetWidth(impl->FileNameInput, 40);

    hr = AnxTuiCreateButton(&impl->OkButton);
    if (FAILED(hr)) {
        impl->FileList->Vtbl->Release(impl->FileList);
        impl->PathInput->Vtbl->Release(impl->PathInput);
        impl->FileNameInput->Vtbl->Release(impl->FileNameInput);
        free(impl);
        *FileDialog = NULL;
        return hr;
    }
    impl->OkButton->Vtbl->SetLabel(impl->OkButton, "  OK  ");

    hr = AnxTuiCreateButton(&impl->CancelButton);
    if (FAILED(hr)) {
        impl->FileList->Vtbl->Release(impl->FileList);
        impl->PathInput->Vtbl->Release(impl->PathInput);
        impl->FileNameInput->Vtbl->Release(impl->FileNameInput);
        impl->OkButton->Vtbl->Release(impl->OkButton);
        free(impl);
        *FileDialog = NULL;
        return hr;
    }
    impl->CancelButton->Vtbl->SetLabel(impl->CancelButton, "Cancel");

    *FileDialog = &impl->Interface;
    return S_OK;
}
