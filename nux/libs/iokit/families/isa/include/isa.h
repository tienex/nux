/**
 * @file isa.h
 * @brief ISA/EISA/VLB Family Interface - Legacy PC Bus Support
 *
 * This header defines the ISA (Industry Standard Architecture), EISA (Extended ISA),
 * and VLB (VESA Local Bus) family interfaces for legacy PC expansion buses.
 *
 * Supported buses:
 * - ISA 8-bit (XT bus): 4.77 MHz, 8-bit data, I/O 0x000-0x3FF
 * - ISA 16-bit (AT bus): 8 MHz, 16-bit data, IRQ 0-15, DMA 0-7
 * - EISA: 32-bit, 8.33 MHz, 33 MB/s burst, bus mastering
 * - VLB: 32-bit local bus, 33/40 MHz, 132 MB/s, CPU-tied
 * - ISA Plug and Play: Automatic resource configuration
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_ISA_H
#define IOKIT_ISA_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOISABus interface GUID
 * {1A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6D}
 */
DEFINE_GUID(IID_IIOISABus,
    0x1A2B3C4D, 0x5E6F, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
 * @brief IIOISADevice interface GUID
 * {2B3C4D5E-6F7A-8B9C-0D1E-2F3A4B5C6D7E}
 */
DEFINE_GUID(IID_IIOISADevice,
    0x2B3C4D5E, 0x6F7A, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

/**
 * @brief IIOEISABus interface GUID
 * {3C4D5E6F-7A8B-9C0D-1E2F-3A4B5C6D7E8F}
 */
DEFINE_GUID(IID_IIOEISABus,
    0x3C4D5E6F, 0x7A8B, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
 * @brief IIOEISADevice interface GUID
 * {4D5E6F7A-8B9C-0D1E-2F3A-4B5C6D7E8F9A}
 */
DEFINE_GUID(IID_IIOEISADevice,
    0x4D5E6F7A, 0x8B9C, 0x0D1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A);

/**
 * @brief IIOVLBBus interface GUID
 * {5E6F7A8B-9C0D-1E2F-3A4B-5C6D7E8F9A0B}
 */
DEFINE_GUID(IID_IIOVLBBus,
    0x5E6F7A8B, 0x9C0D, 0x1E2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B);

/**
 * @brief IIOVLBDevice interface GUID
 * {6F7A8B9C-0D1E-2F3A-4B5C-6D7E8F9A0B1C}
 */
DEFINE_GUID(IID_IIOVLBDevice,
    0x6F7A8B9C, 0x0D1E, 0x2F3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C);

/**
 * @brief IIOISAPnPDevice interface GUID
 * {7A8B9C0D-1E2F-3A4B-5C6D-7E8F9A0B1C2D}
 */
DEFINE_GUID(IID_IIOISAPnPDevice,
    0x7A8B9C0D, 0x1E2F, 0x3A4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D);

//
// Bus Type Definitions
//

/**
 * @brief ISA bus types
 */
typedef enum _ISA_BUS_TYPE {
    ISA_BUS_TYPE_UNKNOWN        = 0,
    ISA_BUS_TYPE_8BIT           = 1,    /**< 8-bit ISA (XT bus), 4.77 MHz */
    ISA_BUS_TYPE_16BIT          = 2,    /**< 16-bit ISA (AT bus), 8 MHz */
    ISA_BUS_TYPE_EISA           = 3,    /**< 32-bit EISA, 8.33 MHz */
    ISA_BUS_TYPE_VLB            = 4,    /**< VESA Local Bus, 33/40 MHz */
} ISA_BUS_TYPE;

/**
 * @brief ISA interrupt trigger modes
 */
typedef enum _ISA_IRQ_TRIGGER {
    ISA_IRQ_EDGE_TRIGGERED      = 0,    /**< Edge-triggered interrupt */
    ISA_IRQ_LEVEL_TRIGGERED     = 1,    /**< Level-triggered interrupt */
} ISA_IRQ_TRIGGER;

/**
 * @brief ISA DMA transfer modes
 */
typedef enum _ISA_DMA_MODE {
    ISA_DMA_MODE_DEMAND         = 0x00, /**< Demand mode */
    ISA_DMA_MODE_SINGLE         = 0x40, /**< Single transfer mode */
    ISA_DMA_MODE_BLOCK          = 0x80, /**< Block transfer mode */
    ISA_DMA_MODE_CASCADE        = 0xC0, /**< Cascade mode */
} ISA_DMA_MODE;

/**
 * @brief ISA DMA channel width
 */
typedef enum _ISA_DMA_WIDTH {
    ISA_DMA_8BIT                = 0,    /**< 8-bit DMA (channels 0-3) */
    ISA_DMA_16BIT               = 1,    /**< 16-bit DMA (channels 5-7) */
} ISA_DMA_WIDTH;

/**
 * @brief ISA PnP resource types
 */
typedef enum _ISAPNP_RESOURCE_TYPE {
    ISAPNP_RESOURCE_IO          = 0,    /**< I/O port range */
    ISAPNP_RESOURCE_MEMORY      = 1,    /**< Memory range */
    ISAPNP_RESOURCE_IRQ         = 2,    /**< Interrupt */
    ISAPNP_RESOURCE_DMA         = 3,    /**< DMA channel */
} ISAPNP_RESOURCE_TYPE;

//
// 8259A PIC (Programmable Interrupt Controller) Definitions
//

#define ISA_PIC_MASTER_CMD      0x20    /**< Master PIC command */
#define ISA_PIC_MASTER_DATA     0x21    /**< Master PIC data */
#define ISA_PIC_SLAVE_CMD       0xA0    /**< Slave PIC command */
#define ISA_PIC_SLAVE_DATA      0xA1    /**< Slave PIC data */

#define ISA_PIC_ICW1_INIT       0x10    /**< Initialization command */
#define ISA_PIC_ICW1_ICW4       0x01    /**< ICW4 needed */
#define ISA_PIC_ICW4_8086       0x01    /**< 8086 mode */
#define ISA_PIC_OCW2_EOI        0x20    /**< End of interrupt */

#define ISA_IRQ_CASCADE         2       /**< IRQ 2 connects to slave PIC */

//
// 8237A DMA Controller Definitions
//

#define ISA_DMA1_STATUS         0x08    /**< DMA1 status register */
#define ISA_DMA1_CMD            0x08    /**< DMA1 command register */
#define ISA_DMA1_REQUEST        0x09    /**< DMA1 request register */
#define ISA_DMA1_MASK           0x0A    /**< DMA1 mask register */
#define ISA_DMA1_MODE           0x0B    /**< DMA1 mode register */
#define ISA_DMA1_CLEAR_FF       0x0C    /**< DMA1 clear flip-flop */
#define ISA_DMA1_MASTER_CLEAR   0x0D    /**< DMA1 master clear */
#define ISA_DMA1_CLEAR_MASK     0x0E    /**< DMA1 clear mask register */
#define ISA_DMA1_MASK_ALL       0x0F    /**< DMA1 mask all channels */

#define ISA_DMA2_STATUS         0xD0    /**< DMA2 status register */
#define ISA_DMA2_CMD            0xD0    /**< DMA2 command register */
#define ISA_DMA2_REQUEST        0xD2    /**< DMA2 request register */
#define ISA_DMA2_MASK           0xD4    /**< DMA2 mask register */
#define ISA_DMA2_MODE           0xD6    /**< DMA2 mode register */
#define ISA_DMA2_CLEAR_FF       0xD8    /**< DMA2 clear flip-flop */
#define ISA_DMA2_MASTER_CLEAR   0xDA    /**< DMA2 master clear */
#define ISA_DMA2_CLEAR_MASK     0xDC    /**< DMA2 clear mask register */
#define ISA_DMA2_MASK_ALL       0xDE    /**< DMA2 mask all channels */

// DMA page registers
#define ISA_DMA_PAGE_CH0        0x87    /**< Page register for channel 0 */
#define ISA_DMA_PAGE_CH1        0x83    /**< Page register for channel 1 */
#define ISA_DMA_PAGE_CH2        0x81    /**< Page register for channel 2 */
#define ISA_DMA_PAGE_CH3        0x82    /**< Page register for channel 3 */
#define ISA_DMA_PAGE_CH5        0x8B    /**< Page register for channel 5 */
#define ISA_DMA_PAGE_CH6        0x89    /**< Page register for channel 6 */
#define ISA_DMA_PAGE_CH7        0x8A    /**< Page register for channel 7 */

//
// ISA Plug and Play Definitions
//

#define ISAPNP_ADDRESS_PORT     0x279   /**< PnP address port */
#define ISAPNP_WRITE_DATA       0xA79   /**< PnP write data port */
#define ISAPNP_READ_PORT_MIN    0x203   /**< Minimum read port */
#define ISAPNP_READ_PORT_MAX    0x3FF   /**< Maximum read port */

// PnP registers
#define ISAPNP_REG_SET_RD_PORT  0x00    /**< Set read port */
#define ISAPNP_REG_SERIAL_ISO   0x01    /**< Serial isolation */
#define ISAPNP_REG_CONFIG_CTRL  0x02    /**< Configuration control */
#define ISAPNP_REG_WAKE         0x03    /**< Wake[CSN] */
#define ISAPNP_REG_RESOURCE     0x04    /**< Resource data */
#define ISAPNP_REG_STATUS       0x05    /**< Status */
#define ISAPNP_REG_CARD_SELECT  0x06    /**< Card select number */
#define ISAPNP_REG_LOGICAL_DEV  0x07    /**< Logical device number */

#define ISAPNP_REG_ACTIVATE     0x30    /**< Activate logical device */
#define ISAPNP_REG_IO_BASE_HI   0x60    /**< I/O base address high byte */
#define ISAPNP_REG_IO_BASE_LO   0x61    /**< I/O base address low byte */
#define ISAPNP_REG_IRQ_LEVEL    0x70    /**< IRQ level */
#define ISAPNP_REG_IRQ_TYPE     0x71    /**< IRQ type */
#define ISAPNP_REG_DMA_CHANNEL  0x74    /**< DMA channel */

// PnP resource tags
#define ISAPNP_TAG_VENDOR_SHORT     0x01    /**< Vendor-defined (short) */
#define ISAPNP_TAG_LOGICAL_DEV      0x02    /**< Logical device ID */
#define ISAPNP_TAG_COMPAT_DEV       0x03    /**< Compatible device ID */
#define ISAPNP_TAG_IRQ_FORMAT       0x04    /**< IRQ format */
#define ISAPNP_TAG_DMA_FORMAT       0x05    /**< DMA format */
#define ISAPNP_TAG_START_DF         0x06    /**< Start dependent function */
#define ISAPNP_TAG_END_DF           0x07    /**< End dependent function */
#define ISAPNP_TAG_IO_PORT          0x08    /**< I/O port descriptor */
#define ISAPNP_TAG_FIXED_IO         0x09    /**< Fixed I/O port descriptor */
#define ISAPNP_TAG_VENDOR_LARGE     0x84    /**< Vendor-defined (large) */
#define ISAPNP_TAG_MEMORY_RANGE     0x81    /**< Memory range descriptor */
#define ISAPNP_TAG_IDENTIFIER       0x82    /**< Identifier string */
#define ISAPNP_TAG_VENDOR_STRING    0x83    /**< Vendor-defined string */
#define ISAPNP_TAG_MEMORY32_RANGE   0x85    /**< 32-bit memory range */
#define ISAPNP_TAG_MEMORY32_FIXED   0x86    /**< 32-bit fixed memory range */
#define ISAPNP_TAG_END              0x79    /**< End tag */

//
// Common ISA Device Addresses
//

// Serial ports (COM1-4)
#define ISA_COM1_BASE           0x3F8   /**< COM1 base address */
#define ISA_COM2_BASE           0x2F8   /**< COM2 base address */
#define ISA_COM3_BASE           0x3E8   /**< COM3 base address */
#define ISA_COM4_BASE           0x2E8   /**< COM4 base address */

#define ISA_COM1_IRQ            4       /**< COM1 IRQ */
#define ISA_COM2_IRQ            3       /**< COM2 IRQ */
#define ISA_COM3_IRQ            4       /**< COM3 IRQ */
#define ISA_COM4_IRQ            3       /**< COM4 IRQ */

// Parallel ports (LPT1-3)
#define ISA_LPT1_BASE           0x378   /**< LPT1 base address */
#define ISA_LPT2_BASE           0x278   /**< LPT2 base address */
#define ISA_LPT3_BASE           0x3BC   /**< LPT3 base address */

#define ISA_LPT1_IRQ            7       /**< LPT1 IRQ */
#define ISA_LPT2_IRQ            5       /**< LPT2 IRQ */
#define ISA_LPT3_IRQ            7       /**< LPT3 IRQ */

// Floppy disk controller
#define ISA_FDC_BASE            0x3F0   /**< Floppy controller base */
#define ISA_FDC_IRQ             6       /**< Floppy IRQ */
#define ISA_FDC_DMA             2       /**< Floppy DMA channel */

// IDE/ATA controllers
#define ISA_IDE_PRIMARY_BASE    0x1F0   /**< Primary IDE base */
#define ISA_IDE_PRIMARY_CTRL    0x3F6   /**< Primary IDE control */
#define ISA_IDE_PRIMARY_IRQ     14      /**< Primary IDE IRQ */

#define ISA_IDE_SECONDARY_BASE  0x170   /**< Secondary IDE base */
#define ISA_IDE_SECONDARY_CTRL  0x376   /**< Secondary IDE control */
#define ISA_IDE_SECONDARY_IRQ   15      /**< Secondary IDE IRQ */

// Real-time clock / CMOS
#define ISA_RTC_BASE            0x70    /**< RTC/CMOS base */
#define ISA_RTC_IRQ             8       /**< RTC IRQ */

// Game port
#define ISA_GAMEPORT_BASE       0x200   /**< Game port base */
#define ISA_GAMEPORT_ALT        0x201   /**< Game port alternate */

// Sound cards
#define ISA_SOUNDBLASTER_BASE   0x220   /**< Sound Blaster base */
#define ISA_SOUNDBLASTER_IRQ    5       /**< Sound Blaster IRQ */
#define ISA_SOUNDBLASTER_DMA8   1       /**< Sound Blaster 8-bit DMA */
#define ISA_SOUNDBLASTER_DMA16  5       /**< Sound Blaster 16-bit DMA */

#define ISA_ADLIB_BASE          0x388   /**< AdLib base */
#define ISA_WSS_BASE            0x530   /**< Windows Sound System base */

// VGA
#define ISA_VGA_BASE            0x3B0   /**< VGA register base */
#define ISA_VGA_MEM_BASE        0xA0000 /**< VGA memory base */
#define ISA_VGA_MEM_SIZE        0x20000 /**< VGA memory size (128KB) */

//
// Common ISA PnP Device IDs
//

#define ISAPNP_ID_SERIAL        "PNP0501"   /**< Serial port (16550A) */
#define ISAPNP_ID_PARALLEL      "PNP0400"   /**< Parallel port */
#define ISAPNP_ID_FLOPPY        "PNP0700"   /**< Floppy controller */
#define ISAPNP_ID_KEYBOARD      "PNP0303"   /**< PC/AT keyboard */
#define ISAPNP_ID_MOUSE_PS2     "PNP0F13"   /**< PS/2 mouse */
#define ISAPNP_ID_RTC           "PNP0B00"   /**< Real-time clock */
#define ISAPNP_ID_DMA           "PNP0200"   /**< DMA controller */
#define ISAPNP_ID_PIC           "PNP0000"   /**< Interrupt controller */
#define ISAPNP_ID_TIMER         "PNP0100"   /**< System timer */
#define ISAPNP_ID_SPEAKER       "PNP0800"   /**< System speaker */
#define ISAPNP_ID_COPROCESSOR   "PNP0C04"   /**< Math coprocessor */
#define ISAPNP_ID_MOTHERBOARD   "PNP0C01"   /**< System board */
#define ISAPNP_ID_GAMEPORT      "PNPB02F"   /**< Game port */

//
// Structure Definitions
//

/**
 * @brief ISA I/O port range
 */
typedef struct _ISA_IO_RANGE {
    UINT16      Start;                  /**< Starting I/O address */
    UINT16      Length;                 /**< Length in bytes */
    UINT16      Alignment;              /**< Required alignment */
    BOOLEAN     bDecode16Bit;           /**< 16-bit decode */
} ISA_IO_RANGE;

/**
 * @brief ISA memory range
 */
typedef struct _ISA_MEMORY_RANGE {
    UINT32      Start;                  /**< Starting physical address */
    UINT32      Length;                 /**< Length in bytes */
    UINT32      Alignment;              /**< Required alignment */
    BOOLEAN     bWriteable;             /**< Memory is writeable */
    BOOLEAN     bCacheable;             /**< Memory is cacheable */
    BOOLEAN     bShadowable;            /**< Memory is shadowable */
    BOOLEAN     bExpansionROM;          /**< Expansion ROM */
} ISA_MEMORY_RANGE;

/**
 * @brief ISA interrupt resource
 */
typedef struct _ISA_IRQ {
    UINT8       Level;                  /**< IRQ level (0-15) */
    ISA_IRQ_TRIGGER Trigger;            /**< Edge or level triggered */
    BOOLEAN     bShared;                /**< Can be shared */
    BOOLEAN     bHighTrue;              /**< Active high (vs low) */
} ISA_IRQ;

/**
 * @brief ISA DMA resource
 */
typedef struct _ISA_DMA {
    UINT8       Channel;                /**< DMA channel (0-7) */
    ISA_DMA_WIDTH Width;                /**< 8-bit or 16-bit */
    ISA_DMA_MODE Mode;                  /**< Transfer mode */
    BOOLEAN     bBusMaster;             /**< Bus master capable */
    BOOLEAN     bByteMode;              /**< Byte mode (vs word) */
    BOOLEAN     bCountByByte;           /**< Count by bytes */
    UINT8       Speed;                  /**< Speed (0=compatible, 1=A, 2=B, 3=F) */
} ISA_DMA;

/**
 * @brief ISA device information
 */
typedef struct _ISA_DEVICE_INFO {
    ISA_BUS_TYPE    BusType;            /**< Bus type */
    UINT16          Slot;               /**< Slot number (EISA/VLB) */
    UINT32          EISAID;             /**< EISA ID (compressed) */
    CHAR8           DeviceName[64];     /**< Device name */

    // Resources
    ISA_IO_RANGE    IOPorts[8];         /**< I/O port ranges */
    UINT32          uIOCount;           /**< Number of I/O ranges */

    ISA_MEMORY_RANGE Memory[4];         /**< Memory ranges */
    UINT32          uMemoryCount;       /**< Number of memory ranges */

    ISA_IRQ         IRQs[2];            /**< Interrupt resources */
    UINT32          uIRQCount;          /**< Number of IRQs */

    ISA_DMA         DMAs[2];            /**< DMA resources */
    UINT32          uDMACount;          /**< Number of DMA channels */
} ISA_DEVICE_INFO;

/**
 * @brief EISA device information
 */
typedef struct _EISA_DEVICE_INFO {
    ISA_DEVICE_INFO Base;               /**< Base ISA info */
    UINT8           Slot;               /**< EISA slot (1-15) */
    UINT32          CompressedID;       /**< Compressed EISA ID */
    CHAR8           EISAID[8];          /**< Readable EISA ID (e.g., "ABC1234") */
    UINT16          BoardRevision;      /**< Board revision */
    BOOLEAN         bBusMaster;         /**< Bus mastering capable */
    BOOLEAN         bConfigurable;      /**< Configuration supported */
    UINT8           ConfigData[256];    /**< Configuration data */
} EISA_DEVICE_INFO;

/**
 * @brief VLB device information
 */
typedef struct _VLB_DEVICE_INFO {
    ISA_DEVICE_INFO Base;               /**< Base ISA info */
    UINT8           Slot;               /**< VLB slot (1-3) */
    UINT32          ClockSpeed;         /**< Clock speed in Hz */
    BOOLEAN         bBurstMode;         /**< Burst mode capable */
    BOOLEAN         bWritePosting;      /**< Write posting capable */
    UINT8           WaitStates;         /**< Wait states */
} VLB_DEVICE_INFO;

/**
 * @brief ISA PnP vendor/device ID
 */
typedef struct _ISAPNP_ID {
    UINT32      VendorID;               /**< Vendor ID (compressed) */
    UINT32      DeviceID;               /**< Device ID (compressed) */
    UINT32      SerialNumber;           /**< Serial number */
    CHAR8       VendorString[8];        /**< Vendor string (e.g., "PNP") */
    UINT16      ProductNumber;          /**< Product number (hex) */
    UINT8       Revision;               /**< Revision */
} ISAPNP_ID;

/**
 * @brief ISA PnP device information
 */
typedef struct _ISAPNP_DEVICE_INFO {
    ISA_DEVICE_INFO Base;               /**< Base ISA info */
    ISAPNP_ID       ID;                 /**< PnP ID */
    UINT8           CSN;                /**< Card Select Number */
    UINT8           LogicalDevice;      /**< Logical device number */
    CHAR8           DeviceID[8];        /**< Device ID string (e.g., "PNP0501") */
    CHAR8           CompatibleIDs[4][8];/**< Compatible device IDs */
    UINT32          uCompatibleCount;   /**< Number of compatible IDs */
    BOOLEAN         bActivated;         /**< Device is activated */
    UINT8           ResourceData[2048]; /**< Resource data */
    UINT32          uResourceSize;      /**< Resource data size */
} ISAPNP_DEVICE_INFO;

/**
 * @brief UART types for serial ports
 */
typedef enum _ISA_UART_TYPE {
    ISA_UART_UNKNOWN        = 0,
    ISA_UART_8250           = 1,        /**< Original 8250 */
    ISA_UART_16450          = 2,        /**< 16450 */
    ISA_UART_16550          = 3,        /**< 16550 (broken FIFO) */
    ISA_UART_16550A         = 4,        /**< 16550A (working FIFO) */
    ISA_UART_16650          = 5,        /**< 16650 */
    ISA_UART_16750          = 6,        /**< 16750 */
    ISA_UART_16850          = 7,        /**< 16850 */
    ISA_UART_16950          = 8,        /**< 16950 */
} ISA_UART_TYPE;

/**
 * @brief Parallel port modes
 */
typedef enum _ISA_LPT_MODE {
    ISA_LPT_MODE_SPP        = 0,        /**< Standard Parallel Port */
    ISA_LPT_MODE_EPP        = 1,        /**< Enhanced Parallel Port */
    ISA_LPT_MODE_ECP        = 2,        /**< Extended Capabilities Port */
    ISA_LPT_MODE_ECP_EPP    = 3,        /**< ECP + EPP */
} ISA_LPT_MODE;

//
// Forward Declarations
//

DECLARE_INTERFACE_(IIOISABus, IIOService);
DECLARE_INTERFACE_(IIOISADevice, IIOService);
DECLARE_INTERFACE_(IIOEISABus, IIOService);
DECLARE_INTERFACE_(IIOEISADevice, IIOService);
DECLARE_INTERFACE_(IIOVLBBus, IIOService);
DECLARE_INTERFACE_(IIOVLBDevice, IIOService);
DECLARE_INTERFACE_(IIOISAPnPDevice, IIOService);

//
// Interface Definitions
//

/**
 * @brief IIOISABus - ISA Bus Controller Interface
 *
 * Represents an ISA bus controller and provides methods for device
 * enumeration, resource allocation, and hardware management.
 */
#undef INTERFACE
#define INTERFACE IIOISABus

DECLARE_INTERFACE_(IIOISABus, IIOService)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOService methods (inherited)
    STDMETHOD_(IO_RETURN, Probe)(THIS_
        IIOService *pProvider,
        UINT32 *puProbeScore
        ) PURE;

    STDMETHOD_(IO_RETURN, Start)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Stop)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Terminate)(THIS_
        UINT32 uOptions
        ) PURE;

    STDMETHOD_(IO_RETURN, GetProperty)(THIS_
        CONST CHAR8 *pszKey,
        VOID *pValue,
        UINTN *pcbSize,
        UINT32 *puType
        ) PURE;

    STDMETHOD_(IO_RETURN, SetProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        UINT32 uType
        ) PURE;

    STDMETHOD_(IO_RETURN, GetParentService)(THIS_
        IIOService **ppParent
        ) PURE;

    STDMETHOD_(IO_RETURN, GetChildService)(THIS_
        UINT32 uIndex,
        IIOService **ppChild
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceState)(THIS_
        UINT32 *puState
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceName)(THIS_
        CHAR8 *pszName,
        UINTN cbSize
        ) PURE;

    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        UINT32 uOptions
        ) PURE;

    // IIOISABus specific methods

    /**
     * @brief Get ISA bus information
     *
     * @param pBusType      Receives bus type
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBusInfo)(THIS_
        ISA_BUS_TYPE *pBusType
        ) PURE;

    /**
     * @brief Enumerate ISA devices
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOISADevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Allocate I/O port range
     *
     * @param pRange        I/O range to allocate
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_RESOURCES  Resources unavailable
     */
    STDMETHOD_(IO_RETURN, AllocateIO)(THIS_
        CONST ISA_IO_RANGE *pRange
        ) PURE;

    /**
     * @brief Free I/O port range
     *
     * @param pRange        I/O range to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeIO)(THIS_
        CONST ISA_IO_RANGE *pRange
        ) PURE;

    /**
     * @brief Allocate memory range
     *
     * @param pRange        Memory range to allocate
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_RESOURCES  Resources unavailable
     */
    STDMETHOD_(IO_RETURN, AllocateMemory)(THIS_
        CONST ISA_MEMORY_RANGE *pRange
        ) PURE;

    /**
     * @brief Free memory range
     *
     * @param pRange        Memory range to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeMemory)(THIS_
        CONST ISA_MEMORY_RANGE *pRange
        ) PURE;

    /**
     * @brief Allocate IRQ
     *
     * @param pIRQ          IRQ to allocate
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_RESOURCES  IRQ unavailable
     */
    STDMETHOD_(IO_RETURN, AllocateIRQ)(THIS_
        CONST ISA_IRQ *pIRQ
        ) PURE;

    /**
     * @brief Free IRQ
     *
     * @param pIRQ          IRQ to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeIRQ)(THIS_
        CONST ISA_IRQ *pIRQ
        ) PURE;

    /**
     * @brief Allocate DMA channel
     *
     * @param pDMA          DMA channel to allocate
     *
     * @retval IO_SUCCESS       Allocation successful
     * @retval IO_NO_CHANNELS   Channel unavailable
     */
    STDMETHOD_(IO_RETURN, AllocateDMA)(THIS_
        CONST ISA_DMA *pDMA
        ) PURE;

    /**
     * @brief Free DMA channel
     *
     * @param pDMA          DMA channel to free
     *
     * @retval IO_SUCCESS   Deallocation successful
     */
    STDMETHOD_(IO_RETURN, FreeDMA)(THIS_
        CONST ISA_DMA *pDMA
        ) PURE;

    /**
     * @brief Configure 8259A PIC
     *
     * @param bMaster       TRUE for master PIC, FALSE for slave
     * @param uVector       Interrupt vector base
     *
     * @retval IO_SUCCESS   PIC configured successfully
     */
    STDMETHOD_(IO_RETURN, ConfigurePIC)(THIS_
        BOOLEAN bMaster,
        UINT8 uVector
        ) PURE;

    /**
     * @brief Configure 8237A DMA controller
     *
     * @param uChannel      DMA channel to configure
     * @param pDMA          DMA parameters
     *
     * @retval IO_SUCCESS   DMA configured successfully
     */
    STDMETHOD_(IO_RETURN, ConfigureDMA)(THIS_
        UINT8 uChannel,
        CONST ISA_DMA *pDMA
        ) PURE;

    /**
     * @brief Enable/disable device
     *
     * @param pDevice       Device to enable/disable
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Device state changed
     */
    STDMETHOD_(IO_RETURN, EnableDevice)(THIS_
        IIOISADevice *pDevice,
        BOOLEAN bEnable
        ) PURE;
};

