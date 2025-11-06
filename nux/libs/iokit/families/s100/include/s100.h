/**
 * @file s100.h
 * @brief S-100 Bus Family Interface - The Legendary Microcomputer Bus
 *
 * This header defines the S-100 bus family interface, the first industry-standard
 * expansion bus for microcomputers. Named for its 100-pin edge connector, the S-100
 * bus powered the microcomputer revolution from 1975-1985.
 *
 * Bus Specifications:
 * - 100-pin edge connector (50 pins per side, 0.125" spacing)
 * - 8-bit data bus (D0-D7) with optional 16-bit extension (D8-D15)
 * - 16-bit address bus (A0-A15) with 8-bit extension (A16-A23) for 16MB addressing
 * - IEEE 696-1983 standard compliance (S-100 standardization)
 * - Multiple clock signals: φ1, φ2 (2 MHz typical)
 * - Vectored interrupts: VI0-VI7 (8 priority levels)
 * - DMA support with bus arbitration
 * - +8V, +16V, -16V power rails (early) or ±8V (later)
 * - Unregulated power: typically +8V @ 20A, ±16V @ 3A
 *
 * Famous S-100 Systems:
 * - MITS Altair 8800 (1975) - The machine that started it all
 * - IMSAI 8080 (1975) - First Altair clone, used in WarGames movie
 * - Cromemco Z-2 (1977) - High-end business system
 * - North Star Horizon (1977) - Integrated floppy disk system
 * - Processor Technology Sol-20 (1976) - First all-in-one S-100 system
 * - Vector Graphic Vector 1 (1977) - Business-oriented system
 * - CompuPro (Godbout) systems (1979-1989) - Professional workstations
 *
 * Historical Significance:
 * The S-100 bus emerged from the MITS Altair 8800, using the Intel 8080's
 * pinout as its basis. It became the de facto standard for early microcomputers,
 * with hundreds of manufacturers producing compatible cards. The IEEE 696 standard
 * (1983) formalized and extended the bus, adding 16-bit data paths and 24-bit
 * addressing, but by then newer buses (ISA, PC/XT) were taking over.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_S100_H
#define IOKIT_S100_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOS100Bus interface GUID
 * {5100B080-1975-4D49-5453-414C544149}  // S100B080-1975-MITS-ALTAIR
 */
DEFINE_GUID(IID_IIOS100Bus,
    0x5100B080, 0x1975, 0x4D49, 0x54, 0x53, 0x41, 0x4C, 0x54, 0x41, 0x49);

/**
 * @brief IIOS100Device interface GUID
 * {5100D3C3-1975-4956-4345-435241440000}  // S100DEVC-1975-IVECARD
 */
DEFINE_GUID(IID_IIOS100Device,
    0x5100D3C3, 0x1975, 0x4956, 0x43, 0x45, 0x43, 0x52, 0x41, 0x44, 0x00);

//
// S-100 Bus Pin Definitions (100-pin edge connector)
//

// Power rails
#define S100_PIN_GND            1, 20, 50, 100  /**< Ground pins */
#define S100_PIN_PLUS8V         2       /**< +8V unregulated */
#define S100_PIN_PLUS16V        3       /**< +16V unregulated */
#define S100_PIN_MINUS16V       4       /**< -16V unregulated */

// Data bus (8-bit base)
#define S100_PIN_DO0            81      /**< Data bit 0 (output) */
#define S100_PIN_DO1            82      /**< Data bit 1 */
#define S100_PIN_DO2            83      /**< Data bit 2 */
#define S100_PIN_DO3            84      /**< Data bit 3 */
#define S100_PIN_DO4            85      /**< Data bit 4 */
#define S100_PIN_DO5            86      /**< Data bit 5 */
#define S100_PIN_DO6            87      /**< Data bit 6 */
#define S100_PIN_DO7            88      /**< Data bit 7 */

#define S100_PIN_DI0            89      /**< Data bit 0 (input) */
#define S100_PIN_DI1            90      /**< Data bit 1 */
#define S100_PIN_DI2            91      /**< Data bit 2 */
#define S100_PIN_DI3            92      /**< Data bit 3 */
#define S100_PIN_DI4            93      /**< Data bit 4 */
#define S100_PIN_DI5            94      /**< Data bit 5 */
#define S100_PIN_DI6            95      /**< Data bit 6 */
#define S100_PIN_DI7            96      /**< Data bit 7 */

