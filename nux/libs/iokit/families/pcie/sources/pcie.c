/**
 * @file pcie.c
 * @brief PCIe Family Implementation - PCI/PCI-X/PCIe Bus Driver
 *
 * Provides full support for:
 * - Legacy PCI 32-bit (Conventional PCI 2.x/3.0)
 * - PCI-X (PCI Extended)
 * - PCI 64-bit
 * - PCI Express (PCIe) 1.x/2.x/3.x/4.x/5.x
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/pcie/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief PCI configuration mechanism detection
 */
typedef enum _PCI_CONFIG_MECHANISM {
    PCI_CONFIG_MECHANISM_1 = 1,     /**< Configuration mechanism #1 (I/O ports) */
    PCI_CONFIG_MECHANISM_2 = 2,     /**< Configuration mechanism #2 (legacy) */
    PCI_CONFIG_ECAM        = 3,     /**< PCIe ECAM (memory-mapped) */
} PCI_CONFIG_MECHANISM;

/**
 * @brief PCI configuration mechanism #1 I/O ports
 */
#define PCI_CONFIG_ADDRESS      0x0CF8  /**< Configuration address port */
#define PCI_CONFIG_DATA         0x0CFC  /**< Configuration data port */

/**
 * @brief PCIe ECAM (Enhanced Configuration Access Mechanism) base
 */
#define PCIE_ECAM_BASE          0xE0000000ULL

/**
 * @brief Maximum PCI devices and functions
 */
#define PCI_MAX_BUSES           256
#define PCI_MAX_DEVICES         32
#define PCI_MAX_FUNCTIONS       8
#define PCI_MAX_BARS            6

/**
 * @brief PCI device implementation structure
 */
typedef struct _PCI_DEVICE_IMPL {
    IIOPCIDevice        Vtbl;               /**< Virtual function table */
    IIOService         *pService;           /**< Underlying service */
    PCI_DEVICE_INFO     DeviceInfo;         /**< Device information */
    VOID               *pMappedBARs[PCI_MAX_BARS]; /**< Mapped BAR addresses */
    BOOLEAN             bMSIEnabled;        /**< MSI enabled */
    BOOLEAN             bMSIXEnabled;       /**< MSI-X enabled */
    UINT32              uMSIVector;         /**< MSI vector base */
    PCI_CONFIG_MECHANISM ConfigMechanism;   /**< Configuration mechanism */
} PCI_DEVICE_IMPL;

/**
 * @brief Global PCI subsystem state
 */
static struct {
    BOOLEAN             bInitialized;
    PCI_CONFIG_MECHANISM ConfigMechanism;
    UINT64              ECAMBase;
    UINT32              uDeviceCount;
} g_PCISubsystem = { FALSE, PCI_CONFIG_MECHANISM_1, PCIE_ECAM_BASE, 0 };

// Forward declarations
static IO_RETURN PCIDevice_ConfigRead(IIOPCIDevice *pThis, UINT32 uOffset,
                                      UINT32 uSize, UINT32 *puValue);
static IO_RETURN PCIDevice_ConfigWrite(IIOPCIDevice *pThis, UINT32 uOffset,
                                       UINT32 uSize, UINT32 uValue);

/**
 * @brief I/O port access functions (architecture-specific, stubbed for now)
 */
static inline UINT8 inb(UINT16 port) {
    // TODO: Implement architecture-specific I/O port read
    return 0;
}

static inline UINT16 inw(UINT16 port) {
    // TODO: Implement architecture-specific I/O port read
    return 0;
}

static inline UINT32 inl(UINT16 port) {
    // TODO: Implement architecture-specific I/O port read
    return 0;
}

static inline void outb(UINT16 port, UINT8 value) {
    // TODO: Implement architecture-specific I/O port write
}

static inline void outw(UINT16 port, UINT16 value) {
    // TODO: Implement architecture-specific I/O port write
}

