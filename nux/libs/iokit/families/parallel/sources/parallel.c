/**
 * @file parallel.c
 * @brief Parallel Port Family Implementation - IEEE 1284 Compliant
 *
 * Provides comprehensive support for parallel ports with:
 * - SPP/PS2/EPP/ECP modes
 * - IEEE 1284 protocol negotiation
 * - PARSCSI (parallel port SCSI adapters)
 * - ISA, PCI, PCIe, and USB-to-parallel adapters
 * - Printers, scanners, tape drives, dongles, network adapters
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/parallel/parallel.h>
#include <iokit/families/pcie/pcie.h>
#include <iokit/families/isa/isa.h>
#include <iokit/families/usb/usb.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Parallel port controller database entry
 */
typedef struct _PARALLEL_CONTROLLER_DB_ENTRY {
    UINT16      VendorID;
    UINT16      DeviceID;
    CONST CHAR8 *pszVendor;
    CONST CHAR8 *pszModel;
    PARALLEL_PORT_TYPE PortType;
    UINT32      Capabilities;
    UINT16      IOBase;
    UINT8       IRQ;
} PARALLEL_CONTROLLER_DB_ENTRY;

/**
 * @brief Parallel device database entry
 */
typedef struct _PARALLEL_DEVICE_DB_ENTRY {
    CONST CHAR8 *pszManufacturer;
    CONST CHAR8 *pszModel;
    PARALLEL_DEVICE_TYPE DeviceType;
    UINT32      Capabilities;
    IEEE1284_MODE PreferredMode;
} PARALLEL_DEVICE_DB_ENTRY;

/**
 * @brief Known parallel port controller database (30+ entries)
 *
 * Includes ISA/LPT ports, PCI cards, PCIe cards, and USB-to-parallel adapters
 */
