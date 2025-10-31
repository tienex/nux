/*
 * dialog_directory.c - Directory Chooser Dialog
 *
 * Modal dialog for selecting directories with tree navigation.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define getcwd _getcwd
#define chdir _chdir
#else
#include <dirent.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#endif

#define MAX_PATH_LENGTH 1024
#define MAX_ENTRIES 256
#define DIALOG_WIDTH 70
#define DIALOG_HEIGHT 25
#define LIST_HEIGHT 18

typedef struct {
    CHAR8 Name[256];
    BOOLEAN IsDirectory;
} DirectoryEntry;

typedef struct {
    ITuiDirectoryDialog Interface;
    WIDGET_STATE State;

    CHAR8 Title[128];
    CHAR8 CurrentPath[MAX_PATH_LENGTH];
    CHAR8 SelectedPath[MAX_PATH_LENGTH];

    /* Directory entries */
    DirectoryEntry Entries[MAX_ENTRIES];
    UINT32 EntryCount;
    INT32 SelectedIndex;
    INT32 ScrollOffset;

    /* Dialog result */
    BOOLEAN Accepted;
    BOOLEAN Cancelled;

    /* Sub-widgets */
    ITuiButton *OkButton;
    ITuiButton *CancelButton;
    ITuiButton *NewFolderButton;
} TuiDirectoryDialogImpl;

/* Helper: Compare entries for sorting (directories first, then alphabetical) */
static int CompareEntries(const void *a, const void *b)
{
    const DirectoryEntry *ea = (const DirectoryEntry *)a;
    const DirectoryEntry *eb = (const DirectoryEntry *)b;

    /* Directories come before files */
    if (ea->IsDirectory && !eb->IsDirectory) return -1;
    if (!ea->IsDirectory && eb->IsDirectory) return 1;

    /* Within same type, sort alphabetically (case-insensitive) */
    return strcasecmp(ea->Name, eb->Name);
}

/* Helper: Load directory entries */
static HRESULT LoadDirectoryEntries(TuiDirectoryDialogImpl *impl)
{
    impl->EntryCount = 0;
    impl->SelectedIndex = 0;
    impl->ScrollOffset = 0;

#ifdef _WIN32
    /* Windows implementation using FindFirstFile/FindNextFile */
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    CHAR8 searchPath[MAX_PATH_LENGTH];

    snprintf(searchPath, sizeof(searchPath), "%s\\*", impl->CurrentPath);

    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }

    do {
        if (impl->EntryCount >= MAX_ENTRIES) break;

        /* Skip current directory "." */
        if (strcmp(findData.cFileName, ".") == 0) continue;

        DirectoryEntry *entry = &impl->Entries[impl->EntryCount++];
        strncpy(entry->Name, findData.cFileName, sizeof(entry->Name) - 1);
        entry->Name[sizeof(entry->Name) - 1] = '\0';
        entry->IsDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

#else
    /* Unix/Linux implementation using opendir/readdir */
    DIR *dir = opendir(impl->CurrentPath);
    if (!dir) {
        return E_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && impl->EntryCount < MAX_ENTRIES) {
        /* Skip current directory "." */
        if (strcmp(entry->d_name, ".") == 0) continue;

        DirectoryEntry *dirEntry = &impl->Entries[impl->EntryCount++];
        strncpy(dirEntry->Name, entry->d_name, sizeof(dirEntry->Name) - 1);
        dirEntry->Name[sizeof(dirEntry->Name) - 1] = '\0';

        /* Check if it's a directory */
        CHAR8 fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s%c%s",
                 impl->CurrentPath, PATH_SEPARATOR, entry->d_name);

        struct stat st;
        if (stat(fullPath, &st) == 0) {
            dirEntry->IsDirectory = S_ISDIR(st.st_mode);
        } else {
            dirEntry->IsDirectory = FALSE;
        }
    }

    closedir(dir);
#endif

    /* Sort entries (directories first, then alphabetically) */
    if (impl->EntryCount > 0) {
        qsort(impl->Entries, impl->EntryCount, sizeof(DirectoryEntry), CompareEntries);
    }

    return S_OK;
}