// IEEE 696 16-bit data extension
#define S100_PIN_DO8            15      /**< Data bit 8 (IEEE 696) */
#define S100_PIN_DO9            16      /**< Data bit 9 */
#define S100_PIN_DO10           17      /**< Data bit 10 */
#define S100_PIN_DO11           18      /**< Data bit 11 */
#define S100_PIN_DO12           19      /**< Data bit 12 */
#define S100_PIN_DO13           21      /**< Data bit 13 */
#define S100_PIN_DO14           22      /**< Data bit 14 */
#define S100_PIN_DO15           23      /**< Data bit 15 */

// Address bus (16-bit base)
#define S100_PIN_A0             79      /**< Address bit 0 */
#define S100_PIN_A1             80      /**< Address bit 1 */
#define S100_PIN_A2             65      /**< Address bit 2 */
#define S100_PIN_A3             66      /**< Address bit 3 */
#define S100_PIN_A4             67      /**< Address bit 4 */
#define S100_PIN_A5             68      /**< Address bit 5 */
#define S100_PIN_A6             69      /**< Address bit 6 */
#define S100_PIN_A7             70      /**< Address bit 7 */
#define S100_PIN_A8             78      /**< Address bit 8 */
#define S100_PIN_A9             77      /**< Address bit 9 */
#define S100_PIN_A10            76      /**< Address bit 10 */
#define S100_PIN_A11            75      /**< Address bit 11 */
#define S100_PIN_A12            74      /**< Address bit 12 */
#define S100_PIN_A13            73      /**< Address bit 13 */
#define S100_PIN_A14            72      /**< Address bit 14 */
#define S100_PIN_A15            71      /**< Address bit 15 */

// IEEE 696 24-bit address extension (16MB addressing)
#define S100_PIN_A16            64      /**< Address bit 16 (IEEE 696) */
#define S100_PIN_A17            63      /**< Address bit 17 */
#define S100_PIN_A18            62      /**< Address bit 18 */
#define S100_PIN_A19            61      /**< Address bit 19 */
#define S100_PIN_A20            60      /**< Address bit 20 */
#define S100_PIN_A21            59      /**< Address bit 21 */
#define S100_PIN_A22            58      /**< Address bit 22 */
#define S100_PIN_A23            57      /**< Address bit 23 */

// Control signals
#define S100_PIN_PHI1           24      /**< Phase 1 clock (2 MHz) */
#define S100_PIN_PHI2           49      /**< Phase 2 clock */
#define S100_PIN_PSTVAL         26      /**< Status valid */
#define S100_PIN_PHLDA          27      /**< Hold acknowledge */
#define S100_PIN_PRDY           3       /**< Ready (wait state) */
#define S100_PIN_PWR            28      /**< Write strobe */
#define S100_PIN_PHOLD          29      /**< Hold request (DMA) */
#define S100_PIN_PRESET         30      /**< Reset */
#define S100_PIN_PINT           31      /**< Interrupt request */

// Status signals (encoded 8080 status)
#define S100_PIN_PSYNC          76      /**< Sync (instruction fetch) */
#define S100_PIN_PWO            77      /**< Write out */
#define S100_PIN_STACK          65      /**< Stack access */
#define S100_PIN_HLTA           66      /**< Halt acknowledge */
#define S100_PIN_OUT            67      /**< Output operation */
#define S100_PIN_INP            68      /**< Input operation */
#define S100_PIN_MEMR           69      /**< Memory read */
#define S100_PIN_PDBOUT         70      /**< Data bus output */

// IEEE 696 control signals
#define S100_PIN_MWRITE         5       /**< Memory write (IEEE 696) */
#define S100_PIN_MREAD          6       /**< Memory read */
#define S100_PIN_IOWRITE        7       /**< I/O write */
#define S100_PIN_IOREAD         8       /**< I/O read */
#define S100_PIN_SIXTN          9       /**< 16-bit transfer */
#define S100_PIN_INTA           32      /**< Interrupt acknowledge */

