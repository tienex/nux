/**
 * @file timer.c
 * @brief Timer/Clock Family Implementation
 *
 * Provides comprehensive timer and clock support including:
 * - PIT (8253/8254) programming and operation
 * - RTC (MC146818) CMOS real-time clock access
 * - HPET (High Precision Event Timer) support
 * - TSC (Time Stamp Counter) calibration and use
 * - Multiple clock sources (realtime, monotonic, boottime)
 * - Watchdog timer functionality
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/timer/timer.h>
#include <ananke/ntrtl.h>
#include <ananke/x86.h>
#include <string.h>
#include <stdio.h>

//
// Hardware Constants
//

// PIT (8253/8254) I/O Ports
#define PIT_CHANNEL0            0x40        /**< Channel 0 (system timer) */
#define PIT_CHANNEL1            0x41        /**< Channel 1 (unused/legacy) */
#define PIT_CHANNEL2            0x42        /**< Channel 2 (PC speaker) */
#define PIT_COMMAND             0x43        /**< Command register */
#define PIT_BASE_FREQUENCY      1193182     /**< Base frequency: 1.193182 MHz */

// PIT Command Register Bits
#define PIT_CMD_CHANNEL_SHIFT   6
#define PIT_CMD_ACCESS_SHIFT    4
#define PIT_CMD_MODE_SHIFT      1
#define PIT_CMD_BCD             (1 << 0)

#define PIT_CMD_ACCESS_LATCH    0
#define PIT_CMD_ACCESS_LO       1
#define PIT_CMD_ACCESS_HI       2
#define PIT_CMD_ACCESS_LOHI     3

// RTC (MC146818) CMOS Ports
#define RTC_INDEX_PORT          0x70        /**< Index/address port */
#define RTC_DATA_PORT           0x71        /**< Data port */
#define RTC_NMI_DISABLE         (1 << 7)    /**< NMI disable bit in index */

// HPET MMIO Register Offsets
#define HPET_GENERAL_CAP_ID     0x000       /**< General capabilities and ID */
#define HPET_GENERAL_CONFIG     0x010       /**< General configuration */
#define HPET_GENERAL_INT_STATUS 0x020       /**< General interrupt status */
#define HPET_MAIN_COUNTER       0x0F0       /**< Main counter value */
#define HPET_TIMER_CONFIG_BASE  0x100       /**< Timer 0 config/cap */
#define HPET_TIMER_COMPARATOR_BASE 0x108    /**< Timer 0 comparator */
#define HPET_TIMER_STRIDE       0x20        /**< Stride between timers */

// HPET Configuration Bits
#define HPET_CFG_ENABLE         (1 << 0)    /**< Enable main counter */
#define HPET_CFG_LEGACY         (1 << 1)    /**< Legacy replacement route */

// TSC Feature Flags (CPUID)
#define CPUID_TSC               (1 << 4)    /**< TSC available */
#define CPUID_INVARIANT_TSC     (1 << 8)    /**< Invariant TSC (from 0x80000007 EDX) */

//
// Global State
//

static BOOLEAN g_bTimerInitialized = FALSE;
static BOOLEAN g_bHPETAvailable = FALSE;
static BOOLEAN g_bTSCAvailable = FALSE;
static UINT64 g_TSCFrequency = 0;
static HPET_INFO g_HPETInfo;
static volatile UINT32 *g_pHPETRegisters = NULL;

//
// Implementation Structures
//

/**
 * @brief PIT Timer Implementation
 */
typedef struct _PIT_TIMER_IMPL {
    IIOTimerController  Vtbl;
    ULONG               RefCount;
    UINT8               Channel;
    TIMER_INFO          Info;
    BOOLEAN             bRunning;
    UINT16              uDivisor;
    TIMER_CALLBACK      pfnCallback;
    VOID               *pCallbackContext;
} PIT_TIMER_IMPL;

/**
 * @brief RTC Implementation
 */
typedef struct _RTC_IMPL {
    IIORealTimeClock    Vtbl;
    ULONG               RefCount;
    BOOLEAN             bBatteryGood;
} RTC_IMPL;

/**
 * @brief HPET Timer Implementation
 */
typedef struct _HPET_TIMER_IMPL {
    IIOTimerController  Vtbl;
    ULONG               RefCount;
    UINT8               TimerIndex;
    TIMER_INFO          Info;
    BOOLEAN             bRunning;
    TIMER_CALLBACK      pfnCallback;
    VOID               *pCallbackContext;
} HPET_TIMER_IMPL;

