/**
 * @file IOKit.h
 * @brief Master header for IOKit Driver Framework
 *
 * Include this header to access all IOKit framework interfaces.
 * This is the main entry point for driver development using IOKit.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_MASTER_H
#define IOKIT_MASTER_H

// Core framework
#include <iokit/iokit.h>

// Main interfaces
#include <iokit/ioservice.h>
#include <iokit/ioregistry.h>
#include <iokit/iouserclient.h>
#include <iokit/iomemorydescriptor.h>
#include <iokit/ioworkloop.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the IOKit framework
 *
 * @retval IO_SUCCESS       Framework initialized successfully
 * @retval IO_ERROR         Initialization failed
 */
IO_RETURN
STDAPICALLTYPE
IOKitInitialize(
    VOID
    );

/**
 * @brief Shutdown the IOKit framework
 *
 * @retval IO_SUCCESS       Framework shutdown successfully
 */
IO_RETURN
STDAPICALLTYPE
IOKitShutdown(
    VOID
    );

/**
 * @brief Get the global IORegistry instance
 *
 * @param ppRegistry    Receives pointer to global registry
 *
 * @retval IO_SUCCESS       Registry retrieved successfully
 * @retval IO_ERROR         Framework not initialized
 */
IO_RETURN
STDAPICALLTYPE
IOKitGetRegistry(
    IIORegistry **ppRegistry
    );

/**
 * @brief Register a driver service
 *
 * @param pService      Service to register
 * @param pParent       Parent service (NULL for root)
 *
 * @retval IO_SUCCESS       Service registered successfully
 * @retval IO_ERROR         Registration failed
 */
IO_RETURN
STDAPICALLTYPE
IOKitRegisterService(
    IIOService *pService,
    IIOService *pParent
    );

/**
 * @brief Dump the registry tree (debugging)
 *
 * @retval IO_SUCCESS       Registry dumped successfully
 * @retval IO_ERROR         Framework not initialized
 */
IO_RETURN
STDAPICALLTYPE
IOKitDumpRegistry(
    VOID
    );

/**
 * @brief Create a new IOService instance
 *
 * @param pszName       Service name
 * @param ppService     Receives pointer to new service
 *
 * @retval IO_SUCCESS   Service created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
IOServiceCreate(
    CONST CHAR8 *pszName,
    IIOService **ppService
    );

/**
 * @brief Add a child service
 *
 * @param pThis     Parent service
 * @param pChild    Child service to add
 *
 * @retval IO_SUCCESS       Child added successfully
 * @retval IO_NO_RESOURCES  Too many children
 */
IO_RETURN
IOServiceAddChild(
    IIOService *pThis,
    IIOService *pChild
    );

/**
 * @brief Create a new IORegistry instance
 *
 * @param ppRegistry    Receives pointer to new registry
 *
 * @retval IO_SUCCESS   Registry created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
IORegistryCreate(
    IIORegistry **ppRegistry
    );

/**
 * @brief Get the global registry instance
 *
 * @param ppRegistry    Receives pointer to global registry
 *
 * @retval IO_SUCCESS   Registry retrieved successfully
 * @retval IO_ERROR     Registry not initialized
 */
IO_RETURN
IORegistryGetGlobal(
    IIORegistry **ppRegistry
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_MASTER_H */