/* Helper: Navigate to parent directory */
static HRESULT NavigateToParent(TuiDirectoryDialogImpl *impl)
{
    CHAR8 *lastSep = strrchr(impl->CurrentPath, PATH_SEPARATOR);
    if (lastSep && lastSep != impl->CurrentPath) {
        *lastSep = '\0';

        /* On Windows, if we're at drive root (e.g., "C:"), keep the backslash */
#ifdef _WIN32
        if (lastSep == impl->CurrentPath + 2 && impl->CurrentPath[1] == ':') {
            impl->CurrentPath[2] = '\\';
            impl->CurrentPath[3] = '\0';
        }
#endif
    }
#ifdef _WIN32
    /* On Windows, if we're at root (e.g., "C:\"), can't go further up */
    else if (strlen(impl->CurrentPath) == 3 && impl->CurrentPath[1] == ':') {
        return S_FALSE;  /* Already at root */
    }
#else
    /* On Unix, if we're at root "/", can't go further up */
    else if (strcmp(impl->CurrentPath, "/") == 0) {
        return S_FALSE;
    }
#endif

    return LoadDirectoryEntries(impl);
}

/* Helper: Navigate into directory */
static HRESULT NavigateInto(TuiDirectoryDialogImpl *impl, CONST CHAR8 *DirName)
{
    UINTN currentLen = strlen(impl->CurrentPath);
    UINTN nameLen = strlen(DirName);

    /* Check if path would be too long */
    if (currentLen + nameLen + 2 > MAX_PATH_LENGTH) {
        return E_FAIL;
    }

    /* Append directory separator if needed */
    if (impl->CurrentPath[currentLen - 1] != PATH_SEPARATOR) {
        impl->CurrentPath[currentLen++] = PATH_SEPARATOR;
        impl->CurrentPath[currentLen] = '\0';
    }

    /* Append directory name */
    strcat(impl->CurrentPath, DirName);

    return LoadDirectoryEntries(impl);
}

/* IUnknown methods */
static HRESULT ANXAPI DirectoryDialog_QueryInterface(
    ITuiDirectoryDialog *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiDirectoryDialog)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI DirectoryDialog_AddRef(ITuiDirectoryDialog *This)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI DirectoryDialog_Release(ITuiDirectoryDialog *This)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        if (impl->OkButton) {
            impl->OkButton->Vtbl->Release(impl->OkButton);
        }
        if (impl->CancelButton) {
            impl->CancelButton->Vtbl->Release(impl->CancelButton);
        }
        if (impl->NewFolderButton) {
            impl->NewFolderButton->Vtbl->Release(impl->NewFolderButton);
        }
        free(impl);
    }

    return count;
}

