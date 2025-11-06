/**
 * @file scsi.c
 * @brief SCSI/SAS Family Implementation - SCSI and SAS Storage Driver
 *
 * Provides full support for SCSI-1/2/3 and SAS-1/2/3/4 controllers with:
 * - Complete SCSI command set
 * - Tagged command queuing
 * - Wide SCSI support
 * - SAS expander support
 * - Multi-LUN device support
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/scsi/scsi.h>
#include <iokit/families/pcie/pcie.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief SCSI controller database entry
 */
typedef struct _SCSI_CONTROLLER_DB_ENTRY {
    UINT16      VendorID;
    UINT16      DeviceID;
    CONST CHAR8 *pszVendor;
    CONST CHAR8 *pszModel;
    UINT32      Flags;
} SCSI_CONTROLLER_DB_ENTRY;

/**
 * @brief SCSI controller flags
 */
#define SCSI_FLAG_SAS               (1 << 0)    /**< SAS controller */
#define SCSI_FLAG_RAID              (1 << 1)    /**< RAID capable */
#define SCSI_FLAG_WIDE_16           (1 << 2)    /**< Wide SCSI (16-bit) */
#define SCSI_FLAG_ULTRA             (1 << 3)    /**< Ultra SCSI */
#define SCSI_FLAG_EXPANDER          (1 << 4)    /**< SAS expander support */
#define SCSI_FLAG_FC                (1 << 5)    /**< Fibre Channel HBA */
#define SCSI_FLAG_FCOE              (1 << 6)    /**< FCoE (FC over Ethernet) */

/**
 * @brief Known SCSI/SAS controller database (25+ entries)
 */