// Vectored interrupts (IEEE 696)
#define S100_PIN_VI0            33      /**< Vector interrupt 0 (highest) */
#define S100_PIN_VI1            34      /**< Vector interrupt 1 */
#define S100_PIN_VI2            35      /**< Vector interrupt 2 */
#define S100_PIN_VI3            36      /**< Vector interrupt 3 */
#define S100_PIN_VI4            37      /**< Vector interrupt 4 */
#define S100_PIN_VI5            38      /**< Vector interrupt 5 */
#define S100_PIN_VI6            39      /**< Vector interrupt 6 */
#define S100_PIN_VI7            40      /**< Vector interrupt 7 (lowest) */

//
// S-100 Bus Types and Standards
//

/**
 * @brief S-100 bus standard types
 */
typedef enum _S100_STANDARD {
    S100_STANDARD_ORIGINAL  = 0,        /**< Original Altair (1975) */
    S100_STANDARD_CROMEMCO  = 1,        /**< Cromemco extensions */
    S100_STANDARD_IEEE696   = 2,        /**< IEEE 696-1983 */
} S100_STANDARD;

/**
 * @brief S-100 data bus width
 */
typedef enum _S100_DATA_WIDTH {
    S100_DATA_8BIT          = 0,        /**< 8-bit data bus */
    S100_DATA_16BIT         = 1,        /**< 16-bit data bus (IEEE 696) */
} S100_DATA_WIDTH;

/**
 * @brief S-100 address bus width
 */
typedef enum _S100_ADDR_WIDTH {
    S100_ADDR_16BIT         = 0,        /**< 64KB addressing */
    S100_ADDR_24BIT         = 1,        /**< 16MB addressing (IEEE 696) */
} S100_ADDR_WIDTH;

/**
 * @brief S-100 card types
 */
typedef enum _S100_CARD_TYPE {
    S100_CARD_CPU           = 0,        /**< CPU card */
    S100_CARD_MEMORY        = 1,        /**< Memory (RAM/ROM) */
    S100_CARD_SERIAL        = 2,        /**< Serial I/O (SIO, UART) */
    S100_CARD_PARALLEL      = 3,        /**< Parallel I/O */
    S100_CARD_FLOPPY        = 4,        /**< Floppy disk controller */
    S100_CARD_HARDDISK      = 5,        /**< Hard disk controller */
    S100_CARD_GRAPHICS      = 6,        /**< Graphics/video */
    S100_CARD_PROTOTYPE     = 7,        /**< Prototyping card */
    S100_CARD_RTC           = 8,        /**< Real-time clock */
    S100_CARD_DMA           = 9,        /**< DMA controller */
    S100_CARD_MISC          = 255,      /**< Miscellaneous */
} S100_CARD_TYPE;

/**
 * @brief S-100 CPU types
 */
typedef enum _S100_CPU_TYPE {
    S100_CPU_8080           = 0,        /**< Intel 8080 */
    S100_CPU_8085           = 1,        /**< Intel 8085 */
    S100_CPU_Z80            = 2,        /**< Zilog Z80 */
    S100_CPU_8086           = 3,        /**< Intel 8086 */
    S100_CPU_8088           = 4,        /**< Intel 8088 */
    S100_CPU_80186          = 5,        /**< Intel 80186 */
    S100_CPU_80286          = 6,        /**< Intel 80286 */
    S100_CPU_68000          = 7,        /**< Motorola 68000 */
    S100_CPU_68010          = 8,        /**< Motorola 68010 */
    S100_CPU_6502           = 9,        /**< MOS 6502 */
    S100_CPU_6809           = 10,       /**< Motorola 6809 */
} S100_CPU_TYPE;

/**
 * @brief S-100 interrupt priority levels (IEEE 696)
 */
typedef enum _S100_IRQ_LEVEL {
    S100_IRQ_VI0            = 0,        /**< Highest priority */
    S100_IRQ_VI1            = 1,
    S100_IRQ_VI2            = 2,
    S100_IRQ_VI3            = 3,
    S100_IRQ_VI4            = 4,
    S100_IRQ_VI5            = 5,
    S100_IRQ_VI6            = 6,
    S100_IRQ_VI7            = 7,        /**< Lowest priority */
} S100_IRQ_LEVEL;

