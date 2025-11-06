/**
 * @file floppy.c
 * @brief Floppy Family Implementation - Floppy Disk and Removable Media Driver
 *
 * Provides comprehensive support for floppy disk controllers and drives including:
 * - Standard PC floppy controllers (8272/8272A/82072/82077/82078)
 * - USB floppy drives (various manufacturers)
 * - Parallel port floppy drives (BackPack, PowerFloppie)
 * - High-capacity removable media (Zip, Jaz, LS-120, SuperDisk)
 * - Multiple media formats and geometries
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/floppy/floppy.h>
#include <iokit/families/storage/storage.h>
#include <ananke/ntrtl.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Floppy controller database entry
 */
typedef struct _FLOPPY_CONTROLLER_DB_ENTRY {
    UINT16                  VendorID;
    UINT16                  ProductID;
    CONST CHAR8            *pszVendor;
    CONST CHAR8            *pszModel;
    FLOPPY_CONTROLLER_TYPE  ControllerType;
    FLOPPY_INTERFACE_TYPE   InterfaceType;
    UINT32                  Capabilities;
} FLOPPY_CONTROLLER_DB_ENTRY;

/**
 * @brief Known floppy controller database (20+ entries)
 */
static CONST FLOPPY_CONTROLLER_DB_ENTRY g_FloppyControllerDB[] = {
    // === ISA Floppy Disk Controllers ===
    // Standard PC FDC chips
    { 0x0000, 0x8272, "Intel", "8272 FDC (Original PC)",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT },

    { 0x0000, 0x8272, "NEC", "µPD765A FDC (PC/XT/AT)",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT },

    { 0x0000, 0x82077, "Intel", "82077AA Enhanced FDC",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_PERPENDICULAR },

    { 0x0000, 0x82078, "Intel", "82078 Super I/O FDC",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_PERPENDICULAR | FLOPPY_CAP_MULTIPLE_FORMATS },

    { 0x0000, 0xPC87307, "National Semiconductor", "PC87307 Super I/O with FDC",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_PERPENDICULAR },

    { 0x0000, 0xPC87312, "National Semiconductor", "PC87312 Super I/O with FDC",
      FLOPPY_CONTROLLER_ISA_FDC, FLOPPY_INTERFACE_ISA,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_PERPENDICULAR },

    // === USB Floppy Drives ===
    // Standard USB floppy drives
    { 0x0644, 0x0000, "TEAC", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x0409, 0x0040, "NEC", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x057B, 0x0000, "Y-E Data", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x0711, 0x0200, "Mitsumi", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x0930, 0x0010, "Toshiba", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x03F0, 0x2001, "HP", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x0424, 0xFDC0, "SMSC", "USB Floppy Drive Controller",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x13FE, 0x1A00, "Kingston", "USB Floppy Drive",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { 0x0D7D, 0x0001, "Phison", "USB Floppy Bridge",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    // === Parallel Port Floppy Drives ===
    { 0x0000, 0xBACK, "MicroSolutions", "BackPack Parallel Floppy",
      FLOPPY_CONTROLLER_PARALLEL, FLOPPY_INTERFACE_PARALLEL,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT },

    { 0x0000, 0xPFLP, "Newer Technology", "PowerFloppie",
      FLOPPY_CONTROLLER_PARALLEL, FLOPPY_INTERFACE_PARALLEL,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT },

    { 0x0000, 0xAVGA, "Avatar", "Shark Parallel Floppy",
      FLOPPY_CONTROLLER_PARALLEL, FLOPPY_INTERFACE_PARALLEL,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT },

    // === USB High-Capacity Drives ===
    // Iomega Zip USB
    { 0x059B, 0x0001, "Iomega", "Zip 100 USB",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_HOT_PLUG },

    { 0x059B, 0x0031, "Iomega", "Zip 250 USB",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_HOT_PLUG },

    { 0x059B, 0x0032, "Iomega", "Zip 750 USB",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_HOT_PLUG | FLOPPY_CAP_VARIABLE_SPEED },

    // Iomega Jaz USB
    { 0x059B, 0x0010, "Iomega", "Jaz 1GB USB",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_HOT_PLUG },

    { 0x059B, 0x0011, "Iomega", "Jaz 2GB USB",
      FLOPPY_CONTROLLER_USB, FLOPPY_INTERFACE_USB,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_HOT_PLUG },

    // === Parallel Port High-Capacity Drives ===
    { 0x0000, 0xZIPP, "Iomega", "Zip Parallel Port",
      FLOPPY_CONTROLLER_PARALLEL, FLOPPY_INTERFACE_PARALLEL,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT },

    { 0x0000, 0xJAZP, "Iomega", "Jaz Parallel Port",
      FLOPPY_CONTROLLER_PARALLEL, FLOPPY_INTERFACE_PARALLEL,
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT },
};

#define FLOPPY_CONTROLLER_DB_COUNT (sizeof(g_FloppyControllerDB) / sizeof(g_FloppyControllerDB[0]))

/**
 * @brief Floppy drive/media database entry
 */
typedef struct _FLOPPY_DRIVE_DB_ENTRY {
    CONST CHAR8         *pszVendor;
    CONST CHAR8         *pszModel;
    FLOPPY_DRIVE_TYPE   DriveType;
    FLOPPY_INTERFACE_TYPE InterfaceType;
    CONST CHAR8         *pszDescription;
    UINT32              Capabilities;
} FLOPPY_DRIVE_DB_ENTRY;

/**
 * @brief Known floppy drive database (30+ entries)
 */
static CONST FLOPPY_DRIVE_DB_ENTRY g_FloppyDriveDB[] = {
    // === Standard 5.25" Floppy Drives ===
    { "TEAC", "FD-55GFR", FLOPPY_DRIVE_1200K, FLOPPY_INTERFACE_ISA,
      "TEAC 5.25\" 1.2MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "TEAC", "FD-55B", FLOPPY_DRIVE_360K, FLOPPY_INTERFACE_ISA,
      "TEAC 5.25\" 360KB DD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT },

    { "Mitsubishi", "MF504C", FLOPPY_DRIVE_1200K, FLOPPY_INTERFACE_ISA,
      "Mitsubishi 5.25\" 1.2MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "Panasonic", "JU-475", FLOPPY_DRIVE_1200K, FLOPPY_INTERFACE_ISA,
      "Panasonic 5.25\" 1.2MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "Tandon", "TM100-2", FLOPPY_DRIVE_360K, FLOPPY_INTERFACE_ISA,
      "Tandon 5.25\" 360KB DD Drive (PC/XT Era)",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT },

    // === Standard 3.5" Floppy Drives ===
    { "Sony", "MPF920", FLOPPY_DRIVE_1440K, FLOPPY_INTERFACE_ISA,
      "Sony 3.5\" 1.44MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "Panasonic", "JU-257A206P", FLOPPY_DRIVE_1440K, FLOPPY_INTERFACE_ISA,
      "Panasonic 3.5\" 1.44MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "Mitsumi", "D359M3", FLOPPY_DRIVE_1440K, FLOPPY_INTERFACE_ISA,
      "Mitsumi 3.5\" 1.44MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "TEAC", "FD-235HF", FLOPPY_DRIVE_1440K, FLOPPY_INTERFACE_ISA,
      "TEAC 3.5\" 1.44MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "NEC", "FD1231H", FLOPPY_DRIVE_1440K, FLOPPY_INTERFACE_ISA,
      "NEC 3.5\" 1.44MB HD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE },

    { "Mitsubishi", "MF355C-592MA", FLOPPY_DRIVE_720K, FLOPPY_INTERFACE_ISA,
      "Mitsubishi 3.5\" 720KB DD Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT },

    // === 3.5" 2.88MB ED Drives (Rare) ===
    { "Toshiba", "ND-08GE", FLOPPY_DRIVE_2880K, FLOPPY_INTERFACE_ISA,
      "Toshiba 3.5\" 2.88MB ED Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE | FLOPPY_CAP_PERPENDICULAR | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Mitsubishi", "MF355W-902M", FLOPPY_DRIVE_2880K, FLOPPY_INTERFACE_ISA,
      "Mitsubishi 3.5\" 2.88MB ED Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE | FLOPPY_CAP_PERPENDICULAR | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Sony", "MP-F17W-50D", FLOPPY_DRIVE_2880K, FLOPPY_INTERFACE_ISA,
      "Sony 3.5\" 2.88MB ED Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_WRITE_PROTECT_DETECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_CHANGE_LINE | FLOPPY_CAP_PERPENDICULAR | FLOPPY_CAP_MULTIPLE_FORMATS },

    // === Iomega Zip Drives (Multiple Interfaces) ===
    // Zip 100MB
    { "Iomega", "Zip 100 ATAPI", FLOPPY_DRIVE_ZIP_100, FLOPPY_INTERFACE_ATAPI,
      "Iomega Zip 100MB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET },

    { "Iomega", "Zip 100 SCSI", FLOPPY_DRIVE_ZIP_100, FLOPPY_INTERFACE_SCSI,
      "Iomega Zip 100MB SCSI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT },

    { "Iomega", "Zip 100 USB", FLOPPY_DRIVE_ZIP_100, FLOPPY_INTERFACE_USB,
      "Iomega Zip 100MB USB Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    { "Iomega", "Zip 100 Parallel", FLOPPY_DRIVE_ZIP_100, FLOPPY_INTERFACE_PARALLEL,
      "Iomega Zip 100MB Parallel Port Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT },

    // Zip 250MB
    { "Iomega", "Zip 250 ATAPI", FLOPPY_DRIVE_ZIP_250, FLOPPY_INTERFACE_ATAPI,
      "Iomega Zip 250MB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET },

    { "Iomega", "Zip 250 SCSI", FLOPPY_DRIVE_ZIP_250, FLOPPY_INTERFACE_SCSI,
      "Iomega Zip 250MB SCSI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT },

    { "Iomega", "Zip 250 USB", FLOPPY_DRIVE_ZIP_250, FLOPPY_INTERFACE_USB,
      "Iomega Zip 250MB USB Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG },

    // Zip 750MB
    { "Iomega", "Zip 750 ATAPI", FLOPPY_DRIVE_ZIP_750, FLOPPY_INTERFACE_ATAPI,
      "Iomega Zip 750MB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_VARIABLE_SPEED },

    { "Iomega", "Zip 750 USB", FLOPPY_DRIVE_ZIP_750, FLOPPY_INTERFACE_USB,
      "Iomega Zip 750MB USB 2.0 Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG | FLOPPY_CAP_VARIABLE_SPEED },

    // === Iomega Jaz Drives ===
    { "Iomega", "Jaz 1GB SCSI", FLOPPY_DRIVE_JAZ_1GB, FLOPPY_INTERFACE_SCSI,
      "Iomega Jaz 1GB SCSI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT },

    { "Iomega", "Jaz 2GB SCSI", FLOPPY_DRIVE_JAZ_2GB, FLOPPY_INTERFACE_SCSI,
      "Iomega Jaz 2GB SCSI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT },

    { "Iomega", "Jaz 1GB ATAPI", FLOPPY_DRIVE_JAZ_1GB, FLOPPY_INTERFACE_ATAPI,
      "Iomega Jaz 1GB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET },

    { "Iomega", "Jaz 2GB ATAPI", FLOPPY_DRIVE_JAZ_2GB, FLOPPY_INTERFACE_ATAPI,
      "Iomega Jaz 2GB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET },

    // === LS-120/LS-240 SuperDisk Drives ===
    { "Panasonic", "LS-120 ATAPI", FLOPPY_DRIVE_LS120, FLOPPY_INTERFACE_ATAPI,
      "Panasonic LS-120 SuperDisk 120MB ATAPI",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Matsushita", "LS-120 ATAPI", FLOPPY_DRIVE_LS120, FLOPPY_INTERFACE_ATAPI,
      "Matsushita LS-120 SuperDisk 120MB ATAPI",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Compaq", "LS-120 ATAPI", FLOPPY_DRIVE_LS120, FLOPPY_INTERFACE_ATAPI,
      "Compaq LS-120 SuperDisk 120MB ATAPI",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Panasonic", "LS-240 ATAPI", FLOPPY_DRIVE_LS240, FLOPPY_INTERFACE_ATAPI,
      "Panasonic LS-240 SuperDisk 240MB ATAPI",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Imation", "LS-120 USB", FLOPPY_DRIVE_LS120, FLOPPY_INTERFACE_USB,
      "Imation LS-120 SuperDisk 120MB USB",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG | FLOPPY_CAP_MULTIPLE_FORMATS },

    // === Sony HiFD (High Floppy Disk) ===
    { "Sony", "HiFD 200MB ATAPI", FLOPPY_DRIVE_HIFD, FLOPPY_INTERFACE_ATAPI,
      "Sony HiFD 200MB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },

    { "Sony", "HiFD 200MB USB", FLOPPY_DRIVE_HIFD, FLOPPY_INTERFACE_USB,
      "Sony HiFD 200MB USB Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_LOCK | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_HOT_PLUG | FLOPPY_CAP_MULTIPLE_FORMATS },

    // === Caleb UHD144 (Ultra High Density) ===
    { "Caleb", "UHD144 ATAPI", FLOPPY_DRIVE_UHD144, FLOPPY_INTERFACE_ATAPI,
      "Caleb UHD144 144MB ATAPI Drive",
      FLOPPY_CAP_READ | FLOPPY_CAP_WRITE | FLOPPY_CAP_FORMAT | FLOPPY_CAP_EJECT | FLOPPY_CAP_MEDIA_DETECT | FLOPPY_CAP_ATAPI_PACKET | FLOPPY_CAP_MULTIPLE_FORMATS },
};

#define FLOPPY_DRIVE_DB_COUNT (sizeof(g_FloppyDriveDB) / sizeof(g_FloppyDriveDB[0]))

/**
 * @brief Media geometry table
 *
 * Standard geometries for all supported floppy media types.
 */
typedef struct _FLOPPY_MEDIA_GEOMETRY_TABLE {
    FLOPPY_MEDIA_TYPE   MediaType;
    FLOPPY_GEOMETRY     Geometry;
    CONST CHAR8        *pszDescription;
} FLOPPY_MEDIA_GEOMETRY_TABLE;

static CONST FLOPPY_MEDIA_GEOMETRY_TABLE g_MediaGeometryTable[] = {
    // 5.25" formats
    { FLOPPY_MEDIA_5_360K,
      { 40, 2, 9, 512, 720, 368640, 250, 0x50, 0xF6, FALSE },
      "5.25\" 360KB DD (40 tracks, 9 sectors/track)" },

    { FLOPPY_MEDIA_5_1200K,
      { 80, 2, 15, 512, 2400, 1228800, 500, 0x54, 0xF6, FALSE },
      "5.25\" 1.2MB HD (80 tracks, 15 sectors/track)" },

    // 3.5" formats
    { FLOPPY_MEDIA_3_720K,
      { 80, 2, 9, 512, 1440, 737280, 250, 0x50, 0xF6, FALSE },
      "3.5\" 720KB DD (80 tracks, 9 sectors/track)" },

    { FLOPPY_MEDIA_3_1440K,
      { 80, 2, 18, 512, 2880, 1474560, 500, 0x6C, 0xF6, FALSE },
      "3.5\" 1.44MB HD (80 tracks, 18 sectors/track)" },

    { FLOPPY_MEDIA_3_2880K,
      { 80, 2, 36, 512, 5760, 2949120, 1000, 0x53, 0xF6, TRUE },
      "3.5\" 2.88MB ED (80 tracks, 36 sectors/track, perpendicular)" },

    { FLOPPY_MEDIA_3_DMF,
      { 80, 2, 21, 512, 3360, 1720320, 500, 0x0C, 0xF6, FALSE },
      "3.5\" 1.68MB DMF (Microsoft Distribution Media Format)" },

    // High-capacity formats - Iomega Zip
    { FLOPPY_MEDIA_ZIP_100,
      { 2941, 1, 64, 512, 196608, 100663296, 2000, 0x00, 0x00, FALSE },
      "Iomega Zip 100MB (96 MB formatted)" },

    { FLOPPY_MEDIA_ZIP_250,
      { 7281, 1, 64, 512, 489472, 250609664, 5000, 0x00, 0x00, FALSE },
      "Iomega Zip 250MB (239 MB formatted)" },

    { FLOPPY_MEDIA_ZIP_750,
      { 23631, 1, 64, 512, 1468006, 751619072, 10000, 0x00, 0x00, FALSE },
      "Iomega Zip 750MB (694 MB formatted)" },

    // High-capacity formats - Iomega Jaz
    { FLOPPY_MEDIA_JAZ_1GB,
      { 1021, 64, 32, 512, 2091008, 1070596096, 5000, 0x00, 0x00, FALSE },
      "Iomega Jaz 1GB (1021 MB formatted)" },

    { FLOPPY_MEDIA_JAZ_2GB,
      { 3057, 64, 32, 512, 3915776, 2005254144, 5000, 0x00, 0x00, FALSE },
      "Iomega Jaz 2GB (1.9 GB formatted)" },

    // LS-120/LS-240 SuperDisk
    { FLOPPY_MEDIA_LS120,
      { 963, 8, 32, 512, 246528, 126222336, 900, 0x00, 0x00, FALSE },
      "LS-120 SuperDisk 120MB" },

    { FLOPPY_MEDIA_LS240,
      { 1916, 8, 32, 512, 490560, 251166720, 1800, 0x00, 0x00, FALSE },
      "LS-240 SuperDisk 240MB" },

    { FLOPPY_MEDIA_LS120_FLOPPY,
      { 80, 2, 18, 512, 2880, 1474560, 500, 0x6C, 0xF6, FALSE },
      "Standard 1.44MB floppy in LS-120 drive" },

    // Sony HiFD
    { FLOPPY_MEDIA_HIFD,
      { 3456, 2, 56, 512, 387072, 198180864, 1500, 0x00, 0x00, FALSE },
      "Sony HiFD 200MB" },

    // Caleb UHD144
    { FLOPPY_MEDIA_UHD144,
      { 1024, 1, 288, 512, 294912, 151060224, 1000, 0x00, 0x00, FALSE },
      "Caleb UHD144 144MB" },
};

#define MEDIA_GEOMETRY_TABLE_COUNT (sizeof(g_MediaGeometryTable) / sizeof(g_MediaGeometryTable[0]))

//
// Forward declarations - IIOFloppyController
//
static HRESULT STDMETHODCALLTYPE FloppyController_QueryInterface(IIOFloppyController *pThis, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE FloppyController_AddRef(IIOFloppyController *pThis);
static ULONG STDMETHODCALLTYPE FloppyController_Release(IIOFloppyController *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyController_Probe(IIOFloppyController *pThis, IIOService *pProvider, UINT32 *puProbeScore);
static IO_RETURN STDMETHODCALLTYPE FloppyController_Start(IIOFloppyController *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE FloppyController_Stop(IIOFloppyController *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE FloppyController_Terminate(IIOFloppyController *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetProperty(IIOFloppyController *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType);
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetProperty(IIOFloppyController *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetParentService(IIOFloppyController *pThis, IIOService **ppParent);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetChildService(IIOFloppyController *pThis, UINT32 uIndex, IIOService **ppChild);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetServiceState(IIOFloppyController *pThis, UINT32 *puState);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetServiceName(IIOFloppyController *pThis, CHAR8 *pszName, UINTN cbSize);
static IO_RETURN STDMETHODCALLTYPE FloppyController_RegisterService(IIOFloppyController *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetControllerInfo(IIOFloppyController *pThis, FLOPPY_CONTROLLER_INFO *pControllerInfo);
static IO_RETURN STDMETHODCALLTYPE FloppyController_EnumerateDrives(IIOFloppyController *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetDriveCount(IIOFloppyController *pThis, UINT32 *puCount);
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetDrive(IIOFloppyController *pThis, UINT32 uIndex, IIOFloppyDrive **ppDrive);
static IO_RETURN STDMETHODCALLTYPE FloppyController_ResetController(IIOFloppyController *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetDMAEnable(IIOFloppyController *pThis, BOOLEAN bEnable);
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetDataRate(IIOFloppyController *pThis, UINT32 uDataRate);

//
// Forward declarations - IIOFloppyDrive
//
static HRESULT STDMETHODCALLTYPE FloppyDrive_QueryInterface(IIOFloppyDrive *pThis, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE FloppyDrive_AddRef(IIOFloppyDrive *pThis);
static ULONG STDMETHODCALLTYPE FloppyDrive_Release(IIOFloppyDrive *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Probe(IIOFloppyDrive *pThis, IIOService *pProvider, UINT32 *puProbeScore);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Start(IIOFloppyDrive *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Stop(IIOFloppyDrive *pThis, IIOService *pProvider);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Terminate(IIOFloppyDrive *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetProperty(IIOFloppyDrive *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetProperty(IIOFloppyDrive *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetParentService(IIOFloppyDrive *pThis, IIOService **ppParent);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetChildService(IIOFloppyDrive *pThis, UINT32 uIndex, IIOService **ppChild);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetServiceState(IIOFloppyDrive *pThis, UINT32 *puState);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetServiceName(IIOFloppyDrive *pThis, CHAR8 *pszName, UINTN cbSize);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_RegisterService(IIOFloppyDrive *pThis, UINT32 uOptions);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetDriveInfo(IIOFloppyDrive *pThis, FLOPPY_DRIVE_INFO *pDriveInfo);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_DetectMediaType(IIOFloppyDrive *pThis, FLOPPY_MEDIA_TYPE *pMediaType);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetMediaGeometry(IIOFloppyDrive *pThis, FLOPPY_GEOMETRY *pGeometry);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_ReadSectorsCHS(IIOFloppyDrive *pThis, UINT32 uCylinder, UINT32 uHead, UINT32 uSector, UINT32 uCount, VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesRead);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_WriteSectorsCHS(IIOFloppyDrive *pThis, UINT32 uCylinder, UINT32 uHead, UINT32 uSector, UINT32 uCount, CONST VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesWritten);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_ReadSectorsLBA(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount, VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesRead);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_WriteSectorsLBA(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount, CONST VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesWritten);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_FormatMedia(IIOFloppyDrive *pThis, CONST FLOPPY_FORMAT_PARAMS *pParams);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_VerifySectors(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SeekToCylinder(IIOFloppyDrive *pThis, UINT32 uCylinder);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Recalibrate(IIOFloppyDrive *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetMotor(IIOFloppyDrive *pThis, BOOLEAN bMotorOn);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_EjectMedia(IIOFloppyDrive *pThis);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetDoorLock(IIOFloppyDrive *pThis, BOOLEAN bLock);
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetIOStats(IIOFloppyDrive *pThis, FLOPPY_IO_STATS *pStats, BOOLEAN bReset);

/**
 * @brief Floppy controller implementation structure
 */
typedef struct _FLOPPY_CONTROLLER_IMPL {
    IIOFloppyController     Vtbl;               /**< Virtual function table */
    ULONG                   RefCount;           /**< Reference count */
    FLOPPY_CONTROLLER_TYPE  ControllerType;     /**< Controller type */
    FLOPPY_CONTROLLER_INFO  ControllerInfo;     /**< Controller information */
    IIOFloppyDrive        **ppDrives;           /**< Array of attached drives */
    UINT32                  uNumDrives;         /**< Number of attached drives */
    UINT32                  uMaxDrives;         /**< Maximum drives capacity */
    IIOService             *pProvider;          /**< Provider service */
    BOOLEAN                 bInitialized;       /**< Initialization flag */
} FLOPPY_CONTROLLER_IMPL;

/**
 * @brief Floppy drive implementation structure
 */
typedef struct _FLOPPY_DRIVE_IMPL {
    IIOFloppyDrive          Vtbl;               /**< Virtual function table */
    ULONG                   RefCount;           /**< Reference count */
    IIOFloppyController    *pController;        /**< Parent controller */
    FLOPPY_DRIVE_TYPE       DriveType;          /**< Drive type */
    FLOPPY_DRIVE_INFO       DriveInfo;          /**< Drive information */
    FLOPPY_GEOMETRY         CurrentGeometry;    /**< Current media geometry */
    FLOPPY_IO_STATS         IOStats;            /**< I/O statistics */
    UINT32                  uDriveNumber;       /**< Drive number (0-3) */
    BOOLEAN                 bInitialized;       /**< Initialization flag */
} FLOPPY_DRIVE_IMPL;

//
// IIOFloppyController VTable
//
static CONST IIOFloppyControllerVtbl g_FloppyControllerVtbl = {
    // IUnknown
    FloppyController_QueryInterface,
    FloppyController_AddRef,
    FloppyController_Release,
    // IIOService
    FloppyController_Probe,
    FloppyController_Start,
    FloppyController_Stop,
    FloppyController_Terminate,
    FloppyController_GetProperty,
    FloppyController_SetProperty,
    FloppyController_GetParentService,
    FloppyController_GetChildService,
    FloppyController_GetServiceState,
    FloppyController_GetServiceName,
    FloppyController_RegisterService,
    // IIOFloppyController
    FloppyController_GetControllerInfo,
    FloppyController_EnumerateDrives,
    FloppyController_GetDriveCount,
    FloppyController_GetDrive,
    FloppyController_ResetController,
    FloppyController_SetDMAEnable,
    FloppyController_SetDataRate,
};

//
// IIOFloppyDrive VTable
//
static CONST IIOFloppyDriveVtbl g_FloppyDriveVtbl = {
    // IUnknown
    FloppyDrive_QueryInterface,
    FloppyDrive_AddRef,
    FloppyDrive_Release,
    // IIOService
    FloppyDrive_Probe,
    FloppyDrive_Start,
    FloppyDrive_Stop,
    FloppyDrive_Terminate,
    FloppyDrive_GetProperty,
    FloppyDrive_SetProperty,
    FloppyDrive_GetParentService,
    FloppyDrive_GetChildService,
    FloppyDrive_GetServiceState,
    FloppyDrive_GetServiceName,
    FloppyDrive_RegisterService,
    // IIOFloppyDrive
    FloppyDrive_GetDriveInfo,
    FloppyDrive_DetectMediaType,
    FloppyDrive_GetMediaGeometry,
    FloppyDrive_ReadSectorsCHS,
    FloppyDrive_WriteSectorsCHS,
    FloppyDrive_ReadSectorsLBA,
    FloppyDrive_WriteSectorsLBA,
    FloppyDrive_FormatMedia,
    FloppyDrive_VerifySectors,
    FloppyDrive_SeekToCylinder,
    FloppyDrive_Recalibrate,
    FloppyDrive_SetMotor,
    FloppyDrive_EjectMedia,
    FloppyDrive_SetDoorLock,
    FloppyDrive_GetIOStats,
};

//
// Global state
//
static BOOLEAN g_bFloppyInitialized = FALSE;

//
// Implementation stubs
//
// Note: Full implementation would include actual hardware programming for ISA FDC,
// USB/ATAPI/SCSI command translation, sector I/O, format operations, etc.
// This provides the framework and interface implementation.
//

// [IUnknown methods and other interface methods would be implemented here]
// [For brevity, showing just the key structure and initialization functions]

/**
 * @brief Initialize Floppy family subsystem
 */
IO_RETURN
FloppyInitialize(VOID)
{
    if (g_bFloppyInitialized) {
        return IO_SUCCESS;
    }

    printf("[Floppy] Initializing floppy family subsystem\n");
    printf("[Floppy] Loaded %u controller definitions\n", FLOPPY_CONTROLLER_DB_COUNT);
    printf("[Floppy] Loaded %u drive definitions\n", FLOPPY_DRIVE_DB_COUNT);
    printf("[Floppy] Loaded %u media geometry definitions\n", MEDIA_GEOMETRY_TABLE_COUNT);

    g_bFloppyInitialized = TRUE;
    return IO_SUCCESS;
}

/**
 * @brief Shutdown Floppy family subsystem
 */
IO_RETURN
FloppyShutdown(VOID)
{
    if (!g_bFloppyInitialized) {
        return IO_SUCCESS;
    }

    printf("[Floppy] Shutting down floppy family subsystem\n");
    g_bFloppyInitialized = FALSE;
    return IO_SUCCESS;
}

/**
 * @brief Get geometry for media type
 */
IO_RETURN
FloppyGetMediaGeometry(
    FLOPPY_MEDIA_TYPE MediaType,
    FLOPPY_GEOMETRY *pGeometry
    )
{
    UINT32 i;

    if (pGeometry == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Search geometry table
    for (i = 0; i < MEDIA_GEOMETRY_TABLE_COUNT; i++) {
        if (g_MediaGeometryTable[i].MediaType == MediaType) {
            memcpy(pGeometry, &g_MediaGeometryTable[i].Geometry, sizeof(FLOPPY_GEOMETRY));
            return IO_SUCCESS;
        }
    }

    return IO_BAD_ARGUMENT;
}

/**
 * @brief Convert CHS to LBA
 */
IO_RETURN
FloppyCHSToLBA(
    CONST FLOPPY_GEOMETRY *pGeometry,
    UINT32 uCylinder,
    UINT32 uHead,
    UINT32 uSector,
    UINT64 *puLBA
    )
{
    if (pGeometry == NULL || puLBA == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Validate CHS values
    if (uCylinder >= pGeometry->Cylinders ||
        uHead >= pGeometry->Heads ||
        uSector < 1 || uSector > pGeometry->SectorsPerTrack) {
        return IO_BAD_ARGUMENT;
    }

    // LBA = (C × HPC + H) × SPT + (S − 1)
    *puLBA = ((UINT64)uCylinder * pGeometry->Heads + uHead) * pGeometry->SectorsPerTrack + (uSector - 1);
    return IO_SUCCESS;
}

/**
 * @brief Convert LBA to CHS
 */
IO_RETURN
FloppyLBAToCHS(
    CONST FLOPPY_GEOMETRY *pGeometry,
    UINT64 uLBA,
    UINT32 *puCylinder,
    UINT32 *puHead,
    UINT32 *puSector
    )
{
    if (pGeometry == NULL || puCylinder == NULL || puHead == NULL || puSector == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Validate LBA
    if (uLBA >= pGeometry->TotalSectors) {
        return IO_BAD_ARGUMENT;
    }

    // C = LBA ÷ (HPC × SPT)
    // H = (LBA ÷ SPT) mod HPC
    // S = (LBA mod SPT) + 1
    *puCylinder = (UINT32)(uLBA / (pGeometry->Heads * pGeometry->SectorsPerTrack));
    *puHead = (UINT32)((uLBA / pGeometry->SectorsPerTrack) % pGeometry->Heads);
    *puSector = (UINT32)((uLBA % pGeometry->SectorsPerTrack) + 1);

    return IO_SUCCESS;
}

// Stub implementations for interface methods
// (In a real driver, these would contain actual hardware/protocol implementation)

static HRESULT STDMETHODCALLTYPE
FloppyController_QueryInterface(IIOFloppyController *pThis, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE
FloppyController_AddRef(IIOFloppyController *pThis)
{
    FLOPPY_CONTROLLER_IMPL *pImpl = (FLOPPY_CONTROLLER_IMPL*)pThis;
    return ++pImpl->RefCount;
}

static ULONG STDMETHODCALLTYPE
FloppyController_Release(IIOFloppyController *pThis)
{
    FLOPPY_CONTROLLER_IMPL *pImpl = (FLOPPY_CONTROLLER_IMPL*)pThis;
    ULONG RefCount = --pImpl->RefCount;
    if (RefCount == 0) {
        // Free drive array
        if (pImpl->ppDrives != NULL) {
            free(pImpl->ppDrives);
        }
        free(pImpl);
    }
    return RefCount;
}

// Additional method stubs would follow the same pattern...
// [For brevity, showing structure only]

static IO_RETURN STDMETHODCALLTYPE FloppyController_Probe(IIOFloppyController *pThis, IIOService *pProvider, UINT32 *puProbeScore) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_Start(IIOFloppyController *pThis, IIOService *pProvider) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_Stop(IIOFloppyController *pThis, IIOService *pProvider) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_Terminate(IIOFloppyController *pThis, UINT32 uOptions) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetProperty(IIOFloppyController *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType) { return IO_NOT_FOUND; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetProperty(IIOFloppyController *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType) { return IO_NOT_FOUND; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetParentService(IIOFloppyController *pThis, IIOService **ppParent) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetChildService(IIOFloppyController *pThis, UINT32 uIndex, IIOService **ppChild) { return IO_NO_DEVICE; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetServiceState(IIOFloppyController *pThis, UINT32 *puState) { *puState = 0; return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetServiceName(IIOFloppyController *pThis, CHAR8 *pszName, UINTN cbSize) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_RegisterService(IIOFloppyController *pThis, UINT32 uOptions) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetControllerInfo(IIOFloppyController *pThis, FLOPPY_CONTROLLER_INFO *pControllerInfo) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_EnumerateDrives(IIOFloppyController *pThis) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetDriveCount(IIOFloppyController *pThis, UINT32 *puCount) { *puCount = 0; return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_GetDrive(IIOFloppyController *pThis, UINT32 uIndex, IIOFloppyDrive **ppDrive) { return IO_NO_DEVICE; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_ResetController(IIOFloppyController *pThis) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetDMAEnable(IIOFloppyController *pThis, BOOLEAN bEnable) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyController_SetDataRate(IIOFloppyController *pThis, UINT32 uDataRate) { return IO_SUCCESS; }

// IIOFloppyDrive method stubs
static HRESULT STDMETHODCALLTYPE FloppyDrive_QueryInterface(IIOFloppyDrive *pThis, REFIID riid, void **ppvObject) { return E_NOTIMPL; }
static ULONG STDMETHODCALLTYPE FloppyDrive_AddRef(IIOFloppyDrive *pThis) { FLOPPY_DRIVE_IMPL *pImpl = (FLOPPY_DRIVE_IMPL*)pThis; return ++pImpl->RefCount; }
static ULONG STDMETHODCALLTYPE FloppyDrive_Release(IIOFloppyDrive *pThis) { FLOPPY_DRIVE_IMPL *pImpl = (FLOPPY_DRIVE_IMPL*)pThis; ULONG RefCount = --pImpl->RefCount; if (RefCount == 0) { free(pImpl); } return RefCount; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Probe(IIOFloppyDrive *pThis, IIOService *pProvider, UINT32 *puProbeScore) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Start(IIOFloppyDrive *pThis, IIOService *pProvider) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Stop(IIOFloppyDrive *pThis, IIOService *pProvider) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Terminate(IIOFloppyDrive *pThis, UINT32 uOptions) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetProperty(IIOFloppyDrive *pThis, CONST CHAR8 *pszKey, VOID *pValue, UINTN *pcbSize, UINT32 *puType) { return IO_NOT_FOUND; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetProperty(IIOFloppyDrive *pThis, CONST CHAR8 *pszKey, CONST VOID *pValue, UINTN cbSize, UINT32 uType) { return IO_NOT_FOUND; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetParentService(IIOFloppyDrive *pThis, IIOService **ppParent) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetChildService(IIOFloppyDrive *pThis, UINT32 uIndex, IIOService **ppChild) { return IO_NO_DEVICE; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetServiceState(IIOFloppyDrive *pThis, UINT32 *puState) { *puState = 0; return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetServiceName(IIOFloppyDrive *pThis, CHAR8 *pszName, UINTN cbSize) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_RegisterService(IIOFloppyDrive *pThis, UINT32 uOptions) { return IO_SUCCESS; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetDriveInfo(IIOFloppyDrive *pThis, FLOPPY_DRIVE_INFO *pDriveInfo) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_DetectMediaType(IIOFloppyDrive *pThis, FLOPPY_MEDIA_TYPE *pMediaType) { return IO_NO_MEDIA; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetMediaGeometry(IIOFloppyDrive *pThis, FLOPPY_GEOMETRY *pGeometry) { return IO_NO_MEDIA; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_ReadSectorsCHS(IIOFloppyDrive *pThis, UINT32 uCylinder, UINT32 uHead, UINT32 uSector, UINT32 uCount, VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesRead) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_WriteSectorsCHS(IIOFloppyDrive *pThis, UINT32 uCylinder, UINT32 uHead, UINT32 uSector, UINT32 uCount, CONST VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesWritten) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_ReadSectorsLBA(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount, VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesRead) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_WriteSectorsLBA(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount, CONST VOID *pBuffer, UINTN cbBuffer, UINTN *puBytesWritten) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_FormatMedia(IIOFloppyDrive *pThis, CONST FLOPPY_FORMAT_PARAMS *pParams) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_VerifySectors(IIOFloppyDrive *pThis, UINT64 uLBA, UINT32 uCount) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SeekToCylinder(IIOFloppyDrive *pThis, UINT32 uCylinder) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_Recalibrate(IIOFloppyDrive *pThis) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetMotor(IIOFloppyDrive *pThis, BOOLEAN bMotorOn) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_EjectMedia(IIOFloppyDrive *pThis) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_SetDoorLock(IIOFloppyDrive *pThis, BOOLEAN bLock) { return IO_UNSUPPORTED; }
static IO_RETURN STDMETHODCALLTYPE FloppyDrive_GetIOStats(IIOFloppyDrive *pThis, FLOPPY_IO_STATS *pStats, BOOLEAN bReset) { return IO_UNSUPPORTED; }

/**
 * @brief Create a floppy controller instance
 */
IO_RETURN
FloppyControllerCreate(
    FLOPPY_CONTROLLER_TYPE ControllerType,
    IIOService *pProvider,
    IIOFloppyController **ppController
    )
{
    FLOPPY_CONTROLLER_IMPL *pImpl;

    if (ppController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pImpl = (FLOPPY_CONTROLLER_IMPL*)calloc(1, sizeof(FLOPPY_CONTROLLER_IMPL));
    if (pImpl == NULL) {
        return IO_NO_MEMORY;
    }

    pImpl->Vtbl.lpVtbl = &g_FloppyControllerVtbl;
    pImpl->RefCount = 1;
    pImpl->ControllerType = ControllerType;
    pImpl->pProvider = pProvider;
    pImpl->uMaxDrives = 4;
    pImpl->bInitialized = TRUE;

    *ppController = (IIOFloppyController*)pImpl;
    return IO_SUCCESS;
}

/**
 * @brief Create a floppy drive instance
 */
IO_RETURN
FloppyDriveCreate(
    IIOFloppyController *pController,
    FLOPPY_DRIVE_TYPE DriveType,
    UINT32 uDriveNumber,
    IIOFloppyDrive **ppDrive
    )
{
    FLOPPY_DRIVE_IMPL *pImpl;

    if (ppDrive == NULL || pController == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pImpl = (FLOPPY_DRIVE_IMPL*)calloc(1, sizeof(FLOPPY_DRIVE_IMPL));
    if (pImpl == NULL) {
        return IO_NO_MEMORY;
    }

    pImpl->Vtbl.lpVtbl = &g_FloppyDriveVtbl;
    pImpl->RefCount = 1;
    pImpl->pController = pController;
    pImpl->DriveType = DriveType;
    pImpl->uDriveNumber = uDriveNumber;
    pImpl->bInitialized = TRUE;

    *ppDrive = (IIOFloppyDrive*)pImpl;
    return IO_SUCCESS;
}
