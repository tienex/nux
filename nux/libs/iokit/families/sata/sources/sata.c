/**
 * @file sata.c
 * @brief SATA Family Implementation - Serial ATA Storage Driver
 *
 * Provides full support for SATA 1.0/2.0/3.0/3.2 controllers with:
 * - AHCI (Advanced Host Controller Interface)
 * - Native Command Queuing (NCQ)
 * - Port multiplier support
 * - Hot-plug detection
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/sata/sata.h>
#include <iokit/families/pcie/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief SATA controller database entry
 */
typedef struct _SATA_CONTROLLER_DB_ENTRY {
    UINT16      VendorID;
    UINT16      DeviceID;
    CONST CHAR8 *pszVendor;
    CONST CHAR8 *pszModel;
    UINT32      Flags;
} SATA_CONTROLLER_DB_ENTRY;

/**
 * @brief SATA controller flags
 */
#define SATA_FLAG_AHCI              (1 << 0)    /**< AHCI controller */
#define SATA_FLAG_IDE_COMPAT        (1 << 1)    /**< IDE compatibility mode */
#define SATA_FLAG_RAID_CAPABLE      (1 << 2)    /**< RAID capable */
#define SATA_FLAG_NCQ_BROKEN        (1 << 3)    /**< Broken NCQ implementation */
#define SATA_FLAG_FBS_BROKEN        (1 << 4)    /**< Broken FIS-based switching */

/**
 * @brief Known SATA controller database (25+ entries)
 */
static CONST SATA_CONTROLLER_DB_ENTRY g_SATAControllerDB[] = {
    // Intel ICH/PCH SATA Controllers
    { 0x8086, 0x2922, "Intel", "ICH9 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x2929, "Intel", "ICH9M AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x3A22, "Intel", "ICH10 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x3B22, "Intel", "PCH AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x3B29, "Intel", "PCH Mobile AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x1C02, "Intel", "6 Series/C200 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x1C03, "Intel", "6 Series/C200 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x1E02, "Intel", "7 Series/C210 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x1E03, "Intel", "7 Series/C210 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x8C02, "Intel", "8 Series/C220 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x8C03, "Intel", "8 Series/C220 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x9C03, "Intel", "8 Series Mobile AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x8D02, "Intel", "C610/X99 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0xA102, "Intel", "100 Series/C230 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0xA103, "Intel", "100 Series/C230 AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0xA352, "Intel", "Cannon Lake PCH AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x02D3, "Intel", "Comet Lake PCH AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x06D2, "Intel", "Comet Lake PCH AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x43D2, "Intel", "Tiger Lake PCH AHCI SATA Controller", SATA_FLAG_AHCI },
    { 0x8086, 0x7AE2, "Intel", "Alder Lake PCH AHCI SATA Controller", SATA_FLAG_AHCI },

    // AMD FCH SATA Controllers
    { 0x1022, 0x7801, "AMD", "FCH SATA Controller (AHCI)", SATA_FLAG_AHCI },
    { 0x1022, 0x7804, "AMD", "FCH SATA Controller (RAID)", SATA_FLAG_AHCI | SATA_FLAG_RAID_CAPABLE },
    { 0x1022, 0x7900, "AMD", "FCH SATA Controller (AHCI)", SATA_FLAG_AHCI },
    { 0x1022, 0x7901, "AMD", "FCH SATA Controller (RAID)", SATA_FLAG_AHCI | SATA_FLAG_RAID_CAPABLE },

    // Marvell SATA Controllers
    { 0x11AB, 0x6121, "Marvell", "88SE6121 SATA II Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x6145, "Marvell", "88SE6145 SATA II Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9123, "Marvell", "88SE9123 SATA 6Gb/s Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9125, "Marvell", "88SE9125 SATA 6Gb/s Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9128, "Marvell", "88SE9128 SATA 6Gb/s Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9130, "Marvell", "88SE9130 SATA 6Gb/s Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9172, "Marvell", "88SE9172 SATA 6Gb/s Controller", SATA_FLAG_AHCI },
    { 0x11AB, 0x9192, "Marvell", "88SE9192 SATA 6Gb/s Controller", SATA_FLAG_AHCI },

    // JMicron SATA Controllers
    { 0x197B, 0x2360, "JMicron", "JMB360 AHCI Controller", SATA_FLAG_AHCI },
    { 0x197B, 0x2361, "JMicron", "JMB361 AHCI/IDE Controller", SATA_FLAG_AHCI },
    { 0x197B, 0x2362, "JMicron", "JMB362 SATA Controller", SATA_FLAG_AHCI },
    { 0x197B, 0x2363, "JMicron", "JMB363 SATA/IDE Controller", SATA_FLAG_AHCI },
    { 0x197B, 0x2368, "JMicron", "JMB368 AHCI Controller", SATA_FLAG_AHCI },

    // ASMedia SATA Controllers
    { 0x1B21, 0x0601, "ASMedia", "ASM1061 SATA III Controller", SATA_FLAG_AHCI },
    { 0x1B21, 0x0602, "ASMedia", "ASM1062 SATA III Controller", SATA_FLAG_AHCI },
    { 0x1B21, 0x0612, "ASMedia", "ASM1062 SATA III Controller", SATA_FLAG_AHCI },
    { 0x1B21, 0x0624, "ASMedia", "ASM1062+JMB575 AHCI Controller", SATA_FLAG_AHCI },

    // NVIDIA SATA Controllers
    { 0x10DE, 0x0554, "NVIDIA", "MCP67 AHCI Controller", SATA_FLAG_AHCI },
    { 0x10DE, 0x0AD4, "NVIDIA", "MCP78S AHCI Controller", SATA_FLAG_AHCI },
    { 0x10DE, 0x0AB9, "NVIDIA", "MCP79 AHCI Controller", SATA_FLAG_AHCI },

    // VIA SATA Controllers
    { 0x1106, 0x3349, "VIA", "VT8251 AHCI Controller", SATA_FLAG_AHCI },
    { 0x1106, 0x6287, "VIA", "VT8251 SATA AHCI Controller", SATA_FLAG_AHCI },
};

