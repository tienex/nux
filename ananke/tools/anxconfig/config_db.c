/*
 * Configuration Database Implementation (Stub)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/anxconfig.h>

/* Configuration database structure */
typedef struct {
    IConfigDatabase Interface;
    UINTN RefCount;
    IConfigItem *RootItem;
    UINTN ItemCount;
} ConfigDatabase;

/* IUnknown methods */
static HRESULT ANXAPI ConfigDb_QueryInterface(
    IConfigDatabase *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IConfigDatabase)) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ConfigDb_AddRef(IConfigDatabase *This)
{
    ConfigDatabase *db = (ConfigDatabase *)This;
    return ++db->RefCount;
}

static UINTN ANXAPI ConfigDb_Release(IConfigDatabase *This)
{
    ConfigDatabase *db = (ConfigDatabase *)This;
    UINTN refCount = --db->RefCount;

    if (refCount == 0) {
        if (db->RootItem) {
            db->RootItem->Vtbl->Release(db->RootItem);
        }
        free(db);
    }

    return refCount;
}

/* IConfigDatabase methods */
static HRESULT ANXAPI ConfigDb_LoadFromFile(
    IConfigDatabase *This,
    CONST CHAR8 *FilePath
)
{
    /* Stub: Would parse YAML file here */
    printf("[ConfigDb] LoadFromFile: %s (stub)\n", FilePath);
    return S_OK;
}

static HRESULT ANXAPI ConfigDb_LoadValues(
    IConfigDatabase *This,
    CONST CHAR8 *FilePath
)
{
    /* Stub: Would load .config file here */
    printf("[ConfigDb] LoadValues: %s (stub)\n", FilePath);
    return S_OK;
}

static HRESULT ANXAPI ConfigDb_SaveValues(
    IConfigDatabase *This,
    CONST CHAR8 *FilePath
)
{
    /* Stub: Would save .config file here */
    printf("[ConfigDb] SaveValues: %s (stub)\n", FilePath);

    FILE *f = fopen(FilePath, "w");
    if (!f) {
        return E_FAIL;
    }

    fprintf(f, "# ANXCONFIG generated configuration\n");
    fprintf(f, "# Placeholder configuration file\n");
    fprintf(f, "\n");
    fprintf(f, "CONFIG_EXAMPLE=y\n");

    fclose(f);
    return S_OK;
}

static HRESULT ANXAPI ConfigDb_GetRootItem(
    IConfigDatabase *This,
    IConfigItem **RootItem
)
{
    ConfigDatabase *db = (ConfigDatabase *)This;

    if (RootItem == NULL) {
        return E_POINTER;
    }

    if (db->RootItem) {
        *RootItem = db->RootItem;
        db->RootItem->Vtbl->AddRef(db->RootItem);
        return S_OK;
    }

    *RootItem = NULL;
    return E_FAIL;
}

static HRESULT ANXAPI ConfigDb_FindItem(
    IConfigDatabase *This,
    CONST CHAR8 *Name,
    IConfigItem **Item
)
{
    /* Stub */
    if (Item) {
        *Item = NULL;
    }
    return E_NOTIMPL;
}

static HRESULT ANXAPI ConfigDb_EvaluateDependencies(
    IConfigDatabase *This
)
{
    /* Stub */
    return S_OK;
}

static HRESULT ANXAPI ConfigDb_GetItemCount(
    IConfigDatabase *This,
    UINTN *Count
)
{
    ConfigDatabase *db = (ConfigDatabase *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = db->ItemCount;
    return S_OK;
}

/* Vtable */
static CONST IConfigDatabase_Vtbl ConfigDbVtbl = {
    ConfigDb_QueryInterface,
    ConfigDb_AddRef,
    ConfigDb_Release,
    ConfigDb_LoadFromFile,
    ConfigDb_LoadValues,
    ConfigDb_SaveValues,
    ConfigDb_GetRootItem,
    ConfigDb_FindItem,
    ConfigDb_EvaluateDependencies,
    ConfigDb_GetItemCount
};

/* Factory function */
HRESULT ANXAPI AnxConfigCreateDatabase(OUT IConfigDatabase **Database)
{
    ConfigDatabase *db;

    if (Database == NULL) {
        return E_POINTER;
    }

    db = (ConfigDatabase *)calloc(1, sizeof(ConfigDatabase));
    if (db == NULL) {
        *Database = NULL;
        return E_OUTOFMEMORY;
    }

    db->Interface.Vtbl = &ConfigDbVtbl;
    db->RefCount = 1;
    db->RootItem = NULL;
    db->ItemCount = 0;

    *Database = &db->Interface;
    return S_OK;
}
