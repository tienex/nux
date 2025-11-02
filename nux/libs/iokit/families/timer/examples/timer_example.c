/**
 * @file timer_example.c
 * @brief Timer/Clock Family Usage Examples
 *
 * Demonstrates various uses of the Timer/Clock family including:
 * - Reading RTC time
 * - Using PIT for periodic interrupts
 * - Clock sources for timing
 * - TSC-based high precision timing
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#include <iokit/IOKit.h>
#include <iokit/families/timer/timer.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Example 1: Read RTC time
 */
static VOID
Example_ReadRTCTime(VOID)
{
    IIORealTimeClock *pRTC = NULL;
    RTC_TIME time;
    BOOLEAN bBatteryGood;
    IO_RETURN status;

    printf("\n=== Example 1: Reading RTC Time ===\n");

    // Create RTC interface
    status = RTCCreate(&pRTC);
    if (status != IO_SUCCESS) {
        printf("Failed to create RTC interface: 0x%08X\n", status);
        return;
    }

    // Check battery status
    status = IIORealTimeClock_GetBatteryStatus(pRTC, &bBatteryGood);
    if (status == IO_SUCCESS) {
        printf("RTC Battery: %s\n", bBatteryGood ? "Good" : "Dead/Missing");
    }

    // Read current time
    status = IIORealTimeClock_GetTime(pRTC, &time);
    if (status == IO_SUCCESS) {
        printf("Current RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
               time.Year, time.Month, time.Day,
               time.Hour, time.Minute, time.Second);
        printf("Day of Week: %d (0=Sunday)\n", time.DayOfWeek);
        printf("Format: %s\n", time.b24Hour ? "24-hour" : "12-hour");
    } else {
        printf("Failed to read RTC time: 0x%08X\n", status);
    }

    // Release interface
    pRTC->lpVtbl->Release(pRTC);
}

/**
 * @brief Example 2: Set RTC alarm
 */
static VOID
Example_SetRTCAlarm(VOID)
{
    IIORealTimeClock *pRTC = NULL;
    RTC_TIME alarm;
    IO_RETURN status;

    printf("\n=== Example 2: Setting RTC Alarm ===\n");

    // Create RTC interface
    status = RTCCreate(&pRTC);
    if (status != IO_SUCCESS) {
        printf("Failed to create RTC interface: 0x%08X\n", status);
        return;
    }

    // Set alarm for 08:00:00
    memset(&alarm, 0, sizeof(RTC_TIME));
    alarm.Hour = 8;
    alarm.Minute = 0;
    alarm.Second = 0;

    status = IIORealTimeClock_SetAlarm(pRTC, &alarm);
    if (status == IO_SUCCESS) {
        printf("Alarm set for 08:00:00\n");

        // Enable alarm interrupt
        status = IIORealTimeClock_EnableAlarm(pRTC);
        if (status == IO_SUCCESS) {
            printf("Alarm interrupt enabled\n");
        }
    } else {
        printf("Failed to set alarm: 0x%08X\n", status);
    }

    // Release interface
    pRTC->lpVtbl->Release(pRTC);
}

/**
 * @brief Example 3: Use PIT for periodic timer
 */
static VOID
PITTimerCallback(VOID *pContext)
{
    static UINT32 tickCount = 0;
    tickCount++;

    if (tickCount % 100 == 0) {
        printf("PIT Timer tick: %u\n", tickCount);
    }
}

static VOID
Example_PITPeriodicTimer(VOID)
{
    IIOTimerController *pTimer = NULL;
    TIMER_INFO info;
    IO_RETURN status;

    printf("\n=== Example 3: PIT Periodic Timer ===\n");

    // Create PIT channel 0 timer
    status = PITCreate(0, &pTimer);
    if (status != IO_SUCCESS) {
        printf("Failed to create PIT timer: 0x%08X\n", status);
        return;
    }

    // Get timer information
    status = IIOTimerController_GetTimerInfo(pTimer, &info);
    if (status == IO_SUCCESS) {
        printf("Timer: %s\n", info.Name);
        printf("Frequency: %llu Hz\n", info.Frequency);
        printf("Resolution: %llu ns\n", info.Resolution);
        printf("Max Count: %llu\n", info.MaxCount);
    }

    // Register callback
    status = IIOTimerController_RegisterCallback(pTimer, PITTimerCallback, NULL);
    if (status == IO_SUCCESS) {
        printf("Callback registered\n");
    }

    // Start timer at 100 Hz (periodic)
    UINT64 divisor = PIT_BASE_FREQUENCY / 100; // 100 Hz
    status = IIOTimerController_StartTimer(pTimer, divisor, TRUE);
    if (status == IO_SUCCESS) {
        printf("Timer started at 100 Hz\n");
    } else {
        printf("Failed to start timer: 0x%08X\n", status);
    }

    // In a real system, you would let this run...
    // For demonstration, we'll just clean up

    // Stop timer
    IIOTimerController_StopTimer(pTimer);
    printf("Timer stopped\n");

    // Release interface
    pTimer->lpVtbl->Release(pTimer);
}

/**
 * @brief Example 4: High-precision timing with TSC
 */
