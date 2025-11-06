# Timer/Clock Family for IOKit

## Overview

The Timer/Clock family provides comprehensive timer and clock management for the NUX operating system. This family supports a wide range of hardware timers, real-time clocks, and high-precision timing sources.

## Supported Hardware

### Timers
- **PIT (8253/8254)** - Programmable Interval Timer
  - 3 channels (16-bit counters)
  - Base frequency: 1.193182 MHz
  - Supports 6 operating modes
  - Channel 0: System timer (IRQ 0)
  - Channel 1: Legacy (unused)
  - Channel 2: PC speaker

- **HPET** - High Precision Event Timer
  - Multiple independent timers (hardware dependent)
  - Femtosecond precision (typically 10-100 MHz)
  - 64-bit counters
  - Can replace PIT and RTC for legacy compatibility

- **TSC** - Time Stamp Counter
  - CPU cycle counter
  - Read via RDTSC instruction
  - Invariant TSC support (constant rate)
  - Highest precision available

- **APIC Timer** - Local APIC Timer
  - Per-CPU timer
  - One-shot and periodic modes
  - Typically used for scheduling

- **ACPI PM Timer** - ACPI Power Management Timer
  - 24/32-bit counter
  - 3.579545 MHz fixed frequency
  - Always running, even in low power states

### Real-Time Clock
- **MC146818 (CMOS RTC)**
  - Battery-backed real-time clock
  - Date/time storage (year, month, day, hour, minute, second)
  - Alarm functionality
  - Periodic interrupt (up to 8192 Hz)
  - BCD or binary mode
  - 12-hour or 24-hour mode

## Interfaces

### IIOTimerController
Generic timer/counter controller interface.

**Key Methods:**
- `GetTimerInfo()` - Get timer capabilities and configuration
- `StartTimer()` - Start timer (one-shot or periodic)
- `StopTimer()` - Stop timer
- `ResetTimer()` - Reset counter to zero
- `SetFrequency()` - Set timer frequency (if programmable)
- `GetCount()` - Read current counter value
- `SetCount()` - Set counter value
- `RegisterCallback()` - Register interrupt callback
- `UnregisterCallback()` - Unregister callback

### IIOClock
Clock source interface for time measurement.

**Key Methods:**
- `GetTime()` - Get current time (seconds + nanoseconds)
- `GetResolution()` - Get clock resolution in nanoseconds
- `GetFrequency()` - Get underlying hardware frequency
- `GetClockSource()` - Get clock source type
- `IsMonotonic()` - Check if clock is monotonic

**Clock Sources:**
- `CLOCK_REALTIME` - Wall clock time (can jump forward/backward)
- `CLOCK_MONOTONIC` - Monotonic time (never goes backward)
- `CLOCK_BOOTTIME` - Time since boot (includes suspend)
- `CLOCK_UPTIME` - Time since boot (excludes suspend)
- `CLOCK_MONOTONIC_RAW` - Monotonic without NTP adjustment
- `CLOCK_PROCESS_CPUTIME` - Per-process CPU time
- `CLOCK_THREAD_CPUTIME` - Per-thread CPU time

### IIORealTimeClock
Real-time clock (RTC) interface.

**Key Methods:**
- `GetTime()` - Read date/time from RTC
- `SetTime()` - Set RTC date/time
- `GetAlarm()` - Get alarm time
- `SetAlarm()` - Set alarm time
- `EnableAlarm()` - Enable alarm interrupt
- `DisableAlarm()` - Disable alarm interrupt
- `GetBatteryStatus()` - Check RTC battery health
- `ReadRegister()` - Read raw RTC register
- `WriteRegister()` - Write raw RTC register

### IIOWatchdog
Watchdog timer interface.

**Key Methods:**
- `StartWatchdog()` - Start watchdog with timeout
- `StopWatchdog()` - Stop watchdog (if supported)
- `ResetWatchdog()` - Reset watchdog ("pet the dog")
- `SetTimeout()` - Set watchdog timeout
- `GetTimeout()` - Get current timeout
- `SetAction()` - Set action on expiration (reset, NMI, etc.)
- `RegisterResetCallback()` - Register pre-reset callback

## Usage Examples

