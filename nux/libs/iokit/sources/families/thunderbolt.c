/**
 * @file thunderbolt.c
 * @brief Thunderbolt Driver Implementation
 *
 * Implements support for Thunderbolt 1/2/3/4 controllers and devices.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/thunderbolt.h>
#include <iokit/families/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Known Thunderbolt Controller IDs
 */
typedef struct _TB_CONTROLLER_ID {
    UINT16          VendorID;
    UINT16          DeviceID;
    TB_GENERATION   Generation;
    CONST CHAR8    *pszName;
} TB_CONTROLLER_ID;

/**
 * @brief Thunderbolt controller database
 */
static CONST TB_CONTROLLER_ID g_ThunderboltControllers[] = {
    // Intel Thunderbolt 1 (Light Peak)
    { 0x8086, 0x1513, TB_GEN_1, "Intel Light Peak" },
    { 0x8086, 0x151A, TB_GEN_1, "Intel Cactus Ridge 4C" },
    { 0x8086, 0x151B, TB_GEN_1, "Intel Cactus Ridge 2C" },

    // Intel Thunderbolt 2
    { 0x8086, 0x156A, TB_GEN_2, "Intel Falcon Ridge 4C" },
    { 0x8086, 0x156B, TB_GEN_2, "Intel Falcon Ridge 2C" },
    { 0x8086, 0x156C, TB_GEN_2, "Intel Falcon Ridge LP" },
    { 0x8086, 0x156D, TB_GEN_2, "Intel Falcon Ridge 4C" },

    // Intel Thunderbolt 3
    { 0x8086, 0x1575, TB_GEN_3, "Intel Alpine Ridge 4C" },
    { 0x8086, 0x1576, TB_GEN_3, "Intel Alpine Ridge 2C" },
    { 0x8086, 0x1577, TB_GEN_3, "Intel Alpine Ridge LP" },
    { 0x8086, 0x1578, TB_GEN_3, "Intel Alpine Ridge C" },
    { 0x8086, 0x15BF, TB_GEN_3, "Intel Titan Ridge 4C" },
    { 0x8086, 0x15C0, TB_GEN_3, "Intel Titan Ridge 2C" },
    { 0x8086, 0x15D2, TB_GEN_3, "Intel Titan Ridge DD" },
    { 0x8086, 0x15D9, TB_GEN_3, "Intel Titan Ridge 4C" },
    { 0x8086, 0x15DA, TB_GEN_3, "Intel Titan Ridge 2C" },
    { 0x8086, 0x15DB, TB_GEN_3, "Intel Titan Ridge 4C" },
    { 0x8086, 0x15DC, TB_GEN_3, "Intel Titan Ridge 2C" },
    { 0x8086, 0x15DD, TB_GEN_3, "Intel Titan Ridge DD" },
    { 0x8086, 0x15DE, TB_GEN_3, "Intel Titan Ridge DD" },

    // Intel Thunderbolt 4
    { 0x8086, 0x9A1B, TB_GEN_4, "Intel Maple Ridge 4C" },
    { 0x8086, 0x9A1D, TB_GEN_4, "Intel Maple Ridge 2C" },
    { 0x8086, 0x9A1F, TB_GEN_4, "Intel Maple Ridge 4C" },
    { 0x8086, 0x9A21, TB_GEN_4, "Intel Maple Ridge 2C" },
    { 0x8086, 0x9A23, TB_GEN_4, "Intel Maple Ridge 4C" },
    { 0x8086, 0x9A25, TB_GEN_4, "Intel Maple Ridge 2C" },

    // End marker
    { 0, 0, TB_GEN_UNKNOWN, NULL }
};

/**
 * @brief Maximum devices per controller
 */
#define TB_MAX_DEVICES              64
#define TB_MAX_TUNNELS              32
#define TB_MAX_PORTS                12

/**
 * @brief Thunderbolt Controller Implementation
 */
typedef struct _TB_CONTROLLER_IMPL {
    IIOThunderboltController    Vtbl;               /**< Virtual function table */
    IIOService                 *pService;           /**< Underlying service */
    IIOPCIDevice               *pPCIDevice;         /**< PCIe device */
    TB_CONTROLLER_INFO          ControllerInfo;     /**< Controller information */
    IIOThunderboltDevice       *pDevices[TB_MAX_DEVICES]; /**< Connected devices */
    UINT32                      uDeviceCount;       /**< Number of devices */
    VOID                       *pNHIBase;           /**< NHI MMIO base */
    BOOLEAN                     bStarted;           /**< Controller started */
} TB_CONTROLLER_IMPL;

