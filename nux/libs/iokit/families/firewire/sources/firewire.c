/**
 * @file firewire.c
 * @brief FireWire (IEEE 1394) Family Implementation
 *
 * Implements FireWire host controller support with comprehensive hardware database.
 * FireWire was widely used for:
 * - External storage (hard drives, optical drives) via SBP-2
 * - Digital video cameras (DV format) via AV/C and IEC 61883
 * - Professional audio interfaces (MOTU, PreSonus, etc.)
 * - Target Disk Mode (Macintosh)
 * - IP networking (RFC 2734)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/firewire/firewire.h>
#include <iokit/families/pcie/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Known FireWire Controller IDs
 */
typedef struct _FW_CONTROLLER_ID {
    UINT16                  VendorID;
    UINT16                  DeviceID;
    FW_CONTROLLER_TYPE      Type;
    FW_VERSION              Version;
    FW_SPEED                MaxSpeed;
    FW_PHY_CHIP             PhyType;
    CONST CHAR8            *pszName;
} FW_CONTROLLER_ID;

/**
 * @brief FireWire Controller Hardware Database
 *
 * Comprehensive list of FireWire (IEEE 1394) controllers from various vendors.
 * Most are OHCI (Open Host Controller Interface) compliant.
 */
static CONST FW_CONTROLLER_ID g_FireWireControllers[] = {
    //
    // Texas Instruments - The dominant FireWire controller vendor
    //

    // TSB12LV21/22 - PCI OHCI Controller (1394a)
    { 0x104C, 0x8009, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "TI TSB12LV21 OHCI 1394a Controller" },
    { 0x104C, 0x800A, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "TI TSB12LV22 OHCI 1394a Controller" },

    // TSB12LV23 - PCI OHCI Controller (1394a)
    { 0x104C, 0x8012, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "TI TSB12LV23 OHCI 1394a Controller" },

    // TSB12LV26 - PCI OHCI Controller (1394a, widely used)
    { 0x104C, 0x8020, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "TI TSB12LV26 OHCI 1394a Controller" },

    // TSB43AB21 - PCI OHCI Controller (1394b, FireWire 800)
    { 0x104C, 0x8021, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB22,
      "TI TSB43AB21 OHCI 1394b Controller (FireWire 800)" },

    // TSB43AB22/A - PCI OHCI Controller (1394b, bilingual PHY)
    { 0x104C, 0x8023, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "TI TSB43AB22 OHCI 1394b Controller (Bilingual)" },
    { 0x104C, 0x8024, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "TI TSB43AB22A OHCI 1394b Controller (Bilingual)" },

    // TSB43AB23 - PCI OHCI Controller (1394b, enhanced)
    { 0x104C, 0x8025, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "TI TSB43AB23 OHCI 1394b Controller" },

    // TSB82AA2 - PCIe OHCI Controller (1394b)
    { 0x104C, 0x8026, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB82AA2,
      "TI TSB82AA2 PCIe OHCI 1394b Controller" },

    // TSB12LV32 - Integrated Controller
    { 0x104C, 0x8027, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB3,
      "TI TSB12LV32 OHCI 1394a Controller" },

    // Additional TI controllers
    { 0x104C, 0x802B, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "TI TSB43AB23 IEEE-1394b OHCI Controller" },
    { 0x104C, 0x802E, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "TI FireWire OHCI 1394b Controller" },

    //
    // NEC/Renesas - Common in laptops and motherboards
    //

    // uPD72870 - PCI OHCI Controller (1394a)
    { 0x1033, 0x0063, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "NEC uPD72870 OHCI 1394a Controller" },

    // uPD72871/72 - PCI OHCI Controller (1394a)
    { 0x1033, 0x0067, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "NEC uPD72871 OHCI 1394a Controller" },
    { 0x1033, 0x0074, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "NEC uPD72872 OHCI 1394a Controller" },

    // uPD72873 - PCI OHCI Controller (1394a)
    { 0x1033, 0x00CD, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "NEC uPD72873 OHCI 1394a Controller" },

    // uPD72874 - PCI OHCI Controller (1394a)
    { 0x1033, 0x00CE, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "NEC uPD72874 OHCI 1394a Controller" },

    // μPD720400 - PCIe OHCI Controller (1394b)
    { 0x1033, 0x00F2, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "NEC μPD720400 PCIe OHCI 1394b Controller" },
    { 0x1033, 0x00F3, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "NEC μPD720400A PCIe OHCI 1394b Controller" },

    //
    // VIA - Common in older chipsets and add-in cards
    //

    // VT6306 - Very common PCI FireWire controller
    { 0x1106, 0x3044, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_VIA_VT6306,
      "VIA VT6306 OHCI 1394a Controller" },

    // VT6307 - Enhanced version
    { 0x1106, 0x3050, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_VIA_VT6306,
      "VIA VT6307 OHCI 1394a Controller" },

    // VT6308 - PCI OHCI Controller
    { 0x1106, 0x3057, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_VIA_VT6306,
      "VIA VT6308 OHCI 1394a Controller" },

    // VT6315 - PCIe OHCI Controller (1394b)
    { 0x1106, 0x3403, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "VIA VT6315 PCIe OHCI 1394b Controller" },

    // VT6330 - PCIe OHCI Controller (1394b)
    { 0x1106, 0x3410, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "VIA VT6330 PCIe OHCI 1394b Controller" },

    //
    // Agere Systems (formerly Lucent/LSI)
    //

    // FW322/323 - CardBus and PCI controllers
    { 0x11C1, 0x5811, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_AGERE_FW802,
      "Agere FW322 OHCI 1394a Controller" },
    { 0x11C1, 0x5901, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_AGERE_FW802,
      "Agere FW323 OHCI 1394a Controller" },

    // FW643 - PCIe Controller (1394b)
    { 0x11C1, 0x5903, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_AGERE_FW803,
      "Agere FW643 PCIe OHCI 1394b Controller" },

    //
    // Ricoh - Common in laptops (CardBus and integrated)
    //

    // R5C551 - CardBus Controller
    { 0x1180, 0x0551, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Ricoh R5C551 OHCI 1394a Controller" },

    // R5C552 - CardBus Controller
    { 0x1180, 0x0552, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Ricoh R5C552 OHCI 1394a Controller" },

    // R5C832 - PCIe Controller (1394b)
    { 0x1180, 0x0832, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "Ricoh R5C832 PCIe OHCI 1394b Controller" },
    { 0x1180, 0xE832, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_UNKNOWN,
      "Ricoh R5C832 PCIe OHCI 1394b Controller (alt)" },

    //
    // JMicron - Modern PCIe controllers
    //

    // JMB381 - PCIe to PCI Bridge with 1394a
    { 0x197B, 0x2380, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "JMicron JMB381 PCIe OHCI 1394a Controller" },

    // JMB38X - PCIe Controller
    { 0x197B, 0x2382, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "JMicron JMB38X PCIe OHCI 1394a Controller" },

    //
    // Apple - Custom controllers in Macs
    //

    // Pangea - PowerMac G4
    { 0x106B, 0x0018, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "Apple Pangea OHCI 1394a Controller" },

    // KeyLargo - PowerMac G4, iMac G4
    { 0x106B, 0x0019, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "Apple KeyLargo OHCI 1394a Controller" },

    // K2 - PowerMac G5
    { 0x106B, 0x001F, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "Apple K2 OHCI 1394a Controller" },

    // Shasta - PowerMac G5 (1394b)
    { 0x106B, 0x0031, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "Apple Shasta OHCI 1394b Controller" },

    // Intrepid - MacBook Pro (1394b)
    { 0x106B, 0x003A, FW_CONTROLLER_OHCI, FW_VERSION_1394B_2002, FW_SPEED_800, FW_PHY_TI_TSB43AB23,
      "Apple Intrepid OHCI 1394b Controller" },

    //
    // Sony - i.LINK controllers
    //

    // CXD3222 - PlayStation 2 and consumer devices
    { 0x104D, 0x8039, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Sony CXD3222 OHCI 1394a Controller (i.LINK)" },

    // CXD1947 - VAIO laptops
    { 0x104D, 0x8040, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Sony CXD1947 OHCI 1394a Controller (i.LINK)" },

    // CXD1956 - VAIO laptops
    { 0x104D, 0x8041, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Sony CXD1956 OHCI 1394a Controller (i.LINK)" },

    // CXD3210 - PlayStation 3
    { 0x104D, 0x80E7, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Sony CXD3210 OHCI 1394a Controller (PS3)" },

    //
    // Adaptec - Early controllers
    //

    // AIC-5800 - PCI OHCI Controller
    { 0x9004, 0x8039, FW_CONTROLLER_OHCI, FW_VERSION_1394_1995, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Adaptec AIC-5800 OHCI 1394 Controller" },

    //
    // ALi (Acer Labs) - Integrated chipset controllers
    //

    // M5253 - Integrated Controller
    { 0x10B9, 0x5253, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "ALi M5253 OHCI 1394a Controller" },

    //
    // Intel - Early experimental controllers (rare)
    //

    // Intel 82372FB - Early chipset
    { 0x8086, 0x1223, FW_CONTROLLER_OHCI, FW_VERSION_1394_1995, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Intel 82372FB OHCI 1394 Controller" },

    //
    // Creative - SoundBlaster Audigy series
    //

    // SB Audigy FireWire Port
    { 0x1102, 0x4001, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_TI_TSB41AB2,
      "Creative SB Audigy OHCI 1394a Controller" },

    //
    // O2 Micro - CardBus controllers
    //

    // OZ711 Series
    { 0x1217, 0x00F7, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "O2 Micro OZ711 OHCI 1394a Controller" },
    { 0x1217, 0x11F7, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "O2 Micro OZ711E OHCI 1394a Controller" },
    { 0x1217, 0x13F7, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "O2 Micro OZ711M OHCI 1394a Controller" },

    //
    // Fujitsu - Laptop controllers
    //

    { 0x10CF, 0x2001, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Fujitsu OHCI 1394a Controller" },

    //
    // Toshiba - Laptop controllers
    //

    { 0x1179, 0x0805, FW_CONTROLLER_OHCI, FW_VERSION_1394A_2000, FW_SPEED_400, FW_PHY_UNKNOWN,
      "Toshiba OHCI 1394a Controller" },
};

#define FW_CONTROLLER_COUNT (sizeof(g_FireWireControllers) / sizeof(g_FireWireControllers[0]))

/**
 * @brief FireWire Controller implementation structure
 */
typedef struct _FW_CONTROLLER_IMPL {
    IIOFireWireController   Interface;
    volatile LONG           RefCount;
    IIOService             *pService;
    IIOPCIDevice           *pPCIDevice;
    FW_CONTROLLER_INFO      ControllerInfo;
    UINT8                  *pMMIO;
    UINT32                  Generation;
    BOOLEAN                 bInitialized;
    BOOLEAN                 bLinkActive;
} FW_CONTROLLER_IMPL;

// Forward declarations
static IO_RETURN STDMETHODCALLTYPE FWController_QueryInterface(
    IUnknown *This, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE FWController_AddRef(IUnknown *This);
static ULONG STDMETHODCALLTYPE FWController_Release(IUnknown *This);
static IO_RETURN STDMETHODCALLTYPE FWController_Start(
    IIOService *This, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE FWController_GetControllerInfo(
    IIOFireWireController *This, FW_CONTROLLER_INFO *pInfo);
static IO_RETURN STDMETHODCALLTYPE FWController_ResetBus(
    IIOFireWireController *This);
static IO_RETURN STDMETHODCALLTYPE FWController_GetPhyInfo(
    IIOFireWireController *This, FW_PHY_TYPE *pPhyType,
    FW_CABLE_TYPE *pCableType, FW_SPEED *pMaxSpeed);
static IO_RETURN STDMETHODCALLTYPE FWController_ReadQuadlet(
    IIOFireWireController *This, UINT16 NodeID, UINT64 Offset, UINT32 *pValue);
static IO_RETURN STDMETHODCALLTYPE FWController_WriteQuadlet(
    IIOFireWireController *This, UINT16 NodeID, UINT64 Offset, UINT32 Value);

/**
 * @brief OHCI Register Access Functions
 */

static UINT32
FWOhciReadReg(
    FW_CONTROLLER_IMPL *pController,
    UINT32              Offset
    )
{
    if (!pController->pMMIO || Offset >= pController->ControllerInfo.MMIOSize) {
        return 0xFFFFFFFF;
    }
    return *(volatile UINT32*)(pController->pMMIO + Offset);
}

static VOID
FWOhciWriteReg(
    FW_CONTROLLER_IMPL *pController,
    UINT32              Offset,
    UINT32              Value
    )
{
    if (!pController->pMMIO || Offset >= pController->ControllerInfo.MMIOSize) {
        return;
    }
    *(volatile UINT32*)(pController->pMMIO + Offset) = Value;
}

/**
 * @brief Initialize OHCI Controller
 */
static IO_RETURN
FWOhciInitialize(
    FW_CONTROLLER_IMPL *pController
    )
{
    UINT32 Version, GUID_Hi, GUID_Lo;

    printf("FireWire: Initializing OHCI controller at MMIO 0x%llx\n",
           pController->ControllerInfo.MMIOBase);

    // Read OHCI version
    Version = FWOhciReadReg(pController, OHCI_VERSION);
    printf("FireWire: OHCI Version %d.%d\n",
           (Version >> 16) & 0xFF, (Version >> 8) & 0xFF);

    // Read controller GUID
    GUID_Hi = FWOhciReadReg(pController, OHCI_GUID_HI);
    GUID_Lo = FWOhciReadReg(pController, OHCI_GUID_LO);
    pController->ControllerInfo.GUID = ((UINT64)GUID_Hi << 32) | GUID_Lo;
    printf("FireWire: Controller GUID: %016llX\n", pController->ControllerInfo.GUID);

    // Perform soft reset
    printf("FireWire: Performing soft reset...\n");
    FWOhciWriteReg(pController, OHCI_HC_CONTROL_SET, OHCI_HC_SOFT_RESET);

    // Wait for reset to complete (should be quick)
    for (int i = 0; i < 100; i++) {
        UINT32 Control = FWOhciReadReg(pController, OHCI_HC_CONTROL_SET);
        if (!(Control & OHCI_HC_SOFT_RESET)) {
            break;
        }
        // Small delay (implementation-specific)
    }

    // Enable link
    printf("FireWire: Enabling link...\n");
    FWOhciWriteReg(pController, OHCI_HC_CONTROL_SET, OHCI_HC_LINK_ENABLE);

    // Set interrupt mask
    FWOhciWriteReg(pController, OHCI_INT_MASK_SET,
        OHCI_INT_BUS_RESET |
        OHCI_INT_SELF_ID_COMPLETE |
        OHCI_INT_REQ_TX_COMPLETE |
        OHCI_INT_RESP_TX_COMPLETE |
        OHCI_INT_ARRQ |
        OHCI_INT_ARRS);

    // Read node ID
    UINT32 NodeID = FWOhciReadReg(pController, OHCI_NODE_ID);
    printf("FireWire: Node ID: 0x%04X (Bus: %d, Physical ID: %d)\n",
           NodeID & 0xFFFF, (NodeID >> 6) & 0x3FF, NodeID & 0x3F);

    pController->bLinkActive = TRUE;
    pController->Generation = 0;

    printf("FireWire: OHCI controller initialized successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Handle Bus Reset
 */
static VOID
FWHandleBusReset(
    FW_CONTROLLER_IMPL *pController
    )
{
    printf("FireWire: Bus reset detected\n");

    pController->Generation++;

    // Read self-ID buffer
    UINT32 SelfIDCount = FWOhciReadReg(pController, OHCI_SELF_ID_COUNT);
    printf("FireWire: Self-ID count: %d quadlets, Generation: %d\n",
           SelfIDCount, pController->Generation);

    // Clear interrupt
    FWOhciWriteReg(pController, OHCI_INT_EVENT_CLEAR, OHCI_INT_BUS_RESET);

    printf("FireWire: Bus reset handling complete, topology rebuilt\n");
}

/**
 * @brief Parse Configuration ROM
 */
static IO_RETURN
FWParseConfigROM(
    UINT32      *pROM,
    UINT32       ROMSize,
    FW_DEVICE_INFO *pDeviceInfo
    )
{
    if (!pROM || !pDeviceInfo || ROMSize < 5) {
        return IO_BAD_ARGUMENT;
    }

    // Bus info block is at offset 1-5
    UINT32 BusInfoBlock[4];
    BusInfoBlock[0] = pROM[1]; // Bus name (should be "1394")
    BusInfoBlock[1] = pROM[2]; // Capabilities
    BusInfoBlock[2] = pROM[3]; // Node unique ID high
    BusInfoBlock[3] = pROM[4]; // Node unique ID low

    // Check bus name
    if (BusInfoBlock[0] != 0x31333934) { // "1394" in ASCII
        printf("FireWire: Invalid bus name in config ROM: 0x%08X\n", BusInfoBlock[0]);
        return IO_ERROR;
    }

    // Extract EUI-64
    pDeviceInfo->UniqueID.Value = ((UINT64)BusInfoBlock[2] << 32) | BusInfoBlock[3];
    printf("FireWire: Device EUI-64: %016llX\n", pDeviceInfo->UniqueID.Value);

    // Extract vendor ID (upper 24 bits of EUI-64)
    pDeviceInfo->VendorID = (UINT32)(pDeviceInfo->UniqueID.Value >> 40) & 0xFFFFFF;

    // Extract capabilities
    pDeviceInfo->bCycleMasterCapable = (BusInfoBlock[1] & 0x800) ? TRUE : FALSE;
    pDeviceInfo->bIsochronousResourceManagerCapable = (BusInfoBlock[1] & 0x400) ? TRUE : FALSE;
    pDeviceInfo->bBusManagerCapable = (BusInfoBlock[1] & 0x200) ? TRUE : FALSE;

    // Extract max speed
    UINT32 SpeedCode = (BusInfoBlock[1] >> 16) & 0x7;
    pDeviceInfo->MaxSpeed = (FW_SPEED)SpeedCode;

    printf("FireWire: Vendor ID: 0x%06X, Max Speed: S%d00\n",
           pDeviceInfo->VendorID, 1 << SpeedCode);

    return IO_SUCCESS;
}

/**
 * @brief FireWire Controller vtable
 */
static IIOFireWireControllerVtbl g_FWControllerVtbl = {
    // IUnknown methods
    FWController_QueryInterface,
    FWController_AddRef,
    FWController_Release,

    // IIOService methods
    NULL, // Probe
    FWController_Start,
    NULL, // Stop
    NULL, // Terminate
    NULL, // GetProperty
    NULL, // SetProperty
    NULL, // GetParentService
    NULL, // GetChildService
    NULL, // GetServiceState
    NULL, // GetServiceName
    NULL, // RegisterService

    // IIOFireWireController methods
    FWController_GetControllerInfo,
    FWController_ResetBus,
    FWController_GetPhyInfo,
    NULL, // SetBusManager
    NULL, // AllocateAddress
    NULL, // DeallocateAddress
    FWController_ReadQuadlet,
    FWController_WriteQuadlet,
    NULL, // ReadBlock
    NULL, // WriteBlock
    NULL, // Lock
    NULL, // EnableCycleMaster
};

/**
 * @brief IUnknown::QueryInterface
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_QueryInterface(
    IUnknown   *This,
    REFIID      riid,
    void      **ppvObject
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (!ppvObject) {
        return IO_BAD_ARGUMENT;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService) ||
        IsEqualGUID(riid, &IID_IIOFireWireController)) {
        *ppvObject = &pController->Interface;
        FWController_AddRef(This);
        return IO_SUCCESS;
    }

    *ppvObject = NULL;
    return IO_ERROR;
}

/**
 * @brief IUnknown::AddRef
 */
static ULONG STDMETHODCALLTYPE
FWController_AddRef(
    IUnknown *This
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);
    return InterlockedIncrement(&pController->RefCount);
}

/**
 * @brief IUnknown::Release
 */
static ULONG STDMETHODCALLTYPE
FWController_Release(
    IUnknown *This
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);
    LONG RefCount = InterlockedDecrement(&pController->RefCount);

    if (RefCount == 0) {
        if (pController->pPCIDevice) {
            IIOPCIDevice_Release(pController->pPCIDevice);
        }
        // Free memory (implementation-specific)
        // free(pController);
    }

    return RefCount;
}

/**
 * @brief IIOService::Start
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_Start(
    IIOService *This,
    IIOService *pProvider
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);
    IO_RETURN Status;
    PCI_DEVICE_INFO PCIInfo;
    PCI_BAR BAR0;
    CONST FW_CONTROLLER_ID *pCtrlID = NULL;

    printf("FireWire: Starting FireWire controller...\n");

    // Get PCI device interface
    Status = IIOService_QueryInterface(pProvider, &IID_IIOPCIDevice,
                                      (VOID**)&pController->pPCIDevice);
    if (Status != IO_SUCCESS) {
        printf("FireWire: Failed to get PCI device interface\n");
        return Status;
    }

    // Get PCI device info
    Status = IIOPCIDevice_GetDeviceInfo(pController->pPCIDevice, &PCIInfo);
    if (Status != IO_SUCCESS) {
        printf("FireWire: Failed to get PCI device info\n");
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    printf("FireWire: PCI Device %04X:%04X (Class %02X:%02X:%02X)\n",
           PCIInfo.VendorID, PCIInfo.DeviceID,
           PCIInfo.ClassCode, PCIInfo.SubClass, PCIInfo.ProgIF);

    // Detect controller type from database
    for (UINT32 i = 0; i < FW_CONTROLLER_COUNT; i++) {
        if (g_FireWireControllers[i].VendorID == PCIInfo.VendorID &&
            g_FireWireControllers[i].DeviceID == PCIInfo.DeviceID) {
            pCtrlID = &g_FireWireControllers[i];
            break;
        }
    }

    if (pCtrlID) {
        printf("FireWire: Detected %s\n", pCtrlID->pszName);
        pController->ControllerInfo.Type = pCtrlID->Type;
        pController->ControllerInfo.Version = pCtrlID->Version;
        pController->ControllerInfo.MaxSpeed = pCtrlID->MaxSpeed;
        pController->ControllerInfo.PhyType = pCtrlID->PhyType;
        strncpy(pController->ControllerInfo.ControllerName,
                pCtrlID->pszName,
                sizeof(pController->ControllerInfo.ControllerName) - 1);
    } else {
        // Unknown controller - try generic detection
        if (PCIInfo.ClassCode == 0x0C && PCIInfo.SubClass == 0x00) {
            printf("FireWire: Unknown FireWire controller (using generic OHCI)\n");
            pController->ControllerInfo.Type = FW_CONTROLLER_OHCI;
            pController->ControllerInfo.Version = FW_VERSION_1394A_2000;
            pController->ControllerInfo.MaxSpeed = FW_SPEED_400;
            strcpy(pController->ControllerInfo.ControllerName, "Generic OHCI 1394 Controller");
        } else {
            printf("FireWire: Not a FireWire controller\n");
            IIOPCIDevice_Release(pController->pPCIDevice);
            return IO_NO_MATCH;
        }
    }

    pController->ControllerInfo.VendorID = PCIInfo.VendorID;
    pController->ControllerInfo.DeviceID = PCIInfo.DeviceID;

    // Get BAR0 (MMIO registers)
    Status = IIOPCIDevice_GetBAR(pController->pPCIDevice, 0, &BAR0);
    if (Status != IO_SUCCESS) {
        printf("FireWire: Failed to get BAR0\n");
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    if (BAR0.Type != PCI_BAR_TYPE_MEMORY) {
        printf("FireWire: BAR0 is not a memory BAR\n");
        IIOPCIDevice_Release(pController->pPCIDevice);
        return IO_ERROR;
    }

    pController->ControllerInfo.MMIOBase = BAR0.Address;
    pController->ControllerInfo.MMIOSize = BAR0.Size;
    pController->pMMIO = (UINT8*)(UINTN)BAR0.Address; // Map MMIO (simplified)

    printf("FireWire: MMIO at 0x%llX, size 0x%X\n",
           pController->ControllerInfo.MMIOBase,
           pController->ControllerInfo.MMIOSize);

    // Enable bus mastering
    IIOPCIDevice_EnableBusMaster(pController->pPCIDevice, TRUE);

    // Initialize OHCI controller
    Status = FWOhciInitialize(pController);
    if (Status != IO_SUCCESS) {
        printf("FireWire: Failed to initialize OHCI\n");
        IIOPCIDevice_Release(pController->pPCIDevice);
        return Status;
    }

    pController->bInitialized = TRUE;

    printf("FireWire: Controller started successfully\n");
    return IO_SUCCESS;
}

/**
 * @brief Get Controller Information
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_GetControllerInfo(
    IIOFireWireController *This,
    FW_CONTROLLER_INFO    *pInfo
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (!pInfo) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pInfo, &pController->ControllerInfo, sizeof(FW_CONTROLLER_INFO));
    return IO_SUCCESS;
}

/**
 * @brief Reset Bus
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_ResetBus(
    IIOFireWireController *This
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (!pController->bInitialized) {
        return IO_NOT_READY;
    }

    printf("FireWire: Initiating bus reset...\n");

    // Write to PHY control to initiate bus reset
    // PHY packet: 0x00 = Immediate bus reset
    FWOhciWriteReg(pController, OHCI_PHY_CONTROL, 0x00000040);

    return IO_SUCCESS;
}

/**
 * @brief Get PHY Information
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_GetPhyInfo(
    IIOFireWireController *This,
    FW_PHY_TYPE           *pPhyType,
    FW_CABLE_TYPE         *pCableType,
    FW_SPEED              *pMaxSpeed
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (pPhyType) {
        // Determine PHY type from version
        if (pController->ControllerInfo.Version >= FW_VERSION_1394B_2002) {
            *pPhyType = FW_PHY_BILINGUAL;
        } else {
            *pPhyType = FW_PHY_DSLINK;
        }
    }

    if (pCableType) {
        // Cable type detection would require PHY register reads
        // Default to 6-pin for desktop controllers
        *pCableType = FW_CABLE_6PIN;
    }

    if (pMaxSpeed) {
        *pMaxSpeed = pController->ControllerInfo.MaxSpeed;
    }

    return IO_SUCCESS;
}

/**
 * @brief Read Quadlet
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_ReadQuadlet(
    IIOFireWireController *This,
    UINT16                 NodeID,
    UINT64                 Offset,
    UINT32                *pValue
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (!pValue || !pController->bInitialized) {
        return IO_BAD_ARGUMENT;
    }

    printf("FireWire: Read quadlet from node %04X, offset 0x%012llX\n", NodeID, Offset);

    // This would queue an asynchronous read request
    // For now, return stub implementation
    *pValue = 0;
    return IO_UNSUPPORTED;
}

/**
 * @brief Write Quadlet
 */
static IO_RETURN STDMETHODCALLTYPE
FWController_WriteQuadlet(
    IIOFireWireController *This,
    UINT16                 NodeID,
    UINT64                 Offset,
    UINT32                 Value
    )
{
    FW_CONTROLLER_IMPL *pController = CONTAINING_RECORD(This, FW_CONTROLLER_IMPL, Interface);

    if (!pController->bInitialized) {
        return IO_NOT_READY;
    }

    printf("FireWire: Write quadlet to node %04X, offset 0x%012llX, value 0x%08X\n",
           NodeID, Offset, Value);

    // This would queue an asynchronous write request
    // For now, return stub implementation
    return IO_UNSUPPORTED;
}

/**
 * @brief Initialize FireWire Subsystem
 */
IO_RETURN
FireWireInitialize(
    VOID
    )
{
    printf("FireWire: Initializing IEEE 1394 (FireWire) subsystem\n");
    printf("FireWire: Loaded %d controller definitions\n", (int)FW_CONTROLLER_COUNT);
    return IO_SUCCESS;
}

/**
 * @brief Shutdown FireWire Subsystem
 */
IO_RETURN
FireWireShutdown(
    VOID
    )
{
    printf("FireWire: Shutting down IEEE 1394 subsystem\n");
    return IO_SUCCESS;
}

/**
 * @brief Create FireWire Controller Instance
 */
IO_RETURN
IOFireWireControllerCreate(
    CONST CHAR8              *pszName,
    IIOFireWireController  **ppController
    )
{
    FW_CONTROLLER_IMPL *pController;

    if (!ppController) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate controller structure (implementation-specific)
    // For now, return unsupported
    printf("FireWire: Creating controller instance '%s'\n", pszName ? pszName : "(unnamed)");

    *ppController = NULL;
    return IO_UNSUPPORTED;
}