/**
 * @brief IIOISADevice - ISA Device Interface
 *
 * Represents an ISA device and provides methods for resource management
 * and device I/O operations.
 */
#undef INTERFACE
#define INTERFACE IIOISADevice

DECLARE_INTERFACE_(IIOISADevice, IIOService)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOService methods (inherited)
    STDMETHOD_(IO_RETURN, Probe)(THIS_
        IIOService *pProvider,
        UINT32 *puProbeScore
        ) PURE;

    STDMETHOD_(IO_RETURN, Start)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Stop)(THIS_
        IIOService *pProvider
        ) PURE;

    STDMETHOD_(IO_RETURN, Terminate)(THIS_
        UINT32 uOptions
        ) PURE;

    STDMETHOD_(IO_RETURN, GetProperty)(THIS_
        CONST CHAR8 *pszKey,
        VOID *pValue,
        UINTN *pcbSize,
        UINT32 *puType
        ) PURE;

    STDMETHOD_(IO_RETURN, SetProperty)(THIS_
        CONST CHAR8 *pszKey,
        CONST VOID *pValue,
        UINTN cbSize,
        UINT32 uType
        ) PURE;

    STDMETHOD_(IO_RETURN, GetParentService)(THIS_
        IIOService **ppParent
        ) PURE;

    STDMETHOD_(IO_RETURN, GetChildService)(THIS_
        UINT32 uIndex,
        IIOService **ppChild
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceState)(THIS_
        UINT32 *puState
        ) PURE;

    STDMETHOD_(IO_RETURN, GetServiceName)(THIS_
        CHAR8 *pszName,
        UINTN cbSize
        ) PURE;

    STDMETHOD_(IO_RETURN, RegisterService)(THIS_
        UINT32 uOptions
        ) PURE;

    // IIOISADevice specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        ISA_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Read from I/O port
     *
     * @param uPort         Port address
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param puValue       Receives value
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadPort)(THIS_
        UINT16 uPort,
        UINT8 uSize,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Write to I/O port
     *
     * @param uPort         Port address
     * @param uSize         Size (1, 2, or 4 bytes)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WritePort)(THIS_
        UINT16 uPort,
        UINT8 uSize,
        UINT32 uValue
        ) PURE;

    /**
     * @brief Enable interrupt
     *
     * @param uIRQ          IRQ level
     * @param pfnHandler    Interrupt handler
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       Interrupt enabled
     * @retval IO_NO_INTERRUPT  IRQ not available
     */
    STDMETHOD_(IO_RETURN, EnableInterrupt)(THIS_
        UINT8 uIRQ,
        VOID (*pfnHandler)(VOID *pContext),
        VOID *pContext
        ) PURE;

    /**
     * @brief Disable interrupt
     *
     * @param uIRQ          IRQ level
     *
     * @retval IO_SUCCESS   Interrupt disabled
     */
    STDMETHOD_(IO_RETURN, DisableInterrupt)(THIS_
        UINT8 uIRQ
        ) PURE;

    /**
     * @brief Setup DMA transfer
     *
     * @param uChannel      DMA channel
     * @param pBuffer       Buffer address
     * @param cbLength      Transfer length
     * @param bWrite        TRUE for memory-to-device, FALSE for device-to-memory
     *
     * @retval IO_SUCCESS   DMA configured
     */
    STDMETHOD_(IO_RETURN, SetupDMATransfer)(THIS_
        UINT8 uChannel,
        VOID *pBuffer,
        UINT32 cbLength,
        BOOLEAN bWrite
        ) PURE;
};