/**
 * @brief Thunderbolt Device Implementation
 */
typedef struct _TB_DEVICE_IMPL {
    IIOThunderboltDevice        Vtbl;               /**< Virtual function table */
    IIOService                 *pService;           /**< Underlying service */
    TB_DEVICE_INFO              DeviceInfo;         /**< Device information */
    TB_CONNECTION_STATE         ConnectionState;    /**< Connection state */
    TB_TUNNEL_INFO              Tunnels[TB_MAX_TUNNELS]; /**< Active tunnels */
    UINT32                      uTunnelCount;       /**< Number of tunnels */
    TB_CONTROLLER_IMPL         *pController;        /**< Parent controller */
} TB_DEVICE_IMPL;

/**
 * @brief Global Thunderbolt subsystem state
 */
static struct {
    BOOLEAN                     bInitialized;
    IIOThunderboltController   *pControllers[8];
    UINT32                      uControllerCount;
} g_TBSubsystem = { FALSE, { NULL }, 0 };

// Forward declarations
static IO_RETURN TB_ReadNHIRegister(TB_CONTROLLER_IMPL *pController,
                                    UINT32 uOffset, UINT32 *puValue);
static IO_RETURN TB_WriteNHIRegister(TB_CONTROLLER_IMPL *pController,
                                     UINT32 uOffset, UINT32 uValue);

/**
 * @brief Identify Thunderbolt controller from PCI device
 *
 * @param VendorID      PCI vendor ID
 * @param DeviceID      PCI device ID
 *
 * @return Pointer to controller ID structure, or NULL if not recognized
 */
static CONST TB_CONTROLLER_ID *
TB_IdentifyController(
    UINT16 VendorID,
    UINT16 DeviceID
    )
{
    UINT32 i;

    for (i = 0; g_ThunderboltControllers[i].pszName != NULL; i++) {
        if (g_ThunderboltControllers[i].VendorID == VendorID &&
            g_ThunderboltControllers[i].DeviceID == DeviceID) {
            return &g_ThunderboltControllers[i];
        }
    }

    return NULL;
}

/**
 * @brief Read NHI (Native Host Interface) register
 *
 * @param pController   Controller instance
 * @param uOffset       Register offset
 * @param puValue       Receives register value
 *
 * @retval IO_SUCCESS   Read successful
 */
