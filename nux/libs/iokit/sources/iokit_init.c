/**
 * @file iokit_init.c
 * @brief IOKit framework initialization
 *
 * Provides initialization functions for the IOKit driver framework.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/ioservice.h>
#include <iokit/ioregistry.h>
#include <ananke/ntrtl.h>
#include <stdio.h>

/**
 * @brief Global registry instance
 */
static IIORegistry *g_pIORegistry = NULL;

/**
 * @brief Initialize the IOKit framework
 *
 * This function must be called during kernel initialization before
 * any drivers are loaded.
 *
 * @retval IO_SUCCESS       Framework initialized successfully
 * @retval IO_ERROR         Initialization failed
 */
IO_RETURN
STDAPICALLTYPE
IOKitInitialize(
    VOID
    )
{
    IO_RETURN Status;

    printf("IOKit: Initializing driver framework...\n");

    // Create global registry
    Status = IORegistryCreate(&g_pIORegistry);
    if (Status != IO_SUCCESS) {
        printf("IOKit: Failed to create registry (status=0x%08X)\n", Status);
        return Status;
    }

    printf("IOKit: Framework initialized successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Shutdown the IOKit framework
 *
 * This function should be called during kernel shutdown to cleanup
 * all driver framework resources.
 *
 * @retval IO_SUCCESS       Framework shutdown successfully
 */
IO_RETURN
STDAPICALLTYPE
IOKitShutdown(
    VOID
    )
{
    printf("IOKit: Shutting down driver framework...\n");

    if (g_pIORegistry != NULL) {
        IIORegistry_Release(g_pIORegistry);
        g_pIORegistry = NULL;
    }

    printf("IOKit: Framework shutdown complete\n");
    return IO_SUCCESS;
}

/**
 * @brief Get the global IORegistry instance
 *
 * @param ppRegistry    Receives pointer to global registry
 *
 * @retval IO_SUCCESS       Registry retrieved successfully
 * @retval IO_ERROR         Framework not initialized
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
STDAPICALLTYPE
IOKitGetRegistry(
    IIORegistry **ppRegistry
    )
{
    if (ppRegistry == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (g_pIORegistry == NULL) {
        return IO_ERROR;
    }

    *ppRegistry = g_pIORegistry;
    IIORegistry_AddRef(g_pIORegistry);
    return IO_SUCCESS;
}

/**
 * @brief Register a driver service with the framework
 *
 * This is a convenience function that registers a service with the
 * global registry and triggers matching.
 *
 * @param pService      Service to register
 * @param pParent       Parent service (NULL for root level)
 *
 * @retval IO_SUCCESS       Service registered successfully
 * @retval IO_ERROR         Registration failed
 */
IO_RETURN
STDAPICALLTYPE
IOKitRegisterService(
    IIOService *pService,
    IIOService *pParent
    )
{
    IO_RETURN Status;
    CHAR8 szName[64];

    if (pService == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (g_pIORegistry == NULL) {
        return IO_ERROR;
    }

    // Get service name for logging
    IIOService_GetServiceName(pService, szName, sizeof(szName));
    printf("IOKit: Registering service '%s'\n", szName);

    // Register with registry
    Status = IIORegistry_RegisterService(g_pIORegistry, pService, pParent);
    if (Status != IO_SUCCESS) {
        printf("IOKit: Failed to register service '%s' (status=0x%08X)\n", szName, Status);
        return Status;
    }

    // Mark service as registered
    Status = IIOService_RegisterService(pService, 0);
    if (Status != IO_SUCCESS) {
        printf("IOKit: Failed to update service state for '%s' (status=0x%08X)\n", szName, Status);
        return Status;
    }

    printf("IOKit: Service '%s' registered successfully\n", szName);
    return IO_SUCCESS;
}

/**
 * @brief Dump the registry tree
 *
 * This is a convenience function for debugging that dumps the entire
 * registry tree to the console.
 *
 * @retval IO_SUCCESS       Registry dumped successfully
 * @retval IO_ERROR         Framework not initialized
 */
IO_RETURN
STDAPICALLTYPE
IOKitDumpRegistry(
    VOID
    )
{
    if (g_pIORegistry == NULL) {
        return IO_ERROR;
    }

    return IIORegistry_DumpRegistry(g_pIORegistry, NULL, NULL, 0);
}
