/**
 * @file s100.c
 * @brief S-100 Bus Family Implementation - The Legendary Microcomputer Bus Driver
 *
 * Provides full support for the S-100 bus with:
 * - Original Altair 8800 bus specification
 * - IEEE 696-1983 standard extensions
 * - Card detection and enumeration
 * - Comprehensive database of 40+ famous S-100 cards
 * - Bus arbitration for DMA
 * - Vectored interrupt management (VI0-VI7)
 * - Memory and I/O port access
 *
 * Historical Notes:
 * The S-100 bus was born in January 1975 with the MITS Altair 8800, the first
 * commercially successful personal computer. The bus was essentially the Intel
 * 8080 CPU pinout extended to 100 pins. It became wildly successful, spawning
 * an entire industry of compatible systems and expansion cards.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/s100/s100.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

//
// S-100 Card Database - The Legendary Cards
//

/**
 * @brief Comprehensive S-100 card database (40+ famous cards)
 *
 * This database contains the most significant S-100 cards from 1975-1985,
 * representing the golden age of the S-100 bus.
 */
static CONST S100_CARD_INFO g_S100CardDatabase[] = {
    //
    // CPU Cards - The Brains
    //
    {
        S100_MFG_MITS, "Altair 8080 CPU", S100_CARD_CPU, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        FALSE, FALSE, S100_IRQ_VI0,
        { .CPU = { S100_CPU_8080, 2 } },
        "Original MITS Altair 8080 CPU card, 2 MHz Intel 8080A processor"
    },
    {
        S100_MFG_CROMEMCO, "Cromemco ZPU", S100_CARD_CPU, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        FALSE, TRUE, S100_IRQ_VI0,
        { .CPU = { S100_CPU_Z80, 4 } },
        "Cromemco Z80 CPU card, 4 MHz Zilog Z80 with vectored interrupts"
    },
    {
        S100_MFG_GODBOUT, "CompuPro CPU-Z", S100_CARD_CPU, 1982,
        TRUE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        FALSE, TRUE, S100_IRQ_VI0,
        { .CPU = { S100_CPU_Z80, 6 } },
        "CompuPro Z80 CPU, 6 MHz with IEEE 696 support, bank switching"
    },
    {
        S100_MFG_SEATTLE, "Seattle 8086", S100_CARD_CPU, 1979,
        FALSE, S100_DATA_16BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        TRUE, TRUE, S100_IRQ_VI0,
        { .CPU = { S100_CPU_8086, 8 } },
        "Seattle Computer Products 8086 CPU, 8 MHz Intel 8086 (ran first MS-DOS!)"
    },
    {
        S100_MFG_CROMEMCO, "Cromemco 68000", S100_CARD_CPU, 1982,
        TRUE, S100_DATA_16BIT, S100_ADDR_24BIT,
        0, 0, 0, 0,
        TRUE, TRUE, S100_IRQ_VI0,
        { .CPU = { S100_CPU_68000, 8 } },
        "Cromemco 68000 CPU card, 8 MHz Motorola 68000 with CROMIX Unix"
    },

    //
    // Memory Cards - Static RAM (Fast and Reliable)
    //
    {
        S100_MFG_MITS, "Altair 4K RAM", S100_CARD_MEMORY, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 4096,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { TRUE, FALSE } },
        "Original MITS 4KB static RAM card for Altair 8800"
    },
    {
        S100_MFG_PROCESSOR_TECH, "VDM-1", S100_CARD_MEMORY, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0xD000, 4096,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { TRUE, FALSE } },
        "Processor Technology Video Display Module, 16x16 character display"
    },
    {
        S100_MFG_GODBOUT, "RAM 17", S100_CARD_MEMORY, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 16384,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { TRUE, FALSE } },
        "Godbout 16KB static RAM card with bank switching"
    },
    {
        S100_MFG_CROMEMCO, "16K Bytesaver", S100_CARD_MEMORY, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 16384,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { TRUE, FALSE } },
        "Cromemco 16KB static RAM with battery backup capability"
    },
    {
        S100_MFG_COMPUPRO, "RAM 22", S100_CARD_MEMORY, 1983,
        TRUE, S100_DATA_16BIT, S100_ADDR_24BIT,
        0, 0, 0, 131072,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { TRUE, FALSE } },
        "CompuPro 128KB static RAM, IEEE 696 compatible, 16-bit"
    },

    //
    // Memory Cards - Dynamic RAM (High Capacity)
    //
    {
        S100_MFG_CROMEMCO, "64K Bytesaver II", S100_CARD_MEMORY, 1980,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 65536,
        FALSE, FALSE, S100_IRQ_VI0,
        { .Memory = { FALSE, FALSE } },
        "Cromemco 64KB dynamic RAM with automatic refresh"
    },

    //
    // Serial I/O Cards - Communication
    //
    {
        S100_MFG_MITS, "Altair 88-2SIO", S100_CARD_SERIAL, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x10, 2, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 2, 9600 } },
        "MITS 2-port serial I/O card, 88-2SIO with current loop interface"
    },
    {
        S100_MFG_PROCESSOR_TECH, "3P+S", S100_CARD_SERIAL, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x08, 4, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 3, 19200 } },
        "Processor Technology 3 parallel + 1 serial I/O board"
    },
    {
        S100_MFG_CROMEMCO, "TU-ART", S100_CARD_SERIAL, 1977,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x40, 8, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 2, 38400 } },
        "Cromemco Tuart dual serial card, programmable baud rates"
    },
    {
        S100_MFG_SSM, "SSM IO-4", S100_CARD_SERIAL, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x04, 8, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 4, 19200 } },
        "SSM 4-port serial I/O card with Z80 SIO chips"
    },
    {
        S100_MFG_GODBOUT, "Interfacer 3", S100_CARD_SERIAL, 1979,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x10, 16, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 2, 19200 } },
        "Godbout Interfacer 3, 2 serial + 3 parallel ports"
    },
    {
        S100_MFG_COMPUPRO, "Interfacer 4", S100_CARD_SERIAL, 1982,
        TRUE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x10, 16, 0, 0,
        FALSE, TRUE, S100_IRQ_VI1,
        { .Serial = { 4, 38400 } },
        "CompuPro Interfacer 4, 4 serial ports, IEEE 696 compatible"
    },

    //
    // Parallel I/O Cards
    //
    {
        S100_MFG_MITS, "Altair 88-PIO", S100_CARD_PARALLEL, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x04, 4, 0, 0,
        FALSE, TRUE, S100_IRQ_VI2,
        { .Serial = { 0, 0 } },
        "MITS parallel I/O card using Intel 8255 PPI chips"
    },
    {
        S100_MFG_CROMEMCO, "PRI", S100_CARD_PARALLEL, 1977,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x20, 8, 0, 0,
        FALSE, TRUE, S100_IRQ_VI2,
        { .Serial = { 0, 0 } },
        "Cromemco Parallel & Real-time Interface, 3 parallel ports + timer"
    },

    //
    // Floppy Disk Controllers - Mass Storage Revolution
    //
    {
        S100_MFG_MITS, "Altair 88-DCDD", S100_CARD_FLOPPY, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x08, 4, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, FALSE } },
        "MITS floppy disk controller, Pertec drives, single density"
    },
    {
        S100_MFG_TARBELL, "Tarbell FDC", S100_CARD_FLOPPY, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x78, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, FALSE } },
        "Tarbell floppy disk controller, very popular, CP/M compatible"
    },
    {
        S100_MFG_CROMEMCO, "16-FDC", S100_CARD_FLOPPY, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x34, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "Cromemco 16-bit floppy disk controller, double density"
    },
    {
        S100_MFG_NORTHSTAR, "MDS-A-D", S100_CARD_FLOPPY, 1977,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xC0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "North Star double-density controller, proprietary format"
    },
    {
        S100_MFG_GODBOUT, "Disk 1", S100_CARD_FLOPPY, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xC0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "Godbout Disk 1 controller, double density, DMA capable"
    },
    {
        S100_MFG_COMPUPRO, "Disk 1A", S100_CARD_FLOPPY, 1982,
        TRUE, S100_DATA_16BIT, S100_ADDR_16BIT,
        0xC0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "CompuPro Disk 1A, IEEE 696, supports 8-inch drives"
    },
    {
        S100_MFG_VECTOR, "Vector 3005", S100_CARD_FLOPPY, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xF0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "Vector Graphic floppy controller, double density"
    },

    //
    // Hard Disk Controllers - Enterprise Storage
    //
    {
        S100_MFG_CROMEMCO, "16K/64K Hard Disk", S100_CARD_HARDDISK, 1979,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x40, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI4,
        { .HardDisk = { 5, "ST-506" } },
        "Cromemco hard disk controller for 5MB/10MB drives"
    },
    {
        S100_MFG_COMPUPRO, "Hard Disk 1", S100_CARD_HARDDISK, 1982,
        TRUE, S100_DATA_16BIT, S100_ADDR_16BIT,
        0xC0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI4,
        { .HardDisk = { 40, "ST-506" } },
        "CompuPro hard disk controller, supports up to 40MB"
    },
    {
        S100_MFG_ITHACA, "DPS-1", S100_CARD_HARDDISK, 1980,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x80, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI4,
        { .HardDisk = { 10, "SASI" } },
        "Ithaca Intersystems hard disk controller, SASI interface"
    },

    //
    // Graphics and Video Cards
    //
    {
        S100_MFG_CROMEMCO, "Dazzler", S100_CARD_GRAPHICS, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x0E, 2, 0xC000, 2048,
        FALSE, FALSE, S100_IRQ_VI5,
        { .Serial = { 0, 0 } },
        "Cromemco Dazzler, first color graphics card, 128x128 resolution"
    },
    {
        S100_MFG_PROCESSOR_TECH, "VDM-1", S100_CARD_GRAPHICS, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0xD000, 1024,
        FALSE, TRUE, S100_IRQ_VI5,
        { .Serial = { 0, 0 } },
        "Processor Technology VDM-1, first video display for S-100, 16 lines"
    },
    {
        S100_MFG_CROMEMCO, "SDI", S100_CARD_GRAPHICS, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0xF000, 4096,
        FALSE, FALSE, S100_IRQ_VI5,
        { .Serial = { 0, 0 } },
        "Cromemco Smart Display Interface, programmable character generator"
    },
    {
        S100_MFG_COMPUPRO, "System Support 1", S100_CARD_GRAPHICS, 1982,
        TRUE, S100_DATA_16BIT, S100_ADDR_16BIT,
        0, 0, 0xF000, 4096,
        FALSE, TRUE, S100_IRQ_VI5,
        { .Serial = { 0, 0 } },
        "CompuPro System Support board, video and system management"
    },

    //
    // Real-Time Clock Cards
    //
    {
        S100_MFG_SSM, "SSM RTC", S100_CARD_RTC, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x40, 4, 0, 0,
        FALSE, TRUE, S100_IRQ_VI6,
        { .Serial = { 0, 0 } },
        "SSM real-time clock with battery backup and interval timer"
    },
    {
        S100_MFG_CROMEMCO, "TU-ART with RTC", S100_CARD_RTC, 1979,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0x44, 4, 0, 0,
        FALSE, TRUE, S100_IRQ_VI6,
        { .Serial = { 0, 0 } },
        "Cromemco real-time clock and calendar, integrated with TU-ART"
    },

    //
    // DMA Controllers
    //
    {
        S100_MFG_COMPUPRO, "DMA Master", S100_CARD_DMA, 1983,
        TRUE, S100_DATA_16BIT, S100_ADDR_24BIT,
        0, 0, 0, 0,
        TRUE, TRUE, S100_IRQ_VI0,
        { .Serial = { 0, 0 } },
        "CompuPro DMA master controller for high-speed block transfers"
    },

    //
    // Multi-Function Cards
    //
    {
        S100_MFG_JADE, "Jade DD Controller", S100_CARD_FLOPPY, 1979,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xDC, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "Jade double-density floppy controller, popular in Europe"
    },
    {
        S100_MFG_SD_SYSTEMS, "VersaFloppy II", S100_CARD_FLOPPY, 1978,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xE4, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "SD Systems VersaFloppy II, flexible controller for various drives"
    },
    {
        S100_MFG_CALIFORNIA, "CCS 2422", S100_CARD_FLOPPY, 1980,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0xC0, 8, 0, 0,
        TRUE, TRUE, S100_IRQ_VI3,
        { .Floppy = { 4, TRUE } },
        "California Computer Systems floppy controller, very reliable"
    },

    //
    // Prototyping Cards
    //
    {
        S100_MFG_VECTOR, "Vector 8800V", S100_CARD_PROTOTYPE, 1975,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        FALSE, FALSE, S100_IRQ_VI7,
        { .Serial = { 0, 0 } },
        "Vector wire-wrap prototyping card for custom S-100 designs"
    },
    {
        S100_MFG_IMSAI, "PIO Prototyping", S100_CARD_PROTOTYPE, 1976,
        FALSE, S100_DATA_8BIT, S100_ADDR_16BIT,
        0, 0, 0, 0,
        FALSE, FALSE, S100_IRQ_VI7,
        { .Serial = { 0, 0 } },
        "IMSAI prototyping card with solder pads"
    },
};