static VOID
Example_TSCTiming(VOID)
{
    IIOClock *pClock = NULL;
    CLOCK_TIME start, end;
    TSC_INFO tscInfo;
    BOOLEAN bMonotonic;
    UINT64 frequency, resolution;
    IO_RETURN status;

    printf("\n=== Example 4: High-Precision TSC Timing ===\n");

    // Get TSC information
    status = TSCGetInfo(&tscInfo);
    if (status == IO_SUCCESS) {
        printf("TSC Frequency: %llu Hz (~%llu MHz)\n",
               tscInfo.Frequency, tscInfo.Frequency / 1000000);
        printf("Invariant TSC: %s\n", tscInfo.bInvariant ? "Yes" : "No");
        printf("Reliable: %s\n", tscInfo.bReliable ? "Yes" : "No");
    } else {
        printf("TSC not available\n");
        return;
    }

    // Create TSC-based clock
    status = TSCClockCreate(&pClock);
    if (status != IO_SUCCESS) {
        printf("Failed to create TSC clock: 0x%08X\n", status);
        return;
    }

    // Get clock properties
    IIOClock_GetFrequency(pClock, &frequency);
    IIOClock_GetResolution(pClock, &resolution);
    IIOClock_IsMonotonic(pClock, &bMonotonic);

    printf("Clock Frequency: %llu Hz\n", frequency);
    printf("Clock Resolution: %llu ns\n", resolution);
    printf("Monotonic: %s\n", bMonotonic ? "Yes" : "No");

    // Measure time for a simple operation
    IIOClock_GetTime(pClock, &start);

    // Do some work (example: simple loop)
    volatile UINT64 sum = 0;
    for (UINT32 i = 0; i < 1000000; i++) {
        sum += i;
    }

    IIOClock_GetTime(pClock, &end);

    // Calculate elapsed time
    UINT64 elapsedNs = (end.Seconds - start.Seconds) * 1000000000ULL +
                       (end.Nanoseconds - start.Nanoseconds);

    printf("Operation took: %llu nanoseconds (%.3f microseconds)\n",
           elapsedNs, elapsedNs / 1000.0);

    // Release interface
    pClock->lpVtbl->Release(pClock);
}

/**
 * @brief Example 5: Using different clock sources
 */
static VOID
Example_ClockSources(VOID)
{
    IIOClock *pClockRealtime = NULL;
    IIOClock *pClockMonotonic = NULL;
    IIOClock *pClockBoottime = NULL;
    CLOCK_TIME time;
    CLOCK_SOURCE source;
    IO_RETURN status;

    printf("\n=== Example 5: Different Clock Sources ===\n");

    // Create realtime clock
    status = ClockCreate(CLOCK_REALTIME, &pClockRealtime);
    if (status == IO_SUCCESS) {
        IIOClock_GetClockSource(pClockRealtime, &source);
        IIOClock_GetTime(pClockRealtime, &time);
        printf("REALTIME: %llu.%09llu seconds\n", time.Seconds, time.Nanoseconds);
        pClockRealtime->lpVtbl->Release(pClockRealtime);
    }

    // Create monotonic clock
    status = ClockCreate(CLOCK_MONOTONIC, &pClockMonotonic);
    if (status == IO_SUCCESS) {
        IIOClock_GetClockSource(pClockMonotonic, &source);
        IIOClock_GetTime(pClockMonotonic, &time);
        printf("MONOTONIC: %llu.%09llu seconds\n", time.Seconds, time.Nanoseconds);
        pClockMonotonic->lpVtbl->Release(pClockMonotonic);
    }

    // Create boottime clock
    status = ClockCreate(CLOCK_BOOTTIME, &pClockBoottime);
    if (status == IO_SUCCESS) {
        IIOClock_GetClockSource(pClockBoottime, &source);
        IIOClock_GetTime(pClockBoottime, &time);
        printf("BOOTTIME: %llu.%09llu seconds (time since boot)\n",
               time.Seconds, time.Nanoseconds);
        pClockBoottime->lpVtbl->Release(pClockBoottime);
    }
}

/**
 * @brief Example 6: HPET information
 */
static VOID
Example_HPETInfo(VOID)
{
    HPET_INFO info;
    IO_RETURN status;

    printf("\n=== Example 6: HPET Information ===\n");

    status = HPETGetInfo(&info);
    if (status == IO_SUCCESS) {
        printf("HPET Base Address: 0x%016llX\n", info.BaseAddress);
        printf("Vendor ID: 0x%04X\n", info.VendorID);
        printf("Number of Timers: %d\n", info.NumTimers);
        printf("Clock Period: %u femtoseconds\n", info.Period);
        printf("Frequency: %llu Hz (~%llu MHz)\n",
               1000000000000000ULL / info.Period,
               1000000000ULL / info.Period);
        printf("64-bit Counter: %s\n", info.b64BitCounter ? "Yes" : "No");
        printf("Legacy Replacement: %s\n", info.bLegacyReplacement ? "Yes" : "No");
    } else {
        printf("HPET not available or not detected\n");
    }
}

/**
 * @brief Main function
 */
int main(int argc, char **argv)
{
    IO_RETURN status;

    printf("Timer/Clock Family Examples\n");
    printf("===========================\n");

    // Initialize timer subsystem
    status = TimerInitialize();
    if (status != IO_SUCCESS) {
        printf("Failed to initialize timer subsystem: 0x%08X\n", status);
        return 1;
    }

    // Run examples
    Example_ReadRTCTime();
    Example_SetRTCAlarm();
    Example_PITPeriodicTimer();
    Example_TSCTiming();
    Example_ClockSources();
    Example_HPETInfo();

    // Shutdown timer subsystem
    TimerShutdown();

    printf("\n=== Examples Complete ===\n");

    return 0;
}