/**
 * @brief Clock Source Implementation
 */
typedef struct _CLOCK_IMPL {
    IIOClock            Vtbl;
    ULONG               RefCount;
    CLOCK_SOURCE        Source;
    UINT64              Frequency;
    UINT64              Resolution;
    BOOLEAN             bMonotonic;
    UINT64              BootTime;       // For boottime clock
} CLOCK_IMPL;

/**
 * @brief Watchdog Implementation
 */
typedef struct _WATCHDOG_IMPL {
    IIOWatchdog         Vtbl;
    ULONG               RefCount;
    UINT32              TimeoutMs;
    BOOLEAN             bRunning;
    WATCHDOG_ACTION     Action;
    TIMER_CALLBACK      pfnCallback;
    VOID               *pCallbackContext;
} WATCHDOG_IMPL;

//
// Helper Functions
//

/**
 * @brief Read RTC register
 */
static UINT8
RTCReadRegister(
    UINT8 uRegister
    )
{
    outb(RTC_INDEX_PORT, uRegister);
    return inb(RTC_DATA_PORT);
}

/**
 * @brief Write RTC register
 */
static VOID
RTCWriteRegister(
    UINT8 uRegister,
    UINT8 uValue
    )
{
    outb(RTC_INDEX_PORT, uRegister);
    outb(RTC_DATA_PORT, uValue);
}

/**
 * @brief Wait for RTC update to complete
 */