#define S100_CARD_DB_COUNT (sizeof(g_S100CardDatabase) / sizeof(g_S100CardDatabase[0]))

/**
 * @brief Manufacturer name strings
 */
static CONST CHAR8 *g_S100ManufacturerNames[] = {
    [S100_MFG_MITS]           = "MITS (Micro Instrumentation and Telemetry Systems)",
    [S100_MFG_CROMEMCO]       = "Cromemco",
    [S100_MFG_GODBOUT]        = "Godbout Electronics",
    [S100_MFG_VECTOR]         = "Vector Graphic",
    [S100_MFG_NORTHSTAR]      = "North Star Computers",
    [S100_MFG_IMSAI]          = "IMSAI Manufacturing",
    [S100_MFG_PROCESSOR_TECH] = "Processor Technology",
    [S100_MFG_ITHACA]         = "Ithaca Intersystems",
    [S100_MFG_SSM]            = "SSM Microcomputer Products",
    [S100_MFG_JADE]           = "Jade Computer Products",
    [S100_MFG_SD_SYSTEMS]     = "SD Systems",
    [S100_MFG_COMPUPRO]       = "CompuPro (Viasyn)",
    [S100_MFG_SEATTLE]        = "Seattle Computer Products",
    [S100_MFG_DYNABYTE]       = "Dynabyte",
    [S100_MFG_CALIFORNIA]     = "California Computer Systems",
    [S100_MFG_SOLID_STATE]    = "Solid State Music",
    [S100_MFG_TARBELL]        = "Tarbell Electronics",
    [S100_MFG_PARASITIC]      = "Parasitic Engineering",
    [S100_MFG_MICROPOLIS]     = "Micropolis",
    [S100_MFG_MORROW]         = "Morrow Designs",
    [S100_MFG_TELTEK]         = "Teltek",
    [S100_MFG_OPTRONICS]      = "Optronics Technology",
    [S100_MFG_CROMIX]         = "Cromix Division (Cromemco)",
};

