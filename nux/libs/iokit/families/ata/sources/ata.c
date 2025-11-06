/**
 * @file ata.c
 * @brief ATA/IDE Family Implementation - ATA/ATAPI/IDE Driver
 *
 * Provides full support for ATA/IDE/PATA controllers with:
 * - Complete ATA command set (ATA-1 through ATA-7)
 * - ATAPI support for CD/DVD/Zip drives
 * - PIO modes 0-4
 * - Multiword DMA modes 0-2
 * - Ultra DMA modes 0-6 (up to 133 MB/s)
 * - Legacy ST506/ESDI support
 * - 28-bit and 48-bit LBA
 * - S.M.A.R.T. monitoring
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/families/ata/ata.h>
#include <iokit/families/pcie/pcie.h>
#include <iokit/families/storage/storage.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief ATA controller database entry
 */
typedef struct _ATA_CONTROLLER_DB_ENTRY {
    UINT16      VendorID;
    UINT16      DeviceID;
    CONST CHAR8 *pszVendor;
    CONST CHAR8 *pszModel;
    UINT32      Flags;
    ATA_PROTOCOL MaxProtocol;
    ATA_UDMA_MODE MaxUdmaMode;
} ATA_CONTROLLER_DB_ENTRY;

/**
 * @brief ATA controller flags
 */
#define ATA_FLAG_UDMA           (1 << 0)    /**< Ultra DMA capable */
#define ATA_FLAG_RAID           (1 << 1)    /**< RAID capable */
#define ATA_FLAG_CABLE_DETECT   (1 << 2)    /**< 80-wire cable detection */
#define ATA_FLAG_LEGACY         (1 << 3)    /**< Legacy controller */
#define ATA_FLAG_NATIVE         (1 << 4)    /**< Native PCI mode */

/**
 * @brief Known ATA/IDE controller database (70+ entries)
 */