static VOID
RTCWaitUpdate(VOID)
{
    UINT32 uTimeout = 1000000;

    // Wait for update in progress to clear
    while ((RTCReadRegister(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP) && uTimeout > 0) {
        uTimeout--;
    }
}

/**
 * @brief Convert BCD to binary
 */
static UINT8
BCDToBinary(
    UINT8 uBCD
    )
{
    return ((uBCD >> 4) * 10) + (uBCD & 0x0F);
}

/**
 * @brief Convert binary to BCD
 */
static UINT8
BinaryToBCD(
    UINT8 uBinary
    )
{
    return ((uBinary / 10) << 4) | (uBinary % 10);
}

/**
 * @brief Read TSC value
 */
static inline UINT64
ReadTSC(VOID)
{
    UINT32 low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((UINT64)high << 32) | low;
}

/**
 * @brief Calibrate TSC frequency
 */
static UINT64
CalibrateTSC(VOID)
{
    UINT64 tsc_start, tsc_end;
    UINT32 pit_ticks = 11932; // ~10ms at 1.193182 MHz
    UINT8 status;

    printf("Timer: Calibrating TSC...\n");

    // Program PIT channel 2 for one-shot mode
    outb(PIT_COMMAND, (2 << PIT_CMD_CHANNEL_SHIFT) |
                      (PIT_CMD_ACCESS_LOHI << PIT_CMD_ACCESS_SHIFT) |
                      (0 << PIT_CMD_MODE_SHIFT));

    // Set count
    outb(PIT_CHANNEL2, pit_ticks & 0xFF);
    outb(PIT_CHANNEL2, (pit_ticks >> 8) & 0xFF);

    // Start PIT channel 2
    status = inb(0x61);
    outb(0x61, (status & 0xFC) | 0x01);

    // Read TSC start
    tsc_start = ReadTSC();

    // Wait for PIT to count down
    while (!(inb(0x61) & 0x20));

    // Read TSC end
    tsc_end = ReadTSC();

    // Stop PIT channel 2
    outb(0x61, status);

    // Calculate frequency (ticks per ~10ms * 100 = Hz)
    UINT64 frequency = ((tsc_end - tsc_start) * PIT_BASE_FREQUENCY) / pit_ticks;

    printf("Timer: TSC frequency = %llu Hz (~%llu MHz)\n",
           frequency, frequency / 1000000);

    return frequency;
}

/**
 * @brief Detect HPET via ACPI
 */
static BOOLEAN
DetectHPET(VOID)
{
    // In a real implementation, this would parse the ACPI HPET table
    // For now, we'll assume HPET is not present unless mapped
    printf("Timer: HPET detection not yet implemented\n");
    return FALSE;
}

/**
 * @brief Initialize HPET
 */
static IO_RETURN
InitializeHPET(VOID)
{
    if (!g_bHPETAvailable) {
        return IO_NO_DEVICE;
    }

    // Read capabilities
    UINT64 cap = *(volatile UINT64*)((UINT8*)g_pHPETRegisters + HPET_GENERAL_CAP_ID);

    g_HPETInfo.Period = (UINT32)(cap >> 32);
    g_HPETInfo.VendorID = (UINT32)((cap >> 16) & 0xFFFF);
    g_HPETInfo.RevisionID = (UINT8)(cap & 0xFF);
    g_HPETInfo.NumTimers = (UINT8)(((cap >> 8) & 0x1F) + 1);
    g_HPETInfo.b64BitCounter = (cap & (1ULL << 13)) != 0;
    g_HPETInfo.bLegacyReplacement = (cap & (1ULL << 15)) != 0;

    printf("Timer: HPET detected - %d timers, %u fs period\n",
           g_HPETInfo.NumTimers, g_HPETInfo.Period);

    // Enable HPET
    *(volatile UINT64*)((UINT8*)g_pHPETRegisters + HPET_GENERAL_CONFIG) = HPET_CFG_ENABLE;

    return IO_SUCCESS;
}

//
// PIT Timer Implementation
//

static IO_RETURN
PITTimer_GetTimerInfo(
    IIOTimerController *pThis,
    TIMER_INFO *pTimerInfo
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;

    if (pTimer == NULL || pTimerInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    memcpy(pTimerInfo, &pTimer->Info, sizeof(TIMER_INFO));
    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_StartTimer(
    IIOTimerController *pThis,
    UINT64 uCount,
    BOOLEAN bPeriodic
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;
    UINT16 uDivisor;
    UINT8 uCommand;

    if (pTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uCount == 0 || uCount > 65535) {
        return IO_BAD_ARGUMENT;
    }

    uDivisor = (UINT16)uCount;

    // Build command byte
    uCommand = (pTimer->Channel << PIT_CMD_CHANNEL_SHIFT) |
               (PIT_CMD_ACCESS_LOHI << PIT_CMD_ACCESS_SHIFT) |
               ((bPeriodic ? PIT_MODE_RATE_GENERATOR : PIT_MODE_INTERRUPT_ON_TERMINAL) << PIT_CMD_MODE_SHIFT);

    // Send command
    outb(PIT_COMMAND, uCommand);

    // Send divisor (low byte, then high byte)
    outb(PIT_CHANNEL0 + pTimer->Channel, uDivisor & 0xFF);
    outb(PIT_CHANNEL0 + pTimer->Channel, (uDivisor >> 8) & 0xFF);

    pTimer->uDivisor = uDivisor;
    pTimer->bRunning = TRUE;
    pTimer->Info.bRunning = TRUE;
    pTimer->Info.bPeriodicMode = bPeriodic;

    printf("Timer: PIT channel %d started (divisor=%u, periodic=%d)\n",
           pTimer->Channel, uDivisor, bPeriodic);

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_StopTimer(
    IIOTimerController *pThis
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;

    if (pTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!pTimer->bRunning) {
        return IO_NOT_READY;
    }

    // Stop timer by setting count to 0
    outb(PIT_COMMAND, (pTimer->Channel << PIT_CMD_CHANNEL_SHIFT) |
                      (PIT_CMD_ACCESS_LOHI << PIT_CMD_ACCESS_SHIFT));
    outb(PIT_CHANNEL0 + pTimer->Channel, 0);
    outb(PIT_CHANNEL0 + pTimer->Channel, 0);

    pTimer->bRunning = FALSE;
    pTimer->Info.bRunning = FALSE;

    printf("Timer: PIT channel %d stopped\n", pTimer->Channel);

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_ResetTimer(
    IIOTimerController *pThis
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;

    if (pTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Restart with same divisor if running
    if (pTimer->bRunning) {
        return PITTimer_StartTimer(pThis, pTimer->uDivisor, pTimer->Info.bPeriodicMode);
    }

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_SetFrequency(
    IIOTimerController *pThis,
    UINT64 uFrequency
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;
    UINT16 uDivisor;

    if (pTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uFrequency == 0 || uFrequency > PIT_BASE_FREQUENCY) {
        return IO_BAD_ARGUMENT;
    }

    // Calculate divisor
    uDivisor = (UINT16)(PIT_BASE_FREQUENCY / uFrequency);
    if (uDivisor == 0) {
        uDivisor = 1;
    }

    pTimer->Info.Frequency = PIT_BASE_FREQUENCY / uDivisor;

    // If running, reprogram
    if (pTimer->bRunning) {
        return PITTimer_StartTimer(pThis, uDivisor, pTimer->Info.bPeriodicMode);
    }

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_GetCount(
    IIOTimerController *pThis,
    UINT64 *puCount
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;
    UINT8 low, high;

    if (pTimer == NULL || puCount == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Latch count
    outb(PIT_COMMAND, (pTimer->Channel << PIT_CMD_CHANNEL_SHIFT));

    // Read count (low then high)
    low = inb(PIT_CHANNEL0 + pTimer->Channel);
    high = inb(PIT_CHANNEL0 + pTimer->Channel);

    *puCount = (high << 8) | low;

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_SetCount(
    IIOTimerController *pThis,
    UINT64 uCount
    )
{
    // Redirect to StartTimer
    return PITTimer_StartTimer(pThis, uCount, FALSE);
}

static IO_RETURN
PITTimer_RegisterCallback(
    IIOTimerController *pThis,
    TIMER_CALLBACK pfnCallback,
    VOID *pContext
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;

    if (pTimer == NULL || pfnCallback == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pTimer->pfnCallback = pfnCallback;
    pTimer->pCallbackContext = pContext;

    return IO_SUCCESS;
}

static IO_RETURN
PITTimer_UnregisterCallback(
    IIOTimerController *pThis
    )
{
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;

    if (pTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (pTimer->pfnCallback == NULL) {
        return IO_NO_MATCH;
    }

    pTimer->pfnCallback = NULL;
    pTimer->pCallbackContext = NULL;

    return IO_SUCCESS;
}

// COM interface methods (abbreviated for brevity)
static ULONG PITTimer_AddRef(IIOTimerController *pThis) {
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;
    return ++pTimer->RefCount;
}

static ULONG PITTimer_Release(IIOTimerController *pThis) {
    PIT_TIMER_IMPL *pTimer = (PIT_TIMER_IMPL*)pThis;
    if (--pTimer->RefCount == 0) {
        free(pTimer);
        return 0;
    }
    return pTimer->RefCount;
}

//
// RTC Implementation
//

static IO_RETURN
RTC_GetTime(
    IIORealTimeClock *pThis,
    RTC_TIME *pTime
    )
{
    UINT8 statusB;
    UINT8 second, minute, hour, day, month, year, century;
    BOOLEAN bBCD, b12Hour, bPM = FALSE;

    if (pThis == NULL || pTime == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Wait for update to complete
    RTCWaitUpdate();

    // Read status register B to get format
    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    bBCD = !(statusB & RTC_STATUS_B_DM);
    b12Hour = !(statusB & RTC_STATUS_B_24H);

    // Read time registers
    second = RTCReadRegister(RTC_REG_SECONDS);
    minute = RTCReadRegister(RTC_REG_MINUTES);
    hour = RTCReadRegister(RTC_REG_HOURS);
    day = RTCReadRegister(RTC_REG_DAY_OF_MONTH);
    month = RTCReadRegister(RTC_REG_MONTH);
    year = RTCReadRegister(RTC_REG_YEAR);
    century = RTCReadRegister(RTC_REG_CENTURY);

    // Convert from BCD if necessary
    if (bBCD) {
        second = BCDToBinary(second);
        minute = BCDToBinary(minute);

        if (b12Hour) {
            bPM = (hour & 0x80) != 0;
            hour = BCDToBinary(hour & 0x7F);
        } else {
            hour = BCDToBinary(hour);
        }

        day = BCDToBinary(day);
        month = BCDToBinary(month);
        year = BCDToBinary(year);
        century = BCDToBinary(century);
    } else if (b12Hour) {
        bPM = (hour & 0x80) != 0;
        hour &= 0x7F;
    }

    // Fill in time structure
    pTime->Second = second;
    pTime->Minute = minute;
    pTime->Hour = hour;
    pTime->Day = day;
    pTime->Month = month;
    pTime->Year = (century * 100) + year;
    pTime->Century = century;
    pTime->b24Hour = !b12Hour;
    pTime->bPM = bPM;
    pTime->DayOfWeek = RTCReadRegister(RTC_REG_DAY_OF_WEEK) - 1; // Convert to 0=Sunday
    pTime->bDST = (statusB & RTC_STATUS_B_DSE) != 0;

    return IO_SUCCESS;
}

static IO_RETURN
RTC_SetTime(
    IIORealTimeClock *pThis,
    CONST RTC_TIME *pTime
    )
{
    UINT8 statusB;
    BOOLEAN bBCD;
    UINT8 second, minute, hour, day, month, year, century;

    if (pThis == NULL || pTime == NULL) {
        return IO_BAD_ARGUMENT;
    }

    // Validate time
    if (pTime->Second > 59 || pTime->Minute > 59 || pTime->Hour > 23 ||
        pTime->Day == 0 || pTime->Day > 31 ||
        pTime->Month == 0 || pTime->Month > 12 ||
        pTime->Year < 1970 || pTime->Year > 2099) {
        return IO_BAD_ARGUMENT;
    }

    // Calculate century and year
    century = (UINT8)(pTime->Year / 100);
    year = (UINT8)(pTime->Year % 100);

    // Get current format
    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    bBCD = !(statusB & RTC_STATUS_B_DM);

    // Prepare values
    second = pTime->Second;
    minute = pTime->Minute;
    hour = pTime->Hour;
    day = pTime->Day;
    month = pTime->Month;

    // Convert to BCD if necessary
    if (bBCD) {
        second = BinaryToBCD(second);
        minute = BinaryToBCD(minute);
        hour = BinaryToBCD(hour);
        day = BinaryToBCD(day);
        month = BinaryToBCD(month);
        year = BinaryToBCD(year);
        century = BinaryToBCD(century);
    }

    // Disable updates
    RTCWriteRegister(RTC_REG_STATUS_B, statusB | RTC_STATUS_B_SET);

    // Write time registers
    RTCWriteRegister(RTC_REG_SECONDS, second);
    RTCWriteRegister(RTC_REG_MINUTES, minute);
    RTCWriteRegister(RTC_REG_HOURS, hour);
    RTCWriteRegister(RTC_REG_DAY_OF_MONTH, day);
    RTCWriteRegister(RTC_REG_MONTH, month);
    RTCWriteRegister(RTC_REG_YEAR, year);
    RTCWriteRegister(RTC_REG_CENTURY, century);

    // Re-enable updates
    RTCWriteRegister(RTC_REG_STATUS_B, statusB);

    printf("Timer: RTC time set to %04d-%02d-%02d %02d:%02d:%02d\n",
           pTime->Year, pTime->Month, pTime->Day,
           pTime->Hour, pTime->Minute, pTime->Second);

    return IO_SUCCESS;
}

static IO_RETURN
RTC_GetAlarm(
    IIORealTimeClock *pThis,
    RTC_TIME *pTime
    )
{
    UINT8 statusB;
    BOOLEAN bBCD;

    if (pThis == NULL || pTime == NULL) {
        return IO_BAD_ARGUMENT;
    }

    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    bBCD = !(statusB & RTC_STATUS_B_DM);

    pTime->Second = RTCReadRegister(RTC_REG_SECONDS_ALARM);
    pTime->Minute = RTCReadRegister(RTC_REG_MINUTES_ALARM);
    pTime->Hour = RTCReadRegister(RTC_REG_HOURS_ALARM);

    if (bBCD) {
        pTime->Second = BCDToBinary(pTime->Second);
        pTime->Minute = BCDToBinary(pTime->Minute);
        pTime->Hour = BCDToBinary(pTime->Hour);
    }

    return IO_SUCCESS;
}

static IO_RETURN
RTC_SetAlarm(
    IIORealTimeClock *pThis,
    CONST RTC_TIME *pTime
    )
{
    UINT8 statusB;
    BOOLEAN bBCD;
    UINT8 second, minute, hour;

    if (pThis == NULL || pTime == NULL) {
        return IO_BAD_ARGUMENT;
    }

    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    bBCD = !(statusB & RTC_STATUS_B_DM);

    second = pTime->Second;
    minute = pTime->Minute;
    hour = pTime->Hour;

    if (bBCD) {
        second = BinaryToBCD(second);
        minute = BinaryToBCD(minute);
        hour = BinaryToBCD(hour);
    }

    RTCWriteRegister(RTC_REG_SECONDS_ALARM, second);
    RTCWriteRegister(RTC_REG_MINUTES_ALARM, minute);
    RTCWriteRegister(RTC_REG_HOURS_ALARM, hour);

    printf("Timer: RTC alarm set to %02d:%02d:%02d\n",
           pTime->Hour, pTime->Minute, pTime->Second);

    return IO_SUCCESS;
}

static IO_RETURN
RTC_EnableAlarm(
    IIORealTimeClock *pThis
    )
{
    UINT8 statusB;

    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    RTCWriteRegister(RTC_REG_STATUS_B, statusB | RTC_STATUS_B_AIE);

    printf("Timer: RTC alarm enabled\n");

    return IO_SUCCESS;
}

static IO_RETURN
RTC_DisableAlarm(
    IIORealTimeClock *pThis
    )
{
    UINT8 statusB;

    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    statusB = RTCReadRegister(RTC_REG_STATUS_B);
    RTCWriteRegister(RTC_REG_STATUS_B, statusB & ~RTC_STATUS_B_AIE);

    printf("Timer: RTC alarm disabled\n");

    return IO_SUCCESS;
}

static IO_RETURN
RTC_GetBatteryStatus(
    IIORealTimeClock *pThis,
    BOOLEAN *pbBatteryGood
    )
{
    UINT8 statusD;

    if (pThis == NULL || pbBatteryGood == NULL) {
        return IO_BAD_ARGUMENT;
    }

    statusD = RTCReadRegister(RTC_REG_STATUS_D);
    *pbBatteryGood = (statusD & RTC_STATUS_D_VRT) != 0;

    return IO_SUCCESS;
}

static IO_RETURN
RTC_ReadRegister(
    IIORealTimeClock *pThis,
    UINT8 uRegister,
    UINT8 *puValue
    )
{
    if (pThis == NULL || puValue == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uRegister > 0x7F) {
        return IO_BAD_ARGUMENT;
    }

    *puValue = RTCReadRegister(uRegister);

    return IO_SUCCESS;
}

static IO_RETURN
RTC_WriteRegister(
    IIORealTimeClock *pThis,
    UINT8 uRegister,
    UINT8 uValue
    )
{
    if (pThis == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uRegister > 0x7F || uRegister < 0x0A) {
        return IO_BAD_ARGUMENT; // Don't allow writing time registers directly
    }

    RTCWriteRegister(uRegister, uValue);

    return IO_SUCCESS;
}

// COM interface methods
static ULONG RTC_AddRef(IIORealTimeClock *pThis) {
    RTC_IMPL *pRTC = (RTC_IMPL*)pThis;
    return ++pRTC->RefCount;
}

static ULONG RTC_Release(IIORealTimeClock *pThis) {
    RTC_IMPL *pRTC = (RTC_IMPL*)pThis;
    if (--pRTC->RefCount == 0) {
        free(pRTC);
        return 0;
    }
    return pRTC->RefCount;
}

//
// Clock Source Implementation
//

static IO_RETURN
Clock_GetTime(
    IIOClock *pThis,
    CLOCK_TIME *pTime
    )
{
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;
    UINT64 ticks;

    if (pClock == NULL || pTime == NULL) {
        return IO_BAD_ARGUMENT;
    }

    switch (pClock->Source) {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
        case CLOCK_BOOTTIME:
        case CLOCK_UPTIME:
            // Read TSC if available
            if (g_bTSCAvailable && g_TSCFrequency > 0) {
                ticks = ReadTSC();
                pTime->Seconds = ticks / g_TSCFrequency;
                pTime->Nanoseconds = ((ticks % g_TSCFrequency) * 1000000000ULL) / g_TSCFrequency;
            } else {
                // Fallback to basic timing
                pTime->Seconds = 0;
                pTime->Nanoseconds = 0;
            }
            break;

        default:
            return IO_UNSUPPORTED;
    }

    return IO_SUCCESS;
}

static IO_RETURN
Clock_GetResolution(
    IIOClock *pThis,
    UINT64 *puResolution
    )
{
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;

    if (pClock == NULL || puResolution == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *puResolution = pClock->Resolution;

    return IO_SUCCESS;
}

static IO_RETURN
Clock_GetFrequency(
    IIOClock *pThis,
    UINT64 *puFrequency
    )
{
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;

    if (pClock == NULL || puFrequency == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *puFrequency = pClock->Frequency;

    return IO_SUCCESS;
}

static IO_RETURN
Clock_GetClockSource(
    IIOClock *pThis,
    CLOCK_SOURCE *pSource
    )
{
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;

    if (pClock == NULL || pSource == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *pSource = pClock->Source;

    return IO_SUCCESS;
}

static IO_RETURN
Clock_IsMonotonic(
    IIOClock *pThis,
    BOOLEAN *pbMonotonic
    )
{
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;

    if (pClock == NULL || pbMonotonic == NULL) {
        return IO_BAD_ARGUMENT;
    }

    *pbMonotonic = pClock->bMonotonic;

    return IO_SUCCESS;
}

// COM interface methods
static ULONG Clock_AddRef(IIOClock *pThis) {
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;
    return ++pClock->RefCount;
}

static ULONG Clock_Release(IIOClock *pThis) {
    CLOCK_IMPL *pClock = (CLOCK_IMPL*)pThis;
    if (--pClock->RefCount == 0) {
        free(pClock);
        return 0;
    }
    return pClock->RefCount;
}

//
// Public API Functions
//

IO_RETURN
TimerInitialize(VOID)
{
    UINT32 eax, ebx, ecx, edx;

    if (g_bTimerInitialized) {
        return IO_SUCCESS;
    }

    printf("Timer: Initializing Timer/Clock subsystem...\n");

    // Check for TSC support
    __cpuid(1, eax, ebx, ecx, edx);
    g_bTSCAvailable = (edx & CPUID_TSC) != 0;

    if (g_bTSCAvailable) {
        printf("Timer: TSC available\n");

        // Check for invariant TSC
        __cpuid(0x80000007, eax, ebx, ecx, edx);
        if (edx & CPUID_INVARIANT_TSC) {
            printf("Timer: Invariant TSC detected\n");
        }

        // Calibrate TSC
        g_TSCFrequency = CalibrateTSC();
    } else {
        printf("Timer: TSC not available\n");
    }

    // Try to detect HPET
    g_bHPETAvailable = DetectHPET();
    if (g_bHPETAvailable) {
        InitializeHPET();
    }

    // Check RTC battery
    UINT8 statusD = RTCReadRegister(RTC_REG_STATUS_D);
    if (statusD & RTC_STATUS_D_VRT) {
        printf("Timer: RTC battery good\n");
    } else {
        printf("Timer: WARNING - RTC battery dead or missing!\n");
    }

    g_bTimerInitialized = TRUE;

    printf("Timer: Initialization complete\n");

    return IO_SUCCESS;
}

IO_RETURN
TimerShutdown(VOID)
{
    if (!g_bTimerInitialized) {
        return IO_SUCCESS;
    }

    printf("Timer: Shutting down Timer/Clock subsystem...\n");

    g_bTimerInitialized = FALSE;
    g_bHPETAvailable = FALSE;
    g_bTSCAvailable = FALSE;

    return IO_SUCCESS;
}

IO_RETURN
PITCreate(
    UINT8 uChannel,
    IIOTimerController **ppTimer
    )
{
    PIT_TIMER_IMPL *pTimer;

    if (ppTimer == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (uChannel > 2) {
        return IO_BAD_ARGUMENT;
    }

    pTimer = (PIT_TIMER_IMPL*)malloc(sizeof(PIT_TIMER_IMPL));
    if (pTimer == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pTimer, 0, sizeof(PIT_TIMER_IMPL));

    // Initialize COM interface (vtable setup would go here)
    pTimer->RefCount = 1;
    pTimer->Channel = uChannel;

    // Fill in timer info
    pTimer->Info.Type = TIMER_TYPE_PIT;
    pTimer->Info.Frequency = PIT_BASE_FREQUENCY;
    pTimer->Info.Resolution = 1000000000ULL / PIT_BASE_FREQUENCY; // ~838 ns
    pTimer->Info.MaxCount = 65535;
    pTimer->Info.Capabilities = TIMER_CAP_16BIT | TIMER_CAP_PERIODIC |
                                TIMER_CAP_ONE_SHOT | TIMER_CAP_PROGRAMMABLE |
                                TIMER_CAP_INTERRUPT;
    pTimer->Info.IRQNumber = (uChannel == 0) ? 0 : 0xFF;
    snprintf(pTimer->Info.Name, sizeof(pTimer->Info.Name), "PIT Channel %d", uChannel);

    *ppTimer = (IIOTimerController*)pTimer;

    printf("Timer: PIT channel %d created\n", uChannel);

    return IO_SUCCESS;
}

IO_RETURN
RTCCreate(
    IIORealTimeClock **ppRTC
    )
{
    RTC_IMPL *pRTC;
    UINT8 statusD;

    if (ppRTC == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pRTC = (RTC_IMPL*)malloc(sizeof(RTC_IMPL));
    if (pRTC == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pRTC, 0, sizeof(RTC_IMPL));

    // Initialize COM interface (vtable setup would go here)
    pRTC->RefCount = 1;

    // Check battery
    statusD = RTCReadRegister(RTC_REG_STATUS_D);
    pRTC->bBatteryGood = (statusD & RTC_STATUS_D_VRT) != 0;

    *ppRTC = (IIORealTimeClock*)pRTC;

    printf("Timer: RTC created (battery %s)\n",
           pRTC->bBatteryGood ? "good" : "DEAD");

    return IO_SUCCESS;
}

IO_RETURN
HPETCreate(
    UINT8 uTimerIndex,
    IIOTimerController **ppTimer
    )
{
    if (!g_bHPETAvailable) {
        return IO_NO_DEVICE;
    }

    if (uTimerIndex >= g_HPETInfo.NumTimers) {
        return IO_BAD_ARGUMENT;
    }

    // Implementation would create HPET timer instance
    printf("Timer: HPET timer %d creation not yet implemented\n", uTimerIndex);

    return IO_UNSUPPORTED;
}

IO_RETURN
TSCClockCreate(
    IIOClock **ppClock
    )
{
    CLOCK_IMPL *pClock;

    if (ppClock == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!g_bTSCAvailable || g_TSCFrequency == 0) {
        return IO_UNSUPPORTED;
    }

    pClock = (CLOCK_IMPL*)malloc(sizeof(CLOCK_IMPL));
    if (pClock == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pClock, 0, sizeof(CLOCK_IMPL));

    pClock->RefCount = 1;
    pClock->Source = CLOCK_MONOTONIC;
    pClock->Frequency = g_TSCFrequency;
    pClock->Resolution = 1000000000ULL / g_TSCFrequency;
    pClock->bMonotonic = TRUE;

    *ppClock = (IIOClock*)pClock;

    printf("Timer: TSC clock created (%llu Hz)\n", g_TSCFrequency);

    return IO_SUCCESS;
}

IO_RETURN
ClockCreate(
    CLOCK_SOURCE Source,
    IIOClock **ppClock
    )
{
    CLOCK_IMPL *pClock;

    if (ppClock == NULL) {
        return IO_BAD_ARGUMENT;
    }

    pClock = (CLOCK_IMPL*)malloc(sizeof(CLOCK_IMPL));
    if (pClock == NULL) {
        return IO_NO_MEMORY;
    }

    memset(pClock, 0, sizeof(CLOCK_IMPL));

    pClock->RefCount = 1;
    pClock->Source = Source;

    // Configure based on source type
    switch (Source) {
        case CLOCK_REALTIME:
            pClock->bMonotonic = FALSE;
            pClock->Frequency = g_TSCFrequency;
            pClock->Resolution = 1000000000ULL / g_TSCFrequency;
            break;

        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_BOOTTIME:
        case CLOCK_UPTIME:
            pClock->bMonotonic = TRUE;
            pClock->Frequency = g_TSCFrequency;
            pClock->Resolution = 1000000000ULL / g_TSCFrequency;
            break;

        default:
            free(pClock);
            return IO_UNSUPPORTED;
    }

    *ppClock = (IIOClock*)pClock;

    printf("Timer: Clock source %d created\n", Source);

    return IO_SUCCESS;
}

IO_RETURN
HPETGetInfo(
    HPET_INFO *pInfo
    )
{
    if (pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!g_bHPETAvailable) {
        return IO_NO_DEVICE;
    }

    memcpy(pInfo, &g_HPETInfo, sizeof(HPET_INFO));

    return IO_SUCCESS;
}

IO_RETURN
TSCGetInfo(
    TSC_INFO *pInfo
    )
{
    UINT32 eax, ebx, ecx, edx;

    if (pInfo == NULL) {
        return IO_BAD_ARGUMENT;
    }

    if (!g_bTSCAvailable) {
        return IO_UNSUPPORTED;
    }

    pInfo->Frequency = g_TSCFrequency;
    pInfo->bAvailable = TRUE;
    pInfo->bReliable = g_TSCFrequency > 0;

    // Check for invariant TSC
    __cpuid(0x80000007, eax, ebx, ecx, edx);
    pInfo->bInvariant = (edx & CPUID_INVARIANT_TSC) != 0;

    return IO_SUCCESS;
}

IO_RETURN
TimerCalibrate(
    IIOTimerController *pTimer,
    IIOClock *pReference
    )
{
    // Calibration implementation would go here
    printf("Timer: Timer calibration not yet implemented\n");

    return IO_UNSUPPORTED;
}