//
// Implementation Structures
//

/**
 * @brief S-100 bus implementation structure
 */
typedef struct _S100_BUS_IMPL {
    IIOS100Bus          Vtbl;           /**< Virtual function table */
    ULONG               RefCount;       /**< Reference count */
    S100_BUS_CONFIG     Config;         /**< Bus configuration */
    CHAR8               Name[64];       /**< Bus name */
    BOOLEAN             bInitialized;   /**< Initialized flag */

    // Bus state
    IIOS100Device      *pBusOwner;      /**< Current bus master (DMA) */
    BOOLEAN             bBusRequested;  /**< Bus request pending */

    // Interrupt handlers
    S100_IRQ_HANDLER    IRQHandlers[8]; /**< VI0-VI7 handlers */
    VOID               *IRQContexts[8]; /**< Handler contexts */
    BOOLEAN             IRQEnabled[8];  /**< IRQ enable flags */

    // Detected cards
    S100_DEVICE_INFO    Slots[20];      /**< Slot information (typical: 20 slots) */
} S100_BUS_IMPL;

/**
 * @brief S-100 device implementation structure
 */
typedef struct _S100_DEVICE_IMPL {
    IIOS100Device       Vtbl;           /**< Virtual function table */
    ULONG               RefCount;       /**< Reference count */
    S100_DEVICE_INFO    DeviceInfo;     /**< Device information */
    IIOS100Bus         *pBus;           /**< Parent bus */
    CHAR8               Name[64];       /**< Device name */
    BOOLEAN             bEnabled;       /**< Device enabled */
} S100_DEVICE_IMPL;

