/**
 * @file timer.h
 * @brief Timer/Clock Family Interface - System Timers, RTC, and High-Precision Timing
 *
 * This header defines the Timer/Clock family interface providing comprehensive
 * timing and clock management capabilities including programmable interval timers,
 * real-time clocks, high-precision event timers, and watchdog timers.
 *
 * The Timer family provides:
 * - PIT (Programmable Interval Timer 8253/8254)
 * - RTC (Real-Time Clock MC146818, CMOS)
 * - HPET (High Precision Event Timer)
 * - TSC (Time Stamp Counter - CPU)
 * - APIC Timer (Local APIC timer)
 * - ACPI PM Timer (Power Management Timer)
 * - ARM Generic Timer
 * - Watchdog timers
 * - Multiple clock sources (system, wall, monotonic, boot time)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_TIMER_H
#define IOKIT_TIMER_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOTimerController interface GUID
 * {A1B2C3D4-E5F6-4A7B-8C9D-0E1F2A3B4C5D}
 */
DEFINE_GUID(IID_IIOTimerController,
    0xA1B2C3D4, 0xE5F6, 0x4A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
 * @brief IIOClock interface GUID
 * {B2C3D4E5-F6A7-4B8C-9D0E-1F2A3B4C5D6E}
 */
DEFINE_GUID(IID_IIOClock,
    0xB2C3D4E5, 0xF6A7, 0x4B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
 * @brief IIORealTimeClock interface GUID
 * {C3D4E5F6-A7B8-4C9D-0E1F-2A3B4C5D6E7F}
 */
DEFINE_GUID(IID_IIORealTimeClock,
    0xC3D4E5F6, 0xA7B8, 0x4C9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
 * @brief IIOWatchdog interface GUID
 * {D4E5F6A7-B8C9-4D0E-1F2A-3B4C5D6E7F8A}
 */
DEFINE_GUID(IID_IIOWatchdog,
    0xD4E5F6A7, 0xB8C9, 0x4D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

//
// Timer Types
//

/**
 * @brief Timer Hardware Types
 */
typedef enum _TIMER_TYPE {
    TIMER_TYPE_PIT          = 0,    /**< Programmable Interval Timer (8253/8254) */
    TIMER_TYPE_RTC          = 1,    /**< Real-Time Clock (MC146818 CMOS) */
    TIMER_TYPE_HPET         = 2,    /**< High Precision Event Timer */
    TIMER_TYPE_TSC          = 3,    /**< Time Stamp Counter (CPU) */
    TIMER_TYPE_APIC         = 4,    /**< Local APIC Timer */
    TIMER_TYPE_ACPI_PM      = 5,    /**< ACPI Power Management Timer */
    TIMER_TYPE_ARM_GENERIC  = 6,    /**< ARM Generic Timer */
    TIMER_TYPE_WATCHDOG     = 7,    /**< Watchdog Timer */
} TIMER_TYPE;

/**
 * @brief Clock Source Types
 */
typedef enum _CLOCK_SOURCE {
    CLOCK_REALTIME          = 0,    /**< Wall clock time (real time, can jump) */
    CLOCK_MONOTONIC         = 1,    /**< Monotonic time (never goes backward) */
    CLOCK_BOOTTIME          = 2,    /**< Time since boot (includes suspend) */
    CLOCK_MONOTONIC_RAW     = 3,    /**< Monotonic time (no NTP adjustment) */
    CLOCK_PROCESS_CPUTIME   = 4,    /**< Per-process CPU time */
    CLOCK_THREAD_CPUTIME    = 5,    /**< Per-thread CPU time */
    CLOCK_UPTIME            = 6,    /**< Time since boot (excludes suspend) */
} CLOCK_SOURCE;

/**
 * @brief Timer Capabilities
 */
#define TIMER_CAP_ONE_SHOT          (1 << 0)    /**< Supports one-shot mode */
#define TIMER_CAP_PERIODIC          (1 << 1)    /**< Supports periodic mode */
#define TIMER_CAP_COUNTDOWN         (1 << 2)    /**< Countdown timer */
#define TIMER_CAP_COUNT_UP          (1 << 3)    /**< Count-up timer */
#define TIMER_CAP_INTERRUPT         (1 << 4)    /**< Can generate interrupts */
#define TIMER_CAP_64BIT             (1 << 5)    /**< 64-bit counter */
#define TIMER_CAP_32BIT             (1 << 6)    /**< 32-bit counter */
#define TIMER_CAP_16BIT             (1 << 7)    /**< 16-bit counter */
#define TIMER_CAP_PROGRAMMABLE      (1 << 8)    /**< Programmable frequency */
#define TIMER_CAP_READ_WHILE_RUNNING (1 << 9)   /**< Can read while running */
#define TIMER_CAP_HIGH_PRECISION    (1 << 10)   /**< High precision (< 1us) */

/**
 * @brief PIT Operating Modes
 */
typedef enum _PIT_MODE {
    PIT_MODE_INTERRUPT_ON_TERMINAL = 0,     /**< Mode 0: Interrupt on terminal count */
    PIT_MODE_HARDWARE_RETRIGGERABLE = 1,    /**< Mode 1: Hardware retriggerable one-shot */
    PIT_MODE_RATE_GENERATOR = 2,            /**< Mode 2: Rate generator */
    PIT_MODE_SQUARE_WAVE = 3,               /**< Mode 3: Square wave generator */
    PIT_MODE_SOFTWARE_STROBE = 4,           /**< Mode 4: Software triggered strobe */
    PIT_MODE_HARDWARE_STROBE = 5,           /**< Mode 5: Hardware triggered strobe */
} PIT_MODE;

/**
 * @brief RTC Register Indices
 */
typedef enum _RTC_REGISTER {
    RTC_REG_SECONDS         = 0x00,     /**< Seconds (0-59) */
    RTC_REG_SECONDS_ALARM   = 0x01,     /**< Seconds alarm */
    RTC_REG_MINUTES         = 0x02,     /**< Minutes (0-59) */
    RTC_REG_MINUTES_ALARM   = 0x03,     /**< Minutes alarm */
    RTC_REG_HOURS           = 0x04,     /**< Hours (0-23 or 1-12) */
    RTC_REG_HOURS_ALARM     = 0x05,     /**< Hours alarm */
    RTC_REG_DAY_OF_WEEK     = 0x06,     /**< Day of week (1-7) */
    RTC_REG_DAY_OF_MONTH    = 0x07,     /**< Day of month (1-31) */
    RTC_REG_MONTH           = 0x08,     /**< Month (1-12) */
    RTC_REG_YEAR            = 0x09,     /**< Year (0-99) */
    RTC_REG_STATUS_A        = 0x0A,     /**< Status Register A */
    RTC_REG_STATUS_B        = 0x0B,     /**< Status Register B */
    RTC_REG_STATUS_C        = 0x0C,     /**< Status Register C */
    RTC_REG_STATUS_D        = 0x0D,     /**< Status Register D */
    RTC_REG_CENTURY         = 0x32,     /**< Century (BCD) */
} RTC_REGISTER;

/**
 * @brief RTC Status Register A Bits
 */
#define RTC_STATUS_A_UIP            (1 << 7)    /**< Update in progress */
#define RTC_STATUS_A_DV_MASK        0x70        /**< Divider select mask */
#define RTC_STATUS_A_RS_MASK        0x0F        /**< Rate select mask */

/**
 * @brief RTC Status Register B Bits
 */
#define RTC_STATUS_B_SET            (1 << 7)    /**< Set time (disable updates) */
#define RTC_STATUS_B_PIE            (1 << 6)    /**< Periodic interrupt enable */
#define RTC_STATUS_B_AIE            (1 << 5)    /**< Alarm interrupt enable */
#define RTC_STATUS_B_UIE            (1 << 4)    /**< Update-ended interrupt enable */
#define RTC_STATUS_B_SQWE           (1 << 3)    /**< Square wave enable */
#define RTC_STATUS_B_DM             (1 << 2)    /**< Data mode (1=binary, 0=BCD) */
#define RTC_STATUS_B_24H            (1 << 1)    /**< 24-hour mode (1=24h, 0=12h) */
#define RTC_STATUS_B_DSE            (1 << 0)    /**< Daylight saving enable */

/**
 * @brief RTC Status Register C Bits (Read-only)
 */
#define RTC_STATUS_C_IRQF           (1 << 7)    /**< Interrupt request flag */
#define RTC_STATUS_C_PF             (1 << 6)    /**< Periodic interrupt flag */
#define RTC_STATUS_C_AF             (1 << 5)    /**< Alarm interrupt flag */
#define RTC_STATUS_C_UF             (1 << 4)    /**< Update-ended interrupt flag */

/**
 * @brief RTC Status Register D Bits (Read-only)
 */
#define RTC_STATUS_D_VRT            (1 << 7)    /**< Valid RAM and time */

/**
 * @brief Timer Information Structure
 */
typedef struct _TIMER_INFO {
    TIMER_TYPE      Type;               /**< Timer type */
    UINT64          Frequency;          /**< Frequency in Hz */
    UINT64          Resolution;         /**< Resolution in nanoseconds */
    UINT64          MaxCount;           /**< Maximum count value */
    UINT32          Capabilities;       /**< Capability flags (TIMER_CAP_*) */
    UINT8           IRQNumber;          /**< IRQ number (if interrupt capable) */
    BOOLEAN         bRunning;           /**< Timer is currently running */
    BOOLEAN         bPeriodicMode;      /**< In periodic mode */
    CHAR8           Name[32];           /**< Timer name */
} TIMER_INFO;

/**
 * @brief Real-Time Clock Time Structure
 */
typedef struct _RTC_TIME {
    UINT16          Year;               /**< Year (1970-2099) */
    UINT8           Month;              /**< Month (1-12) */
    UINT8           Day;                /**< Day of month (1-31) */
    UINT8           Hour;               /**< Hour (0-23) */
    UINT8           Minute;             /**< Minute (0-59) */
    UINT8           Second;             /**< Second (0-59) */
    UINT8           DayOfWeek;          /**< Day of week (0=Sunday, 6=Saturday) */
    UINT8           Century;            /**< Century (19, 20, 21, etc.) */
    BOOLEAN         bDST;               /**< Daylight saving time flag */
    BOOLEAN         b24Hour;            /**< 24-hour mode (vs 12-hour) */
    BOOLEAN         bPM;                /**< PM flag (for 12-hour mode) */
} RTC_TIME;

/**
 * @brief Clock Time Structure (nanosecond precision)
 */
typedef struct _CLOCK_TIME {
    UINT64          Seconds;            /**< Seconds since epoch */
    UINT64          Nanoseconds;        /**< Nanoseconds (0-999999999) */
} CLOCK_TIME;

/**
 * @brief HPET Capabilities
 */
typedef struct _HPET_INFO {
    UINT64          BaseAddress;        /**< MMIO base address */
    UINT32          VendorID;           /**< Vendor ID */
    UINT32          Period;             /**< Clock period in femtoseconds */
    UINT8           NumTimers;          /**< Number of timers */
    UINT8           RevisionID;         /**< Hardware revision */
    BOOLEAN         b64BitCounter;      /**< 64-bit main counter */
    BOOLEAN         bLegacyReplacement; /**< Can replace PIT/RTC */
} HPET_INFO;

/**
 * @brief TSC Information
 */
typedef struct _TSC_INFO {
    UINT64          Frequency;          /**< TSC frequency in Hz */
    BOOLEAN         bInvariant;         /**< Invariant TSC (constant rate) */
    BOOLEAN         bReliable;          /**< Reliable for timekeeping */
    BOOLEAN         bAvailable;         /**< TSC available on all CPUs */
} TSC_INFO;

/**
 * @brief Timer Callback Function Type
 *
 * Called when a timer expires (one-shot) or fires (periodic).
 *
 * @param pContext      User-provided context pointer
 */
typedef VOID (*TIMER_CALLBACK)(
    VOID *pContext
    );

/**
 * @brief Watchdog Action Types
 */
typedef enum _WATCHDOG_ACTION {
    WATCHDOG_ACTION_NONE        = 0,    /**< No action */
    WATCHDOG_ACTION_INTERRUPT   = 1,    /**< Generate interrupt */
    WATCHDOG_ACTION_RESET       = 2,    /**< System reset */
    WATCHDOG_ACTION_POWER_OFF   = 3,    /**< System power off */
    WATCHDOG_ACTION_NMI         = 4,    /**< Non-maskable interrupt */
} WATCHDOG_ACTION;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOTimerController, IIOService);
DECLARE_INTERFACE_(IIOClock, IIOService);
DECLARE_INTERFACE_(IIORealTimeClock, IIOService);
DECLARE_INTERFACE_(IIOWatchdog, IIOService);

/**
 * @brief IIOTimerController - Timer/Counter Controller Interface
 *
 * This interface represents a hardware timer/counter and provides methods
 * for configuring, starting, stopping, and reading timer values.
 */
#undef INTERFACE
#define INTERFACE IIOTimerController

DECLARE_INTERFACE_(IIOTimerController, IIOService)
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

    // IIOTimerController methods

    /**
     * @brief Get timer information
     *
     * Retrieves comprehensive timer information including type, frequency,
     * resolution, and capabilities.
     *
     * @param pTimerInfo    Receives timer information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetTimerInfo)(THIS_
        TIMER_INFO *pTimerInfo
        ) PURE;

    /**
     * @brief Start timer
     *
     * Starts the timer with the specified count value.
     *
     * @param uCount        Count value (timer-specific interpretation)
     * @param bPeriodic     TRUE for periodic mode, FALSE for one-shot
     *
     * @retval IO_SUCCESS       Timer started successfully
     * @retval IO_BUSY          Timer already running
     * @retval IO_BAD_ARGUMENT  Invalid count value
     */
    STDMETHOD_(IO_RETURN, StartTimer)(THIS_
        UINT64 uCount,
        BOOLEAN bPeriodic
        ) PURE;

    /**
     * @brief Stop timer
     *
     * Stops the timer.
     *
     * @retval IO_SUCCESS       Timer stopped successfully
     * @retval IO_NOT_READY     Timer not running
     */
    STDMETHOD_(IO_RETURN, StopTimer)(THIS) PURE;

    /**
     * @brief Reset timer
     *
     * Resets the timer counter to zero.
     *
     * @retval IO_SUCCESS       Timer reset successfully
     */
    STDMETHOD_(IO_RETURN, ResetTimer)(THIS) PURE;

    /**
     * @brief Set timer frequency
     *
     * Sets the timer frequency (for programmable timers).
     *
     * @param uFrequency    Desired frequency in Hz
     *
     * @retval IO_SUCCESS       Frequency set successfully
     * @retval IO_UNSUPPORTED   Timer does not support frequency setting
     * @retval IO_BAD_ARGUMENT  Frequency out of range
     */
    STDMETHOD_(IO_RETURN, SetFrequency)(THIS_
        UINT64 uFrequency
        ) PURE;

    /**
     * @brief Get current count
     *
     * Reads the current timer counter value.
     *
     * @param puCount       Receives current count value
     *
     * @retval IO_SUCCESS       Count retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     Timer not initialized
     */
    STDMETHOD_(IO_RETURN, GetCount)(THIS_
        UINT64 *puCount
        ) PURE;

    /**
     * @brief Set count value
     *
     * Sets the timer counter to a specific value.
     *
     * @param uCount        Count value to set
     *
     * @retval IO_SUCCESS       Count set successfully
     * @retval IO_UNSUPPORTED   Timer does not support setting count
     * @retval IO_BAD_ARGUMENT  Invalid count value
     */
    STDMETHOD_(IO_RETURN, SetCount)(THIS_
        UINT64 uCount
        ) PURE;

    /**
     * @brief Register timer callback
     *
     * Registers a callback to be invoked when the timer expires or fires.
     *
     * @param pfnCallback   Callback function
     * @param pContext      Context pointer passed to callback
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_BAD_ARGUMENT  Invalid callback
     * @retval IO_UNSUPPORTED   Timer does not support interrupts
     */
    STDMETHOD_(IO_RETURN, RegisterCallback)(THIS_
        TIMER_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Unregister timer callback
     *
     * Unregisters a previously registered timer callback.
     *
     * @retval IO_SUCCESS       Callback unregistered successfully
     * @retval IO_NO_MATCH      No callback registered
     */
    STDMETHOD_(IO_RETURN, UnregisterCallback)(THIS) PURE;
};

#undef INTERFACE

/**
 * @brief IIOClock - Clock Source Interface
 *
 * This interface represents a system clock source and provides methods
 * for reading time with nanosecond precision.
 */
#undef INTERFACE
#define INTERFACE IIOClock

DECLARE_INTERFACE_(IIOClock, IIOService)
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

    // IIOClock methods

    /**
     * @brief Get clock time
     *
     * Returns the current time from this clock source with nanosecond precision.
     *
     * @param pTime         Receives current time
     *
     * @retval IO_SUCCESS       Time retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     Clock not initialized
     */
    STDMETHOD_(IO_RETURN, GetTime)(THIS_
        CLOCK_TIME *pTime
        ) PURE;

    /**
     * @brief Get clock resolution
     *
     * Returns the resolution (precision) of this clock in nanoseconds.
     *
     * @param puResolution  Receives resolution in nanoseconds
     *
     * @retval IO_SUCCESS       Resolution retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetResolution)(THIS_
        UINT64 *puResolution
        ) PURE;

    /**
     * @brief Get clock frequency
     *
     * Returns the frequency of the underlying hardware counter.
     *
     * @param puFrequency   Receives frequency in Hz
     *
     * @retval IO_SUCCESS       Frequency retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetFrequency)(THIS_
        UINT64 *puFrequency
        ) PURE;

    /**
     * @brief Get clock source type
     *
     * Returns the type of this clock source.
     *
     * @param pSource       Receives clock source type
     *
     * @retval IO_SUCCESS       Source type retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetClockSource)(THIS_
        CLOCK_SOURCE *pSource
        ) PURE;

    /**
     * @brief Check if clock is monotonic
     *
     * Returns TRUE if this clock never goes backward.
     *
     * @param pbMonotonic   Receives TRUE if monotonic, FALSE otherwise
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, IsMonotonic)(THIS_
        BOOLEAN *pbMonotonic
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIORealTimeClock - Real-Time Clock Interface
 *
 * This interface represents a battery-backed real-time clock (RTC) device
 * and provides methods for reading/setting time, alarms, and battery status.
 */
#undef INTERFACE
#define INTERFACE IIORealTimeClock

DECLARE_INTERFACE_(IIORealTimeClock, IIOService)
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

    // IIORealTimeClock methods

    /**
     * @brief Get RTC time
     *
     * Reads the current date and time from the RTC.
     *
     * @param pTime         Receives current RTC time
     *
     * @retval IO_SUCCESS       Time retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     RTC not ready (update in progress)
     */
    STDMETHOD_(IO_RETURN, GetTime)(THIS_
        RTC_TIME *pTime
        ) PURE;

    /**
     * @brief Set RTC time
     *
     * Sets the RTC date and time.
     *
     * @param pTime         Time to set
     *
     * @retval IO_SUCCESS       Time set successfully
     * @retval IO_BAD_ARGUMENT  Invalid time value
     * @retval IO_NOT_PERMITTED Operation not permitted
     */
    STDMETHOD_(IO_RETURN, SetTime)(THIS_
        CONST RTC_TIME *pTime
        ) PURE;

    /**
     * @brief Get alarm time
     *
     * Reads the currently configured alarm time.
     *
     * @param pTime         Receives alarm time
     *
     * @retval IO_SUCCESS       Alarm time retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   Alarm not supported
     */
    STDMETHOD_(IO_RETURN, GetAlarm)(THIS_
        RTC_TIME *pTime
        ) PURE;

    /**
     * @brief Set alarm time
     *
     * Configures the RTC alarm to trigger at the specified time.
     *
     * @param pTime         Alarm time to set
     *
     * @retval IO_SUCCESS       Alarm set successfully
     * @retval IO_BAD_ARGUMENT  Invalid time value
     * @retval IO_UNSUPPORTED   Alarm not supported
     */
    STDMETHOD_(IO_RETURN, SetAlarm)(THIS_
        CONST RTC_TIME *pTime
        ) PURE;

    /**
     * @brief Enable alarm
     *
     * Enables the RTC alarm interrupt.
     *
     * @retval IO_SUCCESS       Alarm enabled successfully
     * @retval IO_UNSUPPORTED   Alarm not supported
     */
    STDMETHOD_(IO_RETURN, EnableAlarm)(THIS) PURE;

    /**
     * @brief Disable alarm
     *
     * Disables the RTC alarm interrupt.
     *
     * @retval IO_SUCCESS       Alarm disabled successfully
     * @retval IO_UNSUPPORTED   Alarm not supported
     */
    STDMETHOD_(IO_RETURN, DisableAlarm)(THIS) PURE;

    /**
     * @brief Get battery status
     *
     * Checks if the RTC battery is good.
     *
     * @param pbBatteryGood Receives TRUE if battery is good, FALSE if dead/missing
     *
     * @retval IO_SUCCESS       Battery status retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetBatteryStatus)(THIS_
        BOOLEAN *pbBatteryGood
        ) PURE;

    /**
     * @brief Read RTC register
     *
     * Reads a raw RTC register value.
     *
     * @param uRegister     Register index (RTC_REGISTER)
     * @param puValue       Receives register value
     *
     * @retval IO_SUCCESS       Register read successfully
     * @retval IO_BAD_ARGUMENT  Invalid register or argument
     */
    STDMETHOD_(IO_RETURN, ReadRegister)(THIS_
        UINT8 uRegister,
        UINT8 *puValue
        ) PURE;

    /**
     * @brief Write RTC register
     *
     * Writes a raw RTC register value.
     *
     * @param uRegister     Register index (RTC_REGISTER)
     * @param uValue        Value to write
     *
     * @retval IO_SUCCESS       Register written successfully
     * @retval IO_BAD_ARGUMENT  Invalid register
     * @retval IO_NOT_PERMITTED Write not permitted
     */
    STDMETHOD_(IO_RETURN, WriteRegister)(THIS_
        UINT8 uRegister,
        UINT8 uValue
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOWatchdog - Watchdog Timer Interface
 *
 * This interface represents a watchdog timer device that can reset the system
 * if not periodically serviced.
 */
#undef INTERFACE
#define INTERFACE IIOWatchdog

DECLARE_INTERFACE_(IIOWatchdog, IIOService)
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

    // IIOWatchdog methods

    /**
     * @brief Start watchdog
     *
     * Starts the watchdog timer with the specified timeout.
     *
     * @param uTimeoutMs    Timeout in milliseconds
     *
     * @retval IO_SUCCESS       Watchdog started successfully
     * @retval IO_BUSY          Watchdog already running
     * @retval IO_BAD_ARGUMENT  Timeout out of range
     */
    STDMETHOD_(IO_RETURN, StartWatchdog)(THIS_
        UINT32 uTimeoutMs
        ) PURE;

    /**
     * @brief Stop watchdog
     *
     * Stops the watchdog timer (if supported).
     *
     * @retval IO_SUCCESS       Watchdog stopped successfully
     * @retval IO_UNSUPPORTED   Watchdog cannot be stopped once started
     * @retval IO_NOT_READY     Watchdog not running
     */
    STDMETHOD_(IO_RETURN, StopWatchdog)(THIS) PURE;

    /**
     * @brief Reset watchdog (pet the dog)
     *
     * Resets the watchdog timer, preventing it from expiring.
     *
     * @retval IO_SUCCESS       Watchdog reset successfully
     * @retval IO_NOT_READY     Watchdog not running
     */
    STDMETHOD_(IO_RETURN, ResetWatchdog)(THIS) PURE;

    /**
     * @brief Set timeout
     *
     * Sets the watchdog timeout value.
     *
     * @param uTimeoutMs    Timeout in milliseconds
     *
     * @retval IO_SUCCESS       Timeout set successfully
     * @retval IO_BAD_ARGUMENT  Timeout out of range
     * @retval IO_BUSY          Cannot change timeout while running
     */
    STDMETHOD_(IO_RETURN, SetTimeout)(THIS_
        UINT32 uTimeoutMs
        ) PURE;

    /**
     * @brief Get timeout
     *
     * Returns the current watchdog timeout value.
     *
     * @param puTimeoutMs   Receives timeout in milliseconds
     *
     * @retval IO_SUCCESS       Timeout retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetTimeout)(THIS_
        UINT32 *puTimeoutMs
        ) PURE;

    /**
     * @brief Set watchdog action
     *
     * Sets the action to take when the watchdog expires.
     *
     * @param Action        Action to perform (reset, NMI, etc.)
     *
     * @retval IO_SUCCESS       Action set successfully
     * @retval IO_UNSUPPORTED   Action not supported
     */
    STDMETHOD_(IO_RETURN, SetAction)(THIS_
        WATCHDOG_ACTION Action
        ) PURE;

    /**
     * @brief Register reset callback
     *
     * Registers a callback to be invoked before the watchdog resets the system.
     *
     * @param pfnCallback   Callback function
     * @param pContext      Context pointer passed to callback
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_BAD_ARGUMENT  Invalid callback
     */
    STDMETHOD_(IO_RETURN, RegisterResetCallback)(THIS_
        TIMER_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOTimerController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOTimerController_GetTimerInfo(p,a)        (p)->lpVtbl->GetTimerInfo(p,a)
#define IIOTimerController_StartTimer(p,a,b)        (p)->lpVtbl->StartTimer(p,a,b)
#define IIOTimerController_StopTimer(p)             (p)->lpVtbl->StopTimer(p)
#define IIOTimerController_ResetTimer(p)            (p)->lpVtbl->ResetTimer(p)
#define IIOTimerController_SetFrequency(p,a)        (p)->lpVtbl->SetFrequency(p,a)
#define IIOTimerController_GetCount(p,a)            (p)->lpVtbl->GetCount(p,a)
#define IIOTimerController_SetCount(p,a)            (p)->lpVtbl->SetCount(p,a)
#define IIOTimerController_RegisterCallback(p,a,b)  (p)->lpVtbl->RegisterCallback(p,a,b)
#define IIOTimerController_UnregisterCallback(p)    (p)->lpVtbl->UnregisterCallback(p)

#define IIOClock_GetTime(p,a)                       (p)->lpVtbl->GetTime(p,a)
#define IIOClock_GetResolution(p,a)                 (p)->lpVtbl->GetResolution(p,a)
#define IIOClock_GetFrequency(p,a)                  (p)->lpVtbl->GetFrequency(p,a)
#define IIOClock_GetClockSource(p,a)                (p)->lpVtbl->GetClockSource(p,a)
#define IIOClock_IsMonotonic(p,a)                   (p)->lpVtbl->IsMonotonic(p,a)

#define IIORealTimeClock_GetTime(p,a)               (p)->lpVtbl->GetTime(p,a)
#define IIORealTimeClock_SetTime(p,a)               (p)->lpVtbl->SetTime(p,a)
#define IIORealTimeClock_GetAlarm(p,a)              (p)->lpVtbl->GetAlarm(p,a)
#define IIORealTimeClock_SetAlarm(p,a)              (p)->lpVtbl->SetAlarm(p,a)
#define IIORealTimeClock_EnableAlarm(p)             (p)->lpVtbl->EnableAlarm(p)
#define IIORealTimeClock_DisableAlarm(p)            (p)->lpVtbl->DisableAlarm(p)
#define IIORealTimeClock_GetBatteryStatus(p,a)      (p)->lpVtbl->GetBatteryStatus(p,a)
#define IIORealTimeClock_ReadRegister(p,a,b)        (p)->lpVtbl->ReadRegister(p,a,b)
#define IIORealTimeClock_WriteRegister(p,a,b)       (p)->lpVtbl->WriteRegister(p,a,b)

#define IIOWatchdog_StartWatchdog(p,a)              (p)->lpVtbl->StartWatchdog(p,a)
#define IIOWatchdog_StopWatchdog(p)                 (p)->lpVtbl->StopWatchdog(p)
#define IIOWatchdog_ResetWatchdog(p)                (p)->lpVtbl->ResetWatchdog(p)
#define IIOWatchdog_SetTimeout(p,a)                 (p)->lpVtbl->SetTimeout(p,a)
#define IIOWatchdog_GetTimeout(p,a)                 (p)->lpVtbl->GetTimeout(p,a)
#define IIOWatchdog_SetAction(p,a)                  (p)->lpVtbl->SetAction(p,a)
#define IIOWatchdog_RegisterResetCallback(p,a,b)    (p)->lpVtbl->RegisterResetCallback(p,a,b)

#endif

/**
 * @brief Initialize Timer/Clock family subsystem
 *
 * Initializes the timer/clock subsystem, detects available timers,
 * and sets up the system clock sources.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
TimerInitialize(
    VOID
    );

/**
 * @brief Shutdown Timer/Clock family subsystem
 *
 * Shuts down the timer/clock subsystem and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
TimerShutdown(
    VOID
    );

/**
 * @brief Create PIT timer controller
 *
 * Creates a PIT (Programmable Interval Timer) controller interface.
 *
 * @param uChannel      PIT channel (0-2)
 * @param ppTimer       Receives timer interface
 *
 * @retval IO_SUCCESS       Timer created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_BAD_ARGUMENT  Invalid channel
 */
IO_RETURN
PITCreate(
    UINT8 uChannel,
    IIOTimerController **ppTimer
    );

/**
 * @brief Create RTC interface
 *
 * Creates a Real-Time Clock interface for the CMOS RTC.
 *
 * @param ppRTC         Receives RTC interface
 *
 * @retval IO_SUCCESS       RTC created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     RTC not present
 */
IO_RETURN
RTCCreate(
    IIORealTimeClock **ppRTC
    );

/**
 * @brief Create HPET timer controller
 *
 * Creates an HPET (High Precision Event Timer) controller interface.
 *
 * @param uTimerIndex   HPET timer index (0-based)
 * @param ppTimer       Receives timer interface
 *
 * @retval IO_SUCCESS       Timer created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_NO_DEVICE     HPET not present
 * @retval IO_BAD_ARGUMENT  Invalid timer index
 */
IO_RETURN
HPETCreate(
    UINT8 uTimerIndex,
    IIOTimerController **ppTimer
    );

/**
 * @brief Create TSC clock source
 *
 * Creates a TSC (Time Stamp Counter) based clock source.
 *
 * @param ppClock       Receives clock interface
 *
 * @retval IO_SUCCESS       Clock created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_UNSUPPORTED   TSC not available or not reliable
 */
IO_RETURN
TSCClockCreate(
    IIOClock **ppClock
    );

/**
 * @brief Create system clock source
 *
 * Creates a system clock source of the specified type.
 *
 * @param Source        Clock source type
 * @param ppClock       Receives clock interface
 *
 * @retval IO_SUCCESS       Clock created successfully
 * @retval IO_NO_MEMORY     Insufficient memory
 * @retval IO_UNSUPPORTED   Clock source not available
 */
IO_RETURN
ClockCreate(
    CLOCK_SOURCE Source,
    IIOClock **ppClock
    );

/**
 * @brief Get HPET information
 *
 * Retrieves HPET hardware information.
 *
 * @param pInfo         Receives HPET information
 *
 * @retval IO_SUCCESS       Information retrieved successfully
 * @retval IO_NO_DEVICE     HPET not present
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
HPETGetInfo(
    HPET_INFO *pInfo
    );

/**
 * @brief Get TSC information
 *
 * Retrieves TSC hardware information.
 *
 * @param pInfo         Receives TSC information
 *
 * @retval IO_SUCCESS       Information retrieved successfully
 * @retval IO_UNSUPPORTED   TSC not available
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
TSCGetInfo(
    TSC_INFO *pInfo
    );

/**
 * @brief Calibrate timer
 *
 * Calibrates a timer against a reference clock source.
 *
 * @param pTimer        Timer to calibrate
 * @param pReference    Reference clock (may be NULL for auto-select)
 *
 * @retval IO_SUCCESS       Calibration successful
 * @retval IO_ERROR         Calibration failed
 */
IO_RETURN
TimerCalibrate(
    IIOTimerController *pTimer,
    IIOClock *pReference
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_TIMER_H */
