/**
 * @file modem_example.c
 * @brief Modem Family Usage Examples
 *
 * This example demonstrates:
 * - Modem enumeration and identification
 * - AT command execution
 * - Dialing and answering calls
 * - Data mode communication
 * - Fax mode operations
 * - Voice mode and caller ID
 * - Modem database exploration
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/iokit.h>
#include <iokit/families/modem/modem.h>
#include <iokit/families/serial/serial.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Example 1: Initialize modem subsystem and explore database
 */
static int
Example_Modem_Database(void)
{
    IO_RETURN Status;
    UINT32 uCount;
    UINT32 i;
    MODEM_CONTROLLER_INFO ControllerInfo;

    printf("\n");
    printf("========================================\n");
    printf("  Example 1: Modem Database\n");
    printf("========================================\n\n");

    // Initialize modem subsystem
    printf("Initializing modem subsystem...\n");
    Status = ModemInitialize();
    if (Status != IO_SUCCESS) {
        printf("ERROR: Modem initialization failed (status=0x%08X)\n", Status);
        return -1;
    }
    printf("\n");

    // Get database statistics
    uCount = ModemGetDatabaseCount();
    printf("Modem database contains %u entries\n\n", uCount);

    // Display sample modems from database
    printf("Sample Hardware Modems (first 10):\n");
    printf("==================================\n\n");

    for (i = 0; i < 10 && i < uCount; i++) {
        Status = ModemGetByIndex(i, &ControllerInfo);
        if (Status == IO_SUCCESS) {
            printf("%u. %s\n", i + 1, ControllerInfo.ControllerName);
            printf("   Vendor:   %s\n", ControllerInfo.Vendor);
            printf("   Chipset:  %s\n", ControllerInfo.ChipsetName);
            printf("   Type:     ");
            switch (ControllerInfo.ModemType) {
                case MODEM_TYPE_HARDWARE:   printf("Hardware Modem\n"); break;
                case MODEM_TYPE_WINMODEM:   printf("WinModem/Softmodem\n"); break;
                case MODEM_TYPE_CONTROLLER: printf("Controller-based\n"); break;
                default:                    printf("Unknown\n"); break;
            }
            printf("   Max Speed: %u bps\n", ControllerInfo.MaxSpeed);
            printf("   VID:DID:  %04X:%04X\n",
                   ControllerInfo.VendorID, ControllerInfo.DeviceID);

            // Display capabilities
            printf("   Features: ");
            if (ControllerInfo.Capabilities & MODEM_CAP_DATA)
                printf("Data ");
            if (ControllerInfo.Capabilities & MODEM_CAP_FAX)
                printf("Fax ");
            if (ControllerInfo.Capabilities & MODEM_CAP_VOICE)
                printf("Voice ");
            if (ControllerInfo.Capabilities & MODEM_CAP_CALLER_ID)
                printf("CallerID ");
            printf("\n");

            // Display supported standards
            printf("   Standards:");
            if (ControllerInfo.SupportedStandards & MODEM_V32BIS)
                printf(" V.32bis");
            if (ControllerInfo.SupportedStandards & MODEM_V34)
                printf(" V.34");
            if (ControllerInfo.SupportedStandards & MODEM_V90)
                printf(" V.90");
            if (ControllerInfo.SupportedStandards & MODEM_V92)
                printf(" V.92");
            if (ControllerInfo.SupportedStandards & MODEM_K56FLEX)
                printf(" K56flex");
            if (ControllerInfo.SupportedStandards & MODEM_X2)
                printf(" X2");
            printf("\n\n");
        }
    }

    return 0;
}

/**
 * @brief Example 2: AT command basics
 */