static inline void outl(UINT16 port, UINT32 value) {
    // TODO: Implement architecture-specific I/O port write
}

/**
 * @brief Make PCI configuration address for mechanism #1
 *
 * @param pLocation     PCI device location
 * @param uOffset       Configuration register offset
 *
 * @return Configuration address value
 */
static UINT32
PCIMakeConfigAddress(
    CONST PCI_LOCATION *pLocation,
    UINT32 uOffset
    )
{
    return (UINT32)(
        0x80000000 |                            // Enable bit
        ((UINT32)pLocation->Bus << 16) |        // Bus number
        ((UINT32)pLocation->Device << 11) |     // Device number
        ((UINT32)pLocation->Function << 8) |    // Function number
        (uOffset & 0xFC)                        // Register offset (aligned to 4)
    );
}

/**
 * @brief Read from PCI configuration space (Mechanism #1)
 *
 * Supports legacy PCI, PCI-X, and PCI 64-bit through I/O port mechanism.
 *
 * @param pLocation     PCI device location
 * @param uOffset       Configuration register offset
 * @param uSize         Size of read (1, 2, or 4 bytes)
 * @param puValue       Receives the value
 *
 * @retval IO_SUCCESS       Read successful
 */
static IO_RETURN
PCIConfigReadMechanism1(
    CONST PCI_LOCATION *pLocation,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 *puValue
    )
{
    UINT32 uAddress;
    UINT32 uData;

    if (puValue == NULL || uOffset >= 256) {
        return IO_BAD_ARGUMENT;
    }

    // Create configuration address
    uAddress = PCIMakeConfigAddress(pLocation, uOffset);

    // Write address
    outl(PCI_CONFIG_ADDRESS, uAddress);

    // Read data based on size
    switch (uSize) {
        case 1:
            *puValue = inb(PCI_CONFIG_DATA + (uOffset & 3));
            break;
        case 2:
            if (uOffset & 1) {
                return IO_NOT_ALIGNED;
            }
            *puValue = inw(PCI_CONFIG_DATA + (uOffset & 2));
            break;
        case 4:
            if (uOffset & 3) {
                return IO_NOT_ALIGNED;
            }
            *puValue = inl(PCI_CONFIG_DATA);
            break;
        default:
            return IO_BAD_ARGUMENT;
    }

    return IO_SUCCESS;
}

/**
 * @brief Write to PCI configuration space (Mechanism #1)
 *
 * Supports legacy PCI, PCI-X, and PCI 64-bit through I/O port mechanism.
 *
 * @param pLocation     PCI device location
 * @param uOffset       Configuration register offset
 * @param uSize         Size of write (1, 2, or 4 bytes)
 * @param uValue        Value to write
 *
 * @retval IO_SUCCESS       Write successful
 */
static IO_RETURN
PCIConfigWriteMechanism1(
    CONST PCI_LOCATION *pLocation,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 uValue
    )
{
    UINT32 uAddress;

    if (uOffset >= 256) {
        return IO_BAD_ARGUMENT;
    }

    // Create configuration address
    uAddress = PCIMakeConfigAddress(pLocation, uOffset);

    // Write address
    outl(PCI_CONFIG_ADDRESS, uAddress);

    // Write data based on size
    switch (uSize) {
        case 1:
            outb(PCI_CONFIG_DATA + (uOffset & 3), (UINT8)uValue);
            break;
        case 2:
            if (uOffset & 1) {
                return IO_NOT_ALIGNED;
            }
            outw(PCI_CONFIG_DATA + (uOffset & 2), (UINT16)uValue);
            break;
        case 4:
            if (uOffset & 3) {
                return IO_NOT_ALIGNED;
            }
            outl(PCI_CONFIG_DATA, uValue);
            break;
        default:
            return IO_BAD_ARGUMENT;
    }

    return IO_SUCCESS;
}