//
// S-100 Card Database Structures
//

/**
 * @brief S-100 card manufacturer
 */
typedef enum _S100_MANUFACTURER {
    S100_MFG_MITS           = 0,        /**< MITS (Altair) */
    S100_MFG_CROMEMCO       = 1,        /**< Cromemco */
    S100_MFG_GODBOUT        = 2,        /**< Godbout Electronics (CompuPro) */
    S100_MFG_VECTOR         = 3,        /**< Vector Graphic */
    S100_MFG_NORTHSTAR      = 4,        /**< North Star Computers */
    S100_MFG_IMSAI          = 5,        /**< IMSAI */
    S100_MFG_PROCESSOR_TECH = 6,        /**< Processor Technology */
    S100_MFG_ITHACA         = 7,        /**< Ithaca Intersystems */
    S100_MFG_SSM            = 8,        /**< SSM Microcomputer Products */
    S100_MFG_JADE           = 9,        /**< Jade Computer Products */
    S100_MFG_SD_SYSTEMS     = 10,       /**< SD Systems */
    S100_MFG_COMPUPRO       = 11,       /**< CompuPro (later Godbout) */
    S100_MFG_SEATTLE        = 12,       /**< Seattle Computer */
    S100_MFG_DYNABYTE       = 13,       /**< Dynabyte */
    S100_MFG_CALIFORNIA     = 14,       /**< California Computer Systems */
    S100_MFG_SOLID_STATE    = 15,       /**< Solid State Music */
    S100_MFG_TARBELL        = 16,       /**< Tarbell Electronics */
    S100_MFG_PARASITIC      = 17,       /**< Parasitic Engineering */
    S100_MFG_MICROPOLIS     = 18,       /**< Micropolis */
    S100_MFG_MORROW         = 19,       /**< Morrow Designs */
    S100_MFG_TELTEK         = 20,       /**< Teltek */
    S100_MFG_OPTRONICS      = 21,       /**< Optronics Technology */
    S100_MFG_CROMIX         = 22,       /**< Cromix (Cromemco OS division) */
} S100_MANUFACTURER;

/**
 * @brief S-100 card database entry
 */
typedef struct _S100_CARD_INFO {
    S100_MANUFACTURER   Manufacturer;   /**< Card manufacturer */
    CONST CHAR8        *pszName;        /**< Card name */
    S100_CARD_TYPE      CardType;       /**< Card type */
    UINT16              Year;           /**< Introduction year */

    // Card capabilities
    BOOLEAN             bIEEE696;       /**< IEEE 696 compliant */
    S100_DATA_WIDTH     DataWidth;      /**< 8-bit or 16-bit */
    S100_ADDR_WIDTH     AddrWidth;      /**< 16-bit or 24-bit */

    // Resources
    UINT32              uIOBase;        /**< I/O port base (0 = none) */
    UINT32              uIOSize;        /**< I/O port range size */
    UINT32              uMemBase;       /**< Memory base address (0 = configurable) */
    UINT32              uMemSize;       /**< Memory size in bytes */

    BOOLEAN             bDMA;           /**< DMA capable */
    BOOLEAN             bInterrupt;     /**< Interrupt capable */
    S100_IRQ_LEVEL      IRQLevel;       /**< Default IRQ level */

    // Specific information
    union {
        struct {
            S100_CPU_TYPE   CPUType;    /**< CPU type */
            UINT32          ClockMHz;   /**< Clock speed in MHz */
        } CPU;

        struct {
            BOOLEAN         bStatic;    /**< Static RAM vs dynamic */
            BOOLEAN         bEPROM;     /**< EPROM/EEPROM support */
        } Memory;

        struct {
            UINT8           NumPorts;   /**< Number of serial ports */
            UINT32          MaxBaud;    /**< Maximum baud rate */
        } Serial;

        struct {
            UINT8           NumDrives;  /**< Number of drives supported */
            BOOLEAN         bDoubleDensity; /**< Double density support */
        } Floppy;

        struct {
            UINT32          MaxCapacity; /**< Maximum capacity in MB */
            CONST CHAR8    *pszInterface; /**< Interface type (ST-506, SASI, etc.) */
        } HardDisk;
    } Info;

    CONST CHAR8        *pszDescription; /**< Detailed description */
} S100_CARD_INFO;

