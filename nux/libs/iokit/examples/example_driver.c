/**
 * @file example_driver.c
 * @brief Example IOKit driver implementation
 *
 * This file demonstrates how to implement a simple kernel-space driver
 * using the IOKit framework. The driver can be used as a template for
 * creating new drivers.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/ioservice.h>
#include <iokit/ioregistry.h>
#include <ananke/ntrtl.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example driver private data structure
 */
typedef struct _EXAMPLE_DRIVER {
    IIOService  *pService;          /**< Service interface */
    UINT32       uDeviceId;         /**< Device ID */
    BOOLEAN      bStarted;          /**< Driver started flag */
    UINT32       uOperationCount;   /**< Operation counter */
} EXAMPLE_DRIVER;

/**
 * @brief Custom probe function for example driver
 *
 * This function is called during driver matching to determine if this
 * driver can support the given provider.
 *
 * @param pThis         Service instance
 * @param pProvider     Provider service
 * @param puProbeScore  Receives probe score
 *
 * @retval IO_SUCCESS   Driver can support this device
 * @retval IO_NO_MATCH  Driver cannot support this device
 */
static IO_RETURN
ExampleDriver_Probe(
    IIOService *pThis,
    IIOService *pProvider,
    UINT32 *puProbeScore
    )
{
    CHAR8 szProviderName[64];
    UINT32 uDeviceClass;
    UINTN cbSize;

    printf("ExampleDriver: Probe called\n");

    // Get provider name
    IIOService_GetServiceName(pProvider, szProviderName, sizeof(szProviderName));
    printf("ExampleDriver: Provider is '%s'\n", szProviderName);

    // Check for "device-class" property
    cbSize = sizeof(uDeviceClass);
    if (IIOService_GetProperty(pProvider, "device-class", &uDeviceClass, &cbSize, NULL) == IO_SUCCESS) {
        printf("ExampleDriver: Device class = 0x%08X\n", uDeviceClass);

        // We support device class 0x12345678
        if (uDeviceClass == 0x12345678) {
            *puProbeScore = 1000;  // High priority
            printf("ExampleDriver: Probe successful (score=%u)\n", *puProbeScore);
            return IO_SUCCESS;
        }
    }

    printf("ExampleDriver: Probe failed - device not supported\n");
    return IO_NO_MATCH;
}

/**
 * @brief Start the example driver
 *
 * This function is called after successful probe to initialize the driver
 * and start servicing the device.
 *
 * @param pThis         Service instance
 * @param pProvider     Provider service
 *
 * @retval IO_SUCCESS   Driver started successfully
 * @retval IO_ERROR     Failed to start driver
 */