/**
 * @brief Read from PCIe configuration space (ECAM)
 *
 * Uses memory-mapped PCIe Extended Configuration Access Mechanism.
 * Supports full 4KB configuration space for PCIe devices.
 *
 * @param pLocation     PCI device location
 * @param uOffset       Configuration register offset (0-4095)
 * @param uSize         Size of read (1, 2, or 4 bytes)
 * @param puValue       Receives the value
 *
 * @retval IO_SUCCESS       Read successful
 */
static IO_RETURN
PCIConfigReadECAM(
    CONST PCI_LOCATION *pLocation,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 *puValue
    )
{
    UINT64 uAddress;
    volatile UINT8 *pConfig;

    if (puValue == NULL || uOffset >= 4096) {
        return IO_BAD_ARGUMENT;
    }

    // Calculate ECAM address
    // Address = ECAM_base + (Bus << 20) + (Device << 15) + (Function << 12) + Offset
    uAddress = g_PCISubsystem.ECAMBase +
               ((UINT64)pLocation->Bus << 20) +
               ((UINT64)pLocation->Device << 15) +
               ((UINT64)pLocation->Function << 12) +
               uOffset;

    pConfig = (volatile UINT8 *)uAddress;

    // Read based on size
    switch (uSize) {
        case 1:
            *puValue = *pConfig;
            break;
        case 2:
            if (uOffset & 1) {
                return IO_NOT_ALIGNED;
            }
            *puValue = *(volatile UINT16 *)pConfig;
            break;
        case 4:
            if (uOffset & 3) {
                return IO_NOT_ALIGNED;
            }
            *puValue = *(volatile UINT32 *)pConfig;
            break;
        default:
            return IO_BAD_ARGUMENT;
    }

    return IO_SUCCESS;
}

/**
 * @brief Write to PCIe configuration space (ECAM)
 *
 * Uses memory-mapped PCIe Extended Configuration Access Mechanism.
 *
 * @param pLocation     PCI device location
 * @param uOffset       Configuration register offset (0-4095)
 * @param uSize         Size of write (1, 2, or 4 bytes)
 * @param uValue        Value to write
 *
 * @retval IO_SUCCESS       Write successful
 */
static IO_RETURN
PCIConfigWriteECAM(
    CONST PCI_LOCATION *pLocation,
    UINT32 uOffset,
    UINT32 uSize,
    UINT32 uValue
    )
{
    UINT64 uAddress;
    volatile UINT8 *pConfig;

    if (uOffset >= 4096) {
        return IO_BAD_ARGUMENT;
    }

    // Calculate ECAM address
    uAddress = g_PCISubsystem.ECAMBase +
               ((UINT64)pLocation->Bus << 20) +
               ((UINT64)pLocation->Device << 15) +
               ((UINT64)pLocation->Function << 12) +
               uOffset;

    pConfig = (volatile UINT8 *)uAddress;

    // Write based on size
    switch (uSize) {
        case 1:
            *pConfig = (UINT8)uValue;
            break;
        case 2:
            if (uOffset & 1) {
                return IO_NOT_ALIGNED;
            }
            *(volatile UINT16 *)pConfig = (UINT16)uValue;
            break;
        case 4:
            if (uOffset & 3) {
                return IO_NOT_ALIGNED;
            }
            *(volatile UINT32 *)pConfig = uValue;
            break;
        default:
            return IO_BAD_ARGUMENT;
    }

    return IO_SUCCESS;
}

/**
 * @brief Detect PCI configuration mechanism
 *
 * Detects whether the system supports:
 * - PCIe ECAM (preferred)
 * - PCI Configuration Mechanism #1 (legacy PCI/PCI-X)
 * - PCI Configuration Mechanism #2 (very old systems)
 *
 * @retval IO_SUCCESS   Detection successful
 */