### Example 1: Reading RTC Time

```c
#include <iokit/families/timer/timer.h>

IIORealTimeClock *pRTC = NULL;
RTC_TIME time;

// Create RTC interface
RTCCreate(&pRTC);

// Read current time
IIORealTimeClock_GetTime(pRTC, &time);

printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
       time.Year, time.Month, time.Day,
       time.Hour, time.Minute, time.Second);

// Release interface
pRTC->lpVtbl->Release(pRTC);
```

### Example 2: PIT Periodic Timer

```c
#include <iokit/families/timer/timer.h>

void TimerCallback(void *pContext) {
    printf("Timer tick!\n");
}

IIOTimerController *pTimer = NULL;

// Create PIT channel 0
PITCreate(0, &pTimer);

// Register callback
IIOTimerController_RegisterCallback(pTimer, TimerCallback, NULL);

// Start at 100 Hz (periodic)
UINT64 divisor = 1193182 / 100; // Base frequency / desired frequency
IIOTimerController_StartTimer(pTimer, divisor, TRUE);

// ... timer runs ...

// Stop and release
IIOTimerController_StopTimer(pTimer);
pTimer->lpVtbl->Release(pTimer);
```

### Example 3: High-Precision Timing

```c
#include <iokit/families/timer/timer.h>

IIOClock *pClock = NULL;
CLOCK_TIME start, end;

// Create TSC-based clock
TSCClockCreate(&pClock);

// Measure operation time
IIOClock_GetTime(pClock, &start);

// ... do work ...

IIOClock_GetTime(pClock, &end);

// Calculate elapsed time in nanoseconds
UINT64 elapsedNs = (end.Seconds - start.Seconds) * 1000000000ULL +
                   (end.Nanoseconds - start.Nanoseconds);

printf("Operation took: %llu ns\n", elapsedNs);

pClock->lpVtbl->Release(pClock);
```

### Example 4: Setting RTC Alarm

```c
#include <iokit/families/timer/timer.h>

IIORealTimeClock *pRTC = NULL;
RTC_TIME alarm;

RTCCreate(&pRTC);

// Set alarm for 08:00:00
memset(&alarm, 0, sizeof(RTC_TIME));
alarm.Hour = 8;
alarm.Minute = 0;
alarm.Second = 0;

IIORealTimeClock_SetAlarm(pRTC, &alarm);
IIORealTimeClock_EnableAlarm(pRTC);

pRTC->lpVtbl->Release(pRTC);
```

### Example 5: Clock Sources

```c
#include <iokit/families/timer/timer.h>

IIOClock *pClockRealtime = NULL;
IIOClock *pClockMonotonic = NULL;
CLOCK_TIME time;

// Realtime clock (wall clock)
ClockCreate(CLOCK_REALTIME, &pClockRealtime);
IIOClock_GetTime(pClockRealtime, &time);
printf("Wall time: %llu.%09llu\n", time.Seconds, time.Nanoseconds);

// Monotonic clock (for intervals)
ClockCreate(CLOCK_MONOTONIC, &pClockMonotonic);
IIOClock_GetTime(pClockMonotonic, &time);
printf("Monotonic: %llu.%09llu\n", time.Seconds, time.Nanoseconds);

pClockRealtime->lpVtbl->Release(pClockRealtime);
pClockMonotonic->lpVtbl->Release(pClockMonotonic);
```

## Hardware Details

### PIT (8253/8254) Programming

The PIT has three channels, each with a 16-bit counter. Channel 0 is typically used for the system timer.

**Operating Modes:**
- Mode 0: Interrupt on terminal count
- Mode 1: Hardware retriggerable one-shot
- Mode 2: Rate generator (periodic)
- Mode 3: Square wave generator
- Mode 4: Software triggered strobe
- Mode 5: Hardware triggered strobe

**I/O Ports:**
- 0x40 - Channel 0 data
- 0x41 - Channel 1 data
- 0x42 - Channel 2 data
- 0x43 - Command register

**Base Frequency:** 1.193182 MHz (1193182 Hz)

**Frequency Calculation:**
```
output_freq = 1193182 / divisor
divisor = 1193182 / desired_freq
```

### RTC (MC146818) Register Map