/**
 * @brief IIOEISABus - EISA Bus Controller Interface
 */
#undef INTERFACE
#define INTERFACE IIOEISABus

DECLARE_INTERFACE_(IIOEISABus, IIOISABus)
{
    // All IIOISABus methods inherited...

    /**
     * @brief Get EISA slot information
     *
     * @param uSlot         Slot number (1-15)
     * @param pInfo         Receives EISA device info
     *
     * @retval IO_SUCCESS   Information retrieved
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, GetSlotInfo)(THIS_
        UINT8 uSlot,
        EISA_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Enable EISA bus mastering
     *
     * @param pDevice       Device to enable
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Bus mastering configured
     */
    STDMETHOD_(IO_RETURN, EnableBusMastering)(THIS_
        IIOEISADevice *pDevice,
        BOOLEAN bEnable
        ) PURE;
};

/**
 * @brief IIOEISADevice - EISA Device Interface
 */
#undef INTERFACE
#define INTERFACE IIOEISADevice

DECLARE_INTERFACE_(IIOEISADevice, IIOISADevice)
{
    // All IIOISADevice methods inherited...

    /**
     * @brief Get EISA device information
     *
     * @param pInfo         Receives EISA device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetEISAInfo)(THIS_
        EISA_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Read EISA configuration data
     *
     * @param uOffset       Configuration offset
     * @param pData         Receives data
     * @param cbLength      Data length
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadConfigData)(THIS_
        UINT8 uOffset,
        VOID *pData,
        UINT32 cbLength
        ) PURE;
};

/**
 * @brief IIOVLBBus - VLB Bus Controller Interface
 */
#undef INTERFACE
#define INTERFACE IIOVLBBus

DECLARE_INTERFACE_(IIOVLBBus, IIOISABus)
{
    // All IIOISABus methods inherited...

    /**
     * @brief Get VLB slot information
     *
     * @param uSlot         Slot number (1-3)
     * @param pInfo         Receives VLB device info
     *
     * @retval IO_SUCCESS   Information retrieved
     * @retval IO_NO_DEVICE No device in slot
     */
    STDMETHOD_(IO_RETURN, GetVLBSlotInfo)(THIS_
        UINT8 uSlot,
        VLB_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Configure VLB timing
     *
     * @param pDevice       Device to configure
     * @param uWaitStates   Number of wait states
     *
     * @retval IO_SUCCESS   Timing configured
     */
    STDMETHOD_(IO_RETURN, ConfigureTiming)(THIS_
        IIOVLBDevice *pDevice,
        UINT8 uWaitStates
        ) PURE;
};

/**
 * @brief IIOVLBDevice - VLB Device Interface
 */
#undef INTERFACE
#define INTERFACE IIOVLBDevice

DECLARE_INTERFACE_(IIOVLBDevice, IIOISADevice)
{
    // All IIOISADevice methods inherited...

    /**
     * @brief Get VLB device information
     *
     * @param pInfo         Receives VLB device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetVLBInfo)(THIS_
        VLB_DEVICE_INFO *pInfo
        ) PURE;
};

/**
 * @brief IIOISAPnPDevice - ISA Plug and Play Device Interface
 */
#undef INTERFACE
#define INTERFACE IIOISAPnPDevice

DECLARE_INTERFACE_(IIOISAPnPDevice, IIOISADevice)
{
    // All IIOISADevice methods inherited...

    /**
     * @brief Get PnP device information
     *
     * @param pInfo         Receives PnP device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetPnPInfo)(THIS_
        ISAPNP_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get vendor ID
     *
     * @param pVendorID     Receives vendor ID
     *
     * @retval IO_SUCCESS   Vendor ID retrieved
     */
    STDMETHOD_(IO_RETURN, GetVendorID)(THIS_
        UINT32 *pVendorID
        ) PURE;

    /**
     * @brief Get device ID
     *
     * @param pszDeviceID   Receives device ID string
     * @param cbSize        Buffer size
     *
     * @retval IO_SUCCESS   Device ID retrieved
     */
    STDMETHOD_(IO_RETURN, GetDeviceID)(THIS_
        CHAR8 *pszDeviceID,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Get resource data
     *
     * @param pData         Receives resource data
     * @param pcbSize       On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS   Resource data retrieved
     */
    STDMETHOD_(IO_RETURN, GetResourceData)(THIS_
        VOID *pData,
        UINT32 *pcbSize
        ) PURE;

    /**
     * @brief Get logical device number
     *
     * @param puLogicalDev  Receives logical device number
     *
     * @retval IO_SUCCESS   Logical device number retrieved
     */
    STDMETHOD_(IO_RETURN, GetLogicalDevice)(THIS_
        UINT8 *puLogicalDev
        ) PURE;

    /**
     * @brief Activate device
     *
     * @param bActivate     TRUE to activate, FALSE to deactivate
     *
     * @retval IO_SUCCESS   Device activated/deactivated
     */
    STDMETHOD_(IO_RETURN, ActivateDevice)(THIS_
        BOOLEAN bActivate
        ) PURE;

    /**
     * @brief Set device resources
     *
     * @param pInfo         Device information with resources
     *
     * @retval IO_SUCCESS   Resources configured
     */
    STDMETHOD_(IO_RETURN, SetResources)(THIS_
        CONST ISA_DEVICE_INFO *pInfo
        ) PURE;
};

#undef INTERFACE

//
// Convenience Macros
//

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOISABus_GetBusInfo(p,a)               (p)->lpVtbl->GetBusInfo(p,a)
#define IIOISABus_EnumerateDevices(p,a,b)       (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOISABus_AllocateIO(p,a)               (p)->lpVtbl->AllocateIO(p,a)
#define IIOISABus_FreeIO(p,a)                   (p)->lpVtbl->FreeIO(p,a)
#define IIOISABus_AllocateMemory(p,a)           (p)->lpVtbl->AllocateMemory(p,a)
#define IIOISABus_FreeMemory(p,a)               (p)->lpVtbl->FreeMemory(p,a)
#define IIOISABus_AllocateIRQ(p,a)              (p)->lpVtbl->AllocateIRQ(p,a)
#define IIOISABus_FreeIRQ(p,a)                  (p)->lpVtbl->FreeIRQ(p,a)
#define IIOISABus_AllocateDMA(p,a)              (p)->lpVtbl->AllocateDMA(p,a)
#define IIOISABus_FreeDMA(p,a)                  (p)->lpVtbl->FreeDMA(p,a)
#define IIOISABus_ConfigurePIC(p,a,b)           (p)->lpVtbl->ConfigurePIC(p,a,b)
#define IIOISABus_ConfigureDMA(p,a,b)           (p)->lpVtbl->ConfigureDMA(p,a,b)
#define IIOISABus_EnableDevice(p,a,b)           (p)->lpVtbl->EnableDevice(p,a,b)

#define IIOISADevice_GetDeviceInfo(p,a)         (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOISADevice_ReadPort(p,a,b,c)          (p)->lpVtbl->ReadPort(p,a,b,c)
#define IIOISADevice_WritePort(p,a,b,c)         (p)->lpVtbl->WritePort(p,a,b,c)
#define IIOISADevice_EnableInterrupt(p,a,b,c)   (p)->lpVtbl->EnableInterrupt(p,a,b,c)
#define IIOISADevice_DisableInterrupt(p,a)      (p)->lpVtbl->DisableInterrupt(p,a)
#define IIOISADevice_SetupDMATransfer(p,a,b,c,d) (p)->lpVtbl->SetupDMATransfer(p,a,b,c,d)

#define IIOEISABus_GetSlotInfo(p,a,b)           (p)->lpVtbl->GetSlotInfo(p,a,b)
#define IIOEISABus_EnableBusMastering(p,a,b)    (p)->lpVtbl->EnableBusMastering(p,a,b)

#define IIOEISADevice_GetEISAInfo(p,a)          (p)->lpVtbl->GetEISAInfo(p,a)
#define IIOEISADevice_ReadConfigData(p,a,b,c)   (p)->lpVtbl->ReadConfigData(p,a,b,c)

#define IIOVLBBus_GetVLBSlotInfo(p,a,b)         (p)->lpVtbl->GetVLBSlotInfo(p,a,b)
#define IIOVLBBus_ConfigureTiming(p,a,b)        (p)->lpVtbl->ConfigureTiming(p,a,b)

#define IIOVLBDevice_GetVLBInfo(p,a)            (p)->lpVtbl->GetVLBInfo(p,a)

#define IIOISAPnPDevice_GetPnPInfo(p,a)         (p)->lpVtbl->GetPnPInfo(p,a)
#define IIOISAPnPDevice_GetVendorID(p,a)        (p)->lpVtbl->GetVendorID(p,a)
#define IIOISAPnPDevice_GetDeviceID(p,a,b)      (p)->lpVtbl->GetDeviceID(p,a,b)
#define IIOISAPnPDevice_GetResourceData(p,a,b)  (p)->lpVtbl->GetResourceData(p,a,b)
#define IIOISAPnPDevice_GetLogicalDevice(p,a)   (p)->lpVtbl->GetLogicalDevice(p,a)
#define IIOISAPnPDevice_ActivateDevice(p,a)     (p)->lpVtbl->ActivateDevice(p,a)
#define IIOISAPnPDevice_SetResources(p,a)       (p)->lpVtbl->SetResources(p,a)

#endif

//
// Public API Functions
//

/**
 * @brief Initialize ISA subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
ISAInitialize(
    VOID
    );

/**
 * @brief Shutdown ISA subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
ISAShutdown(
    VOID
    );

/**
 * @brief Create an ISA bus instance
 *
 * @param BusType       Bus type (ISA/EISA/VLB)
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS           Bus created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_UNSUPPORTED       Bus type not supported
 */
IO_RETURN
IOISABusCreate(
    ISA_BUS_TYPE BusType,
    IIOISABus **ppBus
    );

/**
 * @brief Create an ISA device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
IOISADeviceCreate(
    CONST CHAR8 *pszName,
    IIOISADevice **ppDevice
    );

/**
 * @brief Detect ISA Plug and Play devices
 *
 * @param ppDevices     Receives array of PnP device interfaces
 * @param puCount       On input: max devices; On output: actual count
 *
 * @retval IO_SUCCESS   Detection successful
 */
IO_RETURN
ISAPnPDetectDevices(
    IIOISAPnPDevice **ppDevices,
    UINT32 *puCount
    );

/**
 * @brief Decode EISA ID from compressed format
 *
 * @param uCompressed   Compressed EISA ID
 * @param pszID         Receives readable ID string (8 bytes)
 *
 * @retval IO_SUCCESS   ID decoded successfully
 */
IO_RETURN
ISADecodeEISAID(
    UINT32 uCompressed,
    CHAR8 *pszID
    );

/**
 * @brief Encode EISA ID to compressed format
 *
 * @param pszID         Readable ID string (e.g., "ABC1234")
 * @param pCompressed   Receives compressed ID
 *
 * @retval IO_SUCCESS   ID encoded successfully
 */
IO_RETURN
ISAEncodeEISAID(
    CONST CHAR8 *pszID,
    UINT32 *pCompressed
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_ISA_H */