static IO_RETURN
TB_ReadNHIRegister(
    TB_CONTROLLER_IMPL *pController,
    UINT32 uOffset,
    UINT32 *puValue
    )
{
    volatile UINT32 *pReg;

    if (pController->pNHIBase == NULL || puValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pReg = (volatile UINT32 *)((UINT8 *)pController->pNHIBase + uOffset);
    *puValue = *pReg;

    return IO_SUCCESS;
}

/**
 * @brief Write NHI register
 *
 * @param pController   Controller instance
 * @param uOffset       Register offset
 * @param uValue        Value to write
 *
 * @retval IO_SUCCESS   Write successful
 */
static IO_RETURN
TB_WriteNHIRegister(
    TB_CONTROLLER_IMPL *pController,
    UINT32 uOffset,
    UINT32 uValue
    )
{
    volatile UINT32 *pReg;

    if (pController->pNHIBase == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pReg = (volatile UINT32 *)((UINT8 *)pController->pNHIBase + uOffset);
    *pReg = uValue;

    return IO_SUCCESS;
}

/**
 * @brief Initialize Thunderbolt controller hardware
 *
 * @param pController   Controller instance
 *
 * @retval IO_SUCCESS   Initialization successful
 */
static IO_RETURN
TB_InitializeController(
    TB_CONTROLLER_IMPL *pController
    )
{
    UINT32 uVersion;
    UINT32 uSecurity;
    IO_RETURN Status;

    printf("Thunderbolt: Initializing controller hardware...\n");

    // Map NHI BAR0 (Native Host Interface registers)
    UINT64 cbSize;
    Status = IIOPCIDevice_MapBAR(pController->pPCIDevice, 0,
                                 &pController->pNHIBase, &cbSize);
    if (Status != IO_SUCCESS) {
        printf("Thunderbolt: Failed to map NHI registers (status=0x%08X)\n", Status);
        return Status;
    }

    printf("Thunderbolt: NHI mapped at %p (size=0x%llX)\n",
           pController->pNHIBase, cbSize);

    // Read version register
    Status = TB_ReadNHIRegister(pController, TB_REG_VERSION, &uVersion);
    if (Status == IO_SUCCESS) {
        pController->ControllerInfo.NVMVersion = uVersion;
        printf("Thunderbolt: NVM version 0x%08X\n", uVersion);
    }

    // Read security level
    Status = TB_ReadNHIRegister(pController, TB_REG_SECURITY, &uSecurity);
    if (Status == IO_SUCCESS) {
        pController->ControllerInfo.SecurityLevel = (TB_SECURITY_LEVEL)(uSecurity & 0x7);
        printf("Thunderbolt: Security level %u\n",
               pController->ControllerInfo.SecurityLevel);
    }

    // Enable memory and bus mastering on PCI device
    IIOPCIDevice_SetMemoryIOEnable(pController->pPCIDevice, TRUE, FALSE);
    IIOPCIDevice_SetBusMaster(pController->pPCIDevice, TRUE);

    // Initialize rings (TX/RX descriptor rings for packet communication)
    // TODO: Allocate and configure descriptor rings

    // Enable interrupts
    // TODO: Setup MSI/MSI-X interrupts for hot-plug events

    printf("Thunderbolt: Controller initialized successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Scan for connected Thunderbolt devices
 *
 * @param pController   Controller instance
 *
 * @retval IO_SUCCESS   Scan successful
 */
static IO_RETURN
TB_ScanDevices(
    TB_CONTROLLER_IMPL *pController
    )
{
    UINT32 i;

    printf("Thunderbolt: Scanning for devices...\n");

    // TODO: Implement device discovery via configuration packets
    // This involves:
    // 1. Sending configuration read requests
    // 2. Reading DROM (Device ROM) from each device
    // 3. Parsing device capabilities
    // 4. Building device tree

    // For now, just report that scanning is complete
    pController->uDeviceCount = 0;

    printf("Thunderbolt: Scan complete, found %u device(s)\n",
           pController->uDeviceCount);

    return IO_SUCCESS;
}

/**
 * @brief Probe Thunderbolt controller
 *
 * @param pThis         Service instance
 * @param pProvider     Provider service (PCI device)
 * @param puProbeScore  Receives probe score
 *
 * @retval IO_SUCCESS   Probe successful
 * @retval IO_NO_MATCH  Not a Thunderbolt controller
 */
static IO_RETURN STDMETHODCALLTYPE
TBController_Probe(
    IIOService *pThis,
    IIOService *pProvider,
    UINT32 *puProbeScore
    )
{
    PCI_DEVICE_INFO DeviceInfo;
    IIOPCIDevice *pPCIDevice;
    IO_RETURN Status;
    CONST TB_CONTROLLER_ID *pControllerID;

    printf("Thunderbolt: Probe called\n");

    // Get PCI device interface from provider
    Status = IIOService_QueryInterface(pProvider, &IID_IIOPCIDevice,
                                      (void **)&pPCIDevice);
    if (Status != S_OK) {
        return IO_NO_MATCH;
    }

    // Get device information
    Status = IIOPCIDevice_GetDeviceInfo(pPCIDevice, &DeviceInfo);
    IIOPCIDevice_Release(pPCIDevice);

    if (Status != IO_SUCCESS) {
        return IO_NO_MATCH;
    }

    // Check if this is a Thunderbolt controller
    // Thunderbolt controllers are Serial Bus controllers (0x0C),
    // Thunderbolt subclass (0x0A)
    if (DeviceInfo.ClassCode != PCI_CLASS_SERIAL_BUS ||
        DeviceInfo.SubClass != PCI_SUBCLASS_SERIAL_THUNDERBOLT) {
        return IO_NO_MATCH;
    }

    // Try to identify specific controller
    pControllerID = TB_IdentifyController(DeviceInfo.VendorID, DeviceInfo.DeviceID);
    if (pControllerID != NULL) {
        printf("Thunderbolt: Identified controller: %s (Generation %u)\n",
               pControllerID->pszName, pControllerID->Generation);
        *puProbeScore = 1000;  // High priority
        return IO_SUCCESS;
    }

    printf("Thunderbolt: Unknown Thunderbolt controller %04X:%04X\n",
           DeviceInfo.VendorID, DeviceInfo.DeviceID);

    // Still claim it as we support generic Thunderbolt
    *puProbeScore = 500;
    return IO_SUCCESS;
}

/**
 * @brief Start Thunderbolt controller
 *
 * @param pThis         Service instance
 * @param pProvider     Provider service (PCI device)
 *
 * @retval IO_SUCCESS   Controller started successfully
 */
static IO_RETURN STDMETHODCALLTYPE
TBController_Start(
    IIOService *pThis,
    IIOService *pProvider
    )
{
    TB_CONTROLLER_IMPL *pController;
    PCI_DEVICE_INFO DeviceInfo;
    IO_RETURN Status;
    UINTN cbSize;
    CONST TB_CONTROLLER_ID *pControllerID;

    printf("Thunderbolt: Start called\n");

    // Get controller instance
    cbSize = sizeof(VOID *);
    if (IIOService_GetProperty(pThis, "controller-private", &pController,
                               &cbSize, NULL) != IO_SUCCESS) {
        return IO_ERROR;
    }

    // Get PCI device interface
    Status = IIOService_QueryInterface(pProvider, &IID_IIOPCIDevice,
                                      (void **)&pController->pPCIDevice);
    if (Status != S_OK) {
        return IO_ERROR;
    }

    // Get device information
    IIOPCIDevice_GetDeviceInfo(pController->pPCIDevice, &DeviceInfo);

    // Fill in controller information
    pController->ControllerInfo.VendorID = DeviceInfo.VendorID;
    pController->ControllerInfo.DeviceID = DeviceInfo.DeviceID;

    pControllerID = TB_IdentifyController(DeviceInfo.VendorID, DeviceInfo.DeviceID);
    if (pControllerID != NULL) {
        pController->ControllerInfo.Generation = pControllerID->Generation;
        strncpy(pController->ControllerInfo.ControllerName,
                pControllerID->pszName,
                sizeof(pController->ControllerInfo.ControllerName) - 1);
    } else {
        pController->ControllerInfo.Generation = TB_GEN_UNKNOWN;
        snprintf(pController->ControllerInfo.ControllerName,
                sizeof(pController->ControllerInfo.ControllerName),
                "Unknown Thunderbolt Controller");
    }

    // Set default capabilities based on generation
    switch (pController->ControllerInfo.Generation) {
        case TB_GEN_1:
            pController->ControllerInfo.Capabilities =
                TB_CAP_HOTPLUG | TB_CAP_DAISY_CHAIN | TB_CAP_DISPLAYPORT;
            pController->ControllerInfo.MaxDepth = 6;
            pController->ControllerInfo.MaxPortCount = 2;
            break;

        case TB_GEN_2:
            pController->ControllerInfo.Capabilities =
                TB_CAP_HOTPLUG | TB_CAP_DAISY_CHAIN | TB_CAP_DISPLAYPORT;
            pController->ControllerInfo.MaxDepth = 6;
            pController->ControllerInfo.MaxPortCount = 2;
            break;

        case TB_GEN_3:
        case TB_GEN_4:
            pController->ControllerInfo.Capabilities =
                TB_CAP_HOTPLUG | TB_CAP_DAISY_CHAIN | TB_CAP_DISPLAYPORT |
                TB_CAP_USB3 | TB_CAP_POWER_DELIVERY | TB_CAP_CHARGING |
                TB_CAP_WAKE | TB_CAP_IOMMU;
            pController->ControllerInfo.MaxDepth = 6;
            pController->ControllerInfo.MaxPortCount = 4;
            break;

        default:
            pController->ControllerInfo.Capabilities = TB_CAP_HOTPLUG;
            pController->ControllerInfo.MaxDepth = 6;
            pController->ControllerInfo.MaxPortCount = 2;
            break;
    }

    // Default security level (can be overridden by BIOS/firmware)
    pController->ControllerInfo.SecurityLevel = TB_SECURITY_USER;

    // Initialize hardware
    Status = TB_InitializeController(pController);
    if (Status != IO_SUCCESS) {
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    // Scan for devices
    TB_ScanDevices(pController);

    pController->bStarted = TRUE;

    printf("Thunderbolt: Controller started successfully\n");
    printf("Thunderbolt:   Name: %s\n", pController->ControllerInfo.ControllerName);
    printf("Thunderbolt:   Generation: %u\n", pController->ControllerInfo.Generation);
    printf("Thunderbolt:   Security: %u\n", pController->ControllerInfo.SecurityLevel);
    printf("Thunderbolt:   Capabilities: 0x%08X\n", pController->ControllerInfo.Capabilities);

    return IO_SUCCESS;
}

// Continue in next part...