/**
 * @brief S-100 bus configuration
 */
typedef struct _S100_BUS_CONFIG {
    S100_STANDARD       Standard;       /**< Bus standard */
    S100_DATA_WIDTH     DataWidth;      /**< Data bus width */
    S100_ADDR_WIDTH     AddrWidth;      /**< Address bus width */
    UINT32              ClockHz;        /**< Bus clock frequency (Hz) */
    UINT8               NumSlots;       /**< Number of card slots */
    BOOLEAN             bVectoredInt;   /**< Vectored interrupts supported */
    BOOLEAN             bDMA;           /**< DMA supported */
} S100_BUS_CONFIG;

/**
 * @brief S-100 device information
 */
typedef struct _S100_DEVICE_INFO {
    UINT8               Slot;           /**< Slot number (0-based) */
    S100_CARD_INFO      CardInfo;       /**< Card information */
    BOOLEAN             bPresent;       /**< Card present in slot */
    BOOLEAN             bEnabled;       /**< Card enabled */
} S100_DEVICE_INFO;

/**
 * @brief S-100 interrupt handler function
 */
typedef VOID (*S100_IRQ_HANDLER)(
    VOID *pContext
    );

//
// Forward Declarations
//

DECLARE_INTERFACE_(IIOS100Bus, IIOService);
DECLARE_INTERFACE_(IIOS100Device, IIOService);

//
// Interface Definitions
//

/**
 * @brief IIOS100Bus - S-100 Bus Controller Interface
 *
 * Represents an S-100 bus controller and provides methods for card
 * enumeration, bus arbitration, interrupt management, and DMA operations.
 */
#undef INTERFACE
#define INTERFACE IIOS100Bus

DECLARE_INTERFACE_(IIOS100Bus, IIOService)
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

    // IIOS100Bus specific methods

    /**
     * @brief Get S-100 bus configuration
     *
     * @param pConfig       Receives bus configuration
     *
     * @retval IO_SUCCESS   Configuration retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetBusConfig)(THIS_
        S100_BUS_CONFIG *pConfig
        ) PURE;

    /**
     * @brief Enumerate S-100 cards
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS   Enumeration successful
     */
    STDMETHOD_(IO_RETURN, EnumerateCards)(THIS_
        IIOS100Device **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Read from S-100 memory space
     *
     * @param uAddress      Memory address (16-bit or 24-bit)
     * @param pBuffer       Buffer to receive data
     * @param cbLength      Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadMemory)(THIS_
        UINT32 uAddress,
        VOID *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Write to S-100 memory space
     *
     * @param uAddress      Memory address (16-bit or 24-bit)
     * @param pBuffer       Buffer containing data to write
     * @param cbLength      Number of bytes to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WriteMemory)(THIS_
        UINT32 uAddress,
        CONST VOID *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Read from S-100 I/O port
     *
     * @param uPort         I/O port address (8-bit)
     * @param puValue       Receives value read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadIO)(THIS_
        UINT8 uPort,
        UINT8 *puValue
        ) PURE;

    /**
     * @brief Write to S-100 I/O port
     *
     * @param uPort         I/O port address (8-bit)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WriteIO)(THIS_
        UINT8 uPort,
        UINT8 uValue
        ) PURE;

    /**
     * @brief Register interrupt handler (IEEE 696 vectored interrupts)
     *
     * @param Level         Interrupt level (VI0-VI7)
     * @param pfnHandler    Interrupt handler function
     * @param pContext      Handler context
     *
     * @retval IO_SUCCESS       Handler registered
     * @retval IO_NO_INTERRUPT  Level not available
     */
    STDMETHOD_(IO_RETURN, RegisterInterrupt)(THIS_
        S100_IRQ_LEVEL Level,
        S100_IRQ_HANDLER pfnHandler,
        VOID *pContext
        ) PURE;

    /**
     * @brief Unregister interrupt handler
     *
     * @param Level         Interrupt level (VI0-VI7)
     *
     * @retval IO_SUCCESS   Handler unregistered
     */
    STDMETHOD_(IO_RETURN, UnregisterInterrupt)(THIS_
        S100_IRQ_LEVEL Level
        ) PURE;

    /**
     * @brief Request bus for DMA transfer
     *
     * @param pDevice       Device requesting bus
     *
     * @retval IO_SUCCESS       Bus granted
     * @retval IO_BUSY          Bus not available
     */
    STDMETHOD_(IO_RETURN, RequestBus)(THIS_
        IIOS100Device *pDevice
        ) PURE;

    /**
     * @brief Release bus after DMA transfer
     *
     * @param pDevice       Device releasing bus
     *
     * @retval IO_SUCCESS   Bus released
     */
    STDMETHOD_(IO_RETURN, ReleaseBus)(THIS_
        IIOS100Device *pDevice
        ) PURE;

    /**
     * @brief Assert RESET line on bus
     *
     * @retval IO_SUCCESS   Reset asserted
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS
        ) PURE;

    /**
     * @brief Get card information by slot
     *
     * @param uSlot         Slot number
     * @param pInfo         Receives card information
     *
     * @retval IO_SUCCESS   Information retrieved
     * @retval IO_NO_DEVICE No card in slot
     */
    STDMETHOD_(IO_RETURN, GetSlotInfo)(THIS_
        UINT8 uSlot,
        S100_DEVICE_INFO *pInfo
        ) PURE;
};

