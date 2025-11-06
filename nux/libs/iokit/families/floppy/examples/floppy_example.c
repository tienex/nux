/**
 * @file floppy_example.c
 * @brief Floppy Family Usage Examples
 *
 * This file demonstrates how to use the Floppy family driver for:
 * - Floppy controller enumeration and initialization
 * - Drive discovery and media detection
 * - Standard floppy disk operations (1.44MB, 720KB, etc.)
 * - High-capacity removable media (Zip, Jaz, LS-120)
 * - Read/write operations with CHS and LBA addressing
 * - Media formatting operations
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/floppy/floppy.h>
#include <iokit/families/storage/storage.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Example 1: Enumerate floppy controllers
 *
 * This example shows how to find and enumerate all floppy controllers in the system.
 */
void
Example1_EnumerateFloppyControllers(
    VOID
    )
{
    IO_RETURN Status;
    IIOFloppyController *pController = NULL;
    FLOPPY_CONTROLLER_INFO ControllerInfo;

    printf("\n=== Example 1: Enumerate Floppy Controllers ===\n");

    // Initialize Floppy family
    Status = FloppyInitialize();
    if (Status != IO_SUCCESS) {
        printf("Failed to initialize Floppy family: 0x%X\n", Status);
        return;
    }

    // In a real implementation, you would iterate through ISA/USB devices
    // and create controller instances for matching devices
    printf("Floppy controllers found in system:\n");
    printf("  - ISA Floppy Controller (Intel 82077AA Enhanced FDC)\n");
    printf("    I/O Base: 0x3F0, IRQ: 6, DMA: 2\n");
    printf("    Supports: 360KB, 720KB, 1.2MB, 1.44MB, 2.88MB\n");
    printf("  - USB Floppy Drive (TEAC USB)\n");
    printf("    Vendor:Product ID: 0644:0000\n");
    printf("    Supports: 720KB, 1.44MB\n");
    printf("  - Iomega Zip 250 USB\n");
    printf("    Vendor:Product ID: 059B:0031\n");
    printf("    Supports: Zip 100MB, Zip 250MB\n");

    // Example: Get controller information
    if (pController != NULL) {
        Status = IIOFloppyController_GetControllerInfo(pController, &ControllerInfo);
        if (Status == IO_SUCCESS) {
            printf("\nController Information:\n");
            printf("  Name: %s\n", ControllerInfo.ControllerName);
            printf("  Vendor: %s\n", ControllerInfo.VendorName);
            printf("  Type: %d\n", ControllerInfo.ControllerType);
            printf("  Interface: %d\n", ControllerInfo.InterfaceType);
            printf("  Max Drives: %u\n", ControllerInfo.MaxDrives);
            printf("  DMA Support: %s\n", ControllerInfo.bDMA ? "Yes" : "No");
            printf("  FIFO Support: %s\n", ControllerInfo.bFIFO ? "Yes" : "No");
        }
    }
}

/**
 * @brief Example 2: Detect media type and geometry
 *
 * This example demonstrates how to detect the type of media inserted in a drive.
 */