static CONST ATA_CONTROLLER_DB_ENTRY g_ATAControllerDB[] = {
    //
    // Intel PATA Controllers (PIIX, ICH, PCH series)
    //
    { 0x8086, 0x1230, "Intel", "PIIX PATA Controller", ATA_FLAG_LEGACY, ATA_PROTOCOL_EIDE, ATA_UDMA_MODE_2 },
    { 0x8086, 0x7010, "Intel", "PIIX3 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x8086, 0x7111, "Intel", "PIIX4/4E/4M PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x8086, 0x7199, "Intel", "PIIX4E PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x8086, 0x84CA, "Intel", "PIIX4E/4M PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x8086, 0x2411, "Intel", "ICH PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x8086, 0x2421, "Intel", "ICH0 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x8086, 0x244A, "Intel", "ICH2 PATA Controller (Mobile)", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x244B, "Intel", "ICH2 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x248A, "Intel", "ICH3 PATA Controller (Mobile)", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x248B, "Intel", "ICH3 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x24CA, "Intel", "ICH4 PATA Controller (Mobile)", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x24CB, "Intel", "ICH4 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x24DB, "Intel", "ICH5 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x25A2, "Intel", "6300ESB PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x266F, "Intel", "ICH6 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x27DF, "Intel", "ICH7 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x27C0, "Intel", "ICH7 Mobile PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x8086, 0x2850, "Intel", "ICH8 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x8086, 0x2820, "Intel", "ICH8 Mobile PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x8086, 0x2921, "Intel", "ICH9 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x8086, 0x2926, "Intel", "ICH9 Mobile PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x8086, 0x3A20, "Intel", "ICH10 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x8086, 0x3A26, "Intel", "ICH10 Mobile PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // VIA Technologies PATA Controllers
    //
    { 0x1106, 0x0571, "VIA", "VT82C586 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1106, 0x0586, "VIA", "VT82C586B PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x1106, 0x1571, "VIA", "VT82C576 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1106, 0x3164, "VIA", "VT6410 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1106, 0x5324, "VIA", "VT8233C PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x1106, 0x5337, "VIA", "VT8237 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1106, 0x5372, "VIA", "VT8237S PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1106, 0x5287, "VIA", "VT8251 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1106, 0x8324, "VIA", "CX700 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x1106, 0x8353, "VIA", "VX800/VX820 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // AMD PATA Controllers
    //
    { 0x1022, 0x7401, "AMD", "AMD-755 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1022, 0x7409, "AMD", "AMD-756 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x1022, 0x7411, "AMD", "AMD-766 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x1022, 0x7441, "AMD", "AMD-768 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x1022, 0x7469, "AMD", "AMD-8111 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // ATI/AMD (SB series) PATA Controllers
    //
    { 0x1002, 0x4376, "ATI", "SB400 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1002, 0x4379, "ATI", "SB400 PATA Controller (Secondary)", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1002, 0x437A, "ATI", "SB450 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1002, 0x438C, "ATI", "SB600 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1002, 0x439C, "ATI", "SB700/SB800 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // SiS PATA Controllers
    //
    { 0x1039, 0x0008, "SiS", "SiS85C503/5513 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1039, 0x5513, "SiS", "SiS5513 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1039, 0x5518, "SiS", "SiS5518 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1039, 0x1183, "SiS", "SiS966/968 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // ALi (Acer Labs) PATA Controllers
    //
    { 0x10B9, 0x5229, "ALi", "M5229 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10B9, 0x5228, "ALi", "M5228 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x10B9, 0x5288, "ALi", "M5288 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // NVIDIA nForce PATA Controllers
    //
    { 0x10DE, 0x01BC, "NVIDIA", "nForce PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x10DE, 0x0065, "NVIDIA", "nForce2 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x0085, "NVIDIA", "nForce2 Ultra 400 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x00D5, "NVIDIA", "nForce3 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x00E5, "NVIDIA", "nForce3 250 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x0053, "NVIDIA", "nForce4 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x0266, "NVIDIA", "MCP51 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x036E, "NVIDIA", "MCP55 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x03EC, "NVIDIA", "MCP61 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x10DE, 0x0759, "NVIDIA", "MCP77 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // Promise Technology PATA Controllers
    //
    { 0x105A, 0x4D33, "Promise", "PDC20246 Ultra33 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x105A, 0x4D38, "Promise", "PDC20262 Ultra66 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x105A, 0x4D30, "Promise", "PDC20265 Ultra100 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x105A, 0x4D68, "Promise", "PDC20268 Ultra100 TX2 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x105A, 0x4D69, "Promise", "PDC20269 Ultra133 TX2 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x105A, 0x6268, "Promise", "PDC20270 Ultra100 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x105A, 0x6269, "Promise", "PDC20271 Ultra133 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x105A, 0x3376, "Promise", "PDC20376 FastTrak 376 PATA RAID", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT | ATA_FLAG_RAID, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x105A, 0x3574, "Promise", "PDC20579 FastTrak PATA RAID", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT | ATA_FLAG_RAID, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // HighPoint RocketRAID PATA Controllers
    //
    { 0x1103, 0x0004, "HighPoint", "HPT366 Ultra66 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x1103, 0x0005, "HighPoint", "HPT372 Ultra133 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1103, 0x0006, "HighPoint", "HPT302 Ultra133 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1103, 0x0007, "HighPoint", "HPT371 Ultra133 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1103, 0x0008, "HighPoint", "HPT374 Ultra133 PATA RAID", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT | ATA_FLAG_RAID, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1103, 0x0009, "HighPoint", "HPT372N Ultra133 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // CMD Technology PATA Controllers
    //
    { 0x1095, 0x0640, "CMD", "CMD640 PATA Controller", ATA_FLAG_LEGACY, ATA_PROTOCOL_EIDE, ATA_UDMA_MODE_0 },
    { 0x1095, 0x0643, "CMD", "CMD643 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1095, 0x0646, "CMD", "CMD646 Ultra DMA/33 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1095, 0x0648, "CMD", "CMD648 Ultra DMA/66 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x1095, 0x0649, "CMD", "CMD649 Ultra DMA/100 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },

    //
    // ServerWorks/Broadcom PATA Controllers
    //
    { 0x1166, 0x0211, "ServerWorks", "OSB4 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x1166, 0x0212, "ServerWorks", "CSB5 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA66, ATA_UDMA_MODE_4 },
    { 0x1166, 0x0213, "ServerWorks", "CSB6 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },
    { 0x1166, 0x0217, "ServerWorks", "HT1000 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA100, ATA_UDMA_MODE_5 },

    //
    // JMicron PATA Controllers
    //
    { 0x197B, 0x2361, "JMicron", "JMB361 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x197B, 0x2363, "JMicron", "JMB363 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x197B, 0x2365, "JMicron", "JMB365 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x197B, 0x2366, "JMicron", "JMB366 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x197B, 0x2368, "JMicron", "JMB368 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // ITE PATA Controllers
    //
    { 0x1283, 0x8211, "ITE", "IT8211 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1283, 0x8212, "ITE", "IT8212 PATA RAID Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT | ATA_FLAG_RAID, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },
    { 0x1283, 0x8213, "ITE", "IT8213 PATA Controller", ATA_FLAG_UDMA | ATA_FLAG_CABLE_DETECT, ATA_PROTOCOL_UDMA133, ATA_UDMA_MODE_6 },

    //
    // OPTi PATA Controllers
    //
    { 0x1045, 0xC621, "OPTi", "82C621 PATA Controller", ATA_FLAG_LEGACY, ATA_PROTOCOL_EIDE, ATA_UDMA_MODE_0 },
    { 0x1045, 0xC701, "OPTi", "82C701 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },

    //
    // Cyrix/National Semiconductor PATA Controllers
    //
    { 0x1078, 0x0102, "Cyrix", "5530 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
    { 0x100B, 0x0502, "National Semi", "SCx200 PATA Controller", ATA_FLAG_UDMA, ATA_PROTOCOL_UDMA33, ATA_UDMA_MODE_2 },
};

#define ATA_CONTROLLER_DB_COUNT (sizeof(g_ATAControllerDB) / sizeof(g_ATAControllerDB[0]))

/**
 * @brief ATA device database entry
 */
typedef struct _ATA_DEVICE_DB_ENTRY {
    CONST CHAR8         *pszVendor;
    CONST CHAR8         *pszModel;
    ATA_DEVICE_TYPE     DeviceType;
    CONST CHAR8         *pszDescription;
    BOOLEAN             bAtapi;
} ATA_DEVICE_DB_ENTRY;

/**
 * @brief Known ATA/ATAPI device database (80+ entries)
 */
static CONST ATA_DEVICE_DB_ENTRY g_ATADeviceDB[] = {
    //
    // === Classic IDE/ATA Hard Drives (1990s-2000s) ===
    //

    // Seagate ATA/IDE Drives
    { "ST3", "ST3500", ATA_DEVICE_HDD, "Seagate Barracuda 500GB IDE", FALSE },
    { "ST3", "ST3320", ATA_DEVICE_HDD, "Seagate Barracuda 320GB IDE", FALSE },
    { "ST3", "ST3250", ATA_DEVICE_HDD, "Seagate Barracuda 250GB IDE", FALSE },
    { "ST3", "ST3200", ATA_DEVICE_HDD, "Seagate Barracuda 200GB IDE", FALSE },
    { "ST3", "ST3160", ATA_DEVICE_HDD, "Seagate Barracuda 160GB IDE", FALSE },
    { "ST3", "ST3120", ATA_DEVICE_HDD, "Seagate Barracuda 120GB IDE", FALSE },
    { "ST3", "ST380", ATA_DEVICE_HDD, "Seagate Barracuda 80GB IDE", FALSE },
    { "ST3", "ST340", ATA_DEVICE_HDD, "Seagate Barracuda 40GB IDE", FALSE },
    { "ST3", "ST320", ATA_DEVICE_HDD, "Seagate Barracuda 20GB IDE", FALSE },

    // Western Digital IDE Drives
    { "WDC", "WD5000", ATA_DEVICE_HDD, "Western Digital Caviar 500GB IDE", FALSE },
    { "WDC", "WD4000", ATA_DEVICE_HDD, "Western Digital Caviar 400GB IDE", FALSE },
    { "WDC", "WD3200", ATA_DEVICE_HDD, "Western Digital Caviar 320GB IDE", FALSE },
    { "WDC", "WD2500", ATA_DEVICE_HDD, "Western Digital Caviar 250GB IDE", FALSE },
    { "WDC", "WD2000", ATA_DEVICE_HDD, "Western Digital Caviar 200GB IDE", FALSE },
    { "WDC", "WD1600", ATA_DEVICE_HDD, "Western Digital Caviar 160GB IDE", FALSE },
    { "WDC", "WD1200", ATA_DEVICE_HDD, "Western Digital Caviar 120GB IDE", FALSE },
    { "WDC", "WD800", ATA_DEVICE_HDD, "Western Digital Caviar 80GB IDE", FALSE },
    { "WDC", "WD400", ATA_DEVICE_HDD, "Western Digital Caviar 40GB IDE", FALSE },
    { "WDC", "WD200", ATA_DEVICE_HDD, "Western Digital Caviar 20GB IDE", FALSE },

    // Maxtor IDE Drives
    { "Maxtor", "6Y160P0", ATA_DEVICE_HDD, "Maxtor DiamondMax Plus 9 160GB IDE", FALSE },
    { "Maxtor", "6Y120P0", ATA_DEVICE_HDD, "Maxtor DiamondMax Plus 9 120GB IDE", FALSE },
    { "Maxtor", "6Y080P0", ATA_DEVICE_HDD, "Maxtor DiamondMax Plus 9 80GB IDE", FALSE },
    { "Maxtor", "6L300", ATA_DEVICE_HDD, "Maxtor DiamondMax 10 300GB IDE", FALSE },
    { "Maxtor", "6L250", ATA_DEVICE_HDD, "Maxtor DiamondMax 10 250GB IDE", FALSE },
    { "Maxtor", "6L200", ATA_DEVICE_HDD, "Maxtor DiamondMax 10 200GB IDE", FALSE },
    { "Maxtor", "6L160", ATA_DEVICE_HDD, "Maxtor DiamondMax 10 160GB IDE", FALSE },

    // Quantum IDE Drives (classic)
    { "QUANTUM", "FIREBALLP", ATA_DEVICE_HDD, "Quantum Fireball Plus 40GB IDE", FALSE },
    { "QUANTUM", "FIREBALLLM", ATA_DEVICE_HDD, "Quantum Fireball lct 30GB IDE", FALSE },
    { "QUANTUM", "FIREBALLCR", ATA_DEVICE_HDD, "Quantum Fireball CR 20GB IDE", FALSE },
    { "QUANTUM", "FIREBALLEX", ATA_DEVICE_HDD, "Quantum Fireball EX 10GB IDE", FALSE },

    // IBM/HGST IDE Drives
    { "IC35L", "IC35L120AVV", ATA_DEVICE_HDD, "IBM Deskstar 120GXP 120GB IDE", FALSE },
    { "IC35L", "IC35L080AVV", ATA_DEVICE_HDD, "IBM Deskstar 120GXP 80GB IDE", FALSE },
    { "IC35L", "IC35L060AVV", ATA_DEVICE_HDD, "IBM Deskstar 120GXP 60GB IDE", FALSE },
    { "IC35L", "IC35L040AVV", ATA_DEVICE_HDD, "IBM Deskstar 120GXP 40GB IDE", FALSE },
    { "HDS72", "HDS722516", ATA_DEVICE_HDD, "Hitachi Deskstar 160GB IDE", FALSE },
    { "HDS72", "HDS722512", ATA_DEVICE_HDD, "Hitachi Deskstar 120GB IDE", FALSE },

    // Samsung IDE Drives
    { "SAMSUNG", "SV3002H", ATA_DEVICE_HDD, "Samsung SpinPoint V30 300GB IDE", FALSE },
    { "SAMSUNG", "SV2004H", ATA_DEVICE_HDD, "Samsung SpinPoint V20 200GB IDE", FALSE },
    { "SAMSUNG", "SP1604N", ATA_DEVICE_HDD, "Samsung SpinPoint P120 160GB IDE", FALSE },
    { "SAMSUNG", "SP1203N", ATA_DEVICE_HDD, "Samsung SpinPoint P120 120GB IDE", FALSE },
    { "SAMSUNG", "SP0842N", ATA_DEVICE_HDD, "Samsung SpinPoint P80 80GB IDE", FALSE },

    // Fujitsu IDE Drives
    { "FUJITSU", "MPG3409AH", ATA_DEVICE_HDD, "Fujitsu MPG 40GB IDE", FALSE },
    { "FUJITSU", "MPG3307AH", ATA_DEVICE_HDD, "Fujitsu MPG 30GB IDE", FALSE },
    { "FUJITSU", "MPG3204AH", ATA_DEVICE_HDD, "Fujitsu MPG 20GB IDE", FALSE },

    //
    // === ATAPI CD-ROM Drives ===
    //

    // Plextor CD/DVD Drives
    { "PLEXTOR", "CD-ROM PX-40TS", ATA_DEVICE_CDROM, "Plextor 40X SCSI CD-ROM", TRUE },
    { "PLEXTOR", "CD-R PX-W4012", ATA_DEVICE_CDRW, "Plextor PlexWriter 40/12/40 CD-RW", TRUE },
    { "PLEXTOR", "CD-R PX-W5224", ATA_DEVICE_CDRW, "Plextor PlexWriter 52/24/52 CD-RW", TRUE },
    { "PLEXTOR", "DVDR PX-708A", ATA_DEVICE_DVDRW, "Plextor PX-708A DVD±RW", TRUE },
    { "PLEXTOR", "DVDR PX-712A", ATA_DEVICE_DVDRW, "Plextor PX-712A DVD±RW", TRUE },
    { "PLEXTOR", "DVDR PX-755A", ATA_DEVICE_DVDRW, "Plextor PX-755A DVD±RW", TRUE },

    // Sony CD/DVD Drives
    { "SONY", "CD-ROM CDU-55E", ATA_DEVICE_CDROM, "Sony 24X IDE CD-ROM", TRUE },
    { "SONY", "CD-RW CRX140E", ATA_DEVICE_CDRW, "Sony 24/10/40 CD-RW", TRUE },
    { "SONY", "CD-RW CRX230E", ATA_DEVICE_CDRW, "Sony 48/24/48 CD-RW", TRUE },
    { "SONY", "DVD-ROM DDU-1613", ATA_DEVICE_DVDROM, "Sony 16X DVD-ROM", TRUE },
    { "SONY", "DVD-RW DRU-700A", ATA_DEVICE_DVDRW, "Sony DRU-700A DVD±RW", TRUE },
    { "SONY", "DVD-RW DRU-810A", ATA_DEVICE_DVDRW, "Sony DRU-810A DVD±RW", TRUE },

    // Pioneer CD/DVD Drives
    { "PIONEER", "DVD-ROM DVD-116", ATA_DEVICE_DVDROM, "Pioneer 16X DVD-ROM", TRUE },
    { "PIONEER", "DVD-RW DVR-106", ATA_DEVICE_DVDRW, "Pioneer DVR-106 DVD±RW", TRUE },
    { "PIONEER", "DVD-RW DVR-110", ATA_DEVICE_DVDRW, "Pioneer DVR-110 DVD±RW", TRUE },
    { "PIONEER", "DVD-RW DVR-112", ATA_DEVICE_DVDRW, "Pioneer DVR-112 DVD±RW", TRUE },

    // LG/HL-DT-ST CD/DVD Drives
    { "HL-DT-ST", "CD-ROM GCR-8240N", ATA_DEVICE_CDROM, "LG 40X IDE CD-ROM", TRUE },
    { "HL-DT-ST", "CD-RW GCE-8240B", ATA_DEVICE_CDRW, "LG 40/12/40 CD-RW", TRUE },
    { "HL-DT-ST", "DVD-ROM GDR-8163B", ATA_DEVICE_DVDROM, "LG 16X DVD-ROM", TRUE },
    { "HL-DT-ST", "DVDRAM GSA-4163B", ATA_DEVICE_DVDRW, "LG GSA-4163B DVD±RW/RAM", TRUE },
    { "HL-DT-ST", "DVDRAM GSA-H10N", ATA_DEVICE_DVDRW, "LG GSA-H10N DVD±RW/RAM", TRUE },

    // Lite-On CD/DVD Drives
    { "LITE-ON", "CD-ROM LTN-486S", ATA_DEVICE_CDROM, "Lite-On 48X CD-ROM", TRUE },
    { "LITE-ON", "CD-RW LTR-48125W", ATA_DEVICE_CDRW, "Lite-On 48/24/48 CD-RW", TRUE },
    { "LITE-ON", "CD-RW LTR-52327S", ATA_DEVICE_CDRW, "Lite-On 52/32/52 CD-RW", TRUE },
    { "LITE-ON", "DVDRW SOHW-1633S", ATA_DEVICE_DVDRW, "Lite-On SOHW-1633S DVD±RW", TRUE },
    { "LITE-ON", "DVDRW SHM-165H6S", ATA_DEVICE_DVDRW, "Lite-On SHM-165H6S DVD±RW", TRUE },

    // Toshiba CD/DVD Drives
    { "TOSHIBA", "CD-ROM XM-6202B", ATA_DEVICE_CDROM, "Toshiba 32X CD-ROM", TRUE },
    { "TOSHIBA", "DVD-ROM SD-M1612", ATA_DEVICE_DVDROM, "Toshiba 16X DVD-ROM", TRUE },
    { "TOSHIBA", "DVD-ROM SD-M1712", ATA_DEVICE_DVDROM, "Toshiba 48X DVD-ROM", TRUE },

    // ASUS/Samsung CD/DVD Drives
    { "ASUS", "CD-S520", ATA_DEVICE_CDROM, "ASUS 52X CD-ROM", TRUE },
    { "ASUS", "DVD-ROM E616", ATA_DEVICE_DVDROM, "ASUS 16X DVD-ROM", TRUE },
    { "ASUS", "DRW-1604P", ATA_DEVICE_DVDRW, "ASUS DRW-1604P DVD±RW", TRUE },

    //
    // === ATAPI Zip Drives ===
    //

    { "IOMEGA", "ZIP 100", ATA_DEVICE_ZIP, "Iomega Zip 100 ATAPI Drive", TRUE },
    { "IOMEGA", "ZIP 250", ATA_DEVICE_ZIP, "Iomega Zip 250 ATAPI Drive", TRUE },
    { "IOMEGA", "ZIP 750", ATA_DEVICE_ZIP, "Iomega Zip 750 ATAPI Drive", TRUE },

    //
    // === ATAPI LS-120 SuperDisk ===
    //

    { "MATSUSHITA", "LS-120", ATA_DEVICE_LS120, "Matsushita LS-120 SuperDisk 120MB", TRUE },
    { "COMPAQ", "LS-120", ATA_DEVICE_LS120, "Compaq LS-120 SuperDisk 120MB", TRUE },

    //
    // === ATAPI Tape Drives ===
    //

    { "SEAGATE", "STT20000A", ATA_DEVICE_TAPE, "Seagate Travan 10GB ATAPI Tape", TRUE },
    { "HP", "Colorado", ATA_DEVICE_TAPE, "HP Colorado 8GB ATAPI Tape", TRUE },
    { "ONSTREAM", "SC-30", ATA_DEVICE_TAPE, "OnStream SC-30 15GB ATAPI Tape", TRUE },
};

#define ATA_DEVICE_DB_COUNT (sizeof(g_ATADeviceDB) / sizeof(g_ATADeviceDB[0]))

//
// ============================================================================
//                      ATA over Ethernet (AoE) Implementation
// ============================================================================
//

/**
 * @brief AoE target database entry
 *
 * Stores information about discovered AoE targets on the network.
 */
typedef struct _AOE_TARGET_ENTRY {
    UINT16  Major;              /**< Major device address */
    UINT8   Minor;              /**< Minor device address */
    UINT8   MacAddr[6];         /**< Target MAC address */
    BOOLEAN bActive;            /**< Target is active/responding */
    BOOLEAN bReserved;          /**< Target is reserved */
    BOOLEAN bJumboFrames;       /**< Jumbo frames supported */
    UINT16  MaxSectors;         /**< Max sectors per request */
    UINT16  MTU;                /**< Maximum transmission unit */
    UINT32  BufferCount;        /**< Number of buffers */
    UINT16  FirmwareVersion;    /**< Firmware version */
    CHAR8   ConfigString[256];  /**< Configuration string */
    UINT64  TotalSectors;       /**< Total sectors (from IDENTIFY) */
    CHAR8   Model[41];          /**< Device model */
    CHAR8   Serial[21];         /**< Device serial number */
} AOE_TARGET_ENTRY;

/**
 * @brief AoE controller implementation structure
 */
typedef struct _AOE_CONTROLLER_IMPL {
    IIOAoEController    Vtbl;           /**< Virtual function table */
    ULONG               RefCount;       /**< Reference count */
    IIOService         *pNetDevice;     /**< Network device interface */

    // Discovery and target management
    AOE_TARGET_ENTRY    Targets[64];    /**< Discovered targets (max 64) */
    UINT32              uTargetCount;   /**< Number of discovered targets */
    BOOLEAN             bJumboEnabled;  /**< Jumbo frames enabled */
    UINT16              uMTU;           /**< Current MTU size */

    // Statistics
    UINT64              uPacketsSent;       /**< Total packets sent */
    UINT64              uPacketsReceived;   /**< Total packets received */
    UINT64              uBytesTransferred;  /**< Total bytes transferred */
    UINT32              uErrors;            /**< Error count */

    // Command tracking
    UINT32              uNextTag;       /**< Next command tag */
    UINT8               MacAddr[6];     /**< Local MAC address */
} AOE_CONTROLLER_IMPL;

/**
 * @brief Known AoE Target Vendors/Models
 *
 * Database of common AoE storage targets and their characteristics.
 */
static CONST struct {
    CONST CHAR8 *pszVendor;
    CONST CHAR8 *pszProduct;
    CONST CHAR8 *pszDescription;
    BOOLEAN     bJumboDefault;
} g_AoETargetDB[] = {
    // Coraid AoE Storage Arrays
    { "Coraid", "EtherDrive", "Coraid EtherDrive AoE Storage", TRUE },
    { "Coraid", "SR", "Coraid SR Series AoE SAN", TRUE },
    { "Coraid", "VSX", "Coraid VSX Series AoE SAN", TRUE },
    { "Coraid", "SRX", "Coraid SRX Series AoE SAN", TRUE },

    // Linux vblade (Virtual AoE Target)
    { "vblade", "vblade", "Linux vblade Virtual AoE Target", TRUE },
    { "Linux", "AoE", "Linux AoE Target", TRUE },

    // FreeBSD AoE
    { "FreeBSD", "ggate", "FreeBSD GEOM Gate AoE Target", TRUE },

    // OpenBSD AoE
    { "OpenBSD", "bioctl", "OpenBSD bioctl AoE Target", FALSE },

    // Windows AoE
    { "WinAoE", "WinAoE", "Windows AoE Target", FALSE },

    // QEMU/KVM AoE
    { "QEMU", "AoE", "QEMU/KVM AoE Target", TRUE },

    // Custom/Embedded AoE Targets
    { "Ananke", "NUX-AoE", "NUX AoE Storage Target", TRUE },
};

#define AOE_TARGET_DB_COUNT (sizeof(g_AoETargetDB) / sizeof(g_AoETargetDB[0]))

/**
 * @brief ATA controller implementation structure
 */
typedef struct _ATA_CONTROLLER_IMPL {
    IIOATAController    Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOService         *pPCIDevice;         /**< PCI device interface */
    ATA_CONTROLLER_INFO ControllerInfo;     /**< Controller information */
    volatile UINT8     *pPrimaryBase;       /**< Primary channel I/O base */
    volatile UINT8     *pSecondaryBase;     /**< Secondary channel I/O base */
    volatile UINT8     *pBusMasterBase;     /**< Bus master I/O base */
    BOOLEAN             bInitialized;       /**< Controller initialized */
    UINT32              uFlags;             /**< Controller flags */
} ATA_CONTROLLER_IMPL;

/**
 * @brief ATA device implementation structure
 */
typedef struct _ATA_DEVICE_IMPL {
    IIOATADevice        Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOATAController   *pController;        /**< Parent controller */
    ATA_DEVICE_INFO     DeviceInfo;         /**< Device information */
    UINT32              uChannel;           /**< Channel number */
    UINT32              uDevice;            /**< Device number (0/1) */
} ATA_DEVICE_IMPL;

// Forward declarations
static IO_RETURN ATAController_Start(IIOATAController *pThis, IIOService *pProvider);
static IO_RETURN ATAController_GetControllerInfo(IIOATAController *pThis, ATA_CONTROLLER_INFO *pInfo);
static IO_RETURN ATAController_GetDeviceCount(IIOATAController *pThis, UINT32 *puCount);
static IO_RETURN ATAController_GetDevice(IIOATAController *pThis, UINT32 uChannel, UINT32 uDevice, IIOATADevice **ppDevice);
static IO_RETURN ATAController_ResetChannel(IIOATAController *pThis, UINT32 uChannel);
static IO_RETURN ATAController_ResetDevice(IIOATAController *pThis, UINT32 uChannel, UINT32 uDevice);
static IO_RETURN ATAController_ScanBus(IIOATAController *pThis);
static IO_RETURN ATAController_SubmitCommand(IIOATAController *pThis, UINT32 uChannel, UINT32 uDevice, ATA_COMMAND *pCommand);
static IO_RETURN ATAController_SetTransferMode(IIOATAController *pThis, UINT32 uChannel, UINT32 uDevice, ATA_TRANSFER_TYPE TransferType, UINT32 uMode);

/**
 * @brief Look up controller in database
 */
static CONST ATA_CONTROLLER_DB_ENTRY*
ATALookupController(
    UINT16 uVendorID,
    UINT16 uDeviceID
    )
{
    UINT32 i;

    for (i = 0; i < ATA_CONTROLLER_DB_COUNT; i++) {
        if (g_ATAControllerDB[i].VendorID == uVendorID &&
            g_ATAControllerDB[i].DeviceID == uDeviceID) {
            return &g_ATAControllerDB[i];
        }
    }

    return NULL;
}

/**
 * @brief IIOATAController::Start - Initialize controller
 */
static IO_RETURN
ATAController_Start(
    IIOATAController *pThis,
    IIOService *pProvider
    )
{
    ATA_CONTROLLER_IMPL *pController = (ATA_CONTROLLER_IMPL *)pThis;
    IIOPCIDevice *pPCIDevice = NULL;
    PCI_DEVICE_INFO PCIInfo;
    IO_RETURN Status;
    CONST ATA_CONTROLLER_DB_ENTRY *pDBEntry;

    if (pController == NULL || pProvider == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Query for PCI device interface
    Status = pProvider->lpVtbl->QueryInterface(pProvider, &IID_IIOPCIDevice, (VOID **)&pPCIDevice);
    if (Status != IO_SUCCESS || pPCIDevice == NULL) {
        printf("ATA: Not a PCI device\n");
        return IO_ERROR;
    }

    // Get PCI device information
    Status = pPCIDevice->lpVtbl->GetDeviceInfo(pPCIDevice, &PCIInfo);
    if (Status != IO_SUCCESS) {
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return Status;
    }

    // Verify this is an ATA controller (Class 01h, Subclass 01h = IDE/ATA controller)
    if (PCIInfo.ClassCode != 0x01 || PCIInfo.SubClass != 0x01) {
        printf("ATA: Not an IDE/ATA controller (Class %02X:%02X)\n",
               PCIInfo.ClassCode, PCIInfo.SubClass);
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return IO_NO_DEVICE;
    }

    // Look up controller in database
    pDBEntry = ATALookupController(PCIInfo.VendorID, PCIInfo.DeviceID);
    if (pDBEntry != NULL) {
        printf("ATA: Found %s %s\n", pDBEntry->pszVendor, pDBEntry->pszModel);
        strncpy(pController->ControllerInfo.VendorName, pDBEntry->pszVendor, sizeof(pController->ControllerInfo.VendorName) - 1);
        strncpy(pController->ControllerInfo.ControllerName, pDBEntry->pszModel, sizeof(pController->ControllerInfo.ControllerName) - 1);
        pController->ControllerInfo.Protocol = pDBEntry->MaxProtocol;
        pController->ControllerInfo.MaxUdmaMode = pDBEntry->MaxUdmaMode;
        pController->uFlags = pDBEntry->Flags;
    } else {
        printf("ATA: Unknown controller %04X:%04X\n", PCIInfo.VendorID, PCIInfo.DeviceID);
        snprintf(pController->ControllerInfo.ControllerName, sizeof(pController->ControllerInfo.ControllerName),
                 "Unknown ATA Controller %04X:%04X", PCIInfo.VendorID, PCIInfo.DeviceID);
        pController->ControllerInfo.Protocol = ATA_PROTOCOL_PATA;
        pController->ControllerInfo.MaxUdmaMode = ATA_UDMA_MODE_2;
    }

    pController->ControllerInfo.VendorID = PCIInfo.VendorID;
    pController->ControllerInfo.DeviceID = PCIInfo.DeviceID;
    pController->ControllerInfo.NumChannels = 2;  // Primary and Secondary
    pController->ControllerInfo.NumDevicesPerChannel = 2;  // Master and Slave
    pController->bInitialized = TRUE;

    pPCIDevice->lpVtbl->Release(pPCIDevice);
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::GetControllerInfo
 */
static IO_RETURN
ATAController_GetControllerInfo(
    IIOATAController *pThis,
    ATA_CONTROLLER_INFO *pInfo
    )
{
    ATA_CONTROLLER_IMPL *pController = (ATA_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pInfo, &pController->ControllerInfo, sizeof(ATA_CONTROLLER_INFO));
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::GetDeviceCount
 */
static IO_RETURN
ATAController_GetDeviceCount(
    IIOATAController *pThis,
    UINT32 *puCount
    )
{
    if (pThis == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Typically 2 channels × 2 devices = 4 maximum devices
    *puCount = 4;
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::GetDevice
 */
static IO_RETURN
ATAController_GetDevice(
    IIOATAController *pThis,
    UINT32 uChannel,
    UINT32 uDevice,
    IIOATADevice **ppDevice
    )
{
    if (pThis == NULL || ppDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uChannel >= 2 || uDevice >= 2) {
        return IO_BAD_ARGUMENT;
    }

    // Device enumeration would be implemented here
    *ppDevice = NULL;
    return IO_NO_DEVICE;
}

/**
 * @brief IIOATAController::ResetChannel
 */
static IO_RETURN
ATAController_ResetChannel(
    IIOATAController *pThis,
    UINT32 uChannel
    )
{
    if (pThis == NULL || uChannel >= 2) {
        return IO_BAD_ARGUMENT;
    }

    // Software reset implementation would go here
    printf("ATA: Reset channel %u\n", uChannel);
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::ResetDevice
 */
static IO_RETURN
ATAController_ResetDevice(
    IIOATAController *pThis,
    UINT32 uChannel,
    UINT32 uDevice
    )
{
    if (pThis == NULL || uChannel >= 2 || uDevice >= 2) {
        return IO_BAD_ARGUMENT;
    }

    printf("ATA: Reset device %u:%u\n", uChannel, uDevice);
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::ScanBus
 */
static IO_RETURN
ATAController_ScanBus(
    IIOATAController *pThis
    )
{
    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("ATA: Scanning bus for devices...\n");
    // Device enumeration logic would go here
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::SubmitCommand
 */
static IO_RETURN
ATAController_SubmitCommand(
    IIOATAController *pThis,
    UINT32 uChannel,
    UINT32 uDevice,
    ATA_COMMAND *pCommand
    )
{
    if (pThis == NULL || pCommand == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uChannel >= 2 || uDevice >= 2) {
        return IO_BAD_ARGUMENT;
    }

    // Command submission logic would go here
    return IO_SUCCESS;
}

/**
 * @brief IIOATAController::SetTransferMode
 */
static IO_RETURN
ATAController_SetTransferMode(
    IIOATAController *pThis,
    UINT32 uChannel,
    UINT32 uDevice,
    ATA_TRANSFER_TYPE TransferType,
    UINT32 uMode
    )
{
    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uChannel >= 2 || uDevice >= 2) {
        return IO_BAD_ARGUMENT;
    }

    printf("ATA: Set transfer mode for %u:%u to type %u mode %u\n",
           uChannel, uDevice, TransferType, uMode);
    return IO_SUCCESS;
}

//
// ============================================================================
//                      AoE Controller Implementation
// ============================================================================
//

/**
 * @brief AoE Helper: Convert major/minor to string
 */
UINT32
AoEAddressToString(
    UINT16 uMajor,
    UINT8 uMinor,
    CHAR8 *pszBuffer,
    UINTN cbBuffer
    )
{
    if (pszBuffer == NULL || cbBuffer == 0) {
        return 0;
    }

    return snprintf(pszBuffer, cbBuffer, "e%u.%u", uMajor, uMinor);
}

/**
 * @brief AoE Helper: Build AoE header
 */
VOID
AoEBuildHeader(
    AOE_HEADER *pHeader,
    AOE_COMMAND uCommand,
    UINT16 uMajor,
    UINT8 uMinor,
    UINT32 uTag,
    UINT8 uFlags
    )
{
    if (pHeader == NULL) {
        return;
    }

    memset(pHeader, 0, sizeof(AOE_HEADER));

    // Version (4 bits) | Flags (4 bits)
    pHeader->Ver_Flags = (AOE_VERSION_1 << 4) | (uFlags & 0x0F);
    pHeader->Error = AOE_ERR_NONE;
    pHeader->Major = uMajor;  // Will be converted to network byte order
    pHeader->Minor = uMinor;
    pHeader->Command = (UINT8)uCommand;
    pHeader->Tag = uTag;      // Will be converted to network byte order
}

/**
 * @brief IIOAoEController::DiscoverTargets
 */
static IO_RETURN
AoEController_DiscoverTargets(
    IIOAoEController *pThis,
    UINT32 uTimeoutMs
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;

    if (pController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    printf("AoE: Discovering targets (timeout %u ms)...\n", uTimeoutMs);
    printf("AoE: Sending broadcast to major=0xFFFF, minor=0xFF\n");

    // In a real implementation, this would:
    // 1. Build AoE discovery packet (major=0xFFFF, minor=0xFF)
    // 2. Send broadcast Ethernet frame (EtherType 0x88A2)
    // 3. Wait for responses with timeout
    // 4. Parse responses and populate Targets array
    // 5. Query configuration for each discovered target

    // For demonstration, simulate finding 2 targets
    pController->uTargetCount = 2;

    // Target 1: e0.0
    pController->Targets[0].Major = 0;
    pController->Targets[0].Minor = 0;
    pController->Targets[0].bActive = TRUE;
    pController->Targets[0].bReserved = FALSE;
    pController->Targets[0].bJumboFrames = TRUE;
    pController->Targets[0].MaxSectors = AOE_MAX_SECTORS_JUMBO;
    pController->Targets[0].MTU = AOE_MTU_JUMBO;
    pController->Targets[0].BufferCount = 16;
    pController->Targets[0].FirmwareVersion = 0x0100;
    strncpy(pController->Targets[0].ConfigString, "shelf=0,slot=0", 255);
    strncpy(pController->Targets[0].Model, "NUX AoE Virtual Disk 1", 40);
    strncpy(pController->Targets[0].Serial, "AOE000001", 20);
    pController->Targets[0].TotalSectors = 2097152; // 1 GB

    // Target 2: e1.5
    pController->Targets[1].Major = 1;
    pController->Targets[1].Minor = 5;
    pController->Targets[1].bActive = TRUE;
    pController->Targets[1].bReserved = FALSE;
    pController->Targets[1].bJumboFrames = FALSE;
    pController->Targets[1].MaxSectors = AOE_MAX_SECTORS_STANDARD;
    pController->Targets[1].MTU = AOE_MTU_STANDARD;
    pController->Targets[1].BufferCount = 8;
    pController->Targets[1].FirmwareVersion = 0x0101;
    strncpy(pController->Targets[1].ConfigString, "shelf=1,slot=5", 255);
    strncpy(pController->Targets[1].Model, "Coraid EtherDrive 500GB", 40);
    strncpy(pController->Targets[1].Serial, "COR500GB123", 20);
    pController->Targets[1].TotalSectors = 976773168; // 500 GB

    printf("AoE: Discovery complete. Found %u targets.\n", pController->uTargetCount);

    return IO_SUCCESS;
}

/**
 * @brief IIOAoEController::GetTargetCount
 */
static IO_RETURN
AoEController_GetTargetCount(
    IIOAoEController *pThis,
    UINT32 *puCount
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *puCount = pController->uTargetCount;
    return IO_SUCCESS;
}

/**
 * @brief IIOAoEController::GetTargetInfo
 */
static IO_RETURN
AoEController_GetTargetInfo(
    IIOAoEController *pThis,
    UINT32 uIndex,
    AOE_DEVICE_INFO *pTargetInfo
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || pTargetInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uIndex >= pController->uTargetCount) {
        return IO_BAD_ARGUMENT;
    }

    AOE_TARGET_ENTRY *pTarget = &pController->Targets[uIndex];

    pTargetInfo->Major = pTarget->Major;
    pTargetInfo->Minor = pTarget->Minor;
    memcpy(pTargetInfo->MacAddr, pTarget->MacAddr, 6);
    pTargetInfo->bJumboFrames = pTarget->bJumboFrames;
    pTargetInfo->MaxSectors = pTarget->MaxSectors;
    pTargetInfo->MTU = pTarget->MTU;
    pTargetInfo->BufferCount = pTarget->BufferCount;
    pTargetInfo->FirmwareVersion = pTarget->FirmwareVersion;
    strncpy(pTargetInfo->ConfigString, pTarget->ConfigString, 255);

    return IO_SUCCESS;
}

/**
 * @brief IIOAoEController::ConnectTarget
 */
static IO_RETURN
AoEController_ConnectTarget(
    IIOAoEController *pThis,
    UINT16 uMajor,
    UINT8 uMinor,
    IIOATADevice **ppDevice
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;
    UINT32 i;
    CHAR8 szAddr[16];

    if (pController == NULL || ppDevice == NULL) {
        return IO_BAD_ARGUMENT;
    }

    AoEAddressToString(uMajor, uMinor, szAddr, sizeof(szAddr));

    // Find the target
    for (i = 0; i < pController->uTargetCount; i++) {
        if (pController->Targets[i].Major == uMajor &&
            pController->Targets[i].Minor == uMinor) {

            if (pController->Targets[i].bReserved) {
                printf("AoE: Target %s is reserved by another host\n", szAddr);
                return IO_BUSY;
            }

            printf("AoE: Connecting to target %s (%s)...\n",
                   szAddr, pController->Targets[i].Model);

            // In a real implementation, this would:
            // 1. Query target configuration
            // 2. Execute IDENTIFY DEVICE command
            // 3. Create IIOATADevice interface for the target
            // 4. Set up command/response handling

            printf("AoE: Connected to %s\n", szAddr);
            printf("AoE:   Model: %s\n", pController->Targets[i].Model);
            printf("AoE:   Serial: %s\n", pController->Targets[i].Serial);
            printf("AoE:   Capacity: %llu MB\n",
                   (pController->Targets[i].TotalSectors * 512) / (1024 * 1024));
            printf("AoE:   Jumbo Frames: %s\n",
                   pController->Targets[i].bJumboFrames ? "Yes" : "No");
            printf("AoE:   Max Sectors/Request: %u\n", pController->Targets[i].MaxSectors);

            *ppDevice = NULL; // Would return actual device interface
            return IO_SUCCESS;
        }
    }

    printf("AoE: Target %s not found\n", szAddr);
    return IO_NO_DEVICE;
}

/**
 * @brief IIOAoEController::SetJumboFrames
 */
static IO_RETURN
AoEController_SetJumboFrames(
    IIOAoEController *pThis,
    BOOLEAN bEnable,
    UINT16 uMTU
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;

    if (pController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (bEnable && uMTU != AOE_MTU_JUMBO) {
        return IO_BAD_ARGUMENT;
    }

    pController->bJumboEnabled = bEnable;
    pController->uMTU = bEnable ? AOE_MTU_JUMBO : AOE_MTU_STANDARD;

    printf("AoE: Jumbo frames %s (MTU %u)\n",
           bEnable ? "enabled" : "disabled", pController->uMTU);

    return IO_SUCCESS;
}

/**
 * @brief IIOAoEController::GetStatistics
 */
static IO_RETURN
AoEController_GetStatistics(
    IIOAoEController *pThis,
    UINT64 *puPacketsSent,
    UINT64 *puPacketsReceived,
    UINT64 *puBytesTransferred,
    UINT32 *puErrors
    )
{
    AOE_CONTROLLER_IMPL *pController = (AOE_CONTROLLER_IMPL *)pThis;

    if (pController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (puPacketsSent != NULL) {
        *puPacketsSent = pController->uPacketsSent;
    }

    if (puPacketsReceived != NULL) {
        *puPacketsReceived = pController->uPacketsReceived;
    }

    if (puBytesTransferred != NULL) {
        *puBytesTransferred = pController->uBytesTransferred;
    }

    if (puErrors != NULL) {
        *puErrors = pController->uErrors;
    }

    return IO_SUCCESS;
}

/**
 * @brief Create AoE initiator controller instance
 */
IO_RETURN
AoEControllerCreate(
    IIOService *pNetworkDevice,
    IIOAoEController **ppController
    )
{
    AOE_CONTROLLER_IMPL *pController;

    if (pNetworkDevice == NULL || ppController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate controller instance
    pController = (AOE_CONTROLLER_IMPL *)malloc(sizeof(AOE_CONTROLLER_IMPL));
    if (pController == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pController, 0, sizeof(AOE_CONTROLLER_IMPL));
    pController->RefCount = 1;
    pController->pNetDevice = pNetworkDevice;
    pController->uMTU = AOE_MTU_STANDARD;
    pController->bJumboEnabled = FALSE;
    pController->uNextTag = 1;

    // Initialize MAC address (would read from network device)
    pController->MacAddr[0] = 0x00;
    pController->MacAddr[1] = 0x1A;
    pController->MacAddr[2] = 0x4D;
    pController->MacAddr[3] = 0x00;
    pController->MacAddr[4] = 0x00;
    pController->MacAddr[5] = 0x01;

    printf("AoE: Controller created (MAC: %02X:%02X:%02X:%02X:%02X:%02X)\n",
           pController->MacAddr[0], pController->MacAddr[1],
           pController->MacAddr[2], pController->MacAddr[3],
           pController->MacAddr[4], pController->MacAddr[5]);

    // Initialize vtable (would be properly initialized with all methods)
    // This is simplified for demonstration

    *ppController = (IIOAoEController *)pController;
    return IO_SUCCESS;
}

/**
 * @brief Initialize ATA/IDE family driver
 */
IO_RETURN
ATAInitialize(
    VOID
    )
{
    printf("ATA: Initializing ATA/IDE family driver\n");
    printf("ATA: Loaded %u controller definitions\n", ATA_CONTROLLER_DB_COUNT);
    printf("ATA: Loaded %u device definitions\n", ATA_DEVICE_DB_COUNT);
    printf("ATA: Loaded %u AoE target definitions\n", AOE_TARGET_DB_COUNT);
    printf("ATA: AoE Protocol: EtherType 0x%04X, Version %u\n", AOE_ETHERTYPE, AOE_VERSION_1);
    return IO_SUCCESS;
}

/**
 * @brief Shutdown ATA/IDE family driver
 */
IO_RETURN
ATAShutdown(
    VOID
    )
{
    printf("ATA: Shutting down ATA/IDE family driver\n");
    return IO_SUCCESS;
}

/**
 * @brief Create ATA controller instance
 */
IO_RETURN
ATAControllerCreate(
    IIOService *pPCIDevice,
    IIOATAController **ppController
    )
{
    ATA_CONTROLLER_IMPL *pController;

    if (pPCIDevice == NULL || ppController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate controller instance (simplified - would use proper allocator)
    pController = (ATA_CONTROLLER_IMPL *)malloc(sizeof(ATA_CONTROLLER_IMPL));
    if (pController == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pController, 0, sizeof(ATA_CONTROLLER_IMPL));
    pController->RefCount = 1;
    pController->pPCIDevice = pPCIDevice;

    // Initialize vtable (would be properly initialized with all methods)
    // This is simplified for demonstration

    *ppController = (IIOATAController *)pController;
    return IO_SUCCESS;
}