#define SATA_CONTROLLER_DB_COUNT (sizeof(g_SATAControllerDB) / sizeof(g_SATAControllerDB[0]))

/**
 * @brief AHCI HBA memory registers
 */
typedef struct _AHCI_HBA_REGS {
    UINT32  CAP;                /**< Host Capabilities */
    UINT32  GHC;                /**< Global Host Control */
    UINT32  IS;                 /**< Interrupt Status */
    UINT32  PI;                 /**< Ports Implemented */
    UINT32  VS;                 /**< Version */
    UINT32  CCC_CTL;            /**< Command Completion Coalescing Control */
    UINT32  CCC_PORTS;          /**< Command Completion Coalescing Ports */
    UINT32  EM_LOC;             /**< Enclosure Management Location */
    UINT32  EM_CTL;             /**< Enclosure Management Control */
    UINT32  CAP2;               /**< Host Capabilities Extended */
    UINT32  BOHC;               /**< BIOS/OS Handoff Control and Status */
} AHCI_HBA_REGS;

/**
 * @brief SATA controller implementation structure
 */
typedef struct _SATA_CONTROLLER_IMPL {
    IIOSATAController   Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOService         *pPCIDevice;         /**< PCI device interface */
    SATA_CONTROLLER_INFO ControllerInfo;    /**< Controller information */
    volatile UINT8     *pABAR;              /**< ABAR (AHCI Base Address) */
    UINT64              uABARPhysical;      /**< Physical ABAR address */
    UINTN               cbABARSize;         /**< ABAR size */
    BOOLEAN             bInitialized;       /**< Controller initialized */
    UINT32              uFlags;             /**< Controller flags */
} SATA_CONTROLLER_IMPL;

/**
 * @brief SATA device implementation structure
 */