void
Example2_DetectMediaType(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    FLOPPY_MEDIA_TYPE MediaType;
    FLOPPY_GEOMETRY Geometry;
    FLOPPY_DRIVE_INFO DriveInfo;

    printf("\n=== Example 2: Detect Media Type ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    // Get drive information
    Status = IIOFloppyDrive_GetDriveInfo(pDrive, &DriveInfo);
    if (Status == IO_SUCCESS) {
        printf("Drive: %s %s\n", DriveInfo.VendorName, DriveInfo.ModelName);
        printf("Drive Type: %d\n", DriveInfo.DriveType);
        printf("Media Present: %s\n", DriveInfo.bMediaPresent ? "Yes" : "No");
    }

    // Detect media type
    Status = IIOFloppyDrive_DetectMediaType(pDrive, &MediaType);
    if (Status == IO_SUCCESS) {
        printf("\nMedia Type Detected: ");
        switch (MediaType) {
            case FLOPPY_MEDIA_3_720K:
                printf("3.5\" 720KB DD\n");
                break;
            case FLOPPY_MEDIA_3_1440K:
                printf("3.5\" 1.44MB HD\n");
                break;
            case FLOPPY_MEDIA_3_2880K:
                printf("3.5\" 2.88MB ED\n");
                break;
            case FLOPPY_MEDIA_5_360K:
                printf("5.25\" 360KB DD\n");
                break;
            case FLOPPY_MEDIA_5_1200K:
                printf("5.25\" 1.2MB HD\n");
                break;
            case FLOPPY_MEDIA_ZIP_100:
                printf("Iomega Zip 100MB\n");
                break;
            case FLOPPY_MEDIA_ZIP_250:
                printf("Iomega Zip 250MB\n");
                break;
            case FLOPPY_MEDIA_JAZ_1GB:
                printf("Iomega Jaz 1GB\n");
                break;
            case FLOPPY_MEDIA_LS120:
                printf("LS-120 SuperDisk 120MB\n");
                break;
            default:
                printf("Unknown\n");
                break;
        }

        // Get media geometry
        Status = IIOFloppyDrive_GetMediaGeometry(pDrive, &Geometry);
        if (Status == IO_SUCCESS) {
            printf("\nMedia Geometry:\n");
            printf("  Cylinders: %u\n", Geometry.Cylinders);
            printf("  Heads: %u\n", Geometry.Heads);
            printf("  Sectors/Track: %u\n", Geometry.SectorsPerTrack);
            printf("  Bytes/Sector: %u\n", Geometry.BytesPerSector);
            printf("  Total Sectors: %llu\n", Geometry.TotalSectors);
            printf("  Total Capacity: %llu bytes (%.2f MB)\n",
                   Geometry.TotalBytes,
                   (double)Geometry.TotalBytes / (1024.0 * 1024.0));
            printf("  Data Rate: %u Kbps\n", Geometry.DataRate);
        }
    } else if (Status == IO_NO_MEDIA) {
        printf("No media present in drive\n");
    } else {
        printf("Failed to detect media type: 0x%X\n", Status);
    }
}

/**
 * @brief Example 3: Read sectors using CHS addressing
 *
 * This example demonstrates reading sectors using Cylinder/Head/Sector addressing.
 */
void
Example3_ReadSectorsCHS(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    UINT8 Buffer[512];  // One sector
    UINTN BytesRead;
    UINT32 Cylinder = 0;
    UINT32 Head = 0;
    UINT32 Sector = 1;  // Sectors are 1-based

    printf("\n=== Example 3: Read Sectors (CHS) ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    printf("Reading C:H:S = %u:%u:%u (boot sector)...\n", Cylinder, Head, Sector);

    Status = IIOFloppyDrive_ReadSectorsCHS(pDrive, Cylinder, Head, Sector, 1,
                                           Buffer, sizeof(Buffer), &BytesRead);

    if (Status == IO_SUCCESS) {
        printf("Read successful! Bytes read: %zu\n", BytesRead);
        printf("Boot sector signature: 0x%02X%02X\n", Buffer[511], Buffer[510]);

        if (Buffer[510] == 0x55 && Buffer[511] == 0xAA) {
            printf("Valid boot sector detected!\n");
        }

        // Display first 64 bytes
        printf("\nFirst 64 bytes:\n");
        for (UINT32 i = 0; i < 64; i++) {
            printf("%02X ", Buffer[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    } else {
        printf("Read failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 4: Read sectors using LBA addressing
 *
 * This example demonstrates reading sectors using Logical Block Addressing.
 */
void
Example4_ReadSectorsLBA(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    UINT8 Buffer[4096];  // 8 sectors (4KB)
    UINTN BytesRead;
    UINT64 LBA = 0;      // Boot sector
    UINT32 SectorCount = 8;

    printf("\n=== Example 4: Read Sectors (LBA) ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    printf("Reading %u sectors starting at LBA %llu...\n", SectorCount, LBA);

    Status = IIOFloppyDrive_ReadSectorsLBA(pDrive, LBA, SectorCount,
                                           Buffer, sizeof(Buffer), &BytesRead);

    if (Status == IO_SUCCESS) {
        printf("Read successful! Bytes read: %zu\n", BytesRead);

        // Check for FAT filesystem
        if (Buffer[510] == 0x55 && Buffer[511] == 0xAA) {
            printf("Boot sector found\n");

            // Display OEM name (offset 3, 8 bytes)
            char OEMName[9];
            memcpy(OEMName, &Buffer[3], 8);
            OEMName[8] = '\0';
            printf("OEM Name: %s\n", OEMName);

            // Display filesystem label (FAT12/16: offset 43, FAT32: offset 71)
            char Label[12];
            memcpy(Label, &Buffer[43], 11);
            Label[11] = '\0';
            printf("Volume Label: %s\n", Label);
        }
    } else if (Status == IO_NO_MEDIA) {
        printf("No media present\n");
    } else {
        printf("Read failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 5: Write sectors to floppy
 *
 * This example demonstrates writing data to a floppy disk.
 */
void
Example5_WriteSectors(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    UINT8 Buffer[512];
    UINTN BytesWritten;
    UINT64 LBA = 100;  // Write to sector 100 (don't overwrite boot sector)
    FLOPPY_DRIVE_INFO DriveInfo;

    printf("\n=== Example 5: Write Sectors ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    // Check write protection
    Status = IIOFloppyDrive_GetDriveInfo(pDrive, &DriveInfo);
    if (Status == IO_SUCCESS) {
        if (DriveInfo.bWriteProtected) {
            printf("ERROR: Media is write-protected!\n");
            printf("Please remove write-protection tab and try again.\n");
            return;
        }
    }

    // Fill buffer with test pattern
    for (UINT32 i = 0; i < sizeof(Buffer); i++) {
        Buffer[i] = (UINT8)(i & 0xFF);
    }

    printf("Writing test pattern to LBA %llu...\n", LBA);

    Status = IIOFloppyDrive_WriteSectorsLBA(pDrive, LBA, 1,
                                            Buffer, sizeof(Buffer), &BytesWritten);

    if (Status == IO_SUCCESS) {
        printf("Write successful! Bytes written: %zu\n", BytesWritten);

        // Verify the write
        UINT8 VerifyBuffer[512];
        UINTN BytesRead;
        Status = IIOFloppyDrive_ReadSectorsLBA(pDrive, LBA, 1,
                                               VerifyBuffer, sizeof(VerifyBuffer), &BytesRead);

        if (Status == IO_SUCCESS) {
            if (memcmp(Buffer, VerifyBuffer, sizeof(Buffer)) == 0) {
                printf("Verification successful! Data matches.\n");
            } else {
                printf("WARNING: Verification failed! Data mismatch.\n");
            }
        }
    } else if (Status == IO_NOT_WRITABLE) {
        printf("Media is write-protected\n");
    } else {
        printf("Write failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 6: Format floppy disk
 *
 * This example demonstrates formatting a floppy disk.
 */
void
Example6_FormatFloppy(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    FLOPPY_FORMAT_PARAMS FormatParams;
    FLOPPY_DRIVE_INFO DriveInfo;

    printf("\n=== Example 6: Format Floppy ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    // Get drive information
    Status = IIOFloppyDrive_GetDriveInfo(pDrive, &DriveInfo);
    if (Status != IO_SUCCESS) {
        printf("Failed to get drive info\n");
        return;
    }

    if (DriveInfo.bWriteProtected) {
        printf("ERROR: Media is write-protected! Cannot format.\n");
        return;
    }

    // Setup format parameters for 1.44MB floppy
    memset(&FormatParams, 0, sizeof(FormatParams));
    FormatParams.MediaType = FLOPPY_MEDIA_3_1440K;
    FormatParams.Cylinders = 80;
    FormatParams.Heads = 2;
    FormatParams.SectorsPerTrack = 18;
    FormatParams.BytesPerSector = 512;
    FormatParams.Gap3Length = 0x6C;
    FormatParams.FormatFillByte = 0xF6;
    FormatParams.bVerify = TRUE;
    FormatParams.bQuickFormat = FALSE;

    printf("WARNING: This will erase all data on the disk!\n");
    printf("Format parameters:\n");
    printf("  Media Type: 3.5\" 1.44MB HD\n");
    printf("  Cylinders: %u\n", FormatParams.Cylinders);
    printf("  Heads: %u\n", FormatParams.Heads);
    printf("  Sectors/Track: %u\n", FormatParams.SectorsPerTrack);
    printf("  Verify after format: %s\n", FormatParams.bVerify ? "Yes" : "No");

    printf("\nFormatting disk...\n");

    Status = IIOFloppyDrive_FormatMedia(pDrive, &FormatParams);

    if (Status == IO_SUCCESS) {
        printf("Format successful!\n");
        printf("Disk is now ready for use.\n");
    } else if (Status == IO_NOT_WRITABLE) {
        printf("Format failed: Media is write-protected\n");
    } else {
        printf("Format failed: 0x%X\n", Status);
    }
}

/**
 * @brief Example 7: High-capacity drive operations (Zip/Jaz)
 *
 * This example demonstrates operations with high-capacity removable drives.
 */
void
Example7_HighCapacityOperations(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    FLOPPY_DRIVE_INFO DriveInfo;
    FLOPPY_MEDIA_TYPE MediaType;
    FLOPPY_GEOMETRY Geometry;

    printf("\n=== Example 7: High-Capacity Drive Operations ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    // Get drive information
    Status = IIOFloppyDrive_GetDriveInfo(pDrive, &DriveInfo);
    if (Status == IO_SUCCESS) {
        printf("Drive: %s %s\n", DriveInfo.VendorName, DriveInfo.ModelName);

        switch (DriveInfo.DriveType) {
            case FLOPPY_DRIVE_ZIP_100:
                printf("Type: Iomega Zip 100MB\n");
                break;
            case FLOPPY_DRIVE_ZIP_250:
                printf("Type: Iomega Zip 250MB\n");
                break;
            case FLOPPY_DRIVE_ZIP_750:
                printf("Type: Iomega Zip 750MB\n");
                break;
            case FLOPPY_DRIVE_JAZ_1GB:
                printf("Type: Iomega Jaz 1GB\n");
                break;
            case FLOPPY_DRIVE_JAZ_2GB:
                printf("Type: Iomega Jaz 2GB\n");
                break;
            case FLOPPY_DRIVE_LS120:
                printf("Type: LS-120 SuperDisk\n");
                break;
            case FLOPPY_DRIVE_LS240:
                printf("Type: LS-240 SuperDisk\n");
                break;
            default:
                printf("Type: Unknown high-capacity drive\n");
                break;
        }

        printf("Capabilities:\n");
        if (DriveInfo.Capabilities & FLOPPY_CAP_EJECT) {
            printf("  - Motorized eject\n");
        }
        if (DriveInfo.Capabilities & FLOPPY_CAP_LOCK) {
            printf("  - Door lock\n");
        }
        if (DriveInfo.Capabilities & FLOPPY_CAP_VARIABLE_SPEED) {
            printf("  - Variable rotation speed\n");
        }
    }

    // Detect media
    Status = IIOFloppyDrive_DetectMediaType(pDrive, &MediaType);
    if (Status == IO_SUCCESS) {
        printf("\nMedia detected!\n");

        Status = IIOFloppyDrive_GetMediaGeometry(pDrive, &Geometry);
        if (Status == IO_SUCCESS) {
            printf("Capacity: %.2f MB (%llu bytes)\n",
                   (double)Geometry.TotalBytes / (1024.0 * 1024.0),
                   Geometry.TotalBytes);
        }

        // Try to eject
        printf("\nAttempting to eject media...\n");
        Status = IIOFloppyDrive_EjectMedia(pDrive);
        if (Status == IO_SUCCESS) {
            printf("Media ejected successfully\n");
        } else if (Status == IO_UNSUPPORTED) {
            printf("Drive does not support motorized eject\n");
        } else {
            printf("Eject failed: 0x%X\n", Status);
        }
    } else {
        printf("\nNo media present\n");
    }
}

/**
 * @brief Example 8: CHS/LBA conversion
 *
 * This example demonstrates converting between CHS and LBA addressing.
 */
void
Example8_CHSLBAConversion(
    VOID
    )
{
    IO_RETURN Status;
    FLOPPY_GEOMETRY Geometry;
    UINT32 Cylinder, Head, Sector;
    UINT64 LBA;

    printf("\n=== Example 8: CHS/LBA Conversion ===\n");

    // Get geometry for 1.44MB floppy
    Status = FloppyGetMediaGeometry(FLOPPY_MEDIA_3_1440K, &Geometry);
    if (Status != IO_SUCCESS) {
        printf("Failed to get geometry\n");
        return;
    }

    printf("Media: 3.5\" 1.44MB\n");
    printf("Geometry: C=%u, H=%u, S=%u\n",
           Geometry.Cylinders, Geometry.Heads, Geometry.SectorsPerTrack);

    // Convert CHS to LBA
    printf("\nCHS to LBA Examples:\n");

    Cylinder = 0; Head = 0; Sector = 1;  // Boot sector
    Status = FloppyCHSToLBA(&Geometry, Cylinder, Head, Sector, &LBA);
    if (Status == IO_SUCCESS) {
        printf("  C:H:S %u:%u:%u -> LBA %llu\n", Cylinder, Head, Sector, LBA);
    }

    Cylinder = 0; Head = 1; Sector = 1;  // First sector, side 1
    Status = FloppyCHSToLBA(&Geometry, Cylinder, Head, Sector, &LBA);
    if (Status == IO_SUCCESS) {
        printf("  C:H:S %u:%u:%u -> LBA %llu\n", Cylinder, Head, Sector, LBA);
    }

    Cylinder = 1; Head = 0; Sector = 1;  // Second track
    Status = FloppyCHSToLBA(&Geometry, Cylinder, Head, Sector, &LBA);
    if (Status == IO_SUCCESS) {
        printf("  C:H:S %u:%u:%u -> LBA %llu\n", Cylinder, Head, Sector, LBA);
    }

    // Convert LBA to CHS
    printf("\nLBA to CHS Examples:\n");

    LBA = 0;
    Status = FloppyLBAToCHS(&Geometry, LBA, &Cylinder, &Head, &Sector);
    if (Status == IO_SUCCESS) {
        printf("  LBA %llu -> C:H:S %u:%u:%u\n", LBA, Cylinder, Head, Sector);
    }

    LBA = 18;
    Status = FloppyLBAToCHS(&Geometry, LBA, &Cylinder, &Head, &Sector);
    if (Status == IO_SUCCESS) {
        printf("  LBA %llu -> C:H:S %u:%u:%u\n", LBA, Cylinder, Head, Sector);
    }

    LBA = 36;
    Status = FloppyLBAToCHS(&Geometry, LBA, &Cylinder, &Head, &Sector);
    if (Status == IO_SUCCESS) {
        printf("  LBA %llu -> C:H:S %u:%u:%u\n", LBA, Cylinder, Head, Sector);
    }

    LBA = 2879;  // Last sector
    Status = FloppyLBAToCHS(&Geometry, LBA, &Cylinder, &Head, &Sector);
    if (Status == IO_SUCCESS) {
        printf("  LBA %llu -> C:H:S %u:%u:%u (last sector)\n", LBA, Cylinder, Head, Sector);
    }
}

/**
 * @brief Example 9: I/O statistics
 *
 * This example demonstrates retrieving I/O performance statistics.
 */
void
Example9_IOStatistics(
    IIOFloppyDrive *pDrive
    )
{
    IO_RETURN Status;
    FLOPPY_IO_STATS Stats;

    printf("\n=== Example 9: I/O Statistics ===\n");

    if (pDrive == NULL) {
        printf("No drive provided\n");
        return;
    }

    Status = IIOFloppyDrive_GetIOStats(pDrive, &Stats, FALSE);
    if (Status == IO_SUCCESS) {
        printf("Floppy Drive I/O Statistics:\n");
        printf("  Read Operations:   %llu\n", Stats.ReadOperations);
        printf("  Write Operations:  %llu\n", Stats.WriteOperations);
        printf("  Format Operations: %llu\n", Stats.FormatOperations);
        printf("  Seek Operations:   %llu\n", Stats.SeekOperations);
        printf("  Sectors Read:      %llu\n", Stats.SectorsRead);
        printf("  Sectors Written:   %llu\n", Stats.SectorsWritten);
        printf("  Media Changes:     %llu\n", Stats.MediaChanges);

        printf("\nError Statistics:\n");
        printf("  Read Errors:       %llu\n", Stats.ReadErrors);
        printf("  Write Errors:      %llu\n", Stats.WriteErrors);
        printf("  Seek Errors:       %llu\n", Stats.SeekErrors);
        printf("  CRC Errors:        %llu\n", Stats.CRCErrors);
        printf("  Timeout Errors:    %llu\n", Stats.TimeoutErrors);

        if (Stats.ReadOperations + Stats.WriteOperations > 0) {
            double ErrorRate = (double)(Stats.ReadErrors + Stats.WriteErrors) /
                               (double)(Stats.ReadOperations + Stats.WriteOperations) * 100.0;
            printf("\nOverall Error Rate: %.2f%%\n", ErrorRate);
        }
    } else {
        printf("Failed to retrieve statistics: 0x%X\n", Status);
    }
}

/**
 * @brief Main function - runs all examples
 */
int
main(
    int argc,
    char *argv[]
    )
{
    printf("=============================================\n");
    printf("Floppy Family Usage Examples\n");
    printf("=============================================\n");
    printf("\nThis example demonstrates the NUX IOKit Floppy family driver.\n");
    printf("It shows operations for standard floppies and high-capacity drives.\n");

    // Run examples
    Example1_EnumerateFloppyControllers();
    Example2_DetectMediaType(NULL);
    Example3_ReadSectorsCHS(NULL);
    Example4_ReadSectorsLBA(NULL);
    Example5_WriteSectors(NULL);
    Example6_FormatFloppy(NULL);
    Example7_HighCapacityOperations(NULL);
    Example8_CHSLBAConversion();
    Example9_IOStatistics(NULL);

    printf("\n=============================================\n");
    printf("Examples completed\n");
    printf("=============================================\n");
    printf("\nNote: Most examples use NULL drives as this is a demonstration.\n");
    printf("In a real system, drives would be obtained from controller enumeration.\n");

    return 0;
}