/* Render the dialog */
static HRESULT ANXAPI DirectoryDialog_Render(
    ITuiDirectoryDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    INT32 i, j;
    CHAR8 line[256];

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow */
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        for (j = 0; j < DIALOG_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j + 2, Y + i + 1, ' ', TuiColorBlack, TuiColorBlack);
        }
    }

    /* Draw dialog background */
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        for (j = 0; j < DIALOG_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Draw double-line border */
    /* Top */
    for (j = 1; j < DIALOG_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y, 0x2550, TuiColorBlack, TuiColorWhite);
    }
    /* Bottom */
    for (j = 1; j < DIALOG_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + DIALOG_HEIGHT - 1, 0x2550, TuiColorBlack, TuiColorWhite);
    }
    /* Left and right */
    for (i = 1; i < DIALOG_HEIGHT - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X, Y + i, 0x2551, TuiColorBlack, TuiColorWhite);
        Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y + i, 0x2551, TuiColorBlack, TuiColorWhite);
    }
    /* Corners */
    Screen->Vtbl->WriteChar(Screen, X, Y, 0x2554, TuiColorBlack, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y, 0x2557, TuiColorBlack, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X, Y + DIALOG_HEIGHT - 1, 0x255A, TuiColorBlack, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y + DIALOG_HEIGHT - 1, 0x255D, TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    INT32 titleLen = strlen(impl->Title);
    INT32 titleX = X + (DIALOG_WIDTH - titleLen) / 2;
    Screen->Vtbl->WriteText(Screen, titleX, Y + 1, impl->Title, TuiColorWhite, TuiColorBlue);
    for (j = 2; j < DIALOG_WIDTH - 2; j++) {
        if (j < titleX - X || j >= titleX - X + titleLen) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + 1, ' ', TuiColorWhite, TuiColorBlue);
        }
    }

    /* Draw current path */
    snprintf(line, sizeof(line), "Path: %s", impl->CurrentPath);
    if (strlen(line) > DIALOG_WIDTH - 4) {
        /* Truncate from the left */
        snprintf(line, sizeof(line), "Path: ...%s",
                 impl->CurrentPath + strlen(impl->CurrentPath) - (DIALOG_WIDTH - 12));
    }
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 3, line, TuiColorBlack, TuiColorWhite);

    /* Draw directory list header */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 5, "Directories:", TuiColorBlack, TuiColorWhite);

    /* Draw directory list box */
    for (i = 0; i < LIST_HEIGHT; i++) {
        INT32 entryIndex = impl->ScrollOffset + i;
        BOOLEAN isSelected = (entryIndex == impl->SelectedIndex);
        TUI_COLOR fg = isSelected ? TuiColorBlack : TuiColorBrightBlack;
        TUI_COLOR bg = isSelected ? TuiColorCyan : TuiColorWhite;

        if (entryIndex < impl->EntryCount) {
            DirectoryEntry *entry = &impl->Entries[entryIndex];
            CHAR8 prefix = entry->IsDirectory ? (strcmp(entry->Name, "..") == 0 ? '<' : '+') : ' ';

            snprintf(line, sizeof(line), " %c %-60s", prefix, entry->Name);
            if (strlen(line) > DIALOG_WIDTH - 6) {
                line[DIALOG_WIDTH - 6] = '\0';
            }
        } else {
            memset(line, ' ', DIALOG_WIDTH - 6);
            line[DIALOG_WIDTH - 6] = '\0';
        }

        Screen->Vtbl->WriteText(Screen, X + 3, Y + 6 + i, line, fg, bg);
    }

    /* Draw help text */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + DIALOG_HEIGHT - 3,
        "Use Up/Down to navigate, Enter to select, Esc to cancel",
        TuiColorBrightBlack, TuiColorWhite);

    /* Draw buttons */
    if (impl->OkButton) {
        impl->OkButton->Vtbl->Render(impl->OkButton, Screen,
            X + DIALOG_WIDTH - 34, Y + DIALOG_HEIGHT - 2, FALSE);
    }
    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen,
            X + DIALOG_WIDTH - 20, Y + DIALOG_HEIGHT - 2, FALSE);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI DirectoryDialog_HandleKey(
    ITuiDirectoryDialog *This,
    TUI_KEY Key
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;

    if (Key == TuiKeyEscape) {
        impl->Cancelled = TRUE;
        impl->State.Visible = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyUp) {
        if (impl->SelectedIndex > 0) {
            impl->SelectedIndex--;
            if (impl->SelectedIndex < impl->ScrollOffset) {
                impl->ScrollOffset = impl->SelectedIndex;
            }
        }
        return S_OK;
    }

    if (Key == TuiKeyDown) {
        if (impl->SelectedIndex < impl->EntryCount - 1) {
            impl->SelectedIndex++;
            if (impl->SelectedIndex >= impl->ScrollOffset + LIST_HEIGHT) {
                impl->ScrollOffset = impl->SelectedIndex - LIST_HEIGHT + 1;
            }
        }
        return S_OK;
    }

    if (Key == TuiKeyPageUp) {
        impl->SelectedIndex -= LIST_HEIGHT;
        if (impl->SelectedIndex < 0) impl->SelectedIndex = 0;
        impl->ScrollOffset = impl->SelectedIndex;
        return S_OK;
    }

    if (Key == TuiKeyPageDown) {
        impl->SelectedIndex += LIST_HEIGHT;
        if (impl->SelectedIndex >= impl->EntryCount) {
            impl->SelectedIndex = impl->EntryCount - 1;
        }
        if (impl->SelectedIndex >= impl->ScrollOffset + LIST_HEIGHT) {
            impl->ScrollOffset = impl->SelectedIndex - LIST_HEIGHT + 1;
        }
        return S_OK;
    }

    if (Key == TuiKeyEnter) {
        if (impl->SelectedIndex >= 0 && impl->SelectedIndex < impl->EntryCount) {
            DirectoryEntry *entry = &impl->Entries[impl->SelectedIndex];

            if (entry->IsDirectory) {
                if (strcmp(entry->Name, "..") == 0) {
                    NavigateToParent(impl);
                } else {
                    NavigateInto(impl, entry->Name);
                }
            }
        }
        return S_OK;
    }

    /* O key - accept current directory */
    if (Key == 'o' || Key == 'O') {
        strncpy(impl->SelectedPath, impl->CurrentPath, sizeof(impl->SelectedPath) - 1);
        impl->SelectedPath[sizeof(impl->SelectedPath) - 1] = '\0';
        impl->Accepted = TRUE;
        impl->State.Visible = FALSE;
        return S_OK;
    }

    return S_OK;
}