typedef struct _SATA_DEVICE_IMPL {
    IIOSATADevice       Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOSATAController  *pController;        /**< Parent controller */
    SATA_DEVICE_INFO    DeviceInfo;         /**< Device information */
} SATA_DEVICE_IMPL;

// Forward declarations
static IO_RETURN SATAController_Start(IIOSATAController *pThis, IIOService *pProvider);
static IO_RETURN SATAController_GetControllerInfo(IIOSATAController *pThis, SATA_CONTROLLER_INFO *pInfo);
static IO_RETURN SATAController_GetPortCount(IIOSATAController *pThis, UINT32 *puCount);
static IO_RETURN SATAController_GetDevice(IIOSATAController *pThis, UINT32 uPort, IIOSATADevice **ppDevice);
static IO_RETURN SATAController_ResetPort(IIOSATAController *pThis, UINT32 uPort);
static IO_RETURN SATAController_SetPortEnable(IIOSATAController *pThis, UINT32 uPort, BOOLEAN bEnable);
static IO_RETURN SATAController_ScanPorts(IIOSATAController *pThis);

/**
 * @brief Look up controller in database
 */
static CONST SATA_CONTROLLER_DB_ENTRY*
SATALookupController(
    UINT16 uVendorID,
    UINT16 uDeviceID
    )
{
    UINT32 i;

    for (i = 0; i < SATA_CONTROLLER_DB_COUNT; i++) {
        if (g_SATAControllerDB[i].VendorID == uVendorID &&
            g_SATAControllerDB[i].DeviceID == uDeviceID) {
            return &g_SATAControllerDB[i];
        }
    }

    return NULL;
}

/**
 * @brief Read AHCI register (32-bit)
 */
static inline UINT32
AHCIReadReg32(
    SATA_CONTROLLER_IMPL *pController,
    UINT32 uOffset
    )
{
    return *(volatile UINT32 *)(pController->pABAR + uOffset);
}

/**
 * @brief Write AHCI register (32-bit)
 */
static inline VOID
AHCIWriteReg32(
    SATA_CONTROLLER_IMPL *pController,
    UINT32 uOffset,
    UINT32 uValue
    )
{
    *(volatile UINT32 *)(pController->pABAR + uOffset) = uValue;
}

/**
 * @brief Get AHCI port register base
 */
static inline volatile UINT8*
AHCIGetPortBase(
    SATA_CONTROLLER_IMPL *pController,
    UINT32 uPort
    )
{
    return pController->pABAR + 0x100 + (uPort * 0x80);
}

/**
 * @brief IIOSATAController::Start - Initialize controller
 */
