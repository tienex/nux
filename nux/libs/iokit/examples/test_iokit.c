/**
 * @file test_iokit.c
 * @brief IOKit Framework Test Program
 *
 * This program demonstrates and tests the IOKit framework functionality.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Test IOKit initialization
 */
static int
TestInitialization(void)
{
    IO_RETURN Status;

    printf("\n=== Test 1: IOKit Initialization ===\n");

    Status = IOKitInitialize();
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOKitInitialize returned 0x%08X\n", Status);
        return -1;
    }

    printf("PASS: IOKit initialized successfully\n");
    return 0;
}

/**
 * @brief Test service creation
 */
static int
TestServiceCreation(void)
{
    IO_RETURN Status;
    IIOService *pService = NULL;
    CHAR8 szName[64];

    printf("\n=== Test 2: Service Creation ===\n");

    Status = IOServiceCreate("TestService", &pService);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate returned 0x%08X\n", Status);
        return -1;
    }

    // Get service name
    Status = IIOService_GetServiceName(pService, szName, sizeof(szName));
    if (Status != IO_SUCCESS) {
        printf("FAIL: GetServiceName returned 0x%08X\n", Status);
        IIOService_Release(pService);
        return -1;
    }

    if (strcmp(szName, "TestService") != 0) {
        printf("FAIL: Service name mismatch: expected 'TestService', got '%s'\n", szName);
        IIOService_Release(pService);
        return -1;
    }

    IIOService_Release(pService);
    printf("PASS: Service created and named correctly\n");
    return 0;
}

/**
 * @brief Test property management
 */
static int
TestProperties(void)
{
    IO_RETURN Status;
    IIOService *pService = NULL;
    UINT32 uTestValue = 0x12345678;
    UINT32 uReadValue = 0;
    UINTN cbSize;
    UINT32 uType;

    printf("\n=== Test 3: Property Management ===\n");

    Status = IOServiceCreate("PropertyTest", &pService);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate returned 0x%08X\n", Status);
        return -1;
    }

    // Set property
    Status = IIOService_SetProperty(pService, "test-value", &uTestValue,
                                    sizeof(uTestValue), IO_PROPERTY_TYPE_NUMBER);
    if (Status != IO_SUCCESS) {
        printf("FAIL: SetProperty returned 0x%08X\n", Status);
        IIOService_Release(pService);
        return -1;
    }

    // Get property
    cbSize = sizeof(uReadValue);
    Status = IIOService_GetProperty(pService, "test-value", &uReadValue,
                                    &cbSize, &uType);
    if (Status != IO_SUCCESS) {
        printf("FAIL: GetProperty returned 0x%08X\n", Status);
        IIOService_Release(pService);
        return -1;
    }

    if (uReadValue != uTestValue) {
        printf("FAIL: Property value mismatch: expected 0x%08X, got 0x%08X\n",
               uTestValue, uReadValue);
        IIOService_Release(pService);
        return -1;
    }

    if (uType != IO_PROPERTY_TYPE_NUMBER) {
        printf("FAIL: Property type mismatch: expected %u, got %u\n",
               IO_PROPERTY_TYPE_NUMBER, uType);
        IIOService_Release(pService);
        return -1;
    }

    IIOService_Release(pService);
    printf("PASS: Properties set and retrieved correctly\n");
    return 0;
}

/**
 * @brief Test registry operations
 */