/**
 * @brief IIOS100Device - S-100 Device/Card Interface
 *
 * Represents an S-100 expansion card and provides methods for device
 * I/O, interrupt handling, and DMA operations.
 */
#undef INTERFACE
#define INTERFACE IIOS100Device

DECLARE_INTERFACE_(IIOS100Device, IIOService)
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

    // IIOS100Device specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        S100_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get card information
     *
     * @param pCardInfo     Receives card information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetCardInfo)(THIS_
        S100_CARD_INFO *pCardInfo
        ) PURE;

    /**
     * @brief Read from device I/O ports
     *
     * @param uOffset       Offset from device I/O base
     * @param puValue       Receives value read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadPort)(THIS_
        UINT8 uOffset,
        UINT8 *puValue
        ) PURE;

    /**
     * @brief Write to device I/O ports
     *
     * @param uOffset       Offset from device I/O base
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WritePort)(THIS_
        UINT8 uOffset,
        UINT8 uValue
        ) PURE;

    /**
     * @brief Read from device memory space
     *
     * @param uOffset       Offset from device memory base
     * @param pBuffer       Buffer to receive data
     * @param cbLength      Number of bytes to read
     *
     * @retval IO_SUCCESS   Read successful
     */
    STDMETHOD_(IO_RETURN, ReadDeviceMemory)(THIS_
        UINT32 uOffset,
        VOID *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Write to device memory space
     *
     * @param uOffset       Offset from device memory base
     * @param pBuffer       Buffer containing data to write
     * @param cbLength      Number of bytes to write
     *
     * @retval IO_SUCCESS   Write successful
     */
    STDMETHOD_(IO_RETURN, WriteDeviceMemory)(THIS_
        UINT32 uOffset,
        CONST VOID *pBuffer,
        UINT32 cbLength
        ) PURE;

    /**
     * @brief Enable device
     *
     * @param bEnable       TRUE to enable, FALSE to disable
     *
     * @retval IO_SUCCESS   Device state changed
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS_
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Reset device
     *
     * @retval IO_SUCCESS   Device reset
     */
    STDMETHOD_(IO_RETURN, ResetDevice)(THIS
        ) PURE;
};

#undef INTERFACE