static int
Example_AT_Commands(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;
    CHAR8 szResponse[MAX_AT_RESPONSE_LENGTH];
    UINT16 uCOMPort = 1;

    printf("\n");
    printf("========================================\n");
    printf("  Example 2: AT Command Basics\n");
    printf("========================================\n\n");

    // Create modem controller on COM1
    printf("Creating modem controller on COM%u...\n", uCOMPort);
    Status = ModemControllerCreate(uCOMPort, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available on COM%u (status=0x%08X)\n", uCOMPort, Status);
        printf("This is normal if no modem is attached.\n");
        return 0;
    }

    printf("Modem controller created successfully\n\n");

    // Initialize modem
    printf("Initializing modem...\n");
    Status = IIOModemController_Initialize(pModem);
    if (Status == IO_SUCCESS) {
        printf("Modem initialized successfully\n\n");
    }

    // Send basic AT command
    printf("Sending AT command (attention)...\n");
    Status = IIOModemController_SendATCommand(pModem, "", szResponse,
                                               sizeof(szResponse), 2000);
    if (Status == IO_SUCCESS) {
        printf("Response: %s\n\n", szResponse);
    }

    // Query modem identification (ATI)
    printf("Querying modem identification (ATI)...\n");
    Status = IIOModemController_SendATCommand(pModem, "I", szResponse,
                                               sizeof(szResponse), 2000);
    if (Status == IO_SUCCESS) {
        printf("Modem Info:\n%s\n\n", szResponse);
    }

    // Enable echo
    printf("Enabling command echo (ATE1)...\n");
    Status = IIOModemController_EnableEcho(pModem);
    if (Status == IO_SUCCESS) {
        printf("Echo enabled\n\n");
    }

    // Set verbose result codes
    printf("Setting verbose result codes (ATV1)...\n");
    Status = IIOModemController_SetResultCodeMode(pModem, TRUE);
    if (Status == IO_SUCCESS) {
        printf("Verbose mode enabled\n\n");
    }

    // Set speaker volume and mode
    printf("Configuring speaker (ATL2M1)...\n");
    IIOModemController_SetSpeakerVolume(pModem, 2);  // Medium volume
    IIOModemController_SetSpeakerMode(pModem, 1);    // On until carrier
    printf("Speaker configured\n\n");

    // Release modem
    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 3: Dialing and connection
 */
static int
Example_Modem_Dialing(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;
    MODEM_CONNECTION_PARAMS Params;
    MODEM_STATE State;
    UINT32 uSpeed;

    printf("\n");
    printf("========================================\n");
    printf("  Example 3: Dialing and Connection\n");
    printf("========================================\n\n");

    // Create modem controller
    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    // Initialize modem
    IIOModemController_Initialize(pModem);

    // Configure connection parameters
    memset(&Params, 0, sizeof(Params));
    strncpy((char *)Params.PhoneNumber, "5551234", sizeof(Params.PhoneNumber) - 1);
    Params.Timeout = 60;        // 60 second timeout
    Params.Retries = 3;         // 3 retries
    Params.bBlindDial = FALSE;  // Check for dialtone
    Params.bPulseDialing = FALSE; // Use tone dialing

    printf("Dialing %s...\n", Params.PhoneNumber);
    printf("(Note: This is a demonstration - no actual call will be made)\n\n");

    Status = IIOModemController_Dial(pModem, &Params);

    if (Status == IO_SUCCESS) {
        printf("Dial command issued successfully\n");

        // Get modem state
        IIOModemController_GetState(pModem, &State);
        printf("Modem state: ");
        switch (State) {
            case MODEM_STATE_IDLE:        printf("Idle\n"); break;
            case MODEM_STATE_DIALING:     printf("Dialing\n"); break;
            case MODEM_STATE_CONNECTING:  printf("Connecting\n"); break;
            case MODEM_STATE_CONNECTED:   printf("Connected\n"); break;
            default:                      printf("Unknown\n"); break;
        }

        // If connected, get speed
        if (State == MODEM_STATE_CONNECTED) {
            Status = IIOModemController_GetConnectionSpeed(pModem, &uSpeed);
            if (Status == IO_SUCCESS) {
                printf("Connection speed: %u bps\n", uSpeed);
            }
        }
    } else if (Status == IO_BUSY) {
        printf("Line is busy\n");
    } else if (Status == IO_NO_CARRIER) {
        printf("No carrier detected\n");
    } else {
        printf("Dial failed (status=0x%08X)\n", Status);
    }

    printf("\n");

    // Hang up
    printf("Hanging up...\n");
    IIOModemController_Hangup(pModem);
    printf("Call terminated\n\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 4: Answering incoming calls
 */
static int
Example_Modem_Answering(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;

    printf("\n");
    printf("========================================\n");
    printf("  Example 4: Answering Calls\n");
    printf("========================================\n\n");

    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    // Initialize modem
    IIOModemController_Initialize(pModem);

    // Enable caller ID
    printf("Enabling Caller ID...\n");
    Status = IIOModemController_EnableCallerID(pModem);
    if (Status == IO_SUCCESS) {
        printf("Caller ID enabled\n");
    } else if (Status == IO_UNSUPPORTED) {
        printf("Caller ID not supported by this modem\n");
    }
    printf("\n");

    // In a real application, you would wait for RING
    printf("Waiting for incoming call (RING)...\n");
    printf("(In a real application, this would be event-driven)\n\n");

    // Answer the call (ATA)
    printf("Answering call (ATA)...\n");
    Status = IIOModemController_Answer(pModem);
    if (Status == IO_SUCCESS) {
        printf("Call answered successfully\n");
    } else {
        printf("Failed to answer call\n");
    }
    printf("\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 5: Fax mode operations
 */
static int
Example_Fax_Mode(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;
    MODEM_CONTROLLER_INFO Info;

    printf("\n");
    printf("========================================\n");
    printf("  Example 5: Fax Mode\n");
    printf("========================================\n\n");

    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    // Get modem info to check fax capability
    Status = IIOModemController_GetControllerInfo(pModem, &Info);
    if (Status == IO_SUCCESS) {
        printf("Modem: %s\n", Info.ControllerName);

        if (Info.Capabilities & MODEM_CAP_FAX) {
            printf("Fax capability: Supported\n");

            // Display fax standards
            printf("Fax standards:");
            if (Info.FaxSupport & FAX_V17)
                printf(" V.17");
            if (Info.FaxSupport & FAX_V29)
                printf(" V.29");
            if (Info.FaxSupport & FAX_V27TER)
                printf(" V.27ter");
            printf("\n");

            printf("Fax classes:");
            if (Info.FaxSupport & FAX_CLASS1)
                printf(" Class1");
            if (Info.FaxSupport & FAX_CLASS2)
                printf(" Class2");
            if (Info.FaxSupport & FAX_CLASS2_0)
                printf(" Class2.0");
            printf("\n\n");

            // Enter fax mode (Class 1)
            printf("Entering Fax Class 1 mode (+FCLASS=1)...\n");
            Status = IIOModemController_EnterFaxMode(pModem, 1);
            if (Status == IO_SUCCESS) {
                printf("Fax Class 1 mode activated\n");
            } else {
                printf("Failed to enter fax mode\n");
            }
        } else {
            printf("Fax capability: Not supported\n");
        }
    }
    printf("\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 6: Voice mode and caller ID
 */
static int
Example_Voice_Mode(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;
    MODEM_CONTROLLER_INFO Info;

    printf("\n");
    printf("========================================\n");
    printf("  Example 6: Voice Mode\n");
    printf("========================================\n\n");

    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    // Get modem info
    Status = IIOModemController_GetControllerInfo(pModem, &Info);
    if (Status == IO_SUCCESS) {
        printf("Modem: %s\n", Info.ControllerName);

        if (Info.Capabilities & MODEM_CAP_VOICE) {
            printf("Voice capability: Supported\n");

            // Additional voice features
            if (Info.Capabilities & MODEM_CAP_SPEAKERPHONE)
                printf("  - Speakerphone mode\n");
            if (Info.Capabilities & MODEM_CAP_VOICE_MAIL)
                printf("  - Voice mail\n");
            if (Info.Capabilities & MODEM_CAP_TAM)
                printf("  - Telephone answering machine\n");
            if (Info.Capabilities & MODEM_CAP_CALLER_ID)
                printf("  - Caller ID\n");
            if (Info.Capabilities & MODEM_CAP_DISTINCTIVE_RING)
                printf("  - Distinctive ring\n");

            printf("\n");

            // Enter voice mode
            printf("Entering voice mode...\n");
            Status = IIOModemController_EnterVoiceMode(pModem);
            if (Status == IO_SUCCESS) {
                printf("Voice mode activated\n");
                printf("In voice mode, you can:\n");
                printf("  - Record voice messages\n");
                printf("  - Play voice messages\n");
                printf("  - Implement voicemail systems\n");
            } else {
                printf("Failed to enter voice mode\n");
            }
        } else {
            printf("Voice capability: Not supported\n");
        }
    }
    printf("\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 7: Error correction and compression
 */
static int
Example_Error_Correction(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;

    printf("\n");
    printf("========================================\n");
    printf("  Example 7: Error Correction\n");
    printf("========================================\n\n");

    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    IIOModemController_Initialize(pModem);

    // Enable V.42 error correction
    printf("Enabling V.42 error correction...\n");
    Status = IIOModemController_SetErrorCorrection(pModem, ERROR_CORR_V42, TRUE);
    if (Status == IO_SUCCESS) {
        printf("V.42 error correction enabled\n");
    } else if (Status == IO_UNSUPPORTED) {
        printf("V.42 not supported\n");
    }

    // Enable V.42bis data compression
    printf("Enabling V.42bis data compression...\n");
    Status = IIOModemController_SetDataCompression(pModem, DATA_COMP_V42BIS, TRUE);
    if (Status == IO_SUCCESS) {
        printf("V.42bis compression enabled\n");
    } else if (Status == IO_UNSUPPORTED) {
        printf("V.42bis not supported\n");
    }

    printf("\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 8: Modem statistics
 */
static int
Example_Modem_Statistics(void)
{
    IIOModemController *pModem = NULL;
    IO_RETURN Status;
    MODEM_STATISTICS Stats;

    printf("\n");
    printf("========================================\n");
    printf("  Example 8: Modem Statistics\n");
    printf("========================================\n\n");

    Status = ModemControllerCreate(1, &pModem);
    if (Status != IO_SUCCESS) {
        printf("No modem available for this example\n");
        return 0;
    }

    // Get statistics
    printf("Retrieving modem statistics...\n");
    Status = IIOModemController_GetStatistics(pModem, &Stats, FALSE);
    if (Status == IO_SUCCESS) {
        printf("\nConnection Statistics:\n");
        printf("  Total connections:      %u\n", Stats.TotalConnections);
        printf("  Successful:             %u\n", Stats.SuccessfulConnections);
        printf("  Failed:                 %u\n", Stats.FailedConnections);
        printf("  Dropped:                %u\n", Stats.DroppedConnections);

        printf("\nData Transfer:\n");
        printf("  Bytes sent:             %llu\n",
               (unsigned long long)Stats.TotalBytesSent);
        printf("  Bytes received:         %llu\n",
               (unsigned long long)Stats.TotalBytesReceived);
        printf("  Average speed:          %u bps\n", Stats.AverageSpeed);
        printf("  Peak speed:             %u bps\n", Stats.PeakSpeed);

        printf("\nError Statistics:\n");
        printf("  Frame errors:           %u\n", Stats.FrameErrors);
        printf("  Overrun errors:         %u\n", Stats.OverrunErrors);
        printf("  Parity errors:          %u\n", Stats.ParityErrors);
        printf("  Protocol errors:        %u\n", Stats.ProtocolErrors);
        printf("  Retransmissions:        %u\n", Stats.Retransmissions);

        printf("\nTime Statistics:\n");
        printf("  Total connect time:     %llu seconds\n",
               (unsigned long long)Stats.TotalConnectTime);
        printf("  Average call length:    %u seconds\n", Stats.AverageCallLength);
        printf("  Longest call:           %u seconds\n", Stats.LongestCall);
    }

    printf("\n");

    IIOModemController_Release(pModem);

    return 0;
}

/**
 * @brief Example 9: WinModem vs Hardware Modem detection
 */
static int
Example_Modem_Type_Detection(void)
{
    UINT32 i;
    MODEM_CONTROLLER_INFO Info;
    UINT32 uHardwareCount = 0;
    UINT32 uSoftwareCount = 0;
    UINT32 uControllerCount = 0;
    UINT32 uTotalCount;

    printf("\n");
    printf("========================================\n");
    printf("  Example 9: Modem Type Detection\n");
    printf("========================================\n\n");

    uTotalCount = ModemGetDatabaseCount();

    printf("Analyzing modem database by type...\n\n");

    // Count each type
    for (i = 0; i < uTotalCount; i++) {
        if (ModemGetByIndex(i, &Info) == IO_SUCCESS) {
            switch (Info.ModemType) {
                case MODEM_TYPE_HARDWARE:
                    uHardwareCount++;
                    break;
                case MODEM_TYPE_WINMODEM:
                    uSoftwareCount++;
                    break;
                case MODEM_TYPE_CONTROLLER:
                    uControllerCount++;
                    break;
                default:
                    break;
            }
        }
    }

    printf("Database Breakdown:\n");
    printf("  Hardware modems:     %3u (Traditional with DSP)\n", uHardwareCount);
    printf("  Software modems:     %3u (WinModem/Softmodem)\n", uSoftwareCount);
    printf("  Controller modems:   %3u (Controller-based)\n", uControllerCount);
    printf("  ----------------------------------------\n");
    printf("  Total:               %3u\n", uTotalCount);

    printf("\n");
    printf("WinModem/Softmodem Characteristics:\n");
    printf("  - Use host CPU for DSP processing\n");
    printf("  - Require driver support\n");
    printf("  - Lower hardware cost\n");
    printf("  - Common chipsets: Lucent, Conexant, Motorola SM56\n");

    printf("\n");
    printf("Hardware Modem Characteristics:\n");
    printf("  - Dedicated DSP chip onboard\n");
    printf("  - Work with minimal driver support\n");
    printf("  - Higher hardware cost\n");
    printf("  - Examples: USRobotics Courier, Hayes Smartmodem\n");

    printf("\n");

    return 0;
}

/**
 * @brief Main function - Run all examples
 */
int
main(void)
{
    printf("\n");
    printf("================================================\n");
    printf("  Modem Family Examples\n");
    printf("================================================\n");

    // Run all examples
    Example_Modem_Database();
    Example_AT_Commands();
    Example_Modem_Dialing();
    Example_Modem_Answering();
    Example_Fax_Mode();
    Example_Voice_Mode();
    Example_Error_Correction();
    Example_Modem_Statistics();
    Example_Modem_Type_Detection();

    printf("\n");
    printf("================================================\n");
    printf("  All examples completed\n");
    printf("================================================\n");
    printf("\n");

    return 0;
}