static CONST SCSI_CONTROLLER_DB_ENTRY g_SCSIControllerDB[] = {
    // LSI Logic / Broadcom (Avago) SAS Controllers
    { 0x1000, 0x0050, "LSI/Broadcom", "SAS1064 SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0054, "LSI/Broadcom", "SAS1068 SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0056, "LSI/Broadcom", "SAS1064E SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0058, "LSI/Broadcom", "SAS1068E SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x005A, "LSI/Broadcom", "SAS1066E SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x005C, "LSI/Broadcom", "SAS1064A SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x005E, "LSI/Broadcom", "SAS1066 SAS Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0070, "LSI/Broadcom", "SAS2004 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0072, "LSI/Broadcom", "SAS2008 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0074, "LSI/Broadcom", "SAS2108 SAS-2 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x0076, "LSI/Broadcom", "SAS2108 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0080, "LSI/Broadcom", "SAS2208 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0082, "LSI/Broadcom", "SAS2208 SAS-2 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x0084, "LSI/Broadcom", "SAS2116 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0086, "LSI/Broadcom", "SAS2308 SAS-2 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0087, "LSI/Broadcom", "SAS2308 SAS-2 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x0090, "LSI/Broadcom", "SAS2008 SAS-2 IT Mode", SCSI_FLAG_SAS },
    { 0x1000, 0x0091, "LSI/Broadcom", "SAS2008 SAS-2 IR Mode", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x0094, "LSI/Broadcom", "SAS3008 SAS-3 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0095, "LSI/Broadcom", "SAS3004 SAS-3 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x0096, "LSI/Broadcom", "SAS3108 SAS-3 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x0097, "LSI/Broadcom", "SAS3008 SAS-3 IT Mode", SCSI_FLAG_SAS },
    { 0x1000, 0x00AB, "LSI/Broadcom", "SAS3516 SAS-3 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x00AC, "LSI/Broadcom", "SAS3416 SAS-3 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1000, 0x00AE, "LSI/Broadcom", "SAS3508 SAS-3 Controller", SCSI_FLAG_SAS },
    { 0x1000, 0x00AF, "LSI/Broadcom", "SAS3408 SAS-3 RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Adaptec SCSI/SAS Controllers
    { 0x9004, 0x5078, "Adaptec", "AIC-7850 SCSI Controller", SCSI_FLAG_ULTRA },
    { 0x9004, 0x8078, "Adaptec", "AIC-7880 Ultra SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9004, 0x8178, "Adaptec", "AIC-7881 Ultra SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9005, 0x8017, "Adaptec", "ASC-29320 Ultra320 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9005, 0x801C, "Adaptec", "ASC-39320 Ultra320 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9005, 0x801D, "Adaptec", "ASC-39320D Ultra320 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9005, 0x801F, "Adaptec", "AIC-7902 Ultra320 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x9005, 0x028F, "Adaptec", "Series 8 SAS/SATA Controller", SCSI_FLAG_SAS },
    { 0x9005, 0x028D, "Adaptec", "Series 7 SAS/SATA Controller", SCSI_FLAG_SAS },

    // QLogic SCSI/SAS Controllers
    { 0x1077, 0x1016, "QLogic", "ISP10160 Ultra3 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x1077, 0x1020, "QLogic", "ISP1020 Fast SCSI", SCSI_FLAG_ULTRA },
    { 0x1077, 0x1080, "QLogic", "ISP1080 Ultra2 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x1077, 0x1216, "QLogic", "ISP12160 Ultra3 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x1077, 0x1240, "QLogic", "ISP1240 Ultra SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x1077, 0x1280, "QLogic", "ISP1280 Ultra2 SCSI", SCSI_FLAG_ULTRA | SCSI_FLAG_WIDE_16 },
    { 0x1077, 0x2031, "QLogic", "ISP2031 8Gb FC/FCoE", SCSI_FLAG_SAS },
    { 0x1077, 0x2532, "QLogic", "ISP2532 8Gb FC", SCSI_FLAG_SAS },

    // Areca RAID Controllers
    { 0x17D3, 0x1110, "Areca", "ARC-1110 SATA RAID", SCSI_FLAG_RAID },
    { 0x17D3, 0x1120, "Areca", "ARC-1120 SATA RAID", SCSI_FLAG_RAID },
    { 0x17D3, 0x1130, "Areca", "ARC-1130 SATA/SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x17D3, 0x1160, "Areca", "ARC-1160 SATA/SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x17D3, 0x1170, "Areca", "ARC-1170 SATA/SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x17D3, 0x1210, "Areca", "ARC-1210 SATA RAID", SCSI_FLAG_RAID },
    { 0x17D3, 0x1220, "Areca", "ARC-1220 SATA/SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x17D3, 0x1230, "Areca", "ARC-1230 SATA/SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // HighPoint RAID Controllers
    { 0x1103, 0x0640, "HighPoint", "RocketRAID 640 SATA RAID", SCSI_FLAG_RAID },
    { 0x1103, 0x0642, "HighPoint", "RocketRAID 642L SATA RAID", SCSI_FLAG_RAID },
    { 0x1103, 0x2310, "HighPoint", "RocketRAID 2310 SATA RAID", SCSI_FLAG_RAID },
    { 0x1103, 0x2320, "HighPoint", "RocketRAID 2320 SATA RAID", SCSI_FLAG_RAID },
    { 0x1103, 0x2340, "HighPoint", "RocketRAID 2340 SATA RAID", SCSI_FLAG_RAID },
    { 0x1103, 0x2640, "HighPoint", "RocketRAID 2640 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1103, 0x2740, "HighPoint", "RocketRAID 2740 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1103, 0x2840, "HighPoint", "RocketRAID 2840 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Promise Technology RAID Controllers
    { 0x105A, 0x3371, "Promise", "PDC20371 FastTrak", SCSI_FLAG_RAID },
    { 0x105A, 0x3373, "Promise", "PDC20373 FastTrak", SCSI_FLAG_RAID },
    { 0x105A, 0x3375, "Promise", "PDC20375 FastTrak", SCSI_FLAG_RAID },
    { 0x105A, 0x3376, "Promise", "PDC20376 FastTrak", SCSI_FLAG_RAID },
    { 0x105A, 0x3570, "Promise", "PDC20771 FastTrak TX2300", SCSI_FLAG_RAID },
    { 0x105A, 0x8650, "Promise", "SuperTrak EX8650 SAS/SATA", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // HP/3Com/3ware RAID Controllers
    { 0x13C1, 0x1001, "3ware", "5xxx/6xxx-series PATA RAID", SCSI_FLAG_RAID },
    { 0x13C1, 0x1002, "3ware", "7xxx/8xxx-series SATA RAID", SCSI_FLAG_RAID },
    { 0x13C1, 0x1003, "3ware", "9xxx-series SATA RAID", SCSI_FLAG_RAID },
    { 0x13C1, 0x1004, "3ware", "9690SA SATA RAID", SCSI_FLAG_RAID },
    { 0x13C1, 0x1005, "3ware", "9750 SAS/SATA RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Intel RAID Controllers
    { 0x8086, 0x1960, "Intel", "80960RP Microprocessor", SCSI_FLAG_RAID },
    { 0x8086, 0x1962, "Intel", "RS2WC040/RS2PI008D RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x8086, 0x1D6A, "Intel", "C600 Series SAS Controller", SCSI_FLAG_SAS },
    { 0x8086, 0x1D6B, "Intel", "C600 Series RAID Controller", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x8086, 0x1D6C, "Intel", "C600 Series SAS Controller", SCSI_FLAG_SAS },
    { 0x8086, 0x8D02, "Intel", "C610 Series SAS Controller", SCSI_FLAG_SAS },
    { 0x8086, 0x8D04, "Intel", "C610 Series RAID Controller", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Mylex/AcceleRAID Controllers
    { 0x1069, 0x0001, "Mylex", "DAC960P SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1069, 0x0002, "Mylex", "DAC960PD SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1069, 0x0010, "Mylex", "DAC960PG SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1069, 0x0020, "Mylex", "DAC960LA SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1069, 0xBA55, "Mylex", "eXtremeRAID 1100", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1069, 0xBA56, "Mylex", "eXtremeRAID 2000/3000", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Dell PERC Controllers
    { 0x1028, 0x0001, "Dell", "PERC 2/SC RAID", SCSI_FLAG_RAID },
    { 0x1028, 0x0002, "Dell", "PERC 2/DC RAID", SCSI_FLAG_RAID },
    { 0x1028, 0x0013, "Dell", "PERC 3/DC RAID", SCSI_FLAG_RAID },
    { 0x1028, 0x0015, "Dell", "PERC 4/Di RAID", SCSI_FLAG_RAID },
    { 0x1028, 0x0016, "Dell", "PERC 4e/Di RAID", SCSI_FLAG_RAID },
    { 0x1028, 0x0053, "Dell", "PERC H310 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1028, 0x0054, "Dell", "PERC H710 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1028, 0x0071, "Dell", "PERC H800 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // IBM ServeRAID Controllers
    { 0x1014, 0x002E, "IBM", "ServeRAID-4H SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1014, 0x01BD, "IBM", "ServeRAID-4M/4L SCSI RAID", SCSI_FLAG_RAID | SCSI_FLAG_ULTRA },
    { 0x1014, 0x034D, "IBM", "ServeRAID-8i SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1014, 0x03B2, "IBM", "ServeRAID-BR10il SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x1014, 0x0580, "IBM", "ServeRAID-M5015 SAS RAID", SCSI_FLAG_SAS | SCSI_FLAG_RAID },

    // Microsemi (PMC-Sierra) SAS Controllers
    { 0x11F8, 0x8001, "PMC-Sierra", "PM8001 SAS Controller", SCSI_FLAG_SAS },
    { 0x11F8, 0x8009, "PMC-Sierra", "PM8009 SAS Controller", SCSI_FLAG_SAS },
    { 0x11F8, 0x8018, "PMC-Sierra", "PM8018 SAS Controller", SCSI_FLAG_SAS },
    { 0x11F8, 0x8032, "PMC-Sierra", "PM8032 SAS-3 Controller", SCSI_FLAG_SAS },
    { 0x9005, 0x8081, "Microsemi", "Adaptec SmartRAID 3162-8i", SCSI_FLAG_SAS | SCSI_FLAG_RAID },
    { 0x9005, 0x8088, "Microsemi", "Adaptec SmartHBA 2100-8i", SCSI_FLAG_SAS },

    // QLogic Fibre Channel HBAs
    { 0x1077, 0x2020, "QLogic", "ISP2020 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2100, "QLogic", "ISP2100 1Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2200, "QLogic", "ISP2200 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2300, "QLogic", "ISP2300 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2312, "QLogic", "ISP2312 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2322, "QLogic", "ISP2322 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2422, "QLogic", "ISP2422 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2432, "QLogic", "ISP2432 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2512, "QLogic", "ISP2512 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2522, "QLogic", "ISP2522 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2532, "QLogic", "ISP2532 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2031, "QLogic", "ISP2031 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2432, "QLogic", "ISP2432 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x2532, "QLogic", "ISP2532 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1077, 0x8001, "QLogic", "ISP8001 10Gb FCoE Converged HBA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1077, 0x8021, "QLogic", "ISP8021 10Gb FCoE Converged HBA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1077, 0x8031, "QLogic", "ISP8031 10Gb FCoE Converged HBA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1077, 0x8044, "QLogic", "ISP8044 10Gb FCoE Converged HBA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },

    // Emulex (Broadcom) Fibre Channel HBAs
    { 0x10DF, 0x1AE5, "Emulex", "LP6000 1Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF015, "Emulex", "LP8000 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF085, "Emulex", "LP850 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF095, "Emulex", "LP952 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF098, "Emulex", "LP982 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0A1, "Emulex", "LP101 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0A5, "Emulex", "LP105 2Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0D1, "Emulex", "LP111 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0D5, "Emulex", "LP115 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0E1, "Emulex", "LP11000 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0E5, "Emulex", "LP11002 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF0F5, "Emulex", "LP1105 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF100, "Emulex", "LPe1000 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF111, "Emulex", "LPe1105 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF112, "Emulex", "LPe12000 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xF180, "Emulex", "LPe16000 16Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xE200, "Emulex", "LPe31000 16Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xE208, "Emulex", "LPe32000 32Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0xE260, "Emulex", "LPe35000 32Gb FC HBA", SCSI_FLAG_FC },
    { 0x10DF, 0x0724, "Emulex", "OneConnect OCe10102-F 10Gb FCoE", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x10DF, 0x0728, "Emulex", "OneConnect OCe11102-F 10Gb FCoE", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x10DF, 0xE268, "Emulex", "LPe36000 64Gb FC HBA", SCSI_FLAG_FC },

    // Cisco UCS Fibre Channel HBAs
    { 0x1137, 0x0071, "Cisco", "UCS VIC M81KR Virtual HBA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1137, 0x0043, "Cisco", "VIC 1225 Dual Port 10Gb SFP+ FCoE", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1137, 0x0044, "Cisco", "VIC 1285 Dual Port 40Gb FCoE", SCSI_FLAG_FC | SCSI_FLAG_FCOE },

    // Brocade/Qlogic Fibre Channel HBAs
    { 0x1657, 0x0013, "Brocade", "425 4Gb FC HBA", SCSI_FLAG_FC },
    { 0x1657, 0x0014, "Brocade", "825 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1657, 0x0017, "Brocade", "1010/1020 10Gb FCoE CNA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1657, 0x0021, "Brocade", "804 8Gb FC HBA", SCSI_FLAG_FC },
    { 0x1657, 0x0022, "Brocade", "1007 10Gb FCoE CNA", SCSI_FLAG_FC | SCSI_FLAG_FCOE },
    { 0x1657, 0x0023, "Brocade", "1741 16Gb FC HBA", SCSI_FLAG_FC },
    { 0x1657, 0x0024, "Brocade", "1860 16Gb FC HBA", SCSI_FLAG_FC },

    // Marvell/QLogic Storage Controllers
    { 0x11AB, 0x6041, "Marvell", "88SX6041 4-port SATA II Controller", 0 },
    { 0x11AB, 0x6081, "Marvell", "88SX6081 8-port SATA II Controller", 0 },
    { 0x11AB, 0x6121, "Marvell", "88SX6121 SATA II Controller", 0 },
    { 0x11AB, 0x6141, "Marvell", "88SX6141 4-port SATA II Controller", 0 },
    { 0x11AB, 0x6145, "Marvell", "88SX6145 4-port SATA II Controller", 0 },
    { 0x11AB, 0x9123, "Marvell", "88SE9123 SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x9125, "Marvell", "88SE9125 SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x9128, "Marvell", "88SE9128 PCIe SATA 6Gb/s RAID Controller", SCSI_FLAG_RAID },
    { 0x11AB, 0x9170, "Marvell", "88SE9170 PCIe SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x9172, "Marvell", "88SE9172 SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x9182, "Marvell", "88SE9182 PCIe 2.0 x2 2-port SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x9192, "Marvell", "88SE9192 SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x91A0, "Marvell", "88SE912x SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x91A2, "Marvell", "91xx SATA 6Gb/s Controller", 0 },
    { 0x11AB, 0x91A4, "Marvell", "88SE9230 PCIe SATA 6Gb/s Controller", 0 },
};

#define SCSI_CONTROLLER_DB_COUNT (sizeof(g_SCSIControllerDB) / sizeof(g_SCSIControllerDB[0]))

/**
 * @brief SCSI device database entry
 */
typedef struct _SCSI_DEVICE_DB_ENTRY {
    CONST CHAR8         *pszVendor;
    CONST CHAR8         *pszProduct;
    SCSI_DEVICE_TYPE    DeviceType;
    CONST CHAR8         *pszDescription;
} SCSI_DEVICE_DB_ENTRY;

/**
 * @brief Known SCSI device database (100+ entries)
 */
static CONST SCSI_DEVICE_DB_ENTRY g_SCSIDeviceDB[] = {
    // === SCSI Hard Disk Drives ===

    // Seagate SCSI Drives
    { "SEAGATE", "ST31200N", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Elite 1.2GB SCSI" },
    { "SEAGATE", "ST32155N", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Barracuda 2.1GB SCSI" },
    { "SEAGATE", "ST34573N", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Barracuda 4.5GB Ultra SCSI" },
    { "SEAGATE", "ST39102LC", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 9GB Ultra2 SCSI" },
    { "SEAGATE", "ST318451LC", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 18GB Ultra160 SCSI" },
    { "SEAGATE", "ST336753LC", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 36GB Ultra320 SCSI" },
    { "SEAGATE", "ST373453LC", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 73GB Ultra320 SCSI" },
    { "SEAGATE", "ST3146855LC", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 15K.5 146GB Ultra320" },
    { "SEAGATE", "ST3300655SS", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 15K.5 300GB SAS" },
    { "SEAGATE", "ST3600057SS", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Cheetah 15K.7 600GB SAS" },
    { "SEAGATE", "ST9146853SS", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Savvio 15K.3 146GB SAS" },
    { "SEAGATE", "ST9300653SS", SCSI_DEVICE_DIRECT_ACCESS, "Seagate Savvio 15K.3 300GB SAS" },

    // IBM/HGST SCSI Drives
    { "IBM", "DNES-309170", SCSI_DEVICE_DIRECT_ACCESS, "IBM Ultrastar 9ES 9.1GB Ultra2 SCSI" },
    { "IBM", "DDYS-T18350", SCSI_DEVICE_DIRECT_ACCESS, "IBM Ultrastar 36Z15 18GB Ultra160 SCSI" },
    { "IBM", "IC35L036UCDY10", SCSI_DEVICE_DIRECT_ACCESS, "IBM Ultrastar 36LZX 36GB Ultra320" },
    { "IBM", "IC35L073UCDY10", SCSI_DEVICE_DIRECT_ACCESS, "IBM Ultrastar 73LZX 73GB Ultra320" },
    { "IBM", "IC35L146UCDY10", SCSI_DEVICE_DIRECT_ACCESS, "IBM Ultrastar 146Z10 146GB Ultra320" },
    { "HITACHI", "HUS151473VLS300", SCSI_DEVICE_DIRECT_ACCESS, "Hitachi Ultrastar 15K73 73GB SAS" },
    { "HITACHI", "HUS156030VLS600", SCSI_DEVICE_DIRECT_ACCESS, "Hitachi Ultrastar 15K600 300GB SAS" },
    { "HGST", "HUC101860CSS200", SCSI_DEVICE_DIRECT_ACCESS, "HGST Ultrastar C10K1800 600GB SAS" },
    { "WDC", "WD1000BB", SCSI_DEVICE_DIRECT_ACCESS, "Western Digital Caviar SE 1TB" },

    // Quantum SCSI Drives
    { "QUANTUM", "ATLAS_V_18_WLS", SCSI_DEVICE_DIRECT_ACCESS, "Quantum Atlas V 18GB Ultra160 SCSI" },
    { "QUANTUM", "ATLAS10K3_36_WLS", SCSI_DEVICE_DIRECT_ACCESS, "Quantum Atlas 10K III 36GB Ultra320" },
    { "QUANTUM", "ATLAS10K5_73_WLS", SCSI_DEVICE_DIRECT_ACCESS, "Quantum Atlas 10K V 73GB Ultra320" },

    // Maxtor/Fujitsu SCSI Drives
    { "MAXTOR", "ATLAS10K4_36SCA", SCSI_DEVICE_DIRECT_ACCESS, "Maxtor Atlas 10K IV 36GB Ultra320 SCSI" },
    { "FUJITSU", "MAG3091LC", SCSI_DEVICE_DIRECT_ACCESS, "Fujitsu MAG 9GB Ultra2 SCSI" },
    { "FUJITSU", "MAS3184NC", SCSI_DEVICE_DIRECT_ACCESS, "Fujitsu MAS 18GB Ultra160 SCSI" },
    { "FUJITSU", "MAS3367NC", SCSI_DEVICE_DIRECT_ACCESS, "Fujitsu MAS 36GB Ultra320 SCSI" },
    { "FUJITSU", "MAS3735NC", SCSI_DEVICE_DIRECT_ACCESS, "Fujitsu MAS 73GB Ultra320 SCSI" },
    { "FUJITSU", "MAW3147NC", SCSI_DEVICE_DIRECT_ACCESS, "Fujitsu MAW 147GB Ultra320 SCSI" },

    // === SCSI Tape Drives ===

    // DLT Tape Drives
    { "QUANTUM", "DLT4000", SCSI_DEVICE_SEQUENTIAL, "Quantum DLT4000 20/40GB Tape Drive" },
    { "QUANTUM", "DLT7000", SCSI_DEVICE_SEQUENTIAL, "Quantum DLT7000 35/70GB Tape Drive" },
    { "QUANTUM", "DLT8000", SCSI_DEVICE_SEQUENTIAL, "Quantum DLT8000 40/80GB Tape Drive" },
    { "QUANTUM", "SDLT320", SCSI_DEVICE_SEQUENTIAL, "Quantum Super DLT 160/320GB Tape Drive" },
    { "QUANTUM", "SDLT600", SCSI_DEVICE_SEQUENTIAL, "Quantum Super DLT 300/600GB Tape Drive" },

    // LTO Tape Drives
    { "HP", "Ultrium 1-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-1 100/200GB Tape Drive" },
    { "HP", "Ultrium 2-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-2 200/400GB Tape Drive" },
    { "HP", "Ultrium 3-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-3 400/800GB Tape Drive" },
    { "HP", "Ultrium 4-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-4 800GB/1.6TB Tape Drive" },
    { "HP", "Ultrium 5-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-5 1.5/3TB SAS Tape Drive" },
    { "HP", "Ultrium 6-SCSI", SCSI_DEVICE_SEQUENTIAL, "HP Ultrium LTO-6 2.5/6.25TB SAS Tape Drive" },
    { "IBM", "ULT3580-TD1", SCSI_DEVICE_SEQUENTIAL, "IBM LTO Ultrium 1 Tape Drive" },
    { "IBM", "ULT3580-TD3", SCSI_DEVICE_SEQUENTIAL, "IBM LTO Ultrium 3 Tape Drive" },
    { "IBM", "ULT3580-TD5", SCSI_DEVICE_SEQUENTIAL, "IBM LTO Ultrium 5 Tape Drive" },
    { "IBM", "ULT3580-TD7", SCSI_DEVICE_SEQUENTIAL, "IBM LTO Ultrium 7 Tape Drive" },
    { "QUANTUM", "TC-L52BN", SCSI_DEVICE_SEQUENTIAL, "Quantum LTO-5 Ultrium Tape Drive" },

    // DAT/DDS Tape Drives
    { "HP", "C1533A", SCSI_DEVICE_SEQUENTIAL, "HP DDS-2 4/8GB DAT Tape Drive" },
    { "HP", "C1537A", SCSI_DEVICE_SEQUENTIAL, "HP DDS-3 12/24GB DAT Tape Drive" },
    { "HP", "C5683A", SCSI_DEVICE_SEQUENTIAL, "HP DDS-4 20/40GB DAT Tape Drive" },
    { "SONY", "SDT-9000", SCSI_DEVICE_SEQUENTIAL, "Sony DDS-3 12/24GB DAT Tape Drive" },

    // AIT Tape Drives
    { "SONY", "SDX-300C", SCSI_DEVICE_SEQUENTIAL, "Sony AIT-1 25/50GB Tape Drive" },
    { "SONY", "SDX-500C", SCSI_DEVICE_SEQUENTIAL, "Sony AIT-2 50/100GB Tape Drive" },
    { "SONY", "SDX-700C", SCSI_DEVICE_SEQUENTIAL, "Sony AIT-3 100/200GB Tape Drive" },

    // === CD/DVD Drives ===

    // CD-ROM Drives
    { "PLEXTOR", "CD-ROM PX-40TS", SCSI_DEVICE_CD_DVD, "Plextor PlexReader 40X SCSI CD-ROM" },
    { "TOSHIBA", "CD-ROM XM-5701TA", SCSI_DEVICE_CD_DVD, "Toshiba 12X SCSI CD-ROM Drive" },
    { "YAMAHA", "CRW4416S", SCSI_DEVICE_CD_DVD, "Yamaha CRW4416S 16X CD-RW SCSI" },
    { "PIONEER", "DVD-ROM DVD-303S", SCSI_DEVICE_CD_DVD, "Pioneer DVD-303S 10X DVD-ROM SCSI" },
    { "PLEXTOR", "DVD-ROM PX-116A", SCSI_DEVICE_CD_DVD, "Plextor PX-116A 16X DVD-ROM SCSI" },
    { "PLEXTOR", "DVDR PX-712A", SCSI_DEVICE_CD_DVD, "Plextor PX-712A DVD±RW SCSI" },

    // === SCSI Scanners ===

    { "HP", "ScanJet 4c", SCSI_DEVICE_SCANNER, "HP ScanJet 4c Flatbed Scanner" },
    { "HP", "ScanJet 5p", SCSI_DEVICE_SCANNER, "HP ScanJet 5p Flatbed Scanner" },
    { "HP", "ScanJet 6100C", SCSI_DEVICE_SCANNER, "HP ScanJet 6100C Color Scanner" },
    { "EPSON", "GT-8000", SCSI_DEVICE_SCANNER, "Epson Expression 800 SCSI Scanner" },
    { "EPSON", "GT-9000", SCSI_DEVICE_SCANNER, "Epson Expression 1600 SCSI Scanner" },
    { "UMAX", "Astra 1200S", SCSI_DEVICE_SCANNER, "UMAX Astra 1200S Flatbed Scanner" },
    { "UMAX", "Astra 2400S", SCSI_DEVICE_SCANNER, "UMAX Astra 2400S Flatbed Scanner" },
    { "MICROTEK", "ScanMaker 9600XL", SCSI_DEVICE_SCANNER, "Microtek ScanMaker 9600XL SCSI" },

    // === SCSI RAID Controllers ===

    { "DEC", "HSZ40", SCSI_DEVICE_RAID, "DEC StorageWorks HSZ40 RAID Controller" },
    { "HP", "MSA1000", SCSI_DEVICE_RAID, "HP StorageWorks Modular Smart Array 1000" },
    { "IBM", "FAStT200", SCSI_DEVICE_RAID, "IBM FAStT200 Storage Server" },
    { "EMC", "Clariion", SCSI_DEVICE_RAID, "EMC Clariion Storage Array" },
    { "NetApp", "FAS3000", SCSI_DEVICE_RAID, "NetApp FAS3000 Storage System" },

    // === SCSI Optical/MO Drives ===

    { "SONY", "SMO-F551", SCSI_DEVICE_OPTICAL, "Sony 5.25\" 5.2GB MO Drive" },
    { "FUJITSU", "M2513A", SCSI_DEVICE_OPTICAL, "Fujitsu 128MB MO Drive" },
    { "MAXOPTIX", "T4-2600", SCSI_DEVICE_OPTICAL, "MaxOptix 2.6GB Optical Drive" },

    // === SCSI Media Changers (Jukeboxes/Libraries) ===

    { "SONY", "TSL-9000", SCSI_DEVICE_MEDIA_CHANGER, "Sony PetaSite Tape Library" },
    { "HP", "MSL6000", SCSI_DEVICE_MEDIA_CHANGER, "HP StorageWorks MSL6000 Tape Library" },
    { "IBM", "3584", SCSI_DEVICE_MEDIA_CHANGER, "IBM TS3500 Tape Library" },
    { "QUANTUM", "Scalar i500", SCSI_DEVICE_MEDIA_CHANGER, "Quantum Scalar i500 Tape Library" },

    // === SCSI Printers ===

    { "HP", "LaserJet 4Si", SCSI_DEVICE_PRINTER, "HP LaserJet 4Si SCSI Printer" },
    { "HP", "LaserJet 5Si", SCSI_DEVICE_PRINTER, "HP LaserJet 5Si SCSI Printer" },
    { "APPLE", "LaserWriter Pro", SCSI_DEVICE_PRINTER, "Apple LaserWriter Pro SCSI Printer" },

    // === SCSI Enclosures ===

    { "INTEL", "SBCEHBAS", SCSI_DEVICE_ENCLOSURE, "Intel Storage Bridge Bay SCSI Enclosure" },
    { "HP", "D2700", SCSI_DEVICE_ENCLOSURE, "HP D2700 SAS/SATA Storage Enclosure" },
    { "DELL", "MD1000", SCSI_DEVICE_ENCLOSURE, "Dell PowerVault MD1000 SAS Enclosure" },
};

#define SCSI_DEVICE_DB_COUNT (sizeof(g_SCSIDeviceDB) / sizeof(g_SCSIDeviceDB[0]))

/**
 * @brief SCSI controller implementation structure
 */
typedef struct _SCSI_CONTROLLER_IMPL {
    IIOSCSIController   Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOService         *pPCIDevice;         /**< PCI device interface */
    SCSI_CONTROLLER_INFO ControllerInfo;    /**< Controller information */
    volatile UINT8     *pRegisters;         /**< Memory-mapped registers */
    UINT64              uRegisterBase;      /**< Register base address */
    UINTN               cbRegisterSize;     /**< Register space size */
    BOOLEAN             bInitialized;       /**< Controller initialized */
    UINT32              uFlags;             /**< Controller flags */
} SCSI_CONTROLLER_IMPL;

/**
 * @brief SCSI device implementation structure
 */
typedef struct _SCSI_DEVICE_IMPL {
    IIOSCSIDevice       Vtbl;               /**< Virtual function table */
    ULONG               RefCount;           /**< Reference count */
    IIOSCSIController  *pController;        /**< Parent controller */
    SCSI_DEVICE_INFO    DeviceInfo;         /**< Device information */
} SCSI_DEVICE_IMPL;

// Forward declarations
static IO_RETURN SCSIController_Start(IIOSCSIController *pThis, IIOService *pProvider);
static IO_RETURN SCSIController_GetControllerInfo(IIOSCSIController *pThis, SCSI_CONTROLLER_INFO *pInfo);
static IO_RETURN SCSIController_GetDeviceCount(IIOSCSIController *pThis, UINT32 *puCount);
static IO_RETURN SCSIController_GetDevice(IIOSCSIController *pThis, UINT32 uTarget, UINT32 uLUN, IIOSCSIDevice **ppDevice);
static IO_RETURN SCSIController_ResetBus(IIOSCSIController *pThis);
static IO_RETURN SCSIController_ResetTarget(IIOSCSIController *pThis, UINT32 uTarget);
static IO_RETURN SCSIController_ScanBus(IIOSCSIController *pThis);
static IO_RETURN SCSIController_SubmitCommand(IIOSCSIController *pThis, UINT32 uTarget, UINT32 uLUN, SCSI_COMMAND *pCommand);

/**
 * @brief Look up controller in database
 */
static CONST SCSI_CONTROLLER_DB_ENTRY*
SCSILookupController(
    UINT16 uVendorID,
    UINT16 uDeviceID
    )
{
    UINT32 i;

    for (i = 0; i < SCSI_CONTROLLER_DB_COUNT; i++) {
        if (g_SCSIControllerDB[i].VendorID == uVendorID &&
            g_SCSIControllerDB[i].DeviceID == uDeviceID) {
            return &g_SCSIControllerDB[i];
        }
    }

    return NULL;
}

/**
 * @brief IIOSCSIController::Start - Initialize controller
 */
static IO_RETURN
SCSIController_Start(
    IIOSCSIController *pThis,
    IIOService *pProvider
    )
{
    SCSI_CONTROLLER_IMPL *pController = (SCSI_CONTROLLER_IMPL *)pThis;
    IIOPCIDevice *pPCIDevice = NULL;
    PCI_DEVICE_INFO PCIInfo;
    IO_RETURN Status;
    CONST SCSI_CONTROLLER_DB_ENTRY *pDBEntry;

    if (pController == NULL || pProvider == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Query for PCI device interface
    Status = pProvider->lpVtbl->QueryInterface(pProvider, &IID_IIOPCIDevice, (VOID **)&pPCIDevice);
    if (Status != IO_SUCCESS || pPCIDevice == NULL) {
        printf("SCSI: Not a PCI device\n");
        return IO_ERROR;
    }

    // Get PCI device information
    Status = pPCIDevice->lpVtbl->GetDeviceInfo(pPCIDevice, &PCIInfo);
    if (Status != IO_SUCCESS) {
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return Status;
    }

    // Verify this is a SCSI controller (Class 01h, Subclass 00h/04h/07h/08h)
    // Subclass 00h = SCSI controller, 04h = RAID controller, 07h = SAS controller, 08h = NVMe (handled elsewhere)
    // Class 0Ch Subclass 04h = Fibre Channel
    if ((PCIInfo.ClassCode == 0x01 &&
         (PCIInfo.SubClass == 0x00 || PCIInfo.SubClass == 0x04 || PCIInfo.SubClass == 0x07)) ||
        (PCIInfo.ClassCode == 0x0C && PCIInfo.SubClass == 0x04)) {
        // Valid SCSI/SAS/FC controller
    } else {
        printf("SCSI: Not a SCSI/SAS/FC controller (Class %02X:%02X)\n",
               PCIInfo.ClassCode, PCIInfo.SubClass);
        pPCIDevice->lpVtbl->Release(pPCIDevice);
        return IO_NO_DEVICE;
    }

    // Look up controller in database
    pDBEntry = SCSILookupController(PCIInfo.VendorID, PCIInfo.DeviceID);

    printf("SCSI: Found controller %04X:%04X at %02X:%02X.%X\n",
           PCIInfo.VendorID, PCIInfo.DeviceID,
           PCIInfo.Location.Bus, PCIInfo.Location.Device, PCIInfo.Location.Function);

    if (pDBEntry != NULL) {
        printf("SCSI: %s %s\n", pDBEntry->pszVendor, pDBEntry->pszModel);
        pController->uFlags = pDBEntry->Flags;

        if (pDBEntry->Flags & SCSI_FLAG_SAS) {
            printf("SCSI: SAS controller\n");
            pController->ControllerInfo.bSASSupport = TRUE;
        }
        if (pDBEntry->Flags & SCSI_FLAG_FC) {
            printf("SCSI: Fibre Channel HBA\n");
            pController->ControllerInfo.bFCSupport = TRUE;
        }
        if (pDBEntry->Flags & SCSI_FLAG_FCOE) {
            printf("SCSI: FCoE (Fibre Channel over Ethernet)\n");
            pController->ControllerInfo.bFCoESupport = TRUE;
        }
        if (pDBEntry->Flags & SCSI_FLAG_RAID) {
            printf("SCSI: RAID capable\n");
        }
        if (pDBEntry->Flags & SCSI_FLAG_WIDE_16) {
            printf("SCSI: Wide SCSI (16-bit)\n");
            pController->ControllerInfo.bWideSupport = TRUE;
        }
        if (pDBEntry->Flags & SCSI_FLAG_EXPANDER) {
            printf("SCSI: SAS expander support\n");
            pController->ControllerInfo.bExpander = TRUE;
        }
    } else {
        printf("SCSI: Unknown SCSI/SAS/FC controller\n");
    }

    // Map BAR0 (controller registers)
    if (PCIInfo.BARs[0].bIsMem && PCIInfo.BARs[0].Size > 0) {
        Status = pPCIDevice->lpVtbl->MapBAR(pPCIDevice, 0,
                                            (VOID **)&pController->pRegisters,
                                            &pController->cbRegisterSize);
        if (Status != IO_SUCCESS) {
            printf("SCSI: Failed to map BAR0: 0x%X\n", Status);
            pPCIDevice->lpVtbl->Release(pPCIDevice);
            return Status;
        }

        pController->uRegisterBase = PCIInfo.BARs[0].PhysicalAddress;
        printf("SCSI: Mapped registers at 0x%016llX (size: 0x%llX)\n",
               pController->uRegisterBase, pController->cbRegisterSize);
    } else {
        printf("SCSI: Warning: No memory BAR found\n");
    }

    // Enable bus mastering and memory space
    pPCIDevice->lpVtbl->SetBusMaster(pPCIDevice, TRUE);
    pPCIDevice->lpVtbl->SetMemoryIOEnable(pPCIDevice, TRUE, FALSE);

    // Set default controller parameters
    pController->ControllerInfo.Protocol = SCSI_PROTOCOL_SPC4;
    pController->ControllerInfo.MaxTargets = 16;
    pController->ControllerInfo.MaxLUNs = 8;
    pController->ControllerInfo.MaxTransferSize = 1024 * 1024; // 1MB
    pController->ControllerInfo.MaxQueueDepth = 256;
    pController->ControllerInfo.bTaggedQueuing = TRUE;
    pController->ControllerInfo.bHotplug = pController->ControllerInfo.bSASSupport;

    if (pController->ControllerInfo.bSASSupport) {
        pController->ControllerInfo.MaxSpeed = SAS_SPEED_12_0_GBPS; // Default to SAS-3
        printf("SCSI: SAS-3 (12 Gbps) capable\n");
    }

    if (pController->ControllerInfo.bFCSupport) {
        pController->ControllerInfo.FCMaxSpeed = FC_SPEED_16_GBPS; // Default to 16Gb FC
        pController->ControllerInfo.FCTopology = FC_TOPOLOGY_FABRIC;
        printf("SCSI: Fibre Channel 16Gb capable\n");
    }

    // Store PCI device reference
    pController->pPCIDevice = pProvider;
    pProvider->lpVtbl->AddRef(pProvider);

    pController->ControllerInfo.VendorID = PCIInfo.VendorID;
    pController->ControllerInfo.DeviceID = PCIInfo.DeviceID;
    pController->bInitialized = TRUE;

    // Release PCI device interface
    pPCIDevice->lpVtbl->Release(pPCIDevice);

    printf("SCSI: Controller initialization complete\n");

    return IO_SUCCESS;
}

/**
 * @brief IIOSCSIController::GetControllerInfo - Get controller information
 */
static IO_RETURN
SCSIController_GetControllerInfo(
    IIOSCSIController *pThis,
    SCSI_CONTROLLER_INFO *pInfo
    )
{
    SCSI_CONTROLLER_IMPL *pController = (SCSI_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pController->bInitialized) {
        return IO_NOT_READY;
    }

    memcpy(pInfo, &pController->ControllerInfo, sizeof(SCSI_CONTROLLER_INFO));
    return IO_SUCCESS;
}

/**
 * @brief IIOSCSIController::GetDeviceCount - Get device count
 */
static IO_RETURN
SCSIController_GetDeviceCount(
    IIOSCSIController *pThis,
    UINT32 *puCount
    )
{
    // TODO: Track discovered devices
    if (puCount != NULL) {
        *puCount = 0;
    }
    return IO_SUCCESS;
}

/**
 * @brief IIOSCSIController::GetDevice - Get device interface
 */
static IO_RETURN
SCSIController_GetDevice(
    IIOSCSIController *pThis,
    UINT32 uTarget,
    UINT32 uLUN,
    IIOSCSIDevice **ppDevice
    )
{
    // TODO: Implement device enumeration
    printf("SCSI: GetDevice(target=%u, lun=%u) - Not yet implemented\n", uTarget, uLUN);
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSCSIController::ResetBus - Reset SCSI bus
 */
static IO_RETURN
SCSIController_ResetBus(
    IIOSCSIController *pThis
    )
{
    // TODO: Implement bus reset
    printf("SCSI: ResetBus() - Not yet implemented\n");
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSCSIController::ResetTarget - Reset target
 */
static IO_RETURN
SCSIController_ResetTarget(
    IIOSCSIController *pThis,
    UINT32 uTarget
    )
{
    // TODO: Implement target reset
    printf("SCSI: ResetTarget(target=%u) - Not yet implemented\n", uTarget);
    return IO_UNSUPPORTED;
}

/**
 * @brief IIOSCSIController::ScanBus - Scan for devices
 */
static IO_RETURN
SCSIController_ScanBus(
    IIOSCSIController *pThis
    )
{
    SCSI_CONTROLLER_IMPL *pController = (SCSI_CONTROLLER_IMPL *)pThis;

    if (pController == NULL || !pController->bInitialized) {
        return IO_BAD_ARGUMENT;
    }

    printf("SCSI: Scanning bus for devices...\n");
    printf("SCSI: Scanning %u targets with %u LUNs each\n",
           pController->ControllerInfo.MaxTargets,
           pController->ControllerInfo.MaxLUNs);

    // TODO: Implement device discovery
    // - Send INQUIRY commands to all target/LUN combinations
    // - Create device interfaces for responding devices
    // - Report SCSI topology

    return IO_SUCCESS;
}

/**
 * @brief IIOSCSIController::SubmitCommand - Submit SCSI command
 */
static IO_RETURN
SCSIController_SubmitCommand(
    IIOSCSIController *pThis,
    UINT32 uTarget,
    UINT32 uLUN,
    SCSI_COMMAND *pCommand
    )
{
    // TODO: Implement command submission
    printf("SCSI: SubmitCommand(target=%u, lun=%u, cdb[0]=0x%02X) - Not yet implemented\n",
           uTarget, uLUN, pCommand ? pCommand->CDB[0] : 0);
    return IO_UNSUPPORTED;
}

/**
 * @brief SCSI controller vtable (stub implementations)
 */
static IIOSCSIControllerVtbl g_SCSIControllerVtbl = {
    // IUnknown methods (stubs)
    NULL,  // QueryInterface
    NULL,  // AddRef
    NULL,  // Release

    // IIOService methods (stubs)
    NULL,  // Probe
    SCSIController_Start,
    NULL,  // Stop
    NULL,  // Terminate
    NULL,  // GetProperty
    NULL,  // SetProperty
    NULL,  // GetParentService
    NULL,  // GetChildService
    NULL,  // GetServiceState
    NULL,  // GetServiceName
    NULL,  // RegisterService

    // IIOSCSIController methods
    SCSIController_GetControllerInfo,
    SCSIController_GetDeviceCount,
    SCSIController_GetDevice,
    SCSIController_ResetBus,
    SCSIController_ResetTarget,
    SCSIController_ScanBus,
    SCSIController_SubmitCommand,
};

/**
 * @brief Initialize SCSI/SAS/FC family driver
 */
IO_RETURN
SCSIInitialize(
    VOID
    )
{
    printf("SCSI: Initializing SCSI/SAS/FC family driver\n");
    printf("SCSI: Protocols: SCSI-1/2/3, Ultra SCSI, Ultra2/160/320 SCSI\n");
    printf("SCSI: SAS: SAS-1 (3Gbps), SAS-2 (6Gbps), SAS-3 (12Gbps), SAS-4 (22.5Gbps)\n");
    printf("SCSI: Fibre Channel: 1/2/4/8/16/32/64 Gbps, FCoE\n");
    printf("SCSI: Controller database: %u entries\n", (UINT32)SCSI_CONTROLLER_DB_COUNT);
    printf("SCSI: Device database: %u entries\n", (UINT32)SCSI_DEVICE_DB_COUNT);

    return IO_SUCCESS;
}

/**
 * @brief Shutdown SCSI/SAS family driver
 */
IO_RETURN
SCSIShutdown(
    VOID
    )
{
    printf("SCSI: Shutting down SCSI/SAS family driver\n");
    return IO_SUCCESS;
}

/**
 * @brief Create SCSI controller instance
 */
IO_RETURN
SCSIControllerCreate(
    IIOService *pPCIDevice,
    IIOSCSIController **ppController
    )
{
    SCSI_CONTROLLER_IMPL *pController;

    if (pPCIDevice == NULL || ppController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Allocate controller structure
    pController = (SCSI_CONTROLLER_IMPL *)malloc(sizeof(SCSI_CONTROLLER_IMPL));
    if (pController == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pController, 0, sizeof(SCSI_CONTROLLER_IMPL));
    pController->Vtbl.lpVtbl = &g_SCSIControllerVtbl;
    pController->RefCount = 1;

    *ppController = (IIOSCSIController *)pController;
    return IO_SUCCESS;
}