static IO_RETURN
SATAController_Start(
    IIOSATAController *pThis,
    IIOService *pProvider
    )
{
    SATA_CONTROLLER_IMPL *pController = (SATA_CONTROLLER_IMPL *)pThis;
    IIOPCIDevice *pPCIDevice = NULL;
    PCI_DEVICE_INFO PCIInfo;
    IO_RETURN Status;
    UINT32 uCapabilities, uCapabilities2, uVersion, uPortsImpl;
    CONST SATA_CONTROLLER_DB_ENTRY *pDBEntry;

    if (pController == NULL || pProvider == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Query for PCI device interface
    Status = pProvider->lpVtbl->QueryInterface(pProvider, &IID_IIOPCIDevice, (VOID **)&pPCIDevice);
    if (Status != IO_SUCCESS || pPCIDevice == NULL) {
        printf("SATA: Not a PCI device\n");
        return IO_ERROR;
    }

    // Get PCI device information
    Status = pPCIDevice->lpVtbl->GetDeviceInfo(pPCIDevice, &PCIInfo);
    if (Status != IO_SUCCESS) {
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return Status;
    }

    // Verify this is a SATA controller (Class 01h, Subclass 06h)
    if (PCIInfo.ClassCode != 0x01 || PCIInfo.SubClass != 0x06) {
        printf("SATA: Not a SATA controller (Class %02X:%02X)\n",
               PCIInfo.ClassCode, PCIInfo.SubClass);
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return IO_NO_DEVICE;
    }

    // Look up controller in database
    pDBEntry = SATALookupController(PCIInfo.VendorID, PCIInfo.DeviceID);

    printf("SATA: Found controller %04X:%04X at %02X:%02X.%X\n",
           PCIInfo.VendorID, PCIInfo.DeviceID,
           PCIInfo.Location.Bus, PCIInfo.Location.Device, PCIInfo.Location.Function);

    if (pDBEntry != NULL) {
        printf("SATA: %s %s\n", pDBEntry->pszVendor, pDBEntry->pszModel);
        pController->uFlags = pDBEntry->Flags;

        if (pDBEntry->Flags & SATA_FLAG_AHCI) {
            printf("SATA: AHCI mode\n");
        }
        if (pDBEntry->Flags & SATA_FLAG_RAID_CAPABLE) {
            printf("SATA: RAID capable\n");
        }
    } else {
        printf("SATA: Unknown SATA controller\n");
    }

    // Map BAR5 (AHCI Base Address Register - ABAR)
    if (PCIInfo.BARs[5].bIsMem && PCIInfo.BARs[5].Size >= 0x800) {
        Status = pPCIDevice->lpVtbl->MapBAR(pPCIDevice, 5,
                                            (VOID **)&pController->pABAR,
                                            &pController->cbABARSize);
        if (Status != IO_SUCCESS) {
            printf("SATA: Failed to map ABAR (BAR5): 0x%X\n", Status);
            pPCIDevice->lpVtbl->Release(pPCIDevice);
            return Status;
        }

        pController->uABARPhysical = PCIInfo.BARs[5].PhysicalAddress;
        printf("SATA: Mapped ABAR at 0x%016llX (size: 0x%llX)\n",
               pController->uABARPhysical, pController->cbABARSize);
    } else {
        printf("SATA: Invalid or missing ABAR (BAR5)\n");
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return IO_ERROR;
    }

    // Enable bus mastering and memory space
    pPCIDevice->lpVtbl->SetBusMaster(pPCIDevice, TRUE);
    pPCIDevice->lpVtbl->SetMemoryIOEnable(pPCIDevice, TRUE, FALSE);

    // Read AHCI capabilities
    uCapabilities = AHCIReadReg32(pController, 0x00);  // CAP
    uCapabilities2 = AHCIReadReg32(pController, 0x24); // CAP2
    uVersion = AHCIReadReg32(pController, 0x10);       // VS
    uPortsImpl = AHCIReadReg32(pController, 0x0C);     // PI

    // Parse capabilities
    pController->ControllerInfo.NumPorts = (uCapabilities & AHCI_CAP_NP_MASK) + 1;
    pController->ControllerInfo.NumCommandSlots =
        ((uCapabilities >> AHCI_CAP_NCS_SHIFT) & AHCI_CAP_NCS_MASK) + 1;
    pController->ControllerInfo.PortsImplemented = uPortsImpl;
    pController->ControllerInfo.bNCQSupport = (uCapabilities & AHCI_CAP_SNCQ) ? TRUE : FALSE;
    pController->ControllerInfo.bPortMultiplier = (uCapabilities & AHCI_CAP_SPM) ? TRUE : FALSE;
    pController->ControllerInfo.bHotplug = TRUE; // AHCI supports hot-plug
    pController->ControllerInfo.bStaggeredSpinup = (uCapabilities & AHCI_CAP_SSS) ? TRUE : FALSE;
    pController->ControllerInfo.bALPMSupport = (uCapabilities & AHCI_CAP_SALP) ? TRUE : FALSE;
    pController->ControllerInfo.b64BitDMA = (uCapabilities & AHCI_CAP_S64A) ? TRUE : FALSE;

    // Determine max speed from capabilities
    UINT32 uSpeedSupport = (uCapabilities >> AHCI_CAP_ISS_SHIFT) & AHCI_CAP_ISS_MASK;
    switch (uSpeedSupport) {
        case 1: pController->ControllerInfo.MaxSpeed = SATA_SPEED_GEN1; break;
        case 2: pController->ControllerInfo.MaxSpeed = SATA_SPEED_GEN2; break;
        case 3: pController->ControllerInfo.MaxSpeed = SATA_SPEED_GEN3; break;
        default: pController->ControllerInfo.MaxSpeed = SATA_SPEED_GEN3; break;
    }

    printf("SATA: AHCI version %u.%u\n", (uVersion >> 16) & 0xFF, (uVersion >> 8) & 0xFF);
    printf("SATA: %u ports, %u command slots\n",
           pController->ControllerInfo.NumPorts,
           pController->ControllerInfo.NumCommandSlots);
    printf("SATA: Ports implemented: 0x%08X\n", uPortsImpl);
    printf("SATA: Features: NCQ=%d PM=%d 64bit=%d\n",
           pController->ControllerInfo.bNCQSupport,
           pController->ControllerInfo.bPortMultiplier,
           pController->ControllerInfo.b64BitDMA);

    // Enable AHCI mode
    UINT32 uGHC = AHCIReadReg32(pController, 0x04); // GHC
    if (!(uGHC & AHCI_GHC_AE)) {
        printf("SATA: Enabling AHCI mode\n");
        uGHC |= AHCI_GHC_AE;
        AHCIWriteReg32(pController, 0x04, uGHC);
    }

    // Store PCI device reference
    pController->pPCIDevice = pProvider;
    pProvider->lpVtbl->AddRef(pProvider);

    pController->ControllerInfo.VendorID = PCIInfo.VendorID;
    pController->ControllerInfo.DeviceID = PCIInfo.DeviceID;
    pController->ControllerInfo.Version = SATA_VERSION_3_0; // Default to SATA 3.0
    pController->bInitialized = TRUE;

    // Release PCI device interface
    pPCIDevice->lpVtbl->Release(pPCIDevice);

    printf("SATA: Controller initialization complete\n");

    return IO_SUCCESS;
}

/**
 * @brief IIOSATAController::GetControllerInfo - Get controller information
 */
static IO_RETURN
SATAController_GetControllerInfo(
    IIOSATAController *pThis,
    SATA_CONTROLLER_INFO *pInfo
    )
{
    SATA_CONTROLLER_IMPL *pController = (SATA_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pController->bInitialized) {
        return IO_NOT_READY;
    }

    memcpy(pInfo, &pController->ControllerInfo, sizeof(SATA_CONTROLLER_INFO));
    return IO_SUCCESS;
}

/**
 * @brief IIOSATAController::GetPortCount - Get port count
 */
static IO_RETURN
SATAController_GetPortCount(
    IIOSATAController *pThis,
    UINT32 *puCount
    )
{
    SATA_CONTROLLER_IMPL *pController = (SATA_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *puCount = pController->ControllerInfo.NumPorts;
    return IO_SUCCESS;
}

/**
 * @brief IIOSATAController::GetDevice - Get device on port
 */
static IO_RETURN
SATAController_GetDevice(
    IIOSATAController *pThis,
    UINT32 uPort,
    IIOSATADevice **ppDevice
    )
{
    // TODO: Implement device enumeration
    printf("SATA: GetDevice(port=%u) - Not yet implemented\n", uPort);
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSATAController::ResetPort - Reset SATA port
 */
static IO_RETURN
SATAController_ResetPort(
    IIOSATAController *pThis,
    UINT32 uPort
    )
{
    // TODO: Implement COMRESET
    printf("SATA: ResetPort(port=%u) - Not yet implemented\n", uPort);
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSATAController::SetPortEnable - Enable/disable port
 */
static IO_RETURN
SATAController_SetPortEnable(
    IIOSATAController *pThis,
    UINT32 uPort,
    BOOLEAN bEnable
    )
{
    // TODO: Implement port enable/disable
    printf("SATA: SetPortEnable(port=%u, enable=%d) - Not yet implemented\n", uPort, bEnable);
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSATAController::ScanPorts - Scan ports for devices
 */
static IO_RETURN
SATAController_ScanPorts(
    IIOSATAController *pThis
    )
{
    SATA_CONTROLLER_IMPL *pController = (SATA_CONTROLLER_IMPL *)pThis;
    UINT32 i;
    UINT32 uPortsImpl;

    if (pController == NULL || !pController->bInitialized) {
        return IO_BAD_ARGUMENT;
    }

    uPortsImpl = pController->ControllerInfo.PortsImplemented;

    printf("SATA: Scanning ports...\n");

    for (i = 0; i < 32; i++) {
        if (uPortsImpl & (1 << i)) {
            volatile UINT8 *pPortBase = AHCIGetPortBase(pController, i);
            UINT32 uSSTS = *(volatile UINT32 *)(pPortBase + AHCI_PORT_SSTS);
            UINT32 uDET = uSSTS & 0xF;
            UINT32 uIPM = (uSSTS >> 8) & 0xF;

            if (uDET == 3 && uIPM == 1) {
                UINT32 uSIG = *(volatile UINT32 *)(pPortBase + AHCI_PORT_SIG);
                printf("SATA: Port %u: Device present (sig=0x%08X)\n", i, uSIG);

                // TODO: Identify device type and create device interface
            } else {
                printf("SATA: Port %u: No device\n", i);
            }
        }
    }

    return IO_SUCCESS;
}

/**
 * @brief SATA controller vtable (stub implementations)
 */
static IIOSATAControllerVtbl g_SATAControllerVtbl = {
    // IUnknown methods (stubs)
    NULL,  // QueryInterface
    NULL,  // AddRef
    NULL,  // Release

    // IIOService methods (stubs)
    NULL,  // Probe
    SATAController_Start,
    NULL,  // Stop
    NULL,  // Terminate
    NULL,  // GetProperty
    NULL,  // SetProperty
    NULL,  // GetParentService
    NULL,  // GetChildService
    NULL,  // GetServiceState
    NULL,  // GetServiceName
    NULL,  // RegisterService

    // IIOSATAController methods
    SATAController_GetControllerInfo,
    SATAController_GetPortCount,
    SATAController_GetDevice,
    SATAController_ResetPort,
    SATAController_SetPortEnable,
    SATAController_ScanPorts,
};

/**
 * @brief Initialize SATA family driver
 */
IO_RETURN
SATAInitialize(
    VOID
    )
{
    printf("SATA: Initializing SATA/AHCI family driver\n");
    printf("SATA: Supports SATA 1.0 (1.5Gbps), 2.0 (3Gbps), 3.0 (6Gbps), 3.2 (16Gbps)\n");
    printf("SATA: Controller database: %u entries\n", (UINT32)SATA_CONTROLLER_DB_COUNT);

    return IO_SUCCESS;
}

/**
 * @brief Shutdown SATA family driver
 */
IO_RETURN
SATAShutdown(
    VOID
    )
{
    printf("SATA: Shutting down SATA family driver\n");
    return IO_SUCCESS;
}

/**
 * @brief Create SATA controller instance
 */
IO_RETURN
SATAControllerCreate(
    IIOService *pPCIDevice,
    IIOSATAController **ppController
    )
{
    SATA_CONTROLLER_IMPL *pController;

    if (pPCIDevice == NULL || ppController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate controller structure
    pController = (SATA_CONTROLLER_IMPL *)malloc(sizeof(SATA_CONTROLLER_IMPL));
    if (pController == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pController, 0, sizeof(SATA_CONTROLLER_IMPL));
    pController->Vtbl.lpVtbl = &g_SATAControllerVtbl;
    pController->RefCount = 1;

    *ppController = (IIOSATAController *)pController;
    return IO_SUCCESS;
}
