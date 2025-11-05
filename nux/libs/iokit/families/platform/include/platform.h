/**
 * @file platform.h
 * @brief Platform Firmware and Device Enumeration Interface
 *
 * This header defines platform-specific device enumeration and matching
 * interfaces for various firmware systems:
 *
 * - ACPI: Advanced Configuration and Power Interface
 * - ISA PnP: ISA Plug and Play
 * - Device Tree: Flattened Device Tree (DTB/DTS)
 * - OpenFirmware: IEEE 1275-1994 (Open Firmware)
 * - ARC/ARCS: Advanced RISC Computing (Specification)
 *
 * These matchers allow drivers to discover and bind to hardware across
 * different platform firmware implementations used in x86, ARM, RISC-V,
 * PowerPC, MIPS, SPARC, and other architectures.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_PLATFORM_H
#define IOKIT_PLATFORM_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Interface GUIDs
//=============================================================================

/**
 * @brief IIOPlatformDevice interface GUID
 * {A1B2C3D4-E5F6-7A8B-9C0D-1E2F3A4B5C6D}
 */
DEFINE_GUID(IID_IIOPlatformDevice,
    0xA1B2C3D4, 0xE5F6, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
 * @brief IIOACPIMatcher interface GUID
 * {B2C3D4E5-F6A7-8B9C-0D1E-2F3A4B5C6D7E}
 */
DEFINE_GUID(IID_IIOACPIMatcher,
    0xB2C3D4E5, 0xF6A7, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

/**
 * @brief IIOISAPnPMatcher interface GUID
 * {C3D4E5F6-A7B8-9C0D-1E2F-3A4B5C6D7E8F}
 */
DEFINE_GUID(IID_IIOISAPnPMatcher,
    0xC3D4E5F6, 0xA7B8, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
 * @brief IIODeviceTreeMatcher interface GUID
 * {D4E5F6A7-B8C9-0D1E-2F3A-4B5C6D7E8F9A}
 */
DEFINE_GUID(IID_IIODeviceTreeMatcher,
    0xD4E5F6A7, 0xB8C9, 0x0D1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A);

/**
 * @brief IIOOpenFirmwareMatcher interface GUID
 * {E5F6A7B8-C9D0-1E2F-3A4B-5C6D7E8F9A0B}
 */
DEFINE_GUID(IID_IIOOpenFirmwareMatcher,
    0xE5F6A7B8, 0xC9D0, 0x1E2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B);

/**
 * @brief IIOARCMatcher interface GUID
 * {F6A7B8C9-D0E1-2F3A-4B5C-6D7E8F9A0B1C}
 */
DEFINE_GUID(IID_IIOARCMatcher,
    0xF6A7B8C9, 0xD0E1, 0x2F3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C);

/**
 * @brief IIOPCIMatcher interface GUID
 * {A7B8C9D0-E1F2-3A4B-5C6D-7E8F9A0B1C2D}
 */
DEFINE_GUID(IID_IIOPCIMatcher,
    0xA7B8C9D0, 0xE1F2, 0x3A4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D);

/**
 * @brief IIOLegacyBusMatcher interface GUID
 * {B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E}
 */
DEFINE_GUID(IID_IIOLegacyBusMatcher,
    0xB8C9D0E1, 0xF2A3, 0x4B5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E);

//=============================================================================
// Platform Type Definitions
//=============================================================================

/**
 * @brief Platform firmware types
 */
typedef enum _PLATFORM_FIRMWARE_TYPE {
    PLATFORM_FIRMWARE_UNKNOWN       = 0,
    PLATFORM_FIRMWARE_BIOS          = 1,    /**< Legacy BIOS (x86) */
    PLATFORM_FIRMWARE_UEFI          = 2,    /**< UEFI (x86/ARM/RISC-V) */
    PLATFORM_FIRMWARE_ACPI          = 3,    /**< ACPI (x86/ARM/RISC-V) */
    PLATFORM_FIRMWARE_DEVICETREE    = 4,    /**< Device Tree (ARM/RISC-V/PPC) */
    PLATFORM_FIRMWARE_OPENFIRMWARE  = 5,    /**< OpenFirmware (PPC/SPARC) */
    PLATFORM_FIRMWARE_ARC           = 6,    /**< ARC (MIPS/Alpha) */
    PLATFORM_FIRMWARE_ARCS          = 7,    /**< ARCS (SGI MIPS) */
    PLATFORM_FIRMWARE_EFIBOOT       = 8,    /**< EFI Boot Services */
    PLATFORM_FIRMWARE_COREBOOT      = 9,    /**< coreboot */
} PLATFORM_FIRMWARE_TYPE;

/**
 * @brief Device matching priority
 */
typedef enum _DEVICE_MATCH_PRIORITY {
    DEVICE_MATCH_PRIORITY_NONE      = 0,
    DEVICE_MATCH_PRIORITY_LOW       = 1000,
    DEVICE_MATCH_PRIORITY_DEFAULT   = 5000,
    DEVICE_MATCH_PRIORITY_HIGH      = 9000,
    DEVICE_MATCH_PRIORITY_CRITICAL  = 10000,
} DEVICE_MATCH_PRIORITY;

//=============================================================================
// ACPI Device Matching
//=============================================================================

/**
 * @brief ACPI Hardware ID (HID)
 */
typedef struct _ACPI_DEVICE_HID {
    CHAR8   String[9];      /**< 7-char HID string + null (e.g., "PNP0501") */
    UINT32  Integer;        /**< Integer representation of HID */
} ACPI_DEVICE_HID;

/**
 * @brief ACPI Compatible ID (CID)
 */
typedef struct _ACPI_DEVICE_CID {
    CHAR8   String[9];      /**< 7-char CID string + null */
    UINT32  Integer;        /**< Integer representation of CID */
} ACPI_DEVICE_CID;

/**
 * @brief ACPI Device Unique ID (UID)
 */
typedef struct _ACPI_DEVICE_UID {
    CHAR8   String[65];     /**< UID string (up to 64 chars + null) */
    UINT64  Integer;        /**< Integer representation of UID */
    BOOLEAN bIsString;      /**< TRUE if UID is string, FALSE if integer */
} ACPI_DEVICE_UID;

/**
 * @brief ACPI Device Status flags
 */
typedef enum _ACPI_DEVICE_STATUS {
    ACPI_STA_PRESENT        = 0x01, /**< Device is present */
    ACPI_STA_ENABLED        = 0x02, /**< Device is enabled */
    ACPI_STA_SHOW_IN_UI     = 0x04, /**< Device should be shown in UI */
    ACPI_STA_FUNCTIONAL     = 0x08, /**< Device is functioning properly */
    ACPI_STA_BATTERY        = 0x10, /**< Battery is present */
} ACPI_DEVICE_STATUS;

/**
 * @brief ACPI device information
 */
typedef struct _ACPI_DEVICE_INFO {
    ACPI_DEVICE_HID     HID;            /**< Hardware ID */
    ACPI_DEVICE_CID     CID[8];         /**< Compatible IDs (up to 8) */
    UINT32              uCIDCount;      /**< Number of CIDs */
    ACPI_DEVICE_UID     UID;            /**< Unique ID */
    CHAR8               szName[5];      /**< 4-char name (e.g., "_HID") */
    CHAR8               szPath[256];    /**< Full ACPI path (e.g., "\_SB.PCI0.ISA.COM1") */
    UINT32              uStatus;        /**< Device status (_STA) */
    UINT64              uAddress;       /**< Device address (_ADR) */
    BOOLEAN             bHasPowerResources;  /**< TRUE if _PR0/_PR3 present */
    BOOLEAN             bHasIRQRouting;      /**< TRUE if _PRT present */
} ACPI_DEVICE_INFO;

/**
 * @brief ACPI matcher - matches devices by ACPI HID/CID/UID
 */
typedef struct IIOACPIMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by Hardware ID (HID)
     */
    IO_RETURN (*MatchByHID)(
        struct IIOACPIMatcher   *this,
        CONST CHAR8             *pszHID,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Match device by Compatible ID (CID)
     */
    IO_RETURN (*MatchByCID)(
        struct IIOACPIMatcher   *this,
        CONST CHAR8             *pszCID,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by Unique ID (UID)
     */
    IO_RETURN (*MatchByUID)(
        struct IIOACPIMatcher   *this,
        CONST CHAR8             *pszHID,
        CONST CHAR8             *pszUID,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Match device by ACPI path
     */
    IO_RETURN (*MatchByPath)(
        struct IIOACPIMatcher   *this,
        CONST CHAR8             *pszPath,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Get ACPI device information
     */
    IO_RETURN (*GetDeviceInfo)(
        struct IIOACPIMatcher   *this,
        IIOPlatformDevice       *pDevice,
        ACPI_DEVICE_INFO        *pInfo
    );

    /**
     * @brief Enumerate all ACPI devices
     */
    IO_RETURN (*EnumerateDevices)(
        struct IIOACPIMatcher   *this,
        IIOPlatformDevice       ***pppDevices,
        UINT32                  *puCount
    );
} IIOACPIMatcher;

//=============================================================================
// ISA Plug and Play Device Matching
//=============================================================================

/**
 * @brief ISA PnP Vendor ID (3 characters)
 */
typedef struct _ISAPNP_VENDOR_ID {
    CHAR8   szVendor[4];    /**< 3-char vendor ID + null (e.g., "PNP") */
    UINT16  uVendorCode;    /**< Compressed vendor code */
} ISAPNP_VENDOR_ID;

/**
 * @brief ISA PnP Device ID
 */
typedef struct _ISAPNP_DEVICE_ID {
    ISAPNP_VENDOR_ID    Vendor;     /**< Vendor ID */
    UINT16              uProduct;   /**< Product ID (4 hex digits) */
    CHAR8               szFull[8];  /**< Full ID string (e.g., "PNP0501") */
} ISAPNP_DEVICE_ID;

/**
 * @brief ISA PnP resource types
 */
typedef enum _ISAPNP_RESOURCE_TYPE {
    ISAPNP_RES_IO           = 0,
    ISAPNP_RES_MEMORY       = 1,
    ISAPNP_RES_IRQ          = 2,
    ISAPNP_RES_DMA          = 3,
} ISAPNP_RESOURCE_TYPE;

/**
 * @brief ISA PnP device information
 */
typedef struct _ISAPNP_DEVICE_INFO {
    ISAPNP_DEVICE_ID    DeviceID;          /**< Device ID (HID) */
    ISAPNP_DEVICE_ID    CompatibleIDs[8];  /**< Compatible IDs (CIDs) */
    UINT32              uCompatibleCount;  /**< Number of compatible IDs */
    UINT32              uSerialNumber;     /**< Serial number */
    UINT8               uCSN;              /**< Card Select Number (1-255) */
    UINT8               uLDN;              /**< Logical Device Number (0-255) */
    CHAR8               szName[80];        /**< Device name string */
    UINT16              IOBase[8];         /**< I/O base addresses */
    UINT8               uIOCount;          /**< Number of I/O ranges */
    UINT32              MemBase[4];        /**< Memory base addresses */
    UINT8               uMemCount;         /**< Number of memory ranges */
    UINT8               IRQ[2];            /**< IRQ numbers */
    UINT8               uIRQCount;         /**< Number of IRQs */
    UINT8               DMA[2];            /**< DMA channels */
    UINT8               uDMACount;         /**< Number of DMA channels */
    BOOLEAN             bActivated;        /**< TRUE if device is activated */
} ISAPNP_DEVICE_INFO;

/**
 * @brief ISA PnP matcher - matches ISA Plug and Play devices
 */
typedef struct IIOISAPnPMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by ISA PnP ID
     */
    IO_RETURN (*MatchByID)(
        struct IIOISAPnPMatcher *this,
        CONST CHAR8             *pszPnPID,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Match devices by compatible ID
     */
    IO_RETURN (*MatchByCompatibleID)(
        struct IIOISAPnPMatcher *this,
        CONST CHAR8             *pszCompatID,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by Card Select Number and Logical Device Number
     */
    IO_RETURN (*MatchByCSNLDN)(
        struct IIOISAPnPMatcher *this,
        UINT8                   uCSN,
        UINT8                   uLDN,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Get ISA PnP device information
     */
    IO_RETURN (*GetDeviceInfo)(
        struct IIOISAPnPMatcher *this,
        IIOPlatformDevice       *pDevice,
        ISAPNP_DEVICE_INFO      *pInfo
    );

    /**
     * @brief Activate ISA PnP device
     */
    IO_RETURN (*ActivateDevice)(
        struct IIOISAPnPMatcher *this,
        IIOPlatformDevice       *pDevice
    );

    /**
     * @brief Deactivate ISA PnP device
     */
    IO_RETURN (*DeactivateDevice)(
        struct IIOISAPnPMatcher *this,
        IIOPlatformDevice       *pDevice
    );

    /**
     * @brief Enumerate all ISA PnP devices
     */
    IO_RETURN (*EnumerateDevices)(
        struct IIOISAPnPMatcher *this,
        IIOPlatformDevice       ***pppDevices,
        UINT32                  *puCount
    );
} IIOISAPnPMatcher;

//=============================================================================
// Device Tree (DTB) Device Matching
//=============================================================================

/**
 * @brief Device Tree property
 */
typedef struct _DT_PROPERTY {
    CHAR8       szName[64];     /**< Property name */
    VOID        *pValue;        /**< Property value (binary data) */
    UINT32      uLength;        /**< Length of value in bytes */
} DT_PROPERTY;

/**
 * @brief Device Tree node information
 */
typedef struct _DT_NODE_INFO {
    CHAR8           szName[64];         /**< Node name */
    CHAR8           szFullPath[256];    /**< Full path in tree */
    CHAR8           szCompatible[256];  /**< "compatible" property */
    CHAR8           szDeviceType[64];   /**< "device_type" property */
    UINT64          uReg[8];            /**< "reg" property (base addresses) */
    UINT32          uRegCount;          /**< Number of reg entries */
    UINT32          uPHandle;           /**< phandle (node reference) */
    DT_PROPERTY     *pProperties;       /**< Array of all properties */
    UINT32          uPropertyCount;     /**< Number of properties */
} DT_NODE_INFO;

/**
 * @brief Device Tree matcher - matches devices in Device Tree
 */
typedef struct IIODeviceTreeMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by compatible string
     */
    IO_RETURN (*MatchByCompatible)(
        struct IIODeviceTreeMatcher *this,
        CONST CHAR8                 *pszCompatible,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device by device type
     */
    IO_RETURN (*MatchByDeviceType)(
        struct IIODeviceTreeMatcher *this,
        CONST CHAR8                 *pszDeviceType,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device by path
     */
    IO_RETURN (*MatchByPath)(
        struct IIODeviceTreeMatcher *this,
        CONST CHAR8                 *pszPath,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Match device by phandle
     */
    IO_RETURN (*MatchByPHandle)(
        struct IIODeviceTreeMatcher *this,
        UINT32                      uPHandle,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Get Device Tree node information
     */
    IO_RETURN (*GetNodeInfo)(
        struct IIODeviceTreeMatcher *this,
        IIOPlatformDevice           *pDevice,
        DT_NODE_INFO                *pInfo
    );

    /**
     * @brief Get property value
     */
    IO_RETURN (*GetProperty)(
        struct IIODeviceTreeMatcher *this,
        IIOPlatformDevice           *pDevice,
        CONST CHAR8                 *pszProperty,
        VOID                        *pValue,
        UINT32                      *puLength
    );

    /**
     * @brief Enumerate all Device Tree nodes
     */
    IO_RETURN (*EnumerateNodes)(
        struct IIODeviceTreeMatcher *this,
        IIOPlatformDevice           ***pppDevices,
        UINT32                      *puCount
    );
} IIODeviceTreeMatcher;

//=============================================================================
// OpenFirmware Device Matching (IEEE 1275)
//=============================================================================

/**
 * @brief OpenFirmware device type
 */
typedef enum _OF_DEVICE_TYPE {
    OF_DEVICE_TYPE_UNKNOWN      = 0,
    OF_DEVICE_TYPE_DISPLAY      = 1,
    OF_DEVICE_TYPE_BLOCK        = 2,
    OF_DEVICE_TYPE_NETWORK      = 3,
    OF_DEVICE_TYPE_SERIAL       = 4,
    OF_DEVICE_TYPE_NVRAM        = 5,
    OF_DEVICE_TYPE_RTC          = 6,
    OF_DEVICE_TYPE_CPU          = 7,
    OF_DEVICE_TYPE_MEMORY       = 8,
} OF_DEVICE_TYPE;

/**
 * @brief OpenFirmware node information
 */
typedef struct _OF_NODE_INFO {
    CHAR8           szName[64];         /**< Node name */
    CHAR8           szFullPath[256];    /**< Full path */
    CHAR8           szDeviceType[64];   /**< "device_type" property */
    CHAR8           szModel[64];        /**< "model" property */
    CHAR8           szCompatible[256];  /**< "compatible" property */
    UINT32          uPHandle;           /**< phandle */
    UINT32          uIHandle;           /**< ihandle (instance handle) */
    UINT64          uReg[8];            /**< "reg" property */
    UINT32          uRegCount;          /**< Number of reg entries */
    BOOLEAN         bHasFCode;          /**< TRUE if FCode present */
} OF_NODE_INFO;

/**
 * @brief OpenFirmware matcher - matches devices via IEEE 1275
 */
typedef struct IIOOpenFirmwareMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by name
     */
    IO_RETURN (*MatchByName)(
        struct IIOOpenFirmwareMatcher   *this,
        CONST CHAR8                     *pszName,
        IIOPlatformDevice               **ppDevices,
        UINT32                          *puCount
    );

    /**
     * @brief Match device by device type
     */
    IO_RETURN (*MatchByDeviceType)(
        struct IIOOpenFirmwareMatcher   *this,
        CONST CHAR8                     *pszDeviceType,
        IIOPlatformDevice               **ppDevices,
        UINT32                          *puCount
    );

    /**
     * @brief Match device by compatible string
     */
    IO_RETURN (*MatchByCompatible)(
        struct IIOOpenFirmwareMatcher   *this,
        CONST CHAR8                     *pszCompatible,
        IIOPlatformDevice               **ppDevices,
        UINT32                          *puCount
    );

    /**
     * @brief Match device by path
     */
    IO_RETURN (*MatchByPath)(
        struct IIOOpenFirmwareMatcher   *this,
        CONST CHAR8                     *pszPath,
        IIOPlatformDevice               **ppDevice
    );

    /**
     * @brief Get OpenFirmware node information
     */
    IO_RETURN (*GetNodeInfo)(
        struct IIOOpenFirmwareMatcher   *this,
        IIOPlatformDevice               *pDevice,
        OF_NODE_INFO                    *pInfo
    );

    /**
     * @brief Get property value
     */
    IO_RETURN (*GetProperty)(
        struct IIOOpenFirmwareMatcher   *this,
        IIOPlatformDevice               *pDevice,
        CONST CHAR8                     *pszProperty,
        VOID                            *pValue,
        UINT32                          *puLength
    );

    /**
     * @brief Call OpenFirmware method
     */
    IO_RETURN (*CallMethod)(
        struct IIOOpenFirmwareMatcher   *this,
        IIOPlatformDevice               *pDevice,
        CONST CHAR8                     *pszMethod,
        UINT32                          *pArgs,
        UINT32                          uArgCount,
        UINT32                          *pResults,
        UINT32                          *puResultCount
    );

    /**
     * @brief Enumerate all OpenFirmware nodes
     */
    IO_RETURN (*EnumerateNodes)(
        struct IIOOpenFirmwareMatcher   *this,
        IIOPlatformDevice               ***pppDevices,
        UINT32                          *puCount
    );
} IIOOpenFirmwareMatcher;

//=============================================================================
// ARC/ARCS Device Matching (Advanced RISC Computing)
//=============================================================================

/**
 * @brief ARC component class
 */
typedef enum _ARC_COMPONENT_CLASS {
    ARC_CLASS_SYSTEM            = 0,
    ARC_CLASS_PROCESSOR         = 1,
    ARC_CLASS_CACHE             = 2,
    ARC_CLASS_ADAPTER           = 3,
    ARC_CLASS_CONTROLLER        = 4,
    ARC_CLASS_PERIPHERAL        = 5,
    ARC_CLASS_MEMORY            = 6,
} ARC_COMPONENT_CLASS;

/**
 * @brief ARC component type
 */
typedef enum _ARC_COMPONENT_TYPE {
    // System types
    ARC_TYPE_ARC                = 0,

    // Processor types
    ARC_TYPE_CPU                = 1,
    ARC_TYPE_FPU                = 2,

    // Cache types
    ARC_TYPE_PRIMARY_ICACHE     = 3,
    ARC_TYPE_PRIMARY_DCACHE     = 4,
    ARC_TYPE_SECONDARY_CACHE    = 5,
    ARC_TYPE_SECONDARY_DCACHE   = 6,

    // Adapter types
    ARC_TYPE_EISA_ADAPTER       = 7,
    ARC_TYPE_TC_ADAPTER         = 8,
    ARC_TYPE_SCSI_ADAPTER       = 9,
    ARC_TYPE_DTI_ADAPTER        = 10,
    ARC_TYPE_MULTI_ADAPTER      = 11,

    // Controller types
    ARC_TYPE_DISK_CONTROLLER    = 12,
    ARC_TYPE_TAPE_CONTROLLER    = 13,
    ARC_TYPE_CDROM_CONTROLLER   = 14,
    ARC_TYPE_WORM_CONTROLLER    = 15,
    ARC_TYPE_SERIAL_CONTROLLER  = 16,
    ARC_TYPE_NETWORK_CONTROLLER = 17,
    ARC_TYPE_DISPLAY_CONTROLLER = 18,
    ARC_TYPE_PARALLEL_CONTROLLER= 19,
    ARC_TYPE_POINTER_CONTROLLER = 20,
    ARC_TYPE_KEYBOARD_CONTROLLER= 21,
    ARC_TYPE_AUDIO_CONTROLLER   = 22,
    ARC_TYPE_OTHER_CONTROLLER   = 23,

    // Peripheral types
    ARC_TYPE_DISK_PERIPHERAL    = 24,
    ARC_TYPE_FLOPPY_PERIPHERAL  = 25,
    ARC_TYPE_TAPE_PERIPHERAL    = 26,
    ARC_TYPE_MODEM_PERIPHERAL   = 27,
    ARC_TYPE_MONITOR_PERIPHERAL = 28,
    ARC_TYPE_PRINTER_PERIPHERAL = 29,
    ARC_TYPE_POINTER_PERIPHERAL = 30,
    ARC_TYPE_KEYBOARD_PERIPHERAL= 31,
    ARC_TYPE_TERMINAL_PERIPHERAL= 32,
    ARC_TYPE_OTHER_PERIPHERAL   = 33,
    ARC_TYPE_LINE_PERIPHERAL    = 34,
    ARC_TYPE_NETWORK_PERIPHERAL = 35,

    // Memory types
    ARC_TYPE_MEMORY_UNIT        = 36,
} ARC_COMPONENT_TYPE;

/**
 * @brief ARC component flags
 */
typedef enum _ARC_COMPONENT_FLAGS {
    ARC_FLAG_FAILED             = 0x01,
    ARC_FLAG_READ_ONLY          = 0x02,
    ARC_FLAG_REMOVABLE          = 0x04,
    ARC_FLAG_CONSOLE_IN         = 0x08,
    ARC_FLAG_CONSOLE_OUT        = 0x10,
    ARC_FLAG_INPUT              = 0x20,
    ARC_FLAG_OUTPUT             = 0x40,
} ARC_COMPONENT_FLAGS;

/**
 * @brief ARC component information
 */
typedef struct _ARC_COMPONENT_INFO {
    ARC_COMPONENT_CLASS Class;          /**< Component class */
    ARC_COMPONENT_TYPE  Type;           /**< Component type */
    UINT32              uFlags;         /**< Component flags */
    UINT16              uVersion;       /**< Version */
    UINT16              uRevision;      /**< Revision */
    UINT32              uKey;           /**< Key (device-specific) */
    UINT64              uAffinityMask;  /**< Processor affinity mask */
    UINT32              uConfigDataSize;/**< Configuration data size */
    UINT32              uIdentifierLen; /**< Identifier string length */
    CHAR8               szIdentifier[128]; /**< Identifier string */
    CHAR8               szPath[256];    /**< ARC path (e.g., "scsi(0)disk(0)rdisk(0)partition(1)") */
} ARC_COMPONENT_INFO;

/**
 * @brief ARC/ARCS matcher - matches devices via ARC/ARCS firmware
 */
typedef struct IIOARCMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by class and type
     */
    IO_RETURN (*MatchByClassType)(
        struct IIOARCMatcher    *this,
        ARC_COMPONENT_CLASS     eClass,
        ARC_COMPONENT_TYPE      eType,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by ARC path
     */
    IO_RETURN (*MatchByPath)(
        struct IIOARCMatcher    *this,
        CONST CHAR8             *pszPath,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Match device by identifier string
     */
    IO_RETURN (*MatchByIdentifier)(
        struct IIOARCMatcher    *this,
        CONST CHAR8             *pszIdentifier,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Get ARC component information
     */
    IO_RETURN (*GetComponentInfo)(
        struct IIOARCMatcher    *this,
        IIOPlatformDevice       *pDevice,
        ARC_COMPONENT_INFO      *pInfo
    );

    /**
     * @brief Get configuration data
     */
    IO_RETURN (*GetConfigurationData)(
        struct IIOARCMatcher    *this,
        IIOPlatformDevice       *pDevice,
        VOID                    *pData,
        UINT32                  *puSize
    );

    /**
     * @brief Enumerate all ARC components
     */
    IO_RETURN (*EnumerateComponents)(
        struct IIOARCMatcher    *this,
        IIOPlatformDevice       ***pppDevices,
        UINT32                  *puCount
    );
} IIOARCMatcher;

//=============================================================================
// PCI/PCIe Matcher (Modern Systems)
//=============================================================================

/**
 * @brief Legacy bus types
 */
typedef enum _LEGACY_BUS_TYPE {
    LEGACY_BUS_NUBUS        = 1,    /**< NuBus (Apple Macintosh) */
    LEGACY_BUS_ZORRO        = 2,    /**< Zorro II/III (Commodore Amiga) */
    LEGACY_BUS_SBUS         = 3,    /**< SBus (Sun Microsystems) */
    LEGACY_BUS_TURBOCHANNEL = 4,    /**< TURBOchannel (DEC) */
    LEGACY_BUS_MCA          = 5,    /**< Micro Channel Architecture (IBM PS/2) */
    LEGACY_BUS_VMEBUS       = 6,    /**< VMEbus (industrial/military) */
    LEGACY_BUS_UNIBUS       = 7,    /**< UNIBUS (DEC PDP-11) */
    LEGACY_BUS_QBUS         = 8,    /**< Q-bus (DEC PDP-11/VAX) */
    LEGACY_BUS_CBUS         = 9,    /**< C-bus (NEC PC-9801) */
    LEGACY_BUS_S100         = 10,   /**< S-100 (MITS Altair 8800) */
} LEGACY_BUS_TYPE;

/**
 * @brief PCI device matcher - matches PCI/PCIe devices
 */
typedef struct IIOPCIMatcher {
    IUnknown    Base;

    /**
     * @brief Match device by vendor ID and device ID
     */
    IO_RETURN (*MatchByVendorDevice)(
        struct IIOPCIMatcher    *this,
        UINT16                  uVendorID,
        UINT16                  uDeviceID,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by class code
     */
    IO_RETURN (*MatchByClass)(
        struct IIOPCIMatcher    *this,
        UINT8                   uBaseClass,
        UINT8                   uSubClass,
        UINT8                   uProgIF,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by subsystem vendor/device ID
     */
    IO_RETURN (*MatchBySubsystem)(
        struct IIOPCIMatcher    *this,
        UINT16                  uSubVendorID,
        UINT16                  uSubDeviceID,
        IIOPlatformDevice       **ppDevices,
        UINT32                  *puCount
    );

    /**
     * @brief Match device by bus/device/function
     */
    IO_RETURN (*MatchByBDF)(
        struct IIOPCIMatcher    *this,
        UINT8                   uBus,
        UINT8                   uDevice,
        UINT8                   uFunction,
        IIOPlatformDevice       **ppDevice
    );

    /**
     * @brief Enumerate all PCI devices
     */
    IO_RETURN (*EnumerateDevices)(
        struct IIOPCIMatcher    *this,
        IIOPlatformDevice       ***pppDevices,
        UINT32                  *puCount
    );
} IIOPCIMatcher;

//=============================================================================
// Legacy Bus Matcher (Vintage Systems)
//=============================================================================

/**
 * @brief Legacy bus matcher - universal matcher for classic computer buses
 */
typedef struct IIOLegacyBusMatcher {
    IUnknown    Base;

    /**
     * @brief Match device on NuBus by slot and board ID
     */
    IO_RETURN (*MatchNuBusDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT8                       uSlot,
        UINT32                      uBoardID,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Match device on Zorro bus by manufacturer and product
     */
    IO_RETURN (*MatchZorroDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT16                      uManufacturer,
        UINT8                       uProduct,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device on SBus by name or compatible string
     */
    IO_RETURN (*MatchSBusDevice)(
        struct IIOLegacyBusMatcher  *this,
        CONST CHAR8                 *pszName,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device on TURBOchannel by module ID
     */
    IO_RETURN (*MatchTURBOchannelDevice)(
        struct IIOLegacyBusMatcher  *this,
        CONST CHAR8                 *pszModuleID,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device on MCA by adapter ID
     */
    IO_RETURN (*MatchMCADevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT16                      uAdapterID,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device on VMEbus by manufacturer and board ID
     */
    IO_RETURN (*MatchVMEDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT32                      uManufacturerID,
        UINT32                      uBoardID,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Match device on UNIBUS by CSR address
     */
    IO_RETURN (*MatchUNIBUSDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT32                      uCSRAddress,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Match device on Q-bus by CSR address
     */
    IO_RETURN (*MatchQBusDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT32                      uCSRAddress,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Match device on C-bus by I/O base address
     */
    IO_RETURN (*MatchCBusDevice)(
        struct IIOLegacyBusMatcher  *this,
        UINT16                      uIOBase,
        IIOPlatformDevice           **ppDevice
    );

    /**
     * @brief Match device on S-100 bus by board type
     */
    IO_RETURN (*MatchS100Device)(
        struct IIOLegacyBusMatcher  *this,
        UINT8                       uBoardType,
        IIOPlatformDevice           **ppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Enumerate all devices on a specific legacy bus
     */
    IO_RETURN (*EnumerateByBusType)(
        struct IIOLegacyBusMatcher  *this,
        LEGACY_BUS_TYPE             eBusType,
        IIOPlatformDevice           ***pppDevices,
        UINT32                      *puCount
    );

    /**
     * @brief Detect which legacy buses are present in system
     */
    IO_RETURN (*DetectBuses)(
        struct IIOLegacyBusMatcher  *this,
        LEGACY_BUS_TYPE             *pBusTypes,
        UINT32                      *puCount
    );
} IIOLegacyBusMatcher;

//=============================================================================
// Generic Platform Device Interface
//=============================================================================

/**
 * @brief Generic platform device interface
 */
typedef struct IIOPlatformDevice {
    IIOService  Base;

    /**
     * @brief Get platform firmware type
     */
    IO_RETURN (*GetFirmwareType)(
        struct IIOPlatformDevice    *this,
        PLATFORM_FIRMWARE_TYPE      *pType
    );

    /**
     * @brief Get device name
     */
    IO_RETURN (*GetDeviceName)(
        struct IIOPlatformDevice    *this,
        CHAR8                       *pszName,
        UINT32                      uNameLen
    );

    /**
     * @brief Get device path
     */
    IO_RETURN (*GetDevicePath)(
        struct IIOPlatformDevice    *this,
        CHAR8                       *pszPath,
        UINT32                      uPathLen
    );

    /**
     * @brief Get resource information
     */
    IO_RETURN (*GetResources)(
        struct IIOPlatformDevice    *this,
        VOID                        *pResources,
        UINT32                      *puSize
    );

    /**
     * @brief Set device power state
     */
    IO_RETURN (*SetPowerState)(
        struct IIOPlatformDevice    *this,
        UINT32                      uPowerState
    );

    /**
     * @brief Get matcher interface for this device
     */
    IO_RETURN (*GetMatcher)(
        struct IIOPlatformDevice    *this,
        REFIID                      riid,
        VOID                        **ppMatcher
    );
} IIOPlatformDevice;

//=============================================================================
// Common Device IDs and Strings
//=============================================================================

/**
 * @brief Common ACPI HIDs
 */
#define ACPI_HID_PNP0501    "PNP0501"   /**< 16550A-compatible COM port */
#define ACPI_HID_PNP0510    "PNP0510"   /**< Generic IRDA-compatible device */
#define ACPI_HID_PNP0400    "PNP0400"   /**< Standard LPT printer port */
#define ACPI_HID_PNP0700    "PNP0700"   /**< PC standard floppy disk controller */
#define ACPI_HID_PNP0C01    "PNP0C01"   /**< System board */
#define ACPI_HID_PNP0C02    "PNP0C02"   /**< PnP motherboard resources */
#define ACPI_HID_PNP0C04    "PNP0C04"   /**< Math coprocessor */
#define ACPI_HID_PNP0C0C    "PNP0C0C"   /**< Power button device */
#define ACPI_HID_PNP0C0D    "PNP0C0D"   /**< Lid device */
#define ACPI_HID_PNP0C0E    "PNP0C0E"   /**< Sleep button device */
#define ACPI_HID_PNP0A03    "PNP0A03"   /**< PCI bus */
#define ACPI_HID_PNP0A08    "PNP0A08"   /**< PCI Express bus */

/**
 * @brief Common Device Tree compatible strings
 */
#define DT_COMPAT_SIMPLE_BUS        "simple-bus"
#define DT_COMPAT_ARM_CORTEX_A53    "arm,cortex-a53"
#define DT_COMPAT_ARM_CORTEX_A72    "arm,cortex-a72"
#define DT_COMPAT_ARM_GIC_V2        "arm,gic-400"
#define DT_COMPAT_ARM_GIC_V3        "arm,gic-v3"
#define DT_COMPAT_ARM_PL011         "arm,pl011"
#define DT_COMPAT_ARM_PL022         "arm,pl022"
#define DT_COMPAT_SDHCI             "sdhci"
#define DT_COMPAT_MMC               "mmc"
#define DT_COMPAT_VIRTIO_MMIO       "virtio,mmio"

/**
 * @brief Common ISA PnP IDs
 */
#define ISAPNP_ID_PNP0501   "PNP0501"   /**< 16550A-compatible COM port */
#define ISAPNP_ID_PNP0400   "PNP0400"   /**< Standard LPT printer port */
#define ISAPNP_ID_PNP0303   "PNP0303"   /**< IBM Enhanced (101/102-key) */
#define ISAPNP_ID_PNP0F13   "PNP0F13"   /**< PS/2 Port for PS/2-style Mice */
#define ISAPNP_ID_PNP0700   "PNP0700"   /**< PC standard floppy disk controller */
#define ISAPNP_ID_PNP0B00   "PNP0B00"   /**< AT Real-Time Clock */

//=============================================================================
// Utility Macros
//=============================================================================

/**
 * @brief Create ACPI HID from string
 */
#define ACPI_HID_FROM_STRING(str, hid) \
    do { \
        strncpy((hid).String, (str), sizeof((hid).String) - 1); \
        (hid).String[sizeof((hid).String) - 1] = '\0'; \
        (hid).Integer = 0; /* TODO: Convert string to integer */ \
    } while (0)

/**
 * @brief Check if ACPI device is present and functional
 */
#define ACPI_DEVICE_IS_FUNCTIONAL(status) \
    (((status) & (ACPI_STA_PRESENT | ACPI_STA_FUNCTIONAL)) == \
     (ACPI_STA_PRESENT | ACPI_STA_FUNCTIONAL))

/**
 * @brief Create ISA PnP ID from string
 */
#define ISAPNP_ID_FROM_STRING(str, id) \
    do { \
        strncpy((id).szFull, (str), sizeof((id).szFull) - 1); \
        (id).szFull[sizeof((id).szFull) - 1] = '\0'; \
        /* TODO: Parse vendor and product */ \
    } while (0)

//=============================================================================
// Function Declarations
//=============================================================================

/**
 * @brief Initialize platform device enumeration
 */
IO_RETURN IOPlatformInitialize(VOID);

/**
 * @brief Detect platform firmware type
 */
IO_RETURN IOPlatformDetectFirmware(PLATFORM_FIRMWARE_TYPE *pType);

/**
 * @brief Get ACPI matcher instance
 */
IO_RETURN IOPlatformGetACPIMatcher(IIOACPIMatcher **ppMatcher);

/**
 * @brief Get ISA PnP matcher instance
 */
IO_RETURN IOPlatformGetISAPnPMatcher(IIOISAPnPMatcher **ppMatcher);

/**
 * @brief Get Device Tree matcher instance
 */
IO_RETURN IOPlatformGetDeviceTreeMatcher(IIODeviceTreeMatcher **ppMatcher);

/**
 * @brief Get OpenFirmware matcher instance
 */
IO_RETURN IOPlatformGetOpenFirmwareMatcher(IIOOpenFirmwareMatcher **ppMatcher);

/**
 * @brief Get ARC matcher instance
 */
IO_RETURN IOPlatformGetARCMatcher(IIOARCMatcher **ppMatcher);

/**
 * @brief Get PCI matcher instance
 */
IO_RETURN IOPlatformGetPCIMatcher(IIOPCIMatcher **ppMatcher);

/**
 * @brief Get Legacy Bus matcher instance
 */
IO_RETURN IOPlatformGetLegacyBusMatcher(IIOLegacyBusMatcher **ppMatcher);

/**
 * @brief Enumerate all platform devices
 */
IO_RETURN IOPlatformEnumerateDevices(
    IIOPlatformDevice   ***pppDevices,
    UINT32              *puCount
);

#ifdef __cplusplus
}
#endif

#endif // IOKIT_PLATFORM_H