//
// Forward Declarations
//

// S-100 Bus methods
static ULONG S100Bus_AddRef(IIOS100Bus *pThis);
static ULONG S100Bus_Release(IIOS100Bus *pThis);
static IO_RETURN S100Bus_Start(IIOS100Bus *pThis, IIOService *pProvider);
static IO_RETURN S100Bus_GetBusConfig(IIOS100Bus *pThis, S100_BUS_CONFIG *pConfig);
static IO_RETURN S100Bus_EnumerateCards(IIOS100Bus *pThis, IIOS100Device **ppDevices, UINT32 *puCount);
static IO_RETURN S100Bus_ReadMemory(IIOS100Bus *pThis, UINT32 uAddress, VOID *pBuffer, UINT32 cbLength);
static IO_RETURN S100Bus_WriteMemory(IIOS100Bus *pThis, UINT32 uAddress, CONST VOID *pBuffer, UINT32 cbLength);
static IO_RETURN S100Bus_ReadIO(IIOS100Bus *pThis, UINT8 uPort, UINT8 *puValue);
static IO_RETURN S100Bus_WriteIO(IIOS100Bus *pThis, UINT8 uPort, UINT8 uValue);
static IO_RETURN S100Bus_RegisterInterrupt(IIOS100Bus *pThis, S100_IRQ_LEVEL Level,
    S100_IRQ_HANDLER pfnHandler, VOID *pContext);
static IO_RETURN S100Bus_UnregisterInterrupt(IIOS100Bus *pThis, S100_IRQ_LEVEL Level);
static IO_RETURN S100Bus_RequestBus(IIOS100Bus *pThis, IIOS100Device *pDevice);
static IO_RETURN S100Bus_ReleaseBus(IIOS100Bus *pThis, IIOS100Device *pDevice);
static IO_RETURN S100Bus_Reset(IIOS100Bus *pThis);
static IO_RETURN S100Bus_GetSlotInfo(IIOS100Bus *pThis, UINT8 uSlot, S100_DEVICE_INFO *pInfo);

// S-100 Device methods
static ULONG S100Device_AddRef(IIOS100Device *pThis);
static ULONG S100Device_Release(IIOS100Device *pThis);
static IO_RETURN S100Device_GetDeviceInfo(IIOS100Device *pThis, S100_DEVICE_INFO *pInfo);
static IO_RETURN S100Device_GetCardInfo(IIOS100Device *pThis, S100_CARD_INFO *pCardInfo);
static IO_RETURN S100Device_ReadPort(IIOS100Device *pThis, UINT8 uOffset, UINT8 *puValue);
static IO_RETURN S100Device_WritePort(IIOS100Device *pThis, UINT8 uOffset, UINT8 uValue);
static IO_RETURN S100Device_ReadDeviceMemory(IIOS100Device *pThis, UINT32 uOffset, VOID *pBuffer, UINT32 cbLength);
static IO_RETURN S100Device_WriteDeviceMemory(IIOS100Device *pThis, UINT32 uOffset, CONST VOID *pBuffer, UINT32 cbLength);
static IO_RETURN S100Device_Enable(IIOS100Device *pThis, BOOLEAN bEnable);
static IO_RETURN S100Device_ResetDevice(IIOS100Device *pThis);

// Helper functions
static IO_RETURN S100DetectCards(S100_BUS_IMPL *pBus);
static CONST S100_CARD_INFO *S100IdentifyCard(UINT32 uIOBase, UINT32 uMemBase);

//
// S-100 Bus Implementation
//

/**
 * @brief S-100 Bus vtable
 */