/* Set initial directory */
static HRESULT ANXAPI DirectoryDialog_SetInitialDirectory(
    ITuiDirectoryDialog *This,
    CONST CHAR8 *Path
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;

    if (Path && *Path) {
        strncpy(impl->CurrentPath, Path, sizeof(impl->CurrentPath) - 1);
        impl->CurrentPath[sizeof(impl->CurrentPath) - 1] = '\0';
    } else {
        /* Use current working directory */
        if (!getcwd(impl->CurrentPath, sizeof(impl->CurrentPath))) {
            strcpy(impl->CurrentPath, PATH_SEPARATOR == '\\' ? "C:\\" : "/");
        }
    }

    return LoadDirectoryEntries(impl);
}

/* Get selected directory */
static HRESULT ANXAPI DirectoryDialog_GetSelectedDirectory(
    ITuiDirectoryDialog *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;

    if (!Buffer || BufferSize == 0) return E_INVALIDARG;

    strncpy(Buffer, impl->SelectedPath, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

/* Show dialog (modal) */
static HRESULT ANXAPI DirectoryDialog_Show(
    ITuiDirectoryDialog *This,
    ITuiScreen *Screen,
    BOOLEAN *Result
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;

    impl->Accepted = FALSE;
    impl->Cancelled = FALSE;
    impl->State.Visible = TRUE;

    /* Modal loop - in real implementation, this would be handled by the main event loop */
    /* For now, just return S_OK and let the caller handle the modal behavior */

    if (Result) {
        *Result = impl->Accepted;
    }

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI DirectoryDialog_SetBounds(
    ITuiDirectoryDialog *This,
    CONST TUI_RECT *Bounds
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI DirectoryDialog_GetBounds(
    ITuiDirectoryDialog *This,
    TUI_RECT *Bounds
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI DirectoryDialog_SetVisible(
    ITuiDirectoryDialog *This,
    BOOLEAN Visible
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI DirectoryDialog_IsVisible(ITuiDirectoryDialog *This)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI DirectoryDialog_SetEnabled(
    ITuiDirectoryDialog *This,
    BOOLEAN Enabled
)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI DirectoryDialog_IsEnabled(ITuiDirectoryDialog *This)
{
    TuiDirectoryDialogImpl *impl = (TuiDirectoryDialogImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiDirectoryDialogVtbl DirectoryDialogVtbl = {
    DirectoryDialog_QueryInterface,
    DirectoryDialog_AddRef,
    DirectoryDialog_Release,
    DirectoryDialog_Render,
    DirectoryDialog_HandleKey,
    DirectoryDialog_SetBounds,
    DirectoryDialog_GetBounds,
    DirectoryDialog_SetVisible,
    DirectoryDialog_IsVisible,
    DirectoryDialog_SetEnabled,
    DirectoryDialog_IsEnabled,
    DirectoryDialog_SetInitialDirectory,
    DirectoryDialog_GetSelectedDirectory,
    DirectoryDialog_Show
};

/* Factory function */
HRESULT AnxTuiCreateDirectoryDialog(
    CONST CHAR8 *Title,
    ITuiDirectoryDialog **OutDialog
)
{
    TuiDirectoryDialogImpl *impl;
    HRESULT hr;

    if (!OutDialog) return E_INVALIDARG;

    impl = (TuiDirectoryDialogImpl *)malloc(sizeof(TuiDirectoryDialogImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiDirectoryDialogImpl));
    impl->Interface.Vtbl = &DirectoryDialogVtbl;
    InitWidgetState(&impl->State);

    strncpy(impl->Title, Title ? Title : "Select Directory", sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';

    impl->SelectedIndex = 0;
    impl->ScrollOffset = 0;

    /* Get current working directory */
    if (!getcwd(impl->CurrentPath, sizeof(impl->CurrentPath))) {
        strcpy(impl->CurrentPath, PATH_SEPARATOR == '\\' ? "C:\\" : "/");
    }

    /* Load initial directory */
    LoadDirectoryEntries(impl);

    /* Create buttons */
    hr = AnxTuiCreateButton("OK", NULL, NULL, &impl->OkButton);
    if (FAILED(hr)) {
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Cancel", NULL, NULL, &impl->CancelButton);
    if (FAILED(hr)) {
        impl->OkButton->Vtbl->Release(impl->OkButton);
        free(impl);
        return hr;
    }

    impl->State.Bounds.Width = DIALOG_WIDTH;
    impl->State.Bounds.Height = DIALOG_HEIGHT;

    *OutDialog = &impl->Interface;
    return S_OK;
}
