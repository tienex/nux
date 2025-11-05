/**
 * @file zorro.c
 * @brief Zorro Bus Family Implementation - Commodore Amiga Expansion Bus
 *
 * Implements Zorro II/III bus detection, AutoConfig protocol, card enumeration,
 * and device management for Commodore Amiga expansion cards.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/families/zorro/zorro.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//=============================================================================
// Global State
//=============================================================================

static BOOLEAN g_bInitialized = FALSE;
static BOOLEAN g_bZorroPresent = FALSE;
static ZORRO_BUS_TYPE g_eBusType = ZORRO_BUS_TYPE_UNKNOWN;
static IIOZorroBus *g_pBusInstance = NULL;

//=============================================================================
// Known Zorro Card Database (40+ entries)
//=============================================================================

static CONST ZORRO_CARD_DB_ENTRY g_ZorroCardDB[] = {
    // Commodore cards
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2088,
        "Commodore", "A2088 XT Bridgeboard",
        "8088 PC compatibility board with ISA slots",
        ZORRO_BOARD_TYPE_BRIDGE, ZORRO_BUS_TYPE_II, "a2088"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2286,
        "Commodore", "A2286 AT Bridgeboard",
        "80286 PC/AT compatibility board with ISA slots",
        ZORRO_BOARD_TYPE_BRIDGE, ZORRO_BUS_TYPE_II, "a2286"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2090A,
        "Commodore", "A2090a Hard Disk Controller",
        "ST506/412 MFM/RLL hard disk controller",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "a2090"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2052,
        "Commodore", "A2052 RAM Expansion",
        "512 KB Fast RAM expansion",
        ZORRO_BOARD_TYPE_MEMORY, ZORRO_BUS_TYPE_II, "ramcard"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A590,
        "Commodore", "A590 Hard Drive",
        "SCSI hard drive with 2 MB RAM",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "a590"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2091,
        "Commodore", "A2091 SCSI Controller",
        "SCSI-1 controller with DMA",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "a2091"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2232,
        "Commodore", "A2232 Multi-Serial",
        "7-port RS-232 serial adapter",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "a2232"
    },
    {
        ZORRO_MFG_COMMODORE, ZORRO_PROD_CBM_A2386,
        "Commodore", "A2386-SX AT Bridgeboard",
        "80386SX PC/AT compatibility board",
        ZORRO_BOARD_TYPE_BRIDGE, ZORRO_BUS_TYPE_II, "a2386"
    },

    // GVP (Great Valley Products) cards
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_IMPACT_SERIES_I,
        "Great Valley Products", "Impact Series I",
        "SCSI controller with RAM expansion",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "gvp-impact"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_IMPACT_SERIES_II,
        "Great Valley Products", "Impact Series II",
        "SCSI-2 Fast controller with RAM",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "gvp-impact2"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_IMPACT_3001_IDE,
        "Great Valley Products", "Impact 3001 IDE",
        "IDE controller for A3000/A4000",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "gvp-ide"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_IMPACT_3001_SCSI,
        "Great Valley Products", "Impact 3001 SCSI",
        "SCSI-2 controller for A3000/A4000",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "gvp-scsi"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_A530_TURBO,
        "Great Valley Products", "A530 Turbo",
        "68030 accelerator with SCSI",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "gvp-a530"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_COMBO_030_R4,
        "Great Valley Products", "Combo 030 R4",
        "68030 + SCSI + 8MB RAM",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "gvp-combo030"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_GFORCE_030,
        "Great Valley Products", "G-Force 030",
        "68030 accelerator card",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "gvp-gforce030"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_GFORCE_040,
        "Great Valley Products", "G-Force 040",
        "68040 accelerator card",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "gvp-gforce040"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_PHONEPAL,
        "Great Valley Products", "PhonePak VFX",
        "Voice/Fax/Data modem card",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "gvp-phonepal"
    },
    {
        ZORRO_MFG_GVP, ZORRO_PROD_GVP_IOEXTENDER,
        "Great Valley Products", "I/O Extender",
        "Multi-I/O expansion card",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "gvp-ioext"
    },

    // Phase5 Digital Products cards
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_BLIZZARD_1230,
        "Phase5", "Blizzard 1230-IV",
        "68030 50MHz accelerator for A1200",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "phase5-blizzard1230"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_BLIZZARD_1260,
        "Phase5", "Blizzard 1260",
        "68060 50MHz accelerator for A1200",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "phase5-blizzard1260"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_BLIZZARD_2060,
        "Phase5", "Blizzard 2060",
        "68060 50MHz accelerator for A2000",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "phase5-blizzard2060"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_CYBERSTORM,
        "Phase5", "CyberStorm 060",
        "68060 50MHz accelerator for A3000/A4000",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_III, "phase5-cyberstorm"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_CYBERVISION,
        "Phase5", "CyberVision 64",
        "S3 Trio64 graphics card with 2-4 MB RAM",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_III, "phase5-cv64"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_CYBERVISION3D,
        "Phase5", "CyberVision 64/3D",
        "S3 ViRGE 3D graphics with 4 MB RAM",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_III, "phase5-cv64-3d"
    },
    {
        ZORRO_MFG_PHASE5, ZORRO_PROD_PHASE5_FASTLANE,
        "Phase5", "FastLane Z3 RAM",
        "Zorro III Fast RAM expansion",
        ZORRO_BOARD_TYPE_MEMORY, ZORRO_BUS_TYPE_III, "phase5-fastlane"
    },

    // Village Tronic cards
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_PICASSO_II,
        "Village Tronic", "Picasso II",
        "Cirrus Logic GD5426 graphics, 1-2 MB",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "picasso-ii"
    },
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_PICASSO_II_PLUS,
        "Village Tronic", "Picasso II+",
        "Enhanced Picasso with faster clock",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "picasso-iiplus"
    },
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_PICASSO_IV_Z2,
        "Village Tronic", "Picasso IV Zorro II",
        "Cirrus Logic GD5446 graphics, 2-4 MB",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "picasso-iv-z2"
    },
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_PICASSO_IV_Z3,
        "Village Tronic", "Picasso IV Zorro III",
        "Cirrus Logic GD5446 graphics, 2-4 MB",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_III, "picasso-iv-z3"
    },
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_ARIADNE,
        "Village Tronic", "Ariadne",
        "AMD Lance Ethernet adapter",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "ariadne"
    },
    {
        ZORRO_MFG_VILLAGE_TRONIC, ZORRO_PROD_VT_ARIADNE_II,
        "Village Tronic", "Ariadne II",
        "10/100 Fast Ethernet adapter",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "ariadne2"
    },

    // Progressive Peripherals & Software
    {
        ZORRO_MFG_PROGRESSIVE_PERIPH, ZORRO_PROD_PPS_MERCURY,
        "Progressive Peripherals", "Mercury",
        "68030 accelerator board",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "pps-mercury"
    },
    {
        ZORRO_MFG_PROGRESSIVE_PERIPH, ZORRO_PROD_PPS_A3000_RAM8,
        "Progressive Peripherals", "A3000 8MB RAM",
        "8 MB Fast RAM expansion for A3000",
        ZORRO_BOARD_TYPE_MEMORY, ZORRO_BUS_TYPE_II, "pps-a3000ram8"
    },
    {
        ZORRO_MFG_PROGRESSIVE_PERIPH, ZORRO_PROD_PPS_A3000_RAM2,
        "Progressive Peripherals", "A3000 2MB RAM",
        "2 MB Fast RAM expansion for A3000",
        ZORRO_BOARD_TYPE_MEMORY, ZORRO_BUS_TYPE_II, "pps-a3000ram2"
    },
    {
        ZORRO_MFG_PROGRESSIVE_PERIPH, ZORRO_PROD_PPS_ZEUS_040,
        "Progressive Peripherals", "Zeus 68040",
        "68040 accelerator with SCSI",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "pps-zeus040"
    },

    // MacroSystem cards
    {
        ZORRO_MFG_MACROSYSTEMS, ZORRO_PROD_MACROSYSTEM_WARP,
        "MacroSystem", "Warp Engine",
        "68040 accelerator with SCSI-2",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_III, "warpengine"
    },
    {
        ZORRO_MFG_MACROSYSTEMS, ZORRO_PROD_MACROSYSTEM_RETINA,
        "MacroSystem", "Retina Z2",
        "NCR graphics with 2-4 MB",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "retina-z2"
    },
    {
        ZORRO_MFG_MACROSYSTEMS, ZORRO_PROD_MACROSYSTEM_RETINA_Z3,
        "MacroSystem", "Retina Z3",
        "NCR graphics with 4 MB for Zorro III",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_III, "retina-z3"
    },
    {
        ZORRO_MFG_MACROSYSTEMS, ZORRO_PROD_MACROSYSTEM_VLAB,
        "MacroSystem", "VLab Motion",
        "Video digitizer and motion capture",
        ZORRO_BOARD_TYPE_DMA, ZORRO_BUS_TYPE_II, "vlab"
    },
    {
        ZORRO_MFG_MACROSYSTEMS, ZORRO_PROD_MACROSYSTEM_FALCON,
        "MacroSystem", "Falcon 040",
        "68040 33MHz accelerator",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "falcon040"
    },

    // Individual Computers
    {
        ZORRO_MFG_INDIVIDUAL, ZORRO_PROD_INDIVIDUAL_BUDDHA,
        "Individual Computers", "Buddha",
        "IDE controller with 2-4 ports",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "buddha-ide"
    },
    {
        ZORRO_MFG_INDIVIDUAL, ZORRO_PROD_INDIVIDUAL_CATWEASEL,
        "Individual Computers", "Catweasel",
        "Universal floppy disk controller",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "catweasel"
    },
    {
        ZORRO_MFG_INDIVIDUAL, ZORRO_PROD_INDIVIDUAL_X_SURF,
        "Individual Computers", "X-Surf",
        "RTL8019AS 10/100 Ethernet + IDE",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "x-surf"
    },

    // Additional manufacturers
    {
        ZORRO_MFG_DKB, 0x01,
        "DKB", "MegAChip 2000",
        "2MB Chip RAM expansion",
        ZORRO_BOARD_TYPE_MEMORY, ZORRO_BUS_TYPE_II, "megachip2000"
    },
    {
        ZORRO_MFG_MICROBOTICS, 0x01,
        "MicroBotics", "StarBoard II",
        "Multi-function expansion board",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "starboard2"
    },
    {
        ZORRO_MFG_HYDRA, 0x01,
        "Hydra Systems", "Amiganet",
        "Ethernet networking adapter",
        ZORRO_BOARD_TYPE_IO, ZORRO_BUS_TYPE_II, "hydra-eth"
    },

    // Terminator
    { 0, 0, NULL, NULL, NULL, 0, 0, NULL }
};

//=============================================================================
// Manufacturer Name Database
//=============================================================================

typedef struct _ZORRO_MFG_NAME {
    UINT16          uManufacturer;
    CONST CHAR8     *pszName;
} ZORRO_MFG_NAME;

static CONST ZORRO_MFG_NAME g_ManufacturerNames[] = {
    { ZORRO_MFG_COMMODORE,          "Commodore Business Machines" },
    { ZORRO_MFG_GVP,                "Great Valley Products" },
    { ZORRO_MFG_PHASE5,             "Phase5 Digital Products" },
    { ZORRO_MFG_VILLAGE_TRONIC,     "Village Tronic" },
    { ZORRO_MFG_PROGRESSIVE_PERIPH, "Progressive Peripherals & Software" },
    { ZORRO_MFG_APOLLO,             "Apollo Computer" },
    { ZORRO_MFG_MACROSYSTEMS,       "MacroSystem Development" },
    { ZORRO_MFG_BVISION,            "BVision" },
    { ZORRO_MFG_HYDRA,              "Hydra Systems" },
    { ZORRO_MFG_READYSOFT,          "ReadySoft" },
    { ZORRO_MFG_IMTRONICS,          "Imtronics" },
    { ZORRO_MFG_CARDCO,             "Cardco" },
    { ZORRO_MFG_DKB,                "DKB (Dave Kinzer Buffalo)" },
    { ZORRO_MFG_HELFRICH,           "Helfrich" },
    { ZORRO_MFG_MICROBOTICS,        "MicroBotics" },
    { ZORRO_MFG_EXPANSION_SYS,      "Expansion Systems" },
    { ZORRO_MFG_MTEC,               "M-TEC" },
    { ZORRO_MFG_BSC,                "BSC Alfadata" },
    { ZORRO_MFG_JOCHHEIM,           "Jochheim" },
    { ZORRO_MFG_ACT,                "ACT" },
    { ZORRO_MFG_XETEC,              "Xetec" },
    { ZORRO_MFG_GFX_BASE,           "GFX-Base" },
    { ZORRO_MFG_ROCTEC,             "Roctec" },
    { ZORRO_MFG_BUDDHA,             "Buddha Flash" },
    { ZORRO_MFG_INDIVIDUAL,         "Individual Computers" },
    { ZORRO_MFG_ELBOX,              "Elbox Computer" },
    { 0, NULL }
};

//=============================================================================
// Internal Structures
//=============================================================================

typedef struct _ZorroBus {
    IIOZorroBus     vtbl;
    UINT32          uRefCount;
    ZORRO_BUS_TYPE  eBusType;
    UINT32          uDeviceCount;
    IIOZorroDevice  *pDevices[ZORRO_MAX_SLOTS];
    UINT32          uDMAChannels;       // Bitmask of allocated DMA channels
} ZorroBus;

typedef struct _ZorroDevice {
    IIOZorroDevice  vtbl;
    UINT32          uRefCount;
    ZORRO_CONFIGDEV ConfigDev;
    VOID            *pMappedBase;
    BOOLEAN         bInterruptEnabled[7];  // IRQ 0-6
    VOID            (*pfnIRQHandler[7])(VOID *);
    VOID            *pIRQContext[7];
    BOOLEAN         bDMAActive;
    ZORRO_DMA_PARAMS DMAParams;
} ZorroDevice;

//=============================================================================
// Forward Declarations
//=============================================================================

static IO_RETURN ZorroBus_DetectBus(IIOZorroBus *this, ZORRO_BUS_TYPE *peBusType);
static IO_RETURN ZorroBus_ScanAutoConfig(IIOZorroBus *this, UINT32 *puDeviceCount);
static IO_RETURN ZorroDevice_GetDeviceInfo(IIOZorroDevice *this, ZORRO_DEVICE_INFO *pInfo);

//=============================================================================
// IIOZorroBus Implementation
//=============================================================================

static IO_RETURN IOCALL ZorroBus_QueryInterface(
    IIOZorroBus *this,
    REFIID      riid,
    VOID        **ppvObject
)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService) ||
        IsEqualGUID(riid, &IID_IIOZorroBus))
    {
        *ppvObject = this;
        this->Base.Base.AddRef((IUnknown *)this);
        return IO_SUCCESS;
    }

    *ppvObject = NULL;
    return IO_ERROR_NOT_SUPPORTED;
}

static UINT32 IOCALL ZorroBus_AddRef(IIOZorroBus *this)
{
    ZorroBus *pBus = (ZorroBus *)this;
    return ++pBus->uRefCount;
}

static UINT32 IOCALL ZorroBus_Release(IIOZorroBus *this)
{
    ZorroBus *pBus = (ZorroBus *)this;
    UINT32 uRefCount = --pBus->uRefCount;

    if (uRefCount == 0) {
        // Release all device references
        for (UINT32 i = 0; i < pBus->uDeviceCount; i++) {
            if (pBus->pDevices[i]) {
                pBus->pDevices[i]->Base.Base.Release((IUnknown *)pBus->pDevices[i]);
            }
        }
        free(pBus);
    }

    return uRefCount;
}

static IO_RETURN IOCALL ZorroBus_Probe(
    IIOZorroBus *this,
    IIOService  *pProvider,
    UINT32      *puProbeScore
)
{
    ZORRO_BUS_TYPE eBusType;
    IO_RETURN ret = ZorroBus_DetectBus(this, &eBusType);

    if (ret == IO_SUCCESS) {
        *puProbeScore = 1000;  // High priority for Zorro bus
        return IO_SUCCESS;
    }

    return IO_NO_DEVICE;
}

static IO_RETURN IOCALL ZorroBus_Start(IIOZorroBus *this, IIOService *pProvider)
{
    UINT32 uDeviceCount = 0;
    return ZorroBus_ScanAutoConfig(this, &uDeviceCount);
}

static IO_RETURN IOCALL ZorroBus_Stop(IIOZorroBus *this, IIOService *pProvider)
{
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_Terminate(IIOZorroBus *this, UINT32 uOptions)
{
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_GetProperty(
    IIOZorroBus *this,
    CONST CHAR8 *pszKey,
    VOID        *pValue,
    UINTN       *pcbSize,
    UINT32      *puType
)
{
    return IO_ERROR_NOT_FOUND;
}

static IO_RETURN IOCALL ZorroBus_SetProperty(
    IIOZorroBus *this,
    CONST CHAR8 *pszKey,
    CONST VOID  *pValue,
    UINTN       cbSize,
    UINT32      uType
)
{
    return IO_ERROR_NOT_SUPPORTED;
}

static IO_RETURN IOCALL ZorroBus_GetParentService(IIOZorroBus *this, IIOService **ppParent)
{
    *ppParent = NULL;
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_GetChildService(IIOZorroBus *this, UINT32 uIndex, IIOService **ppChild)
{
    ZorroBus *pBus = (ZorroBus *)this;

    if (uIndex >= pBus->uDeviceCount) {
        return IO_NO_DEVICE;
    }

    *ppChild = (IIOService *)pBus->pDevices[uIndex];
    if (*ppChild) {
        (*ppChild)->Base.AddRef((IUnknown *)*ppChild);
    }

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_GetServiceState(IIOZorroBus *this, UINT32 *puState)
{
    *puState = 1;  // Running
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_GetServiceName(IIOZorroBus *this, CHAR8 *pszName, UINTN cbSize)
{
    snprintf(pszName, cbSize, "ZorroBus");
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_RegisterService(IIOZorroBus *this, UINT32 uOptions)
{
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_DetectBus(
    IIOZorroBus     *this,
    ZORRO_BUS_TYPE  *peBusType
)
{
    if (!peBusType) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    // In a real implementation, this would probe hardware
    // For now, simulate detection based on system type
    // On real hardware, would check for Zorro autoconfig space

    // TODO: Implement actual hardware detection
    // - Check for AutoConfig space at 0x00E80000
    // - Read expansion.library ConfigDev list
    // - Determine Zorro II vs III from first card type

    *peBusType = ZORRO_BUS_TYPE_UNKNOWN;

    // Simulated detection
    g_bZorroPresent = FALSE;  // Change to TRUE on real hardware
    g_eBusType = ZORRO_BUS_TYPE_UNKNOWN;

    if (!g_bZorroPresent) {
        return IO_NO_DEVICE;
    }

    *peBusType = g_eBusType;
    ((ZorroBus *)this)->eBusType = g_eBusType;

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_ScanAutoConfig(
    IIOZorroBus *this,
    UINT32      *puDeviceCount
)
{
    if (!puDeviceCount) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;
    UINT32 uCount = 0;

    // Scan AutoConfig space for devices
    // In real implementation, would read from 0x00E80000
    // For each slot:
    //   1. Read AutoConfig ROM
    //   2. Parse ConfigDev structure
    //   3. Assign base address
    //   4. Shut up device (remove from AutoConfig space)
    //   5. Create device instance

    // TODO: Implement actual AutoConfig protocol
    // - Read Type/Size register at offset 0x00
    // - Read Product number at offset 0x04
    // - Read Flags at offset 0x08
    // - Read Manufacturer ID at offsets 0x10, 0x14
    // - Read Serial number at offsets 0x18-0x24
    // - Assign address based on size
    // - Write address to config registers
    // - Write SHUTUP command

    *puDeviceCount = uCount;
    pBus->uDeviceCount = uCount;

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_GetDeviceBySlot(
    IIOZorroBus     *this,
    UINT8           uSlot,
    IIOZorroDevice  **ppDevice
)
{
    if (!ppDevice) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;

    if (uSlot >= pBus->uDeviceCount || !pBus->pDevices[uSlot]) {
        return IO_NO_DEVICE;
    }

    *ppDevice = pBus->pDevices[uSlot];
    (*ppDevice)->Base.Base.AddRef((IUnknown *)*ppDevice);

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_FindDevice(
    IIOZorroBus     *this,
    UINT16          uManufacturer,
    UINT8           uProduct,
    IIOZorroDevice  **ppDevice
)
{
    if (!ppDevice) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;

    for (UINT32 i = 0; i < pBus->uDeviceCount; i++) {
        if (pBus->pDevices[i]) {
            ZORRO_CONFIGDEV configDev;
            IO_RETURN ret = pBus->pDevices[i]->GetConfigDev(pBus->pDevices[i], &configDev);

            if (ret == IO_SUCCESS &&
                configDev.uManufacturer == uManufacturer &&
                configDev.uProduct == uProduct)
            {
                *ppDevice = pBus->pDevices[i];
                (*ppDevice)->Base.Base.AddRef((IUnknown *)*ppDevice);
                return IO_SUCCESS;
            }
        }
    }

    return IO_NOT_FOUND;
}

static IO_RETURN IOCALL ZorroBus_EnumerateDevices(
    IIOZorroBus     *this,
    IIOZorroDevice  ***pppDevices,
    UINT32          *puCount
)
{
    if (!pppDevices || !puCount) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;

    *pppDevices = (IIOZorroDevice **)pBus->pDevices;
    *puCount = pBus->uDeviceCount;

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_ReadAutoConfigROM(
    IIOZorroBus *this,
    UINT8       uSlot,
    UINT32      uOffset,
    VOID        *pBuffer,
    UINT32      uLength
)
{
    if (!pBuffer || uSlot >= ZORRO_MAX_SLOTS) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    // Calculate AutoConfig ROM address
    UINT32 uROMAddr = ZORRO_AUTOCONFIG_BASE + (uSlot * ZORRO_AUTOCONFIG_SIZE) + uOffset;

    // In real implementation, read from hardware
    // AutoConfig ROM data is stored in every second nibble
    // TODO: Implement actual ROM reading with nibble extraction

    return IO_ERROR_NOT_SUPPORTED;
}

static IO_RETURN IOCALL ZorroBus_ConfigureAddress(
    IIOZorroBus *this,
    UINT8       uSlot,
    UINT32      uBaseAddr
)
{
    if (uSlot >= ZORRO_MAX_SLOTS) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    // In real implementation, write address to AutoConfig registers
    // Address is written to registers 0x48-0x4B (base address)
    // TODO: Implement address configuration protocol

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_ShutUpDevice(
    IIOZorroBus *this,
    UINT8       uSlot
)
{
    if (uSlot >= ZORRO_MAX_SLOTS) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    // Write SHUTUP command to register 0x4C
    // This removes device from AutoConfig space
    // TODO: Implement SHUTUP protocol

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_AllocateDMA(
    IIOZorroBus     *this,
    IIOZorroDevice  *pDevice,
    UINT32          *puChannel
)
{
    if (!pDevice || !puChannel) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;

    // Find free DMA channel (simple bit allocation)
    for (UINT32 i = 0; i < 8; i++) {
        if (!(pBus->uDMAChannels & (1 << i))) {
            pBus->uDMAChannels |= (1 << i);
            *puChannel = i;
            return IO_SUCCESS;
        }
    }

    return IO_NO_CHANNELS;
}

static IO_RETURN IOCALL ZorroBus_FreeDMA(
    IIOZorroBus *this,
    UINT32      uChannel
)
{
    if (uChannel >= 8) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroBus *pBus = (ZorroBus *)this;
    pBus->uDMAChannels &= ~(1 << uChannel);

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroBus_EnableBurstMode(
    IIOZorroBus     *this,
    IIOZorroDevice  *pDevice,
    BOOLEAN         bEnable
)
{
    ZorroBus *pBus = (ZorroBus *)this;

    if (pBus->eBusType != ZORRO_BUS_TYPE_III) {
        return IO_UNSUPPORTED;
    }

    // Configure burst mode for Zorro III device
    // TODO: Implement burst mode configuration

    return IO_SUCCESS;
}

//=============================================================================
// IIOZorroDevice Implementation
//=============================================================================

static IO_RETURN IOCALL ZorroDevice_QueryInterface(
    IIOZorroDevice  *this,
    REFIID          riid,
    VOID            **ppvObject
)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IIOService) ||
        IsEqualGUID(riid, &IID_IIOZorroDevice))
    {
        *ppvObject = this;
        this->Base.Base.AddRef((IUnknown *)this);
        return IO_SUCCESS;
    }

    *ppvObject = NULL;
    return IO_ERROR_NOT_SUPPORTED;
}

static UINT32 IOCALL ZorroDevice_AddRef(IIOZorroDevice *this)
{
    ZorroDevice *pDevice = (ZorroDevice *)this;
    return ++pDevice->uRefCount;
}

static UINT32 IOCALL ZorroDevice_Release(IIOZorroDevice *this)
{
    ZorroDevice *pDevice = (ZorroDevice *)this;
    UINT32 uRefCount = --pDevice->uRefCount;

    if (uRefCount == 0) {
        // Unmap memory if mapped
        if (pDevice->pMappedBase) {
            // TODO: Unmap memory
        }
        free(pDevice);
    }

    return uRefCount;
}

static IO_RETURN IOCALL ZorroDevice_GetDeviceInfo(
    IIOZorroDevice      *this,
    ZORRO_DEVICE_INFO   *pInfo
)
{
    if (!pInfo) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;

    memset(pInfo, 0, sizeof(ZORRO_DEVICE_INFO));
    memcpy(&pInfo->ConfigDev, &pDevice->ConfigDev, sizeof(ZORRO_CONFIGDEV));

    // Look up card in database
    CONST ZORRO_CARD_DB_ENTRY *pEntry = NULL;
    IOZorroGetCardInfo(pDevice->ConfigDev.uManufacturer,
                       pDevice->ConfigDev.uProduct,
                       &pEntry);

    if (pEntry) {
        strncpy(pInfo->szManufacturerName, pEntry->pszManufacturer, sizeof(pInfo->szManufacturerName) - 1);
        strncpy(pInfo->szProductName, pEntry->pszProduct, sizeof(pInfo->szProductName) - 1);
        strncpy(pInfo->szDescription, pEntry->pszDescription, sizeof(pInfo->szDescription) - 1);
        strncpy(pInfo->szDriverName, pEntry->pszDriver, sizeof(pInfo->szDriverName) - 1);
    } else {
        // Unknown card
        ZorroGetManufacturerName(pDevice->ConfigDev.uManufacturer,
                                 pInfo->szManufacturerName,
                                 sizeof(pInfo->szManufacturerName));
        snprintf(pInfo->szProductName, sizeof(pInfo->szProductName),
                 "Unknown Product 0x%02X", pDevice->ConfigDev.uProduct);
    }

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_GetConfigDev(
    IIOZorroDevice  *this,
    ZORRO_CONFIGDEV *pConfigDev
)
{
    if (!pConfigDev) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    memcpy(pConfigDev, &pDevice->ConfigDev, sizeof(ZORRO_CONFIGDEV));

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_GetBaseAddress(
    IIOZorroDevice  *this,
    UINT32          *puBaseAddr
)
{
    if (!puBaseAddr) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    *puBaseAddr = pDevice->ConfigDev.uBaseAddress;

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_GetBoardSize(
    IIOZorroDevice  *this,
    UINT32          *puSize
)
{
    if (!puSize) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    *puSize = pDevice->ConfigDev.uBoardSize;

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_MapMemory(
    IIOZorroDevice  *this,
    UINT32          uOffset,
    UINT32          uLength,
    VOID            **ppMapped
)
{
    if (!ppMapped) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;

    // In real implementation, map physical memory
    // TODO: Implement memory mapping
    *ppMapped = (VOID *)(uintptr_t)(pDevice->ConfigDev.uBaseAddress + uOffset);

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_UnmapMemory(
    IIOZorroDevice  *this,
    VOID            *pMapped,
    UINT32          uLength
)
{
    // TODO: Implement memory unmapping
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_ReadMemory(
    IIOZorroDevice  *this,
    UINT32          uOffset,
    VOID            *pBuffer,
    UINT32          uLength
)
{
    if (!pBuffer) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    volatile UINT8 *pSrc = (volatile UINT8 *)(uintptr_t)(pDevice->ConfigDev.uBaseAddress + uOffset);

    // Simple byte-by-byte copy
    // TODO: Optimize with word/long transfers where appropriate
    for (UINT32 i = 0; i < uLength; i++) {
        ((UINT8 *)pBuffer)[i] = pSrc[i];
    }

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_WriteMemory(
    IIOZorroDevice  *this,
    UINT32          uOffset,
    CONST VOID      *pBuffer,
    UINT32          uLength
)
{
    if (!pBuffer) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    volatile UINT8 *pDest = (volatile UINT8 *)(uintptr_t)(pDevice->ConfigDev.uBaseAddress + uOffset);

    // Simple byte-by-byte copy
    for (UINT32 i = 0; i < uLength; i++) {
        pDest[i] = ((CONST UINT8 *)pBuffer)[i];
    }

    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_EnableInterrupt(
    IIOZorroDevice  *this,
    ZORRO_IRQ_LEVEL eLevel,
    VOID            (*pfnHandler)(VOID *pContext),
    VOID            *pContext
)
{
    if (!pfnHandler || (eLevel != ZORRO_IRQ_INT2 && eLevel != ZORRO_IRQ_INT6)) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;

    pDevice->pfnIRQHandler[eLevel] = pfnHandler;
    pDevice->pIRQContext[eLevel] = pContext;
    pDevice->bInterruptEnabled[eLevel] = TRUE;

    // TODO: Configure interrupt controller
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_DisableInterrupt(
    IIOZorroDevice  *this,
    ZORRO_IRQ_LEVEL eLevel
)
{
    ZorroDevice *pDevice = (ZorroDevice *)this;

    pDevice->bInterruptEnabled[eLevel] = FALSE;
    pDevice->pfnIRQHandler[eLevel] = NULL;
    pDevice->pIRQContext[eLevel] = NULL;

    // TODO: Disable interrupt in controller
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_SetupDMA(
    IIOZorroDevice          *this,
    CONST ZORRO_DMA_PARAMS  *pParams
)
{
    if (!pParams) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;

    if (!pDevice->ConfigDev.bDMACapable) {
        return IO_UNSUPPORTED;
    }

    memcpy(&pDevice->DMAParams, pParams, sizeof(ZORRO_DMA_PARAMS));

    // TODO: Configure DMA controller
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_StartDMA(IIOZorroDevice *this)
{
    ZorroDevice *pDevice = (ZorroDevice *)this;

    if (!pDevice->ConfigDev.bDMACapable) {
        return IO_UNSUPPORTED;
    }

    pDevice->bDMAActive = TRUE;

    // TODO: Start DMA transfer
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_StopDMA(IIOZorroDevice *this)
{
    ZorroDevice *pDevice = (ZorroDevice *)this;
    pDevice->bDMAActive = FALSE;

    // TODO: Stop DMA transfer
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_IsDMAComplete(
    IIOZorroDevice  *this,
    BOOLEAN         *pbComplete
)
{
    if (!pbComplete) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    ZorroDevice *pDevice = (ZorroDevice *)this;
    *pbComplete = !pDevice->bDMAActive;

    // TODO: Check actual DMA status
    return IO_SUCCESS;
}

static IO_RETURN IOCALL ZorroDevice_Enable(
    IIOZorroDevice  *this,
    BOOLEAN         bEnable
)
{
    // TODO: Enable/disable device
    return IO_SUCCESS;
}

//=============================================================================
// Public API Functions
//=============================================================================

IO_RETURN ZorroInitialize(VOID)
{
    if (g_bInitialized) {
        return IO_SUCCESS;
    }

    g_bInitialized = TRUE;
    return IO_SUCCESS;
}

IO_RETURN ZorroShutdown(VOID)
{
    if (!g_bInitialized) {
        return IO_SUCCESS;
    }

    if (g_pBusInstance) {
        g_pBusInstance->Base.Base.Release((IUnknown *)g_pBusInstance);
        g_pBusInstance = NULL;
    }

    g_bInitialized = FALSE;
    return IO_SUCCESS;
}

IO_RETURN IOZorroGetBus(IIOZorroBus **ppBus)
{
    if (!ppBus) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    if (!g_bInitialized) {
        IO_RETURN ret = ZorroInitialize();
        if (ret != IO_SUCCESS) {
            return ret;
        }
    }

    if (!g_pBusInstance) {
        // Create bus instance
        ZorroBus *pBus = (ZorroBus *)malloc(sizeof(ZorroBus));
        if (!pBus) {
            return IO_NO_MEMORY;
        }

        memset(pBus, 0, sizeof(ZorroBus));
        pBus->uRefCount = 1;

        // Initialize vtable
        pBus->vtbl.Base.Base.QueryInterface = (void *)ZorroBus_QueryInterface;
        pBus->vtbl.Base.Base.AddRef = (void *)ZorroBus_AddRef;
        pBus->vtbl.Base.Base.Release = (void *)ZorroBus_Release;
        pBus->vtbl.Base.Probe = ZorroBus_Probe;
        pBus->vtbl.Base.Start = ZorroBus_Start;
        pBus->vtbl.Base.Stop = ZorroBus_Stop;
        pBus->vtbl.Base.Terminate = ZorroBus_Terminate;
        pBus->vtbl.Base.GetProperty = ZorroBus_GetProperty;
        pBus->vtbl.Base.SetProperty = ZorroBus_SetProperty;
        pBus->vtbl.Base.GetParentService = ZorroBus_GetParentService;
        pBus->vtbl.Base.GetChildService = ZorroBus_GetChildService;
        pBus->vtbl.Base.GetServiceState = ZorroBus_GetServiceState;
        pBus->vtbl.Base.GetServiceName = ZorroBus_GetServiceName;
        pBus->vtbl.Base.RegisterService = ZorroBus_RegisterService;

        pBus->vtbl.DetectBus = ZorroBus_DetectBus;
        pBus->vtbl.ScanAutoConfig = ZorroBus_ScanAutoConfig;
        pBus->vtbl.GetDeviceBySlot = ZorroBus_GetDeviceBySlot;
        pBus->vtbl.FindDevice = ZorroBus_FindDevice;
        pBus->vtbl.EnumerateDevices = ZorroBus_EnumerateDevices;
        pBus->vtbl.ReadAutoConfigROM = ZorroBus_ReadAutoConfigROM;
        pBus->vtbl.ConfigureAddress = ZorroBus_ConfigureAddress;
        pBus->vtbl.ShutUpDevice = ZorroBus_ShutUpDevice;
        pBus->vtbl.AllocateDMA = ZorroBus_AllocateDMA;
        pBus->vtbl.FreeDMA = ZorroBus_FreeDMA;
        pBus->vtbl.EnableBurstMode = ZorroBus_EnableBurstMode;

        g_pBusInstance = (IIOZorroBus *)pBus;
    }

    *ppBus = g_pBusInstance;
    g_pBusInstance->Base.Base.AddRef((IUnknown *)g_pBusInstance);

    return IO_SUCCESS;
}

IO_RETURN IOZorroDetect(BOOLEAN *pbPresent)
{
    if (!pbPresent) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    // Check for Zorro bus presence
    // TODO: Implement actual hardware detection
    *pbPresent = FALSE;

    return IO_SUCCESS;
}

IO_RETURN IOZorroGetCardInfo(
    UINT16                      uManufacturer,
    UINT8                       uProduct,
    CONST ZORRO_CARD_DB_ENTRY   **ppEntry
)
{
    if (!ppEntry) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    for (UINT32 i = 0; g_ZorroCardDB[i].pszManufacturer != NULL; i++) {
        if (g_ZorroCardDB[i].uManufacturer == uManufacturer &&
            g_ZorroCardDB[i].uProduct == uProduct)
        {
            *ppEntry = &g_ZorroCardDB[i];
            return IO_SUCCESS;
        }
    }

    return IO_NOT_FOUND;
}

IO_RETURN ZorroParseAutoConfigROM(
    CONST VOID      *pROMData,
    ZORRO_CONFIGDEV *pConfigDev
)
{
    if (!pROMData || !pConfigDev) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    memset(pConfigDev, 0, sizeof(ZORRO_CONFIGDEV));

    CONST UINT8 *pROM = (CONST UINT8 *)pROMData;

    // Parse AutoConfig ROM structure
    // Data is stored in every second nibble (upper nibble of each word)

    // Type and size
    UINT8 uTypeSize = (pROM[ZORRO_ROM_TYPE_SIZE] >> 4) & 0x0F;
    pConfigDev->eBusType = (uTypeSize & ZORRO_TYPE_MASK) == ZORRO_TYPE_ZORROIII ?
                           ZORRO_BUS_TYPE_III : ZORRO_BUS_TYPE_II;

    UINT8 uSizeCode = uTypeSize & ZORRO_SIZE_MASK;
    if (pConfigDev->eBusType == ZORRO_BUS_TYPE_III) {
        pConfigDev->uBoardSize = ZORRO_III_SIZE_TO_BYTES(uSizeCode);
    } else {
        pConfigDev->uBoardSize = ZORRO_II_SIZE_TO_BYTES(uSizeCode);
    }

    // Product number
    pConfigDev->uProduct = (pROM[ZORRO_ROM_PRODUCT] >> 4) & 0x0F;

    // Flags
    pConfigDev->uFlags = (pROM[ZORRO_ROM_FLAGS] >> 4) & 0x0F;
    pConfigDev->bCanShutUp = (pConfigDev->uFlags & ZORRO_FLAG_CAN_SHUTUP) != 0;
    pConfigDev->bMemoryDevice = (pConfigDev->uFlags & ZORRO_FLAG_MEMLIST) != 0;

    // Manufacturer ID (16-bit)
    UINT8 uMfgHi = (pROM[ZORRO_ROM_MANUFACTURER_HI] >> 4) & 0x0F;
    UINT8 uMfgLo = (pROM[ZORRO_ROM_MANUFACTURER_LO] >> 4) & 0x0F;
    pConfigDev->uManufacturer = (uMfgHi << 8) | uMfgLo;

    // Serial number (32-bit)
    pConfigDev->uSerialNumber =
        (((UINT32)(pROM[ZORRO_ROM_SERIAL_1] >> 4) & 0x0F) << 24) |
        (((UINT32)(pROM[ZORRO_ROM_SERIAL_2] >> 4) & 0x0F) << 16) |
        (((UINT32)(pROM[ZORRO_ROM_SERIAL_3] >> 4) & 0x0F) << 8) |
        (((UINT32)(pROM[ZORRO_ROM_SERIAL_4] >> 4) & 0x0F));

    // ROM vector
    UINT8 uROMHi = (pROM[ZORRO_ROM_INIT_DIAG_VEC_HI] >> 4) & 0x0F;
    UINT8 uROMLo = (pROM[ZORRO_ROM_INIT_DIAG_VEC_LO] >> 4) & 0x0F;
    pConfigDev->uROMVector = (uROMHi << 8) | uROMLo;

    pConfigDev->bConfigured = FALSE;

    return IO_SUCCESS;
}

IO_RETURN ZorroGetManufacturerName(
    UINT16  uManufacturer,
    CHAR8   *pszName,
    UINTN   cbSize
)
{
    if (!pszName || cbSize == 0) {
        return IO_ERROR_INVALID_PARAMETER;
    }

    for (UINT32 i = 0; g_ManufacturerNames[i].pszName != NULL; i++) {
        if (g_ManufacturerNames[i].uManufacturer == uManufacturer) {
            strncpy(pszName, g_ManufacturerNames[i].pszName, cbSize - 1);
            pszName[cbSize - 1] = '\0';
            return IO_SUCCESS;
        }
    }

    snprintf(pszName, cbSize, "Unknown Manufacturer 0x%04X", uManufacturer);
    return IO_NOT_FOUND;
}