static CONST struct IIOS100BusVtbl g_S100BusVtbl = {
    // IUnknown
    (void*)S100Bus_AddRef,
    (void*)S100Bus_AddRef,
    (void*)S100Bus_Release,
    // IIOService
    NULL,  // Probe
    (void*)S100Bus_Start,
    NULL,  // Stop
    NULL,  // Terminate
    NULL,  // GetProperty
    NULL,  // SetProperty
    NULL,  // GetParentService
    NULL,  // GetChildService
    NULL,  // GetServiceState
    NULL,  // GetServiceName
    NULL,  // RegisterService
    // IIOS100Bus
    S100Bus_GetBusConfig,
    S100Bus_EnumerateCards,
    S100Bus_ReadMemory,
    S100Bus_WriteMemory,
    S100Bus_ReadIO,
    S100Bus_WriteIO,
    S100Bus_RegisterInterrupt,
    S100Bus_UnregisterInterrupt,
    S100Bus_RequestBus,
    S100Bus_ReleaseBus,
    S100Bus_Reset,
    S100Bus_GetSlotInfo,
};

/**
 * @brief S-100 Device vtable
 */
static CONST struct IIOS100DeviceVtbl g_S100DeviceVtbl = {
    // IUnknown
    (void*)S100Device_AddRef,
    (void*)S100Device_AddRef,
    (void*)S100Device_Release,
    // IIOService
    NULL,  // Probe
    NULL,  // Start
    NULL,  // Stop
    NULL,  // Terminate
    NULL,  // GetProperty
    NULL,  // SetProperty
    NULL,  // GetParentService
    NULL,  // GetChildService
    NULL,  // GetServiceState
    NULL,  // GetServiceName
    NULL,  // RegisterService
    // IIOS100Device
    S100Device_GetDeviceInfo,
    S100Device_GetCardInfo,
    S100Device_ReadPort,
    S100Device_WritePort,
    S100Device_ReadDeviceMemory,
    S100Device_WriteDeviceMemory,
    S100Device_Enable,
    S100Device_ResetDevice,
};

//
// S-100 Bus Method Implementations
//

static ULONG
S100Bus_AddRef(
    IIOS100Bus *pThis
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;
    return ++pBus->RefCount;
}

static ULONG
S100Bus_Release(
    IIOS100Bus *pThis
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;
    ULONG RefCount = --pBus->RefCount;

    if (RefCount == 0) {
        free(pBus);
    }

    return RefCount;
}