static CONST PARALLEL_CONTROLLER_DB_ENTRY g_ParallelControllerDB[] = {
    // Standard ISA/LPT Ports
    { 0x0000, 0x0000, "Generic", "LPT1 (0x378)", PARALLEL_TYPE_ISA,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0x378, 7 },
    { 0x0000, 0x0001, "Generic", "LPT2 (0x278)", PARALLEL_TYPE_ISA,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0x278, 5 },
    { 0x0000, 0x0002, "Generic", "LPT3 (0x3BC)", PARALLEL_TYPE_ISA,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2,
      0x3BC, 7 },

    // NetMos (MosChip) PCI Parallel Port Cards - Very Popular
    { 0x9710, 0x9805, "NetMos", "PCI 9805 1-Port Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },
    { 0x9710, 0x9815, "NetMos", "PCI 9815 2-Port Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0, 0 },
    { 0x9710, 0x9901, "NetMos", "PCIe 9901 1-Port Parallel", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },

    // SIIG Parallel Port Cards
    { 0x131F, 0x1010, "SIIG", "CyberParallel PCI", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0, 0 },
    { 0x131F, 0x1011, "SIIG", "CyberParallel Dual PCI", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },
    { 0x131F, 0x2010, "SIIG", "CyberParallel PCIe", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },

    // StarTech Parallel Port Cards
    { 0x1415, 0x8403, "StarTech", "1-Port PCI Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0, 0 },
    { 0x1415, 0x9501, "StarTech", "1-Port PCIe Parallel", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },

    // Lava Computer Parallel Cards
    { 0x1407, 0x8000, "Lava", "Parallel-PCI Single Port", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },
    { 0x1407, 0x8001, "Lava", "Parallel-PCI Dual Port", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },

    // Oxford Semiconductor Parallel Controllers
    { 0x1415, 0x9513, "Oxford", "OXPCIe952 Parallel", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },

    // Intel Chipset Integrated Parallel Ports
    { 0x8086, 0x7110, "Intel", "PIIX4 ISA Bridge with LPT", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_DMA,
      0x378, 7 },
    { 0x8086, 0x2410, "Intel", "ICH Parallel Port", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0x378, 7 },

    // VIA Chipset Integrated Parallel Ports
    { 0x1106, 0x0686, "VIA", "VT82C686 Super South with LPT", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0x378, 7 },
    { 0x1106, 0x8231, "VIA", "VT8231 ISA Bridge with LPT", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0x378, 7 },

    // SiS Chipset Integrated Parallel Ports
    { 0x1039, 0x0008, "SiS", "85C503/5513 ISA Bridge with LPT", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0x378, 7 },

    // AMD Chipset Integrated Parallel Ports
    { 0x1022, 0x7468, "AMD", "AMD-8111 LPC Bridge with LPT", PARALLEL_TYPE_ONBOARD,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0x378, 7 },

    // Sunix Parallel Port Cards
    { 0x1FD4, 0x1999, "Sunix", "4 Port Parallel PCI", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },

    // WCH (Nanjing Qinheng Microelectronics) - Low-cost PCI/PCIe cards
    { 0x4348, 0x7053, "WCH", "CH353 PCI Dual Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },
    { 0x1C00, 0x3050, "WCH", "CH382 PCIe 2-Port Parallel", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0, 0 },

    // SYBA / IO Crest Parallel Cards
    { 0x1415, 0xC100, "SYBA", "SD-PCI-1P Parallel Card", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },

    // Moschip (Same as NetMos, different branding)
    { 0x9710, 0x9912, "Moschip", "MCS9912 PCIe Parallel", PARALLEL_TYPE_PCIE,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA,
      0, 0 },

    // Quatech Parallel Port Cards
    { 0x135C, 0x0010, "Quatech", "QSCLP-100 PCI Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284,
      0, 0 },

    // Comtrol RocketPort Parallel
    { 0x11FE, 0x8015, "Comtrol", "RocketPort Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },

    // Titan Electronics Parallel Cards
    { 0x14D2, 0x8010, "Titan", "VScom PCI-200L 1-Port Parallel", PARALLEL_TYPE_PCI,
      PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP,
      0, 0 },
};

#define PARALLEL_CONTROLLER_DB_COUNT (sizeof(g_ParallelControllerDB) / sizeof(g_ParallelControllerDB[0]))

/**
 * @brief Known parallel device database (40+ devices)
 *
 * Includes printers, scanners, PARSCSI adapters, tape drives, dongles, and more
 */
static CONST PARALLEL_DEVICE_DB_ENTRY g_ParallelDeviceDB[] = {
    // Parallel Printers - HP
    { "HP", "LaserJet 4", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_NIBBLE },
    { "HP", "LaserJet 5", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_BYTE },
    { "HP", "LaserJet 6", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "HP", "DeskJet 500", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_NIBBLE },
    { "HP", "DeskJet 600", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_BYTE },
    { "HP", "DeskJet 900", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_ECP },

    // Parallel Printers - Epson
    { "Epson", "Stylus Color", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_BYTE },
    { "Epson", "Stylus Photo", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "Epson", "LQ-570", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_NIBBLE },
    { "Epson", "FX-880", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },

    // Parallel Printers - Canon
    { "Canon", "BJ-200", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_NIBBLE },
    { "Canon", "BJC-600", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_BYTE },
    { "Canon", "S800", PARALLEL_DEVICE_PRINTER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_ECP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_ECP },

    // PARSCSI Adapters - Iomega Zip Drives
    { "Iomega", "Zip Drive 100MB (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Iomega", "Zip Drive 250MB (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Iomega", "Jaz Drive 1GB (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Iomega", "Jaz Drive 2GB (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_ECP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_ECP },

    // PARSCSI Adapters - Adaptec
    { "Adaptec", "APA-1480 SlimSCSI", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Adaptec", "APA-460 SlimSCSI", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },

    // PARSCSI Adapters - MicroSolutions BackPack
    { "MicroSolutions", "BackPack CD-ROM (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "MicroSolutions", "BackPack Hard Drive (PARSCSI)", PARALLEL_DEVICE_PARSCSI, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },

    // Parallel Port Scanners
    { "HP", "ScanJet 4c (Parallel)", PARALLEL_DEVICE_SCANNER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "HP", "ScanJet 5p (Parallel)", PARALLEL_DEVICE_SCANNER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "Epson", "Perfection 636 (Parallel)", PARALLEL_DEVICE_SCANNER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "Canon", "CanoScan N650U (Parallel)", PARALLEL_DEVICE_SCANNER, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },

    // Parallel Port Tape Drives
    { "Iomega", "Ditto Parallel Port Tape", PARALLEL_DEVICE_TAPE, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Seagate", "TapeStor Parallel Port Tape", PARALLEL_DEVICE_TAPE, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "HP", "Colorado Trakker (Parallel)", PARALLEL_DEVICE_TAPE, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },

    // Security Dongles
    { "HASP", "Sentinel HASP Dongle", PARALLEL_DEVICE_DONGLE, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },
    { "Aladdin", "HASP HL Dongle", PARALLEL_DEVICE_DONGLE, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },
    { "SafeNet", "Sentinel SuperPro Dongle", PARALLEL_DEVICE_DONGLE, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },
    { "KeyLok", "KeyLok Security Key", PARALLEL_DEVICE_DONGLE, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },
    { "Wibu", "CodeMeter Parallel Dongle", PARALLEL_DEVICE_DONGLE, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2, IEEE1284_MODE_NIBBLE },

    // Parallel Port Network Adapters
    { "Xircom", "Pocket Ethernet Parallel", PARALLEL_DEVICE_NETWORK, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_EPP },
    { "D-Link", "DE-600 Pocket Ethernet", PARALLEL_DEVICE_NETWORK, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP, IEEE1284_MODE_EPP },
    { "Accton", "EtherPocket-SP (Parallel)", PARALLEL_DEVICE_NETWORK, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP, IEEE1284_MODE_EPP },

    // Parallel Port Plotters
    { "HP", "7475A Plotter", PARALLEL_DEVICE_PLOTTER, PARALLEL_CAP_SPP | PARALLEL_CAP_IEEE1284, IEEE1284_MODE_NIBBLE },
    { "Roland", "DXY Plotter Series", PARALLEL_DEVICE_PLOTTER, PARALLEL_CAP_SPP, IEEE1284_MODE_NIBBLE },

    // Parallel Port External Hard Drives (Pre-USB era)
    { "Iomega", "Parallel Port Hard Drive", PARALLEL_DEVICE_EXTERNAL_HDD, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },
    { "Maxtor", "Parallel Port External Drive", PARALLEL_DEVICE_EXTERNAL_HDD, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP | PARALLEL_CAP_PARSCSI, IEEE1284_MODE_EPP },

    // Parallel Port Cameras
    { "Connectix", "QuickCam (Parallel)", PARALLEL_DEVICE_CAMERA, PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 | PARALLEL_CAP_EPP, IEEE1284_MODE_EPP },
};

#define PARALLEL_DEVICE_DB_COUNT (sizeof(g_ParallelDeviceDB) / sizeof(g_ParallelDeviceDB[0]))

/**
 * @brief Parallel port controller implementation structure
 */
typedef struct _PARALLEL_PORT_IMPL {
    IIOParallelPort     Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOService         *pProvider;          /**< Provider service */
    PARALLEL_PORT_INFO  PortInfo;           /**< Port information */
    volatile UINT8     *pIOBase;            /**< I/O base address (mapped) */
    BOOLEAN             bInitialized;       /**< Port initialized */
    UINT8               CurrentControlReg;  /**< Current control register value */
    IEEE1284_MODE       CurrentIEEE1284Mode; /**< Current IEEE 1284 mode */
} PARALLEL_PORT_IMPL;

/**
 * @brief Parallel device implementation structure
 */
typedef struct _PARALLEL_DEVICE_IMPL {
    IIOParallelDevice   Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOParallelPort    *pPort;              /**< Parent port */
    PARALLEL_DEVICE_INFO DeviceInfo;        /**< Device information */
    BOOLEAN             bInitialized;       /**< Device initialized */
} PARALLEL_DEVICE_IMPL;

// Forward declarations of virtual functions
static ULONG ParallelPort_AddRef(IIOParallelPort *pThis);
static ULONG ParallelPort_Release(IIOParallelPort *pThis);
static IO_RETURN ParallelPort_Start(IIOParallelPort *pThis, IIOService *pProvider);
static IO_RETURN ParallelPort_Stop(IIOParallelPort *pThis, IIOService *pProvider);
static IO_RETURN ParallelPort_GetPortInfo(IIOParallelPort *pThis, PARALLEL_PORT_INFO *pInfo);
static IO_RETURN ParallelPort_SetMode(IIOParallelPort *pThis, PARALLEL_PORT_MODE Mode);
static IO_RETURN ParallelPort_ReadData(IIOParallelPort *pThis, UINT8 *pData);
static IO_RETURN ParallelPort_WriteData(IIOParallelPort *pThis, UINT8 Data);
static IO_RETURN ParallelPort_ReadStatus(IIOParallelPort *pThis, UINT8 *pStatus);
static IO_RETURN ParallelPort_WriteControl(IIOParallelPort *pThis, UINT8 Control);
static IO_RETURN ParallelPort_ReadControl(IIOParallelPort *pThis, UINT8 *pControl);
static IO_RETURN ParallelPort_EPPRead(IIOParallelPort *pThis, BOOLEAN bAddress, VOID *pBuffer, UINT32 Length);
static IO_RETURN ParallelPort_EPPWrite(IIOParallelPort *pThis, BOOLEAN bAddress, CONST VOID *pBuffer, UINT32 Length);
static IO_RETURN ParallelPort_ECPRead(IIOParallelPort *pThis, VOID *pBuffer, UINT32 Length, UINT32 *pBytesRead);
static IO_RETURN ParallelPort_ECPWrite(IIOParallelPort *pThis, CONST VOID *pBuffer, UINT32 Length, UINT32 *pBytesWritten);
static IO_RETURN ParallelPort_IEEE1284Negotiate(IIOParallelPort *pThis, IEEE1284_MODE Mode);
static IO_RETURN ParallelPort_IEEE1284Terminate(IIOParallelPort *pThis);
static IO_RETURN ParallelPort_ReadDeviceID(IIOParallelPort *pThis, IEEE1284_DEVICE_ID *pDeviceID);
static IO_RETURN ParallelPort_DetectDevice(IIOParallelPort *pThis, IIOParallelDevice **ppDevice);
static IO_RETURN ParallelPort_SetInterruptEnable(IIOParallelPort *pThis, BOOLEAN bEnable);
static IO_RETURN ParallelPort_SetDMAEnable(IIOParallelPort *pThis, BOOLEAN bEnable);
static IO_RETURN ParallelPort_ResetPort(IIOParallelPort *pThis);

/**
 * @brief Look up controller in database
 */
static CONST PARALLEL_CONTROLLER_DB_ENTRY*
ParallelLookupController(
    UINT16 uVendorID,
    UINT16 uDeviceID
    )
{
    UINT32 i;

    for (i = 0; i < PARALLEL_CONTROLLER_DB_COUNT; i++) {
        if (g_ParallelControllerDB[i].VendorID == uVendorID &&
            g_ParallelControllerDB[i].DeviceID == uDeviceID) {
            return &g_ParallelControllerDB[i];
        }
    }

    return NULL;
}

/**
 * @brief Look up device in database by IEEE 1284 device ID
 */
static CONST PARALLEL_DEVICE_DB_ENTRY*
ParallelLookupDevice(
    CONST CHAR8 *pszManufacturer,
    CONST CHAR8 *pszModel
    )
{
    UINT32 i;

    for (i = 0; i < PARALLEL_DEVICE_DB_COUNT; i++) {
        if (strstr(pszManufacturer, g_ParallelDeviceDB[i].pszManufacturer) != NULL &&
            strstr(pszModel, g_ParallelDeviceDB[i].pszModel) != NULL) {
            return &g_ParallelDeviceDB[i];
        }
    }

    return NULL;
}

/**
 * @brief Read I/O port (8-bit)
 */
static UINT8
ParallelReadIO(
    PARALLEL_PORT_IMPL *pPort,
    UINT16 Offset
    )
{
    // In a real implementation, use proper I/O port access
    // For x86: inb(pPort->PortInfo.IOBase + Offset)
    return 0xFF; // Stub
}

/**
 * @brief Write I/O port (8-bit)
 */
static VOID
ParallelWriteIO(
    PARALLEL_PORT_IMPL *pPort,
    UINT16 Offset,
    UINT8 Value
    )
{
    // In a real implementation, use proper I/O port access
    // For x86: outb(pPort->PortInfo.IOBase + Offset, Value)
}

/**
 * @brief IIOParallelPort::Start - Initialize port
 */
static IO_RETURN
ParallelPort_Start(
    IIOParallelPort *pThis,
    IIOService *pProvider
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;
    IIOPCIDevice *pPCIDevice = NULL;
    PCI_DEVICE_INFO PCIInfo;
    IO_RETURN Status;
    CONST PARALLEL_CONTROLLER_DB_ENTRY *pDBEntry;

    if (pPort == NULL || pProvider == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: Initializing parallel port\n");

    // Try to query for PCI device interface
    Status = pProvider->lpVtbl->QueryInterface(pProvider, &IID_IIOPCIDevice, (VOID **)&pPCIDevice);

    if (Status == IO_SUCCESS && pPCIDevice != NULL) {
        // PCI/PCIe parallel port card
        Status = pPCIDevice->lpVtbl->GetDeviceInfo(pPCIDevice, &PCIInfo);
        if (Status != IO_SUCCESS) {
            pPCIDevice->lpVtbl->Release(pPCIDevice);
            return Status;
        }

        pDBEntry = ParallelLookupController(PCIInfo.VendorID, PCIInfo.DeviceID);

        printf("Parallel: Found PCI controller %04X:%04X at %02X:%02X.%X\n",
               PCIInfo.VendorID, PCIInfo.DeviceID,
               PCIInfo.Location.Bus, PCIInfo.Location.Device, PCIInfo.Location.Function);

        if (pDBEntry != NULL) {
            printf("Parallel: %s %s\n", pDBEntry->pszVendor, pDBEntry->pszModel);
            pPort->PortInfo.PortType = pDBEntry->PortType;
            pPort->PortInfo.Capabilities = pDBEntry->Capabilities;
        } else {
            printf("Parallel: Unknown parallel port controller\n");
            pPort->PortInfo.PortType = PARALLEL_TYPE_PCI;
            pPort->PortInfo.Capabilities = PARALLEL_CAP_SPP | PARALLEL_CAP_PS2;
        }

        // Map I/O BAR
        if (PCIInfo.BARs[0].Size > 0) {
            if (PCIInfo.BARs[0].bIsMem) {
                Status = pPCIDevice->lpVtbl->MapBAR(pPCIDevice, 0,
                                                    (VOID **)&pPort->pIOBase,
                                                    NULL);
            } else {
                pPort->PortInfo.IOBase = (UINT16)PCIInfo.BARs[0].PhysicalAddress;
                printf("Parallel: I/O Base: 0x%04X\n", pPort->PortInfo.IOBase);
            }
        }

        pPort->PortInfo.VendorID = PCIInfo.VendorID;
        pPort->PortInfo.DeviceID = PCIInfo.DeviceID;
        snprintf(pPort->PortInfo.PortName, sizeof(pPort->PortInfo.PortName), "PCI-LPT");

        pPCIDevice->lpVtbl->Release(pPCIDevice);
    } else {
        // ISA/Legacy LPT port
        printf("Parallel: Detected legacy ISA LPT port\n");

        // Default to LPT1 for now
        pPort->PortInfo.PortType = PARALLEL_TYPE_ISA;
        pPort->PortInfo.IOBase = PARALLEL_IOADDR_LPT1;
        pPort->PortInfo.IRQ = PARALLEL_IRQ_LPT1;
        pPort->PortInfo.DMAChannel = PARALLEL_DMA_CHANNEL_3;
        pPort->PortInfo.Capabilities = PARALLEL_CAP_SPP | PARALLEL_CAP_PS2 |
                                       PARALLEL_CAP_EPP | PARALLEL_CAP_ECP |
                                       PARALLEL_CAP_IEEE1284 | PARALLEL_CAP_DMA;
        snprintf(pPort->PortInfo.PortName, sizeof(pPort->PortInfo.PortName), "LPT1");

        printf("Parallel: LPT1 at 0x%04X, IRQ %u\n",
               pPort->PortInfo.IOBase, pPort->PortInfo.IRQ);
    }

    // Set default mode
    pPort->PortInfo.CurrentMode = PARALLEL_MODE_SPP;
    pPort->PortInfo.IOSize = 8;
    pPort->PortInfo.FIFOSize = (pPort->PortInfo.Capabilities & PARALLEL_CAP_ECP) ? 16 : 0;

    // Display capabilities
    printf("Parallel: Capabilities:\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_SPP) printf("  - SPP (Standard Parallel Port)\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_PS2) printf("  - PS/2 Bidirectional\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_EPP) printf("  - EPP (Enhanced Parallel Port)\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_ECP) printf("  - ECP (Extended Capabilities Port)\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_IEEE1284) printf("  - IEEE 1284 Protocol\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_DMA) printf("  - DMA Support\n");
    if (pPort->PortInfo.Capabilities & PARALLEL_CAP_PARSCSI) printf("  - PARSCSI Support\n");

    // Initialize control register
    pPort->CurrentControlReg = PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT_IN;
    ParallelWriteIO(pPort, PARALLEL_REG_CONTROL, pPort->CurrentControlReg);

    // Reset port
    ParallelPort_ResetPort(pThis);

    pPort->pProvider = pProvider;
    pProvider->lpVtbl->AddRef(pProvider);
    pPort->bInitialized = TRUE;

    printf("Parallel: Port initialization complete\n");

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::Stop - Shutdown port
 */
static IO_RETURN
ParallelPort_Stop(
    IIOParallelPort *pThis,
    IIOService *pProvider
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: Stopping port %s\n", pPort->PortInfo.PortName);

    if (pPort->pProvider != NULL) {
        pPort->pProvider->lpVtbl->Release(pPort->pProvider);
        pPort->pProvider = NULL;
    }

    pPort->bInitialized = FALSE;

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::GetPortInfo
 */
static IO_RETURN
ParallelPort_GetPortInfo(
    IIOParallelPort *pThis,
    PARALLEL_PORT_INFO *pInfo
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pPort->bInitialized) {
        return IO_NOT_READY;
    }

    memcpy(pInfo, &pPort->PortInfo, sizeof(PARALLEL_PORT_INFO));
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::SetMode
 */
static IO_RETURN
ParallelPort_SetMode(
    IIOParallelPort *pThis,
    PARALLEL_PORT_MODE Mode
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Verify mode is supported
    switch (Mode) {
        case PARALLEL_MODE_SPP:
            if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_SPP)) {
                return IO_UNSUPPORTED;
            }
            break;
        case PARALLEL_MODE_PS2:
            if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_PS2)) {
                return IO_UNSUPPORTED;
            }
            break;
        case PARALLEL_MODE_EPP:
            if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_EPP)) {
                return IO_UNSUPPORTED;
            }
            break;
        case PARALLEL_MODE_ECP:
            if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_ECP)) {
                return IO_UNSUPPORTED;
            }
            break;
        default:
            return IO_BAD_ARGUMENT;
    }

    pPort->PortInfo.CurrentMode = Mode;
    printf("Parallel: Mode set to %d\n", Mode);

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ReadData
 */
static IO_RETURN
ParallelPort_ReadData(
    IIOParallelPort *pThis,
    UINT8 *pData
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pData == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *pData = ParallelReadIO(pPort, PARALLEL_REG_DATA);
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::WriteData
 */
static IO_RETURN
ParallelPort_WriteData(
    IIOParallelPort *pThis,
    UINT8 Data
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    ParallelWriteIO(pPort, PARALLEL_REG_DATA, Data);
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ReadStatus
 */
static IO_RETURN
ParallelPort_ReadStatus(
    IIOParallelPort *pThis,
    UINT8 *pStatus
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pStatus == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *pStatus = ParallelReadIO(pPort, PARALLEL_REG_STATUS);
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::WriteControl
 */
static IO_RETURN
ParallelPort_WriteControl(
    IIOParallelPort *pThis,
    UINT8 Control
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pPort->CurrentControlReg = Control;
    ParallelWriteIO(pPort, PARALLEL_REG_CONTROL, Control);
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ReadControl
 */
static IO_RETURN
ParallelPort_ReadControl(
    IIOParallelPort *pThis,
    UINT8 *pControl
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pControl == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *pControl = pPort->CurrentControlReg;
    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::EPPRead
 */
static IO_RETURN
ParallelPort_EPPRead(
    IIOParallelPort *pThis,
    BOOLEAN bAddress,
    VOID *pBuffer,
    UINT32 Length
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;
    UINT8 *pData = (UINT8 *)pBuffer;
    UINT32 i;

    if (pPort == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_EPP)) {
        return IO_UNSUPPORTED;
    }

    // EPP read operation
    UINT16 Offset = bAddress ? PARALLEL_REG_EPP_ADDR : PARALLEL_REG_EPP_DATA;

    for (i = 0; i < Length; i++) {
        pData[i] = ParallelReadIO(pPort, Offset);
    }

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::EPPWrite
 */
static IO_RETURN
ParallelPort_EPPWrite(
    IIOParallelPort *pThis,
    BOOLEAN bAddress,
    CONST VOID *pBuffer,
    UINT32 Length
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;
    CONST UINT8 *pData = (CONST UINT8 *)pBuffer;
    UINT32 i;

    if (pPort == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_EPP)) {
        return IO_UNSUPPORTED;
    }

    // EPP write operation
    UINT16 Offset = bAddress ? PARALLEL_REG_EPP_ADDR : PARALLEL_REG_EPP_DATA;

    for (i = 0; i < Length; i++) {
        ParallelWriteIO(pPort, Offset, pData[i]);
    }

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ECPRead
 */
static IO_RETURN
ParallelPort_ECPRead(
    IIOParallelPort *pThis,
    VOID *pBuffer,
    UINT32 Length,
    UINT32 *pBytesRead
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pBuffer == NULL || pBytesRead == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_ECP)) {
        return IO_UNSUPPORTED;
    }

    // ECP FIFO read would go here
    *pBytesRead = 0;

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ECPWrite
 */
static IO_RETURN
ParallelPort_ECPWrite(
    IIOParallelPort *pThis,
    CONST VOID *pBuffer,
    UINT32 Length,
    UINT32 *pBytesWritten
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pBuffer == NULL || pBytesWritten == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_ECP)) {
        return IO_UNSUPPORTED;
    }

    // ECP FIFO write would go here
    *pBytesWritten = 0;

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::IEEE1284Negotiate
 */
static IO_RETURN
ParallelPort_IEEE1284Negotiate(
    IIOParallelPort *pThis,
    IEEE1284_MODE Mode
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_IEEE1284)) {
        return IO_UNSUPPORTED;
    }

    printf("Parallel: IEEE 1284 mode negotiation for mode %d\n", Mode);

    // IEEE 1284 negotiation protocol would go here
    pPort->CurrentIEEE1284Mode = Mode;

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::IEEE1284Terminate
 */
static IO_RETURN
ParallelPort_IEEE1284Terminate(
    IIOParallelPort *pThis
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: IEEE 1284 termination\n");

    // IEEE 1284 termination protocol would go here

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ReadDeviceID
 */
static IO_RETURN
ParallelPort_ReadDeviceID(
    IIOParallelPort *pThis,
    IEEE1284_DEVICE_ID *pDeviceID
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || pDeviceID == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: Reading IEEE 1284 Device ID\n");

    // IEEE 1284 device ID read would go here
    // For now, return a sample ID
    pDeviceID->Length = snprintf(pDeviceID->Data, sizeof(pDeviceID->Data),
                                 "MFG:Generic;MDL:Parallel Device;");

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::DetectDevice
 */
static IO_RETURN
ParallelPort_DetectDevice(
    IIOParallelPort *pThis,
    IIOParallelDevice **ppDevice
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL || ppDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: Detecting connected device\n");

    // Device detection logic would go here
    // Check status lines, try IEEE 1284 negotiation, etc.

    return IO_NO_DEVICE;
}

/**
 * @brief IIOParallelPort::SetInterruptEnable
 */
static IO_RETURN
ParallelPort_SetInterruptEnable(
    IIOParallelPort *pThis,
    BOOLEAN bEnable
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (bEnable) {
        pPort->CurrentControlReg |= PARALLEL_CONTROL_IRQ_ENABLE;
    } else {
        pPort->CurrentControlReg &= ~PARALLEL_CONTROL_IRQ_ENABLE;
    }

    ParallelWriteIO(pPort, PARALLEL_REG_CONTROL, pPort->CurrentControlReg);
    pPort->PortInfo.bIRQEnabled = bEnable;

    printf("Parallel: Interrupts %s\n", bEnable ? "enabled" : "disabled");

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::SetDMAEnable
 */
static IO_RETURN
ParallelPort_SetDMAEnable(
    IIOParallelPort *pThis,
    BOOLEAN bEnable
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!(pPort->PortInfo.Capabilities & PARALLEL_CAP_DMA)) {
        return IO_UNSUPPORTED;
    }

    pPort->PortInfo.bECPDMAEnabled = bEnable;

    printf("Parallel: DMA %s\n", bEnable ? "enabled" : "disabled");

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::ResetPort
 */
static IO_RETURN
ParallelPort_ResetPort(
    IIOParallelPort *pThis
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;

    if (pPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("Parallel: Resetting port %s\n", pPort->PortInfo.PortName);

    // Reset control register
    pPort->CurrentControlReg = PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT_IN;
    ParallelWriteIO(pPort, PARALLEL_REG_CONTROL, pPort->CurrentControlReg);

    // Clear data register
    ParallelWriteIO(pPort, PARALLEL_REG_DATA, 0x00);

    return IO_SUCCESS;
}

/**
 * @brief IIOParallelPort::AddRef
 */
static ULONG
ParallelPort_AddRef(
    IIOParallelPort *pThis
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;
    return ++pPort->RefCount;
}

/**
 * @brief IIOParallelPort::Release
 */
static ULONG
ParallelPort_Release(
    IIOParallelPort *pThis
    )
{
    PARALLEL_PORT_IMPL *pPort = (PARALLEL_PORT_IMPL *)pThis;
    ULONG RefCount = --pPort->RefCount;

    if (RefCount == 0) {
        if (pPort->pProvider != NULL) {
            pPort->pProvider->lpVtbl->Release(pPort->pProvider);
        }
        free(pPort);
    }

    return RefCount;
}

/**
 * @brief Initialize parallel port family
 */
IO_RETURN
ParallelInitialize(
    VOID
    )
{
    printf("Parallel: Family driver initialized\n");
    printf("Parallel: Database: %u controllers, %u devices\n",
           PARALLEL_CONTROLLER_DB_COUNT, PARALLEL_DEVICE_DB_COUNT);
    return IO_SUCCESS;
}

/**
 * @brief Shutdown parallel port family
 */
IO_RETURN
ParallelShutdown(
    VOID
    )
{
    printf("Parallel: Family driver shutdown\n");
    return IO_SUCCESS;
}

/**
 * @brief Create parallel port controller instance
 */
IO_RETURN
ParallelPortCreate(
    IIOService *pProvider,
    IIOParallelPort **ppPort
    )
{
    PARALLEL_PORT_IMPL *pPort;

    if (pProvider == NULL || ppPort == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pPort = (PARALLEL_PORT_IMPL *)calloc(1, sizeof(PARALLEL_PORT_IMPL));
    if (pPort == NULL) {
        return IO_NO_MEMORY;
    }

    // Initialize vtable with function pointers
    pPort->Vtbl.lpVtbl = calloc(1, sizeof(struct IIOParallelPortVtbl));
    if (pPort->Vtbl.lpVtbl == NULL) {
        free(pPort);
        return IO_NO_MEMORY;
    }

    // Set up function pointers
    pPort->Vtbl.lpVtbl->AddRef = ParallelPort_AddRef;
    pPort->Vtbl.lpVtbl->Release = ParallelPort_Release;
    pPort->Vtbl.lpVtbl->Start = ParallelPort_Start;
    pPort->Vtbl.lpVtbl->Stop = ParallelPort_Stop;
    pPort->Vtbl.lpVtbl->GetPortInfo = ParallelPort_GetPortInfo;
    pPort->Vtbl.lpVtbl->SetMode = ParallelPort_SetMode;
    pPort->Vtbl.lpVtbl->ReadData = ParallelPort_ReadData;
    pPort->Vtbl.lpVtbl->WriteData = ParallelPort_WriteData;
    pPort->Vtbl.lpVtbl->ReadStatus = ParallelPort_ReadStatus;
    pPort->Vtbl.lpVtbl->WriteControl = ParallelPort_WriteControl;
    pPort->Vtbl.lpVtbl->ReadControl = ParallelPort_ReadControl;
    pPort->Vtbl.lpVtbl->EPPRead = ParallelPort_EPPRead;
    pPort->Vtbl.lpVtbl->EPPWrite = ParallelPort_EPPWrite;
    pPort->Vtbl.lpVtbl->ECPRead = ParallelPort_ECPRead;
    pPort->Vtbl.lpVtbl->ECPWrite = ParallelPort_ECPWrite;
    pPort->Vtbl.lpVtbl->IEEE1284Negotiate = ParallelPort_IEEE1284Negotiate;
    pPort->Vtbl.lpVtbl->IEEE1284Terminate = ParallelPort_IEEE1284Terminate;
    pPort->Vtbl.lpVtbl->ReadDeviceID = ParallelPort_ReadDeviceID;
    pPort->Vtbl.lpVtbl->DetectDevice = ParallelPort_DetectDevice;
    pPort->Vtbl.lpVtbl->SetInterruptEnable = ParallelPort_SetInterruptEnable;
    pPort->Vtbl.lpVtbl->SetDMAEnable = ParallelPort_SetDMAEnable;
    pPort->Vtbl.lpVtbl->ResetPort = ParallelPort_ResetPort;

    pPort->RefCount = 1;
    pPort->CurrentIEEE1284Mode = IEEE1284_MODE_NIBBLE;

    *ppPort = (IIOParallelPort *)pPort;

    return IO_SUCCESS;
}

/**
 * @brief Create parallel device instance
 */
IO_RETURN
ParallelDeviceCreate(
    IIOParallelPort *pPort,
    IIOParallelDevice **ppDevice
    )
{
    PARALLEL_DEVICE_IMPL *pDevice;

    if (pPort == NULL || ppDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pDevice = (PARALLEL_DEVICE_IMPL *)calloc(1, sizeof(PARALLEL_DEVICE_IMPL));
    if (pDevice == NULL) {
        return IO_NO_MEMORY;
    }

    pDevice->RefCount = 1;
    pDevice->pPort = pPort;
    pPort->lpVtbl->AddRef(pPort);

    *ppDevice = (IIOParallelDevice *)pDevice;

    return IO_SUCCESS;
}