static IO_RETURN
PCIDetectConfigMechanism(
    VOID
    )
{
    UINT32 uOldValue, uTestValue;

    // Try to detect PCIe ECAM first
    // TODO: Parse ACPI MCFG table to get actual ECAM base address
    // For now, use default address and probe

    // Try mechanism #1 (most common for PCI/PCI-X/PCI-64)
    outl(PCI_CONFIG_ADDRESS, 0x80000000);
    uOldValue = inl(PCI_CONFIG_ADDRESS);

    if ((uOldValue & 0x80000000) == 0x80000000) {
        // Mechanism #1 is supported
        g_PCISubsystem.ConfigMechanism = PCI_CONFIG_MECHANISM_1;
        printf("PCI: Using Configuration Mechanism #1 (supports PCI/PCI-X/PCI-64)\n");
        return IO_SUCCESS;
    }

    // TODO: Detect mechanism #2 for very old systems

    printf("PCI: No supported configuration mechanism found\n");
    return IO_ERROR;
}

/**
 * @brief Read BAR value and decode information
 *
 * Reads a BAR register and decodes its type, size, and address.
 * Supports both 32-bit and 64-bit BARs.
 *
 * @param pDevice       PCI device implementation
 * @param uBARIndex     BAR index (0-5)
 * @param pBAR          Receives BAR information
 *
 * @retval IO_SUCCESS   BAR decoded successfully
 */
static IO_RETURN
PCIDecodeBAR(
    PCI_DEVICE_IMPL *pDevice,
    UINT32 uBARIndex,
    PCI_BAR *pBAR
    )
{
    UINT32 uBAROffset;
    UINT32 uBARValue, uBARSize;
    UINT32 uBARHigh = 0;
    IO_RETURN Status;

    if (uBARIndex >= PCI_MAX_BARS) {
        return IO_BAD_ARGUMENT;
    }

    uBAROffset = PCI_CFG_BAR0 + (uBARIndex * 4);

    // Read current BAR value
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset, 4, &uBARValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }

    // Empty BAR
    if (uBARValue == 0) {
        memset(pBAR, 0, sizeof(PCI_BAR));
        return IO_SUCCESS;
    }

    // Decode BAR type
    if (uBARValue & 0x1) {
        // I/O space BAR
        pBAR->bIsMem = FALSE;
        pBAR->bIs64Bit = FALSE;
        pBAR->bIsPrefetchable = FALSE;
        pBAR->PhysicalAddress = uBARValue & 0xFFFFFFFC;

        // Determine size
        PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, 0xFFFFFFFF);
        PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset, 4, &uBARSize);
        PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, uBARValue);

        pBAR->Size = ~(uBARSize & 0xFFFFFFFC) + 1;
    } else {
        // Memory space BAR
        pBAR->bIsMem = TRUE;
        pBAR->bIsPrefetchable = (uBARValue & 0x8) ? TRUE : FALSE;

        UINT32 uType = (uBARValue >> 1) & 0x3;

        if (uType == 0) {
            // 32-bit BAR (Legacy PCI, PCI-X, PCI-64)
            pBAR->bIs64Bit = FALSE;
            pBAR->PhysicalAddress = uBARValue & 0xFFFFFFF0;

            // Determine size
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, 0xFFFFFFFF);
            PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset, 4, &uBARSize);
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, uBARValue);

            pBAR->Size = ~(uBARSize & 0xFFFFFFF0) + 1;
        } else if (uType == 2) {
            // 64-bit BAR (PCIe and PCI-X 2.0)
            pBAR->bIs64Bit = TRUE;

            // Read high 32 bits
            if (uBARIndex < PCI_MAX_BARS - 1) {
                PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset + 4, 4, &uBARHigh);
            }

            pBAR->PhysicalAddress = ((UINT64)uBARHigh << 32) | (uBARValue & 0xFFFFFFF0);

            // Determine size
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, 0xFFFFFFFF);
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset + 4, 4, 0xFFFFFFFF);

            UINT32 uSizeLow, uSizeHigh;
            PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset, 4, &uSizeLow);
            PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, uBAROffset + 4, 4, &uSizeHigh);

            // Restore original values
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset, 4, uBARValue);
            PCIDevice_ConfigWrite((IIOPCIDevice *)pDevice, uBAROffset + 4, 4, uBARHigh);

            UINT64 uSize64 = ((UINT64)uSizeHigh << 32) | (uSizeLow & 0xFFFFFFF0);
            pBAR->Size = ~uSize64 + 1;
        } else {
            // Reserved type
            memset(pBAR, 0, sizeof(PCI_BAR));
            return IO_UNSUPPORTED;
        }
    }

    return IO_SUCCESS;
}