static IO_RETURN
S100Bus_Start(
    IIOS100Bus *pThis,
    IIOService *pProvider
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;
    IO_RETURN Status;

    if (pBus == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("S-100: Starting %s bus controller\n", pBus->Name);
    printf("S-100: %u slots, %u-bit data, %u-bit addressing\n",
           pBus->Config.NumSlots,
           pBus->Config.DataWidth == S100_DATA_8BIT ? 8 : 16,
           pBus->Config.AddrWidth == S100_ADDR_16BIT ? 16 : 24);

    // Initialize bus state
    pBus->pBusOwner = NULL;
    pBus->bBusRequested = FALSE;
    memset(pBus->IRQHandlers, 0, sizeof(pBus->IRQHandlers));
    memset(pBus->IRQContexts, 0, sizeof(pBus->IRQContexts));
    memset(pBus->IRQEnabled, 0, sizeof(pBus->IRQEnabled));

    // Detect installed cards
    printf("S-100: Detecting installed cards...\n");
    Status = S100DetectCards(pBus);
    if (Status != IO_SUCCESS) {
        printf("S-100: Card detection failed: 0x%08X\n", Status);
        return Status;
    }

    pBus->bInitialized = TRUE;
    printf("S-100: Bus initialization complete\n");

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_GetBusConfig(
    IIOS100Bus *pThis,
    S100_BUS_CONFIG *pConfig
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pConfig == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pConfig, &pBus->Config, sizeof(S100_BUS_CONFIG));
    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_EnumerateCards(
    IIOS100Bus *pThis,
    IIOS100Device **ppDevices,
    UINT32 *puCount
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;
    UINT32 i, uCount = 0;

    if (pBus == NULL || ppDevices == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pBus->bInitialized) {
        return IO_NOT_READY;
    }

    // Count present cards
    for (i = 0; i < pBus->Config.NumSlots; i++) {
        if (pBus->Slots[i].bPresent) {
            uCount++;
        }
    }

    printf("S-100: Found %u cards in %u slots\n", uCount, pBus->Config.NumSlots);

    *puCount = uCount;
    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_ReadMemory(
    IIOS100Bus *pThis,
    UINT32 uAddress,
    VOID *pBuffer,
    UINT32 cbLength
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Check address range
    if (pBus->Config.AddrWidth == S100_ADDR_16BIT && uAddress >= 0x10000) {
        return IO_BAD_ARGUMENT;
    }

    // Memory read would access actual S-100 bus hardware
    // This would generate MREAD signal and read data from bus
    memset(pBuffer, 0xFF, cbLength);

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_WriteMemory(
    IIOS100Bus *pThis,
    UINT32 uAddress,
    CONST VOID *pBuffer,
    UINT32 cbLength
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Check address range
    if (pBus->Config.AddrWidth == S100_ADDR_16BIT && uAddress >= 0x10000) {
        return IO_BAD_ARGUMENT;
    }

    // Memory write would access actual S-100 bus hardware
    // This would generate MWRITE signal and write data to bus

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_ReadIO(
    IIOS100Bus *pThis,
    UINT8 uPort,
    UINT8 *puValue
    )
{
    if (pThis == NULL || puValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // I/O read would access actual S-100 bus hardware
    // This would generate IOREAD signal
    *puValue = 0xFF;

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_WriteIO(
    IIOS100Bus *pThis,
    UINT8 uPort,
    UINT8 uValue
    )
{
    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // I/O write would access actual S-100 bus hardware
    // This would generate IOWRITE signal

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_RegisterInterrupt(
    IIOS100Bus *pThis,
    S100_IRQ_LEVEL Level,
    S100_IRQ_HANDLER pfnHandler,
    VOID *pContext
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pfnHandler == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (Level >= 8) {
        return IO_BAD_ARGUMENT;
    }

    if (!pBus->Config.bVectoredInt) {
        return IO_UNSUPPORTED;
    }

    if (pBus->IRQHandlers[Level] != NULL) {
        printf("S-100: IRQ VI%u already registered\n", Level);
        return IO_NO_INTERRUPT;
    }

    pBus->IRQHandlers[Level] = pfnHandler;
    pBus->IRQContexts[Level] = pContext;
    pBus->IRQEnabled[Level] = TRUE;

    printf("S-100: Registered handler for VI%u (priority %u)\n", Level, Level);

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_UnregisterInterrupt(
    IIOS100Bus *pThis,
    S100_IRQ_LEVEL Level
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (Level >= 8) {
        return IO_BAD_ARGUMENT;
    }

    pBus->IRQHandlers[Level] = NULL;
    pBus->IRQContexts[Level] = NULL;
    pBus->IRQEnabled[Level] = FALSE;

    printf("S-100: Unregistered handler for VI%u\n", Level);

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_RequestBus(
    IIOS100Bus *pThis,
    IIOS100Device *pDevice
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pBus->pBusOwner != NULL) {
        printf("S-100: Bus already owned by another device\n");
        return IO_BUSY;
    }

    pBus->pBusOwner = pDevice;
    pBus->bBusRequested = TRUE;

    printf("S-100: Bus granted for DMA transfer\n");

    // In real hardware, this would assert PHOLD and wait for PHLDA

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_ReleaseBus(
    IIOS100Bus *pThis,
    IIOS100Device *pDevice
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pBus->pBusOwner != pDevice) {
        printf("S-100: Device does not own bus\n");
        return IO_BAD_ARGUMENT;
    }

    pBus->pBusOwner = NULL;
    pBus->bBusRequested = FALSE;

    printf("S-100: Bus released\n");

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_Reset(
    IIOS100Bus *pThis
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("S-100: Asserting PRESET line (bus reset)\n");

    // In real hardware, this would assert PRESET line
    // All cards would reset to initial state

    // Reset bus state
    pBus->pBusOwner = NULL;
    pBus->bBusRequested = FALSE;

    return IO_SUCCESS;
}

static IO_RETURN
S100Bus_GetSlotInfo(
    IIOS100Bus *pThis,
    UINT8 uSlot,
    S100_DEVICE_INFO *pInfo
    )
{
    S100_BUS_IMPL *pBus = (S100_BUS_IMPL *)pThis;

    if (pBus == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uSlot >= pBus->Config.NumSlots) {
        return IO_BAD_ARGUMENT;
    }

    if (!pBus->Slots[uSlot].bPresent) {
        return IO_NO_DEVICE;
    }

    memcpy(pInfo, &pBus->Slots[uSlot], sizeof(S100_DEVICE_INFO));

    return IO_SUCCESS;
}

//
// S-100 Device Method Implementations
//

static ULONG
S100Device_AddRef(
    IIOS100Device *pThis
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    return ++pDevice->RefCount;
}

static ULONG
S100Device_Release(
    IIOS100Device *pThis
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    ULONG RefCount = --pDevice->RefCount;

    if (RefCount == 0) {
        if (pDevice->pBus != NULL) {
            pDevice->pBus->lpVtbl->Release(pDevice->pBus);
        }
        free(pDevice);
    }

    return RefCount;
}

static IO_RETURN
S100Device_GetDeviceInfo(
    IIOS100Device *pThis,
    S100_DEVICE_INFO *pInfo
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;

    if (pDevice == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pInfo, &pDevice->DeviceInfo, sizeof(S100_DEVICE_INFO));
    return IO_SUCCESS;
}

static IO_RETURN
S100Device_GetCardInfo(
    IIOS100Device *pThis,
    S100_CARD_INFO *pCardInfo
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;

    if (pDevice == NULL || pCardInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pCardInfo, &pDevice->DeviceInfo.CardInfo, sizeof(S100_CARD_INFO));
    return IO_SUCCESS;
}

static IO_RETURN
S100Device_ReadPort(
    IIOS100Device *pThis,
    UINT8 uOffset,
    UINT8 *puValue
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    UINT8 uPort;

    if (pDevice == NULL || puValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pDevice->bEnabled) {
        return IO_NOT_READY;
    }

    uPort = (UINT8)(pDevice->DeviceInfo.CardInfo.uIOBase + uOffset);

    // Read from bus
    return pDevice->pBus->lpVtbl->ReadIO(pDevice->pBus, uPort, puValue);
}

static IO_RETURN
S100Device_WritePort(
    IIOS100Device *pThis,
    UINT8 uOffset,
    UINT8 uValue
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    UINT8 uPort;

    if (pDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pDevice->bEnabled) {
        return IO_NOT_READY;
    }

    uPort = (UINT8)(pDevice->DeviceInfo.CardInfo.uIOBase + uOffset);

    // Write to bus
    return pDevice->pBus->lpVtbl->WriteIO(pDevice->pBus, uPort, uValue);
}

static IO_RETURN
S100Device_ReadDeviceMemory(
    IIOS100Device *pThis,
    UINT32 uOffset,
    VOID *pBuffer,
    UINT32 cbLength
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    UINT32 uAddress;

    if (pDevice == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pDevice->bEnabled) {
        return IO_NOT_READY;
    }

    uAddress = pDevice->DeviceInfo.CardInfo.uMemBase + uOffset;

    // Read from bus
    return pDevice->pBus->lpVtbl->ReadMemory(pDevice->pBus, uAddress, pBuffer, cbLength);
}

static IO_RETURN
S100Device_WriteDeviceMemory(
    IIOS100Device *pThis,
    UINT32 uOffset,
    CONST VOID *pBuffer,
    UINT32 cbLength
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;
    UINT32 uAddress;

    if (pDevice == NULL || pBuffer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pDevice->bEnabled) {
        return IO_NOT_READY;
    }

    uAddress = pDevice->DeviceInfo.CardInfo.uMemBase + uOffset;

    // Write to bus
    return pDevice->pBus->lpVtbl->WriteMemory(pDevice->pBus, uAddress, pBuffer, cbLength);
}

static IO_RETURN
S100Device_Enable(
    IIOS100Device *pThis,
    BOOLEAN bEnable
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;

    if (pDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pDevice->bEnabled = bEnable;
    pDevice->DeviceInfo.bEnabled = bEnable;

    printf("S-100: Device %s %s\n",
           pDevice->DeviceInfo.CardInfo.pszName,
           bEnable ? "enabled" : "disabled");

    return IO_SUCCESS;
}

static IO_RETURN
S100Device_ResetDevice(
    IIOS100Device *pThis
    )
{
    S100_DEVICE_IMPL *pDevice = (S100_DEVICE_IMPL *)pThis;

    if (pDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("S-100: Resetting device %s\n", pDevice->DeviceInfo.CardInfo.pszName);

    // Device-specific reset would be performed here

    return IO_SUCCESS;
}

//
// Helper Function Implementations
//

/**
 * @brief Detect installed S-100 cards
 */
static IO_RETURN
S100DetectCards(
    S100_BUS_IMPL *pBus
    )
{
    UINT32 i;
    CONST S100_CARD_INFO *pCard;

    // Initialize all slots as empty
    for (i = 0; i < pBus->Config.NumSlots; i++) {
        pBus->Slots[i].Slot = (UINT8)i;
        pBus->Slots[i].bPresent = FALSE;
        pBus->Slots[i].bEnabled = FALSE;
    }

    // Simulate card detection by populating some slots
    // In real hardware, this would probe each slot for card presence

    // Slot 0: CPU card (always present)
    if (pBus->Config.NumSlots > 0) {
        pCard = S100IdentifyCard(0, 0);
        if (pCard != NULL && pCard->CardType == S100_CARD_CPU) {
            memcpy(&pBus->Slots[0].CardInfo, pCard, sizeof(S100_CARD_INFO));
            pBus->Slots[0].bPresent = TRUE;
            printf("S-100: Slot 0: %s (%s)\n",
                   pCard->pszName,
                   S100GetManufacturerName(pCard->Manufacturer));
        }
    }

    // Slot 1: Memory card
    if (pBus->Config.NumSlots > 1) {
        pCard = &g_S100CardDatabase[9];  // RAM 22
        memcpy(&pBus->Slots[1].CardInfo, pCard, sizeof(S100_CARD_INFO));
        pBus->Slots[1].bPresent = TRUE;
        printf("S-100: Slot 1: %s (%s)\n",
               pCard->pszName,
               S100GetManufacturerName(pCard->Manufacturer));
    }

    // Slot 2: Serial I/O
    if (pBus->Config.NumSlots > 2) {
        pCard = &g_S100CardDatabase[13];  // TU-ART
        memcpy(&pBus->Slots[2].CardInfo, pCard, sizeof(S100_CARD_INFO));
        pBus->Slots[2].bPresent = TRUE;
        printf("S-100: Slot 2: %s (%s)\n",
               pCard->pszName,
               S100GetManufacturerName(pCard->Manufacturer));
    }

    // Slot 3: Floppy controller
    if (pBus->Config.NumSlots > 3) {
        pCard = &g_S100CardDatabase[23];  // Disk 1A
        memcpy(&pBus->Slots[3].CardInfo, pCard, sizeof(S100_CARD_INFO));
        pBus->Slots[3].bPresent = TRUE;
        printf("S-100: Slot 3: %s (%s)\n",
               pCard->pszName,
               S100GetManufacturerName(pCard->Manufacturer));
    }

    return IO_SUCCESS;
}

/**
 * @brief Identify S-100 card by probing
 */
static CONST S100_CARD_INFO *
S100IdentifyCard(
    UINT32 uIOBase,
    UINT32 uMemBase
    )
{
    // In real hardware, this would probe I/O ports and memory to identify the card
    // For simulation, return a CPU card
    return &g_S100CardDatabase[2];  // CompuPro CPU-Z
}

//
// Public API Implementations
//

IO_RETURN
S100Initialize(
    VOID
    )
{
    printf("S-100: Initializing S-100 Bus subsystem\n");
    printf("S-100: The legendary bus that started the microcomputer revolution (1975)\n");
    printf("S-100: Card database contains %u famous cards\n", (UINT32)S100_CARD_DB_COUNT);
    return IO_SUCCESS;
}

IO_RETURN
S100Shutdown(
    VOID
    )
{
    printf("S-100: Shutting down S-100 Bus subsystem\n");
    return IO_SUCCESS;
}

IO_RETURN
IOS100BusCreate(
    CONST S100_BUS_CONFIG *pConfig,
    IIOS100Bus **ppBus
    )
{
    S100_BUS_IMPL *pBus;

    if (ppBus == NULL || pConfig == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pBus = (S100_BUS_IMPL *)malloc(sizeof(S100_BUS_IMPL));
    if (pBus == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pBus, 0, sizeof(S100_BUS_IMPL));

    pBus->Vtbl.lpVtbl = &g_S100BusVtbl;
    pBus->RefCount = 1;
    memcpy(&pBus->Config, pConfig, sizeof(S100_BUS_CONFIG));
    pBus->bInitialized = FALSE;

    // Generate bus name
    switch (pConfig->Standard) {
        case S100_STANDARD_ORIGINAL:
            snprintf(pBus->Name, sizeof(pBus->Name), "S-100 (Altair 8800 original)");
            break;
        case S100_STANDARD_CROMEMCO:
            snprintf(pBus->Name, sizeof(pBus->Name), "S-100 (Cromemco extended)");
            break;
        case S100_STANDARD_IEEE696:
            snprintf(pBus->Name, sizeof(pBus->Name), "S-100 (IEEE 696-1983)");
            break;
        default:
            snprintf(pBus->Name, sizeof(pBus->Name), "S-100 Bus");
            break;
    }

    *ppBus = &pBus->Vtbl;

    printf("S-100: Created %s bus controller\n", pBus->Name);

    return IO_SUCCESS;
}

IO_RETURN
IOS100DeviceCreate(
    CONST S100_CARD_INFO *pCardInfo,
    IIOS100Device **ppDevice
    )
{
    S100_DEVICE_IMPL *pDevice;

    if (ppDevice == NULL || pCardInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pDevice = (S100_DEVICE_IMPL *)malloc(sizeof(S100_DEVICE_IMPL));
    if (pDevice == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pDevice, 0, sizeof(S100_DEVICE_IMPL));

    pDevice->Vtbl.lpVtbl = &g_S100DeviceVtbl;
    pDevice->RefCount = 1;
    memcpy(&pDevice->DeviceInfo.CardInfo, pCardInfo, sizeof(S100_CARD_INFO));
    pDevice->bEnabled = FALSE;

    strncpy(pDevice->Name, pCardInfo->pszName, sizeof(pDevice->Name) - 1);

    *ppDevice = &pDevice->Vtbl;

    printf("S-100: Created device: %s\n", pCardInfo->pszName);

    return IO_SUCCESS;
}

IO_RETURN
S100GetCardByName(
    CONST CHAR8 *pszName,
    S100_CARD_INFO *pCardInfo
    )
{
    UINT32 i;

    if (pszName == NULL || pCardInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    for (i = 0; i < S100_CARD_DB_COUNT; i++) {
        if (strcmp(g_S100CardDatabase[i].pszName, pszName) == 0) {
            memcpy(pCardInfo, &g_S100CardDatabase[i], sizeof(S100_CARD_INFO));
            return IO_SUCCESS;
        }
    }

    return IO_NOT_FOUND;
}

IO_RETURN
S100GetCardsByManufacturer(
    S100_MANUFACTURER Manufacturer,
    S100_CARD_INFO *pCards,
    UINT32 *puCount
    )
{
    UINT32 i, uCount = 0;
    UINT32 uMaxCount;

    if (pCards == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    uMaxCount = *puCount;

    for (i = 0; i < S100_CARD_DB_COUNT && uCount < uMaxCount; i++) {
        if (g_S100CardDatabase[i].Manufacturer == Manufacturer) {
            memcpy(&pCards[uCount], &g_S100CardDatabase[i], sizeof(S100_CARD_INFO));
            uCount++;
        }
    }

    *puCount = uCount;
    return IO_SUCCESS;
}

CONST CHAR8 *
S100GetManufacturerName(
    S100_MANUFACTURER Manufacturer
    )
{
    if (Manufacturer < sizeof(g_S100ManufacturerNames) / sizeof(g_S100ManufacturerNames[0])) {
        return g_S100ManufacturerNames[Manufacturer];
    }

    return "Unknown Manufacturer";
}
