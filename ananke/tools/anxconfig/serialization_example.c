/*
 * serialization_example.c - Demonstration of IAmxSerializer Usage
 *
 * Shows how to serialize and deserialize widgets using the universal
 * resource system with YAML.
 */

#include <ananke/tui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example: Simple button object that supports serialization */
typedef struct {
    ITuiSerializable Interface;
    UINTN RefCount;
    CHAR8 Label[256];
    INT32 X, Y, Width, Height;
} SimpleButton;

/* SimpleButton IUnknown methods */
static HRESULT ANXAPI SimpleButton_QueryInterface(
    ITuiSerializable *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ITuiSerializable)) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI SimpleButton_AddRef(ITuiSerializable *This)
{
    SimpleButton *impl = (SimpleButton *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI SimpleButton_Release(ITuiSerializable *This)
{
    SimpleButton *impl = (SimpleButton *)This;
    UINTN count = --impl->RefCount;
    if (count == 0) {
        free(impl);
    }
    return count;
}

/* Serialize to YAML */
static HRESULT ANXAPI SimpleButton_SerializeToYaml(
    ITuiSerializable *This,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    SimpleButton *impl = (SimpleButton *)This;
    CHAR8 *yaml = NULL;
    UINTN length = 0;

    /* Generate YAML */
    yaml = (CHAR8 *)malloc(1024);
    if (!yaml) return E_OUTOFMEMORY;

    length = snprintf(yaml, 1024,
        "type: SimpleButton\n"
        "label: \"%s\"\n"
        "position:\n"
        "  x: %d\n"
        "  y: %d\n"
        "size:\n"
        "  width: %d\n"
        "  height: %d\n",
        impl->Label, impl->X, impl->Y, impl->Width, impl->Height);

    *OutYaml = yaml;
    *OutLength = length;

    return S_OK;
}

/* Deserialize from YAML */
static HRESULT ANXAPI SimpleButton_DeserializeFromYaml(
    ITuiSerializable *This,
    CONST CHAR8 *Yaml,
    UINTN Length
)
{
    SimpleButton *impl = (SimpleButton *)This;

    /* Parse YAML (simplified) */
    CONST CHAR8 *ptr = NULL;

    /* Parse label */
    ptr = strstr(Yaml, "label:");
    if (ptr) {
        ptr += 6;
        while (*ptr == ' ' || *ptr == '"') ptr++;
        INT32 i = 0;
        while (*ptr && *ptr != '"' && *ptr != '\n' && i < sizeof(impl->Label) - 1) {
            impl->Label[i++] = *ptr++;
        }
        impl->Label[i] = '\0';
    }

    /* Parse position */
    ptr = strstr(Yaml, "x:");
    if (ptr) {
        sscanf(ptr, "x: %d", &impl->X);
    }
    ptr = strstr(Yaml, "y:");
    if (ptr) {
        sscanf(ptr, "y: %d", &impl->Y);
    }

    /* Parse size */
    ptr = strstr(Yaml, "width:");
    if (ptr) {
        sscanf(ptr, "width: %d", &impl->Width);
    }
    ptr = strstr(Yaml, "height:");
    if (ptr) {
        sscanf(ptr, "height: %d", &impl->Height);
    }

    return S_OK;
}

/* Get type name */
static HRESULT ANXAPI SimpleButton_GetTypeName(
    ITuiSerializable *This,
    CONST CHAR8 **OutTypeName
)
{
    *OutTypeName = "SimpleButton";
    return S_OK;
}

/* Clone */
static HRESULT ANXAPI SimpleButton_Clone(
    ITuiSerializable *This,
    ITuiSerializable **OutClone
)
{
    SimpleButton *impl = (SimpleButton *)This;
    SimpleButton *clone = NULL;

    clone = (SimpleButton *)calloc(1, sizeof(SimpleButton));
    if (!clone) return E_OUTOFMEMORY;

    memcpy(clone, impl, sizeof(SimpleButton));
    clone->RefCount = 1;

    *OutClone = (ITuiSerializable *)clone;
    return S_OK;
}

/* VTable */
static ITuiSerializable_Vtbl SimpleButtonVtbl = {
    SimpleButton_QueryInterface,
    SimpleButton_AddRef,
    SimpleButton_Release,
    SimpleButton_SerializeToYaml,
    SimpleButton_DeserializeFromYaml,
    SimpleButton_GetTypeName,
    SimpleButton_Clone
};

/* Factory function for SimpleButton */
static HRESULT CreateSimpleButton(ITuiSerializable **OutObject)
{
    SimpleButton *button = (SimpleButton *)calloc(1, sizeof(SimpleButton));
    if (!button) return E_OUTOFMEMORY;

    button->Interface.Vtbl = &SimpleButtonVtbl;
    button->RefCount = 1;
    strcpy(button->Label, "Button");
    button->X = 0;
    button->Y = 0;
    button->Width = 80;
    button->Height = 24;

    *OutObject = (ITuiSerializable *)button;
    return S_OK;
}

/* Demonstration function */
VOID DemonstrateSerializeDeserialize(VOID)
{
    HRESULT hr;
    IAmxSerializer *serializer = NULL;
    IAmxResourceManager *resourceManager = NULL;
    ITuiSerializable *button = NULL;
    ITuiSerializable *loadedButton = NULL;
    CHAR8 *yaml = NULL;
    UINTN length = 0;

    printf("=== YAML Serialization Demonstration ===\n\n");

    /* Create serializer */
    hr = AnxAmxCreateSerializer(&serializer);
    if (FAILED(hr)) {
        printf("Failed to create serializer: 0x%08X\n", hr);
        return;
    }

    /* Register type factory */
    hr = serializer->Vtbl->RegisterTypeFactory(serializer, "SimpleButton", CreateSimpleButton);
    if (FAILED(hr)) {
        printf("Failed to register type factory: 0x%08X\n", hr);
        goto cleanup;
    }

    /* Create a button */
    hr = CreateSimpleButton(&button);
    if (FAILED(hr)) {
        printf("Failed to create button: 0x%08X\n", hr);
        goto cleanup;
    }

    /* Set properties */
    SimpleButton *btnImpl = (SimpleButton *)button;
    strcpy(btnImpl->Label, "Click Me!");
    btnImpl->X = 10;
    btnImpl->Y = 20;
    btnImpl->Width = 100;
    btnImpl->Height = 30;

    printf("Original Button:\n");
    printf("  Label: %s\n", btnImpl->Label);
    printf("  Position: (%d, %d)\n", btnImpl->X, btnImpl->Y);
    printf("  Size: %dx%d\n\n", btnImpl->Width, btnImpl->Height);

    /* Serialize to YAML */
    hr = serializer->Vtbl->Serialize(serializer, button, &yaml, &length);
    if (FAILED(hr)) {
        printf("Failed to serialize: 0x%08X\n", hr);
        goto cleanup;
    }

    printf("Serialized YAML:\n");
    printf("---\n%s---\n\n", yaml);

    /* Deserialize back */
    hr = serializer->Vtbl->Deserialize(serializer, yaml, length, &loadedButton);
    if (FAILED(hr)) {
        printf("Failed to deserialize: 0x%08X\n", hr);
        goto cleanup;
    }

    /* Verify */
    SimpleButton *loadedImpl = (SimpleButton *)loadedButton;
    printf("Deserialized Button:\n");
    printf("  Label: %s\n", loadedImpl->Label);
    printf("  Position: (%d, %d)\n", loadedImpl->X, loadedImpl->Y);
    printf("  Size: %dx%d\n\n", loadedImpl->Width, loadedImpl->Height);

    /* Test file I/O */
    printf("Testing file I/O...\n");
    hr = serializer->Vtbl->SerializeToFile(serializer, button, "/tmp/button_test.yaml");
    if (SUCCEEDED(hr)) {
        printf("  Saved to /tmp/button_test.yaml\n");

        ITuiSerializable *fileButton = NULL;
        hr = serializer->Vtbl->DeserializeFromFile(serializer, "/tmp/button_test.yaml", &fileButton);
        if (SUCCEEDED(hr)) {
            printf("  Loaded from /tmp/button_test.yaml\n");
            SimpleButton *fileImpl = (SimpleButton *)fileButton;
            printf("  Label from file: %s\n", fileImpl->Label);
            fileButton->Vtbl->Release(fileButton);
        } else {
            printf("  Failed to load from file: 0x%08X\n", hr);
        }
    } else {
        printf("  Failed to save to file: 0x%08X\n", hr);
    }

    printf("\n=== Resource Manager Demonstration ===\n\n");

    /* Create resource manager */
    hr = AnxAmxCreateResourceManager(&resourceManager);
    if (FAILED(hr)) {
        printf("Failed to create resource manager: 0x%08X\n", hr);
        goto cleanup;
    }

    /* Get embedded serializer and register factory */
    IAmxSerializer *resMgrSerializer = NULL;
    hr = resourceManager->Vtbl->GetSerializer(resourceManager, &resMgrSerializer);
    if (SUCCEEDED(hr)) {
        resMgrSerializer->Vtbl->RegisterTypeFactory(resMgrSerializer, "SimpleButton", CreateSimpleButton);
        resMgrSerializer->Vtbl->Release(resMgrSerializer);
    }

    /* Register resources */
    IAmxResource *resource1 = NULL;
    hr = resourceManager->Vtbl->RegisterResource(resourceManager, "ui://buttons/main", button, &resource1);
    if (SUCCEEDED(hr)) {
        printf("Registered resource: ui://buttons/main\n");

        /* Set metadata */
        resource1->Vtbl->SetMetadata(resource1, "author", "ANXCONFIG Team");
        resource1->Vtbl->SetMetadata(resource1, "version", "1.0");

        /* Get metadata */
        CONST CHAR8 *author = NULL;
        if (SUCCEEDED(resource1->Vtbl->GetMetadata(resource1, "author", &author))) {
            printf("  Metadata - author: %s\n", author);
        }

        resource1->Vtbl->Release(resource1);
    }

    /* Create another button */
    ITuiSerializable *button2 = NULL;
    CreateSimpleButton(&button2);
    SimpleButton *btn2Impl = (SimpleButton *)button2;
    strcpy(btn2Impl->Label, "Cancel");
    btn2Impl->X = 120;
    btn2Impl->Y = 20;

    resourceManager->Vtbl->RegisterResource(resourceManager, "ui://buttons/cancel", button2, NULL);
    printf("Registered resource: ui://buttons/cancel\n\n");

    /* Save all resources to file */
    printf("Saving resources to /tmp/resources.yaml...\n");
    hr = resourceManager->Vtbl->SaveToFile(resourceManager, "/tmp/resources.yaml");
    if (SUCCEEDED(hr)) {
        printf("  Resources saved successfully!\n");
    } else {
        printf("  Failed to save resources: 0x%08X\n", hr);
    }

    /* Enumerate resources callback */
    static HRESULT EnumerateCallback(CONST CHAR8 *uri, IAmxResource *res, VOID *data) {
        CONST CHAR8 *type = NULL;
        res->Vtbl->GetType(res, &type);
        printf("  - %s (type: %s)\n", uri, type);
        return S_OK;
    }

    /* Enumerate resources */
    printf("\nEnumerating resources:\n");
    resourceManager->Vtbl->EnumerateResources(resourceManager, EnumerateCallback, NULL);

    button2->Vtbl->Release(button2);

cleanup:
    if (loadedButton) loadedButton->Vtbl->Release(loadedButton);
    if (yaml) free(yaml);
    if (button) button->Vtbl->Release(button);
    if (resourceManager) resourceManager->Vtbl->Release(resourceManager);
    if (serializer) serializer->Vtbl->Release(serializer);

    printf("\n=== Demonstration Complete ===\n");
}

/* Optional: Entry point for standalone test */
#ifdef TEST_SERIALIZATION
int main(void)
{
    DemonstrateSerializeDeserialize();
    return 0;
}
#endif