/**
 * @brief Probe PCI device
 *
 * Reads device identification and configuration.
 *
 * @param pDevice       PCI device implementation
 *
 * @retval IO_SUCCESS   Device probed successfully
 */
static IO_RETURN
PCIProbeDevice(
    PCI_DEVICE_IMPL *pDevice
    )
{
    IO_RETURN Status;
    UINT32 uValue;
    UINT32 i;

    // Read vendor ID
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_VENDOR_ID, 2, &uValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }
    pDevice->DeviceInfo.VendorID = (UINT16)uValue;

    // Check if device exists
    if (pDevice->DeviceInfo.VendorID == 0xFFFF) {
        return IO_NO_DEVICE;
    }

    // Read device ID
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_DEVICE_ID, 2, &uValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }
    pDevice->DeviceInfo.DeviceID = (UINT16)uValue;

    // Read class code
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_CLASS_CODE, 1, &uValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }
    pDevice->DeviceInfo.ClassCode = (UINT8)(uValue >> 16);
    pDevice->DeviceInfo.SubClass = (UINT8)(uValue >> 8);
    pDevice->DeviceInfo.ProgIf = (UINT8)uValue;

    // Read revision ID
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_REVISION_ID, 1, &uValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }
    pDevice->DeviceInfo.RevisionID = (UINT8)uValue;

    // Read header type
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_HEADER_TYPE, 1, &uValue);
    if (Status != IO_SUCCESS) {
        return Status;
    }
    pDevice->DeviceInfo.HeaderType = (UINT8)uValue;

    // Read subsystem IDs
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_SUBSYS_VENDOR_ID, 2, &uValue);
    if (Status == IO_SUCCESS) {
        pDevice->DeviceInfo.SubsysVendorID = (UINT16)uValue;
    }

    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_SUBSYS_DEVICE_ID, 2, &uValue);
    if (Status == IO_SUCCESS) {
        pDevice->DeviceInfo.SubsysDeviceID = (UINT16)uValue;
    }

    // Read interrupt configuration
    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_INTERRUPT_LINE, 1, &uValue);
    if (Status == IO_SUCCESS) {
        pDevice->DeviceInfo.InterruptLine = (UINT8)uValue;
    }

    Status = PCIDevice_ConfigRead((IIOPCIDevice *)pDevice, PCI_CFG_INTERRUPT_PIN, 1, &uValue);
    if (Status == IO_SUCCESS) {
        pDevice->DeviceInfo.InterruptPin = (UINT8)uValue;
    }

    // Decode all BARs
    for (i = 0; i < PCI_MAX_BARS; i++) {
        PCIDecodeBAR(pDevice, i, &pDevice->DeviceInfo.BARs[i]);

        // Skip next BAR if this is a 64-bit BAR
        if (pDevice->DeviceInfo.BARs[i].bIs64Bit) {
            i++;
        }
    }

    printf("PCI: Found device %04X:%04X at %02X:%02X.%X (Class %02X:%02X)\n",
           pDevice->DeviceInfo.VendorID,
           pDevice->DeviceInfo.DeviceID,
           pDevice->DeviceInfo.Location.Bus,
           pDevice->DeviceInfo.Location.Device,
           pDevice->DeviceInfo.Location.Function,
           pDevice->DeviceInfo.ClassCode,
           pDevice->DeviceInfo.SubClass);

    return IO_SUCCESS;
}

// Implementation continues in next part...