**Time Registers:**
- 0x00 - Seconds (0-59)
- 0x02 - Minutes (0-59)
- 0x04 - Hours (0-23 or 1-12)
- 0x06 - Day of week (1-7)
- 0x07 - Day of month (1-31)
- 0x08 - Month (1-12)
- 0x09 - Year (0-99)
- 0x32 - Century (BCD)

**Alarm Registers:**
- 0x01 - Seconds alarm
- 0x03 - Minutes alarm
- 0x05 - Hours alarm

**Status Registers:**
- 0x0A - Status Register A (update in progress, rate select)
- 0x0B - Status Register B (format, interrupts)
- 0x0C - Status Register C (interrupt flags)
- 0x0D - Status Register D (battery status)

**I/O Ports:**
- 0x70 - Index/address port
- 0x71 - Data port

### HPET Register Map (MMIO)

**General Registers:**
- 0x000 - General Capabilities and ID
- 0x010 - General Configuration
- 0x020 - General Interrupt Status
- 0x0F0 - Main Counter Value

**Timer Registers (per timer, N=0..31):**
- 0x100 + N*0x20 - Timer N Configuration and Capabilities
- 0x108 + N*0x20 - Timer N Comparator Value
- 0x110 + N*0x20 - Timer N FSB Interrupt Route

### TSC (Time Stamp Counter)

**Reading TSC:**
```c
static inline UINT64 ReadTSC(void) {
    UINT32 low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((UINT64)high << 32) | low;
}
```

**Features:**
- Available on all modern x86 CPUs
- Increments at constant rate (on invariant TSC)
- Highest precision timer available
- Per-CPU (may differ between cores)

**Calibration:**
The TSC frequency must be calibrated against a known timer source (typically PIT). The implementation calibrates TSC during initialization.

## Timer Selection Guidelines

**For system timekeeping:**
- Use HPET if available (best precision, no calibration needed)
- Fall back to TSC if calibrated and invariant
- Otherwise use PIT

**For interval measurement:**
- Use TSC for highest precision (< 1 microsecond)
- Use HPET for medium precision (< 1 nanosecond)
- Use CLOCK_MONOTONIC clock source

**For real-time clock:**
- Use RTC for persistent time across reboots
- Sync system time to RTC on boot
- Update RTC when system time changes

**For periodic interrupts:**
- Use HPET timers if available
- Fall back to PIT (channel 0)
- APIC timer for per-CPU events

**For watchdog:**
- Use dedicated hardware watchdog if available
- Can implement software watchdog with HPET/PIT

## Initialization

The timer subsystem must be initialized before use:

```c
IO_RETURN status = TimerInitialize();
if (status != IO_SUCCESS) {
    // Handle error
}

// ... use timers ...

TimerShutdown();
```

**Initialization performs:**
1. Detect TSC and calibrate frequency
2. Detect and initialize HPET (if present)
3. Check RTC battery status
4. Set up default system clock sources

## Thread Safety

All timer interfaces are thread-safe for reading. Writing (setting time, starting timers) should be serialized by the caller when necessary.

## Performance Considerations

**Reading TSC:**
- Fastest: ~20-30 CPU cycles
- No I/O, no memory access
- May require synchronization across CPUs

**Reading HPET:**
- Medium: ~200-500 ns
- Single memory-mapped read
- Atomic, no synchronization needed

**Reading PIT:**
- Slowest: ~1-2 microseconds
- Requires I/O port access
- May need to latch counter first

**Reading RTC:**
- Very slow: ~10-100 microseconds
- Multiple I/O port accesses
- Must wait for update completion

## Architecture Support

**x86/x86_64:**
- PIT (8253/8254) - Universal
- RTC (MC146818) - Universal
- HPET - Modern systems (2005+)
- TSC - All modern CPUs
- APIC Timer - All modern CPUs with APIC

**ARM:**
- ARM Generic Timer - ARMv7-A/ARMv8
- No PIT or RTC equivalent
- Different register layout and access methods

## See Also

- IOKit documentation
- ACPI specification (for HPET)
- Intel 8254 datasheet (PIT)
- MC146818 datasheet (RTC)
- Intel HPET specification

## License

Copyright (c) 2025 NUX Project