static IO_RETURN
ExampleDriver_Start(
    IIOService *pThis,
    IIOService *pProvider
    )
{
    EXAMPLE_DRIVER *pDriver;
    UINT32 uDeviceId;
    UINTN cbSize;

    printf("ExampleDriver: Start called\n");

    // Allocate private data
    pDriver = (EXAMPLE_DRIVER *)malloc(sizeof(EXAMPLE_DRIVER));
    if (pDriver == NULL) {
        printf("ExampleDriver: Failed to allocate private data\n");
        return IO_NO_MEMORY;
    }

    memset(pDriver, 0, sizeof(EXAMPLE_DRIVER));
    pDriver->pService = pThis;

    // Get device ID from provider
    cbSize = sizeof(uDeviceId);
    if (IIOService_GetProperty(pProvider, "device-id", &uDeviceId, &cbSize, NULL) == IO_SUCCESS) {
        pDriver->uDeviceId = uDeviceId;
        printf("ExampleDriver: Device ID = 0x%08X\n", pDriver->uDeviceId);
    }

    // Store private data in service properties
    IIOService_SetProperty(pThis, "driver-private", &pDriver, sizeof(VOID *), IO_PROPERTY_TYPE_DATA);

    // Mark as started
    pDriver->bStarted = TRUE;

    // Set some driver properties
    IIOService_SetProperty(pThis, "driver-version", "1.0.0", 6, IO_PROPERTY_TYPE_STRING);

    UINT32 uCapabilities = 0x00000001 | 0x00000002;  // Support feature A and B
    IIOService_SetProperty(pThis, "capabilities", &uCapabilities, sizeof(uCapabilities), IO_PROPERTY_TYPE_NUMBER);

    printf("ExampleDriver: Started successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Stop the example driver
 *
 * This function is called to quiesce the driver before termination.
 *
 * @param pThis         Service instance
 * @param pProvider     Provider service
 *
 * @retval IO_SUCCESS   Driver stopped successfully
 */
static IO_RETURN
ExampleDriver_Stop(
    IIOService *pThis,
    IIOService *pProvider
    )
{
    EXAMPLE_DRIVER *pDriver;
    UINTN cbSize;

    printf("ExampleDriver: Stop called\n");

    // Retrieve private data
    cbSize = sizeof(VOID *);
    if (IIOService_GetProperty(pThis, "driver-private", &pDriver, &cbSize, NULL) == IO_SUCCESS) {
        pDriver->bStarted = FALSE;
        printf("ExampleDriver: Operations performed: %u\n", pDriver->uOperationCount);
    }

    printf("ExampleDriver: Stopped successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Terminate the example driver
 *
 * This function is called to cleanup and deallocate all driver resources.
 *
 * @param pThis         Service instance
 * @param uOptions      Termination options
 *
 * @retval IO_SUCCESS   Driver terminated successfully
 */
static IO_RETURN
ExampleDriver_Terminate(
    IIOService *pThis,
    UINT32 uOptions
    )
{
    EXAMPLE_DRIVER *pDriver;
    UINTN cbSize;

    printf("ExampleDriver: Terminate called\n");

    // Retrieve and free private data
    cbSize = sizeof(VOID *);
    if (IIOService_GetProperty(pThis, "driver-private", &pDriver, &cbSize, NULL) == IO_SUCCESS) {
        free(pDriver);
    }

    printf("ExampleDriver: Terminated successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Create an example driver instance
 *
 * @param ppDriver      Receives pointer to new driver service
 *
 * @retval IO_SUCCESS   Driver created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
ExampleDriverCreate(
    IIOService **ppDriver
    )
{
    IO_RETURN Status;
    IIOService *pService;

    printf("ExampleDriver: Creating driver instance\n");

    // Create service
    Status = IOServiceCreate("ExampleDriver", &pService);
    if (Status != IO_SUCCESS) {
        printf("ExampleDriver: Failed to create service (status=0x%08X)\n", Status);
        return Status;
    }

    // Note: In a real implementation, you would override the Probe, Start, Stop,
    // and Terminate methods by creating a custom vtable. For this example, we're
    // using the default implementation.

    *ppDriver = pService;
    printf("ExampleDriver: Driver instance created successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Perform a driver operation
 *
 * This demonstrates a typical driver operation that can be called from
 * kernel code or through an IOUserClient.
 *
 * @param pDriver       Driver service instance
 * @param uOperation    Operation code
 * @param pInput        Input buffer
 * @param cbInputSize   Input buffer size
 * @param pOutput       Output buffer
 * @param cbOutputSize  Output buffer size
 *
 * @retval IO_SUCCESS   Operation completed successfully
 * @retval IO_ERROR     Operation failed
 */
IO_RETURN
ExampleDriverPerformOperation(
    IIOService *pDriver,
    UINT32 uOperation,
    CONST VOID *pInput,
    UINTN cbInputSize,
    VOID *pOutput,
    UINTN cbOutputSize
    )
{
    EXAMPLE_DRIVER *pPrivate;
    UINTN cbSize;

    // Retrieve private data
    cbSize = sizeof(VOID *);
    if (IIOService_GetProperty(pDriver, "driver-private", &pPrivate, &cbSize, NULL) != IO_SUCCESS) {
        return IO_ERROR;
    }

    // Check if started
    if (!pPrivate->bStarted) {
        return IO_NOT_READY;
    }

    // Increment operation counter
    pPrivate->uOperationCount++;

    printf("ExampleDriver: Performing operation %u (count=%u)\n",
           uOperation, pPrivate->uOperationCount);

    // Process operation
    switch (uOperation) {
        case 0x01:  // Get device info
            if (pOutput != NULL && cbOutputSize >= sizeof(UINT32)) {
                *(UINT32 *)pOutput = pPrivate->uDeviceId;
                return IO_SUCCESS;
            }
            break;

        case 0x02:  // Get operation count
            if (pOutput != NULL && cbOutputSize >= sizeof(UINT32)) {
                *(UINT32 *)pOutput = pPrivate->uOperationCount;
                return IO_SUCCESS;
            }
            break;

        default:
            return IO_UNSUPPORTED;
    }

    return IO_BAD_ARGUMENT;
}