//
// Convenience Macros
//

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOS100Bus_GetBusConfig(p,a)            (p)->lpVtbl->GetBusConfig(p,a)
#define IIOS100Bus_EnumerateCards(p,a,b)        (p)->lpVtbl->EnumerateCards(p,a,b)
#define IIOS100Bus_ReadMemory(p,a,b,c)          (p)->lpVtbl->ReadMemory(p,a,b,c)
#define IIOS100Bus_WriteMemory(p,a,b,c)         (p)->lpVtbl->WriteMemory(p,a,b,c)
#define IIOS100Bus_ReadIO(p,a,b)                (p)->lpVtbl->ReadIO(p,a,b)
#define IIOS100Bus_WriteIO(p,a,b)               (p)->lpVtbl->WriteIO(p,a,b)
#define IIOS100Bus_RegisterInterrupt(p,a,b,c)   (p)->lpVtbl->RegisterInterrupt(p,a,b,c)
#define IIOS100Bus_UnregisterInterrupt(p,a)     (p)->lpVtbl->UnregisterInterrupt(p,a)
#define IIOS100Bus_RequestBus(p,a)              (p)->lpVtbl->RequestBus(p,a)
#define IIOS100Bus_ReleaseBus(p,a)              (p)->lpVtbl->ReleaseBus(p,a)
#define IIOS100Bus_Reset(p)                     (p)->lpVtbl->Reset(p)
#define IIOS100Bus_GetSlotInfo(p,a,b)           (p)->lpVtbl->GetSlotInfo(p,a,b)

#define IIOS100Device_GetDeviceInfo(p,a)        (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOS100Device_GetCardInfo(p,a)          (p)->lpVtbl->GetCardInfo(p,a)
#define IIOS100Device_ReadPort(p,a,b)           (p)->lpVtbl->ReadPort(p,a,b)
#define IIOS100Device_WritePort(p,a,b)          (p)->lpVtbl->WritePort(p,a,b)
#define IIOS100Device_ReadDeviceMemory(p,a,b,c) (p)->lpVtbl->ReadDeviceMemory(p,a,b,c)
#define IIOS100Device_WriteDeviceMemory(p,a,b,c) (p)->lpVtbl->WriteDeviceMemory(p,a,b,c)
#define IIOS100Device_Enable(p,a)               (p)->lpVtbl->Enable(p,a)
#define IIOS100Device_ResetDevice(p)            (p)->lpVtbl->ResetDevice(p)

#endif

//
// Public API Functions
//

/**
 * @brief Initialize S-100 subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
S100Initialize(
    VOID
    );

/**
 * @brief Shutdown S-100 subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
S100Shutdown(
    VOID
    );

/**
 * @brief Create an S-100 bus instance
 *
 * @param pConfig       Bus configuration
 * @param ppBus         Receives bus interface
 *
 * @retval IO_SUCCESS           Bus created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid configuration
 */
IO_RETURN
IOS100BusCreate(
    CONST S100_BUS_CONFIG *pConfig,
    IIOS100Bus **ppBus
    );

/**
 * @brief Create an S-100 device instance
 *
 * @param pCardInfo     Card information
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid card info
 */
IO_RETURN
IOS100DeviceCreate(
    CONST S100_CARD_INFO *pCardInfo,
    IIOS100Device **ppDevice
    );

/**
 * @brief Get card information by name
 *
 * @param pszName       Card name
 * @param pCardInfo     Receives card information
 *
 * @retval IO_SUCCESS   Card found
 * @retval IO_NOT_FOUND Card not in database
 */
IO_RETURN
S100GetCardByName(
    CONST CHAR8 *pszName,
    S100_CARD_INFO *pCardInfo
    );

/**
 * @brief Get all cards by manufacturer
 *
 * @param Manufacturer  Manufacturer ID
 * @param pCards        Receives array of card info
 * @param puCount       On input: max cards; On output: actual count
 *
 * @retval IO_SUCCESS   Cards retrieved
 */
IO_RETURN
S100GetCardsByManufacturer(
    S100_MANUFACTURER Manufacturer,
    S100_CARD_INFO *pCards,
    UINT32 *puCount
    );

/**
 * @brief Get manufacturer name
 *
 * @param Manufacturer  Manufacturer ID
 *
 * @return Manufacturer name string
 */
CONST CHAR8 *
S100GetManufacturerName(
    S100_MANUFACTURER Manufacturer
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_S100_H */