static int
TestRegistry(void)
{
    IO_RETURN Status;
    IIORegistry *pRegistry = NULL;
    IIOService *pRoot = NULL;
    IIOService *pService1 = NULL;
    IIOService *pService2 = NULL;
    IIOService *pServices[16];
    UINT32 uCount;
    CHAR8 szName[64];

    printf("\n=== Test 4: Registry Operations ===\n");

    // Get global registry
    Status = IOKitGetRegistry(&pRegistry);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOKitGetRegistry returned 0x%08X\n", Status);
        return -1;
    }

    // Get root service
    Status = IIORegistry_GetRootService(pRegistry, &pRoot);
    if (Status != IO_SUCCESS) {
        printf("FAIL: GetRootService returned 0x%08X\n", Status);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    IIOService_GetServiceName(pRoot, szName, sizeof(szName));
    printf("Root service: %s\n", szName);

    // Create and register test services
    Status = IOServiceCreate("RegistryTest1", &pService1);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate (1) returned 0x%08X\n", Status);
        IIOService_Release(pRoot);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    Status = IOServiceCreate("RegistryTest2", &pService2);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate (2) returned 0x%08X\n", Status);
        IIOService_Release(pService1);
        IIOService_Release(pRoot);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    // Register services
    Status = IOKitRegisterService(pService1, pRoot);
    if (Status != IO_SUCCESS) {
        printf("FAIL: RegisterService (1) returned 0x%08X\n", Status);
        IIOService_Release(pService2);
        IIOService_Release(pService1);
        IIOService_Release(pRoot);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    Status = IOKitRegisterService(pService2, pRoot);
    if (Status != IO_SUCCESS) {
        printf("FAIL: RegisterService (2) returned 0x%08X\n", Status);
        IIOService_Release(pService2);
        IIOService_Release(pService1);
        IIOService_Release(pRoot);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    // Find services by name
    uCount = 16;
    Status = IIORegistry_FindServicesByName(pRegistry, "RegistryTest1", NULL, 0,
                                           pServices, &uCount);
    if (Status != IO_SUCCESS || uCount == 0) {
        printf("FAIL: FindServicesByName returned 0x%08X (count=%u)\n", Status, uCount);
        IIOService_Release(pService2);
        IIOService_Release(pService1);
        IIOService_Release(pRoot);
        IIORegistry_Release(pRegistry);
        return -1;
    }

    printf("Found %u service(s) named 'RegistryTest1'\n", uCount);

    // Release found services
    for (UINT32 i = 0; i < uCount; i++) {
        IIOService_Release(pServices[i]);
    }

    // Cleanup
    IIOService_Release(pService2);
    IIOService_Release(pService1);
    IIOService_Release(pRoot);
    IIORegistry_Release(pRegistry);

    printf("PASS: Registry operations completed successfully\n");
    return 0;
}

/**
 * @brief Test service lifecycle
 */
static int
TestLifecycle(void)
{
    IO_RETURN Status;
    IIOService *pService = NULL;
    IIOService *pProvider = NULL;
    UINT32 uProbeScore;
    UINT32 uState;

    printf("\n=== Test 5: Service Lifecycle ===\n");

    // Create service and provider
    Status = IOServiceCreate("LifecycleTest", &pService);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate returned 0x%08X\n", Status);
        return -1;
    }

    Status = IOServiceCreate("Provider", &pProvider);
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOServiceCreate (provider) returned 0x%08X\n", Status);
        IIOService_Release(pService);
        return -1;
    }

    // Probe
    Status = IIOService_Probe(pService, pProvider, &uProbeScore);
    if (Status != IO_SUCCESS) {
        printf("FAIL: Probe returned 0x%08X\n", Status);
        IIOService_Release(pProvider);
        IIOService_Release(pService);
        return -1;
    }
    printf("Probe score: %u\n", uProbeScore);

    // Start
    Status = IIOService_Start(pService, pProvider);
    if (Status != IO_SUCCESS) {
        printf("FAIL: Start returned 0x%08X\n", Status);
        IIOService_Release(pProvider);
        IIOService_Release(pService);
        return -1;
    }

    // Check state
    Status = IIOService_GetServiceState(pService, &uState);
    if (Status != IO_SUCCESS) {
        printf("FAIL: GetServiceState returned 0x%08X\n", Status);
        IIOService_Release(pProvider);
        IIOService_Release(pService);
        return -1;
    }
    printf("Service state: 0x%08X\n", uState);

    // Stop
    Status = IIOService_Stop(pService, pProvider);
    if (Status != IO_SUCCESS) {
        printf("FAIL: Stop returned 0x%08X\n", Status);
        IIOService_Release(pProvider);
        IIOService_Release(pService);
        return -1;
    }

    // Terminate
    Status = IIOService_Terminate(pService, 0);
    if (Status != IO_SUCCESS) {
        printf("FAIL: Terminate returned 0x%08X\n", Status);
        IIOService_Release(pProvider);
        IIOService_Release(pService);
        return -1;
    }

    IIOService_Release(pProvider);
    IIOService_Release(pService);

    printf("PASS: Service lifecycle completed successfully\n");
    return 0;
}

/**
 * @brief Test shutdown
 */
static int
TestShutdown(void)
{
    IO_RETURN Status;

    printf("\n=== Test 6: IOKit Shutdown ===\n");

    Status = IOKitShutdown();
    if (Status != IO_SUCCESS) {
        printf("FAIL: IOKitShutdown returned 0x%08X\n", Status);
        return -1;
    }

    printf("PASS: IOKit shutdown successfully\n");
    return 0;
}

/**
 * @brief Main test program
 */
int
main(void)
{
    int nFailures = 0;

    printf("\n");
    printf("================================================\n");
    printf("   IOKit Driver Framework Test Suite\n");
    printf("================================================\n");

    if (TestInitialization() != 0) nFailures++;
    if (TestServiceCreation() != 0) nFailures++;
    if (TestProperties() != 0) nFailures++;
    if (TestRegistry() != 0) nFailures++;
    if (TestLifecycle() != 0) nFailures++;
    if (TestShutdown() != 0) nFailures++;

    printf("\n");
    printf("================================================\n");
    if (nFailures == 0) {
        printf("   All tests PASSED!\n");
    } else {
        printf("   %d test(s) FAILED\n", nFailures);
    }
    printf("================================================\n");
    printf("\n");

    return (nFailures == 0) ? 0 : 1;
}
