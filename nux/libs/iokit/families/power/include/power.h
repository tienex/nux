/**
 * @file power.h
 * @brief Power Management Family Interface - ACPI, Battery, and Thermal Management
 *
 * This header defines the Power Management family interface providing comprehensive
 * power management capabilities including ACPI (Advanced Configuration and Power
 * Interface), battery management, thermal monitoring, and CPU frequency scaling.
 *
 * The Power family provides:
 * - ACPI 1.0-6.5 support with table parsing and AML method execution
 * - Advanced Power Management (APM) legacy support
 * - Battery device management (Smart Battery, ACPI Battery)
 * - Thermal zone monitoring and fan control
 * - CPU frequency and voltage scaling (P-states, C-states)
 * - System power state transitions (S0-S5, G0-G3)
 * - Device power state management (D0-D3)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_POWER_H
#define IOKIT_POWER_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOPowerManager interface GUID
 * {D1E2F3A4-B5C6-4D7E-8F9A-0B1C2D3E4F5A}
 */
DEFINE_GUID(IID_IIOPowerManager,
    0xD1E2F3A4, 0xB5C6, 0x4D7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A);

/**
 * @brief IIOACPIController interface GUID
 * {E2F3A4B5-C6D7-4E8F-9A0B-1C2D3E4F5A6B}
 */
DEFINE_GUID(IID_IIOACPIController,
    0xE2F3A4B5, 0xC6D7, 0x4E8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B);

/**
 * @brief IIOBattery interface GUID
 * {F3A4B5C6-D7E8-4F9A-0B1C-2D3E4F5A6B7C}
 */
DEFINE_GUID(IID_IIOBattery,
    0xF3A4B5C6, 0xD7E8, 0x4F9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C);

/**
 * @brief IIOThermal interface GUID
 * {A4B5C6D7-E8F9-4A0B-1C2D-3E4F5A6B7C8D}
 */
DEFINE_GUID(IID_IIOThermal,
    0xA4B5C6D7, 0xE8F9, 0x4A0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D);

//
// ACPI Versions
//

#define ACPI_VERSION_1_0        0x0100      /**< ACPI 1.0 */
#define ACPI_VERSION_2_0        0x0200      /**< ACPI 2.0 */
#define ACPI_VERSION_3_0        0x0300      /**< ACPI 3.0 */
#define ACPI_VERSION_4_0        0x0400      /**< ACPI 4.0 */
#define ACPI_VERSION_5_0        0x0500      /**< ACPI 5.0 */
#define ACPI_VERSION_6_0        0x0600      /**< ACPI 6.0 */
#define ACPI_VERSION_6_5        0x0605      /**< ACPI 6.5 (latest) */

//
// ACPI Table Signatures (4-byte)
//

#define ACPI_SIG_RSDP           "RSD PTR "  /**< Root System Description Pointer */
#define ACPI_SIG_RSDT           "RSDT"      /**< Root System Description Table */
#define ACPI_SIG_XSDT           "XSDT"      /**< Extended System Description Table */
#define ACPI_SIG_FADT           "FACP"      /**< Fixed ACPI Description Table */
#define ACPI_SIG_FACS           "FACS"      /**< Firmware ACPI Control Structure */
#define ACPI_SIG_DSDT           "DSDT"      /**< Differentiated System Description Table */
#define ACPI_SIG_SSDT           "SSDT"      /**< Secondary System Description Table */
#define ACPI_SIG_MADT           "APIC"      /**< Multiple APIC Description Table */
#define ACPI_SIG_HPET           "HPET"      /**< High Precision Event Timer */
#define ACPI_SIG_MCFG           "MCFG"      /**< PCI Express Memory Mapped Config */
#define ACPI_SIG_SRAT           "SRAT"      /**< System Resource Affinity Table */
#define ACPI_SIG_SLIT           "SLIT"      /**< System Locality Information Table */
#define ACPI_SIG_BGRT           "BGRT"      /**< Boot Graphics Resource Table */
#define ACPI_SIG_FPDT           "FPDT"      /**< Firmware Performance Data Table */

//
// System Power States (ACPI Global States)
//

typedef enum _SYSTEM_POWER_STATE {
    PowerSystemWorking      = 0,    /**< G0/S0 - Working (fully on) */
    PowerSystemSleeping1    = 1,    /**< S1 - Power on Suspend (CPU stopped, RAM powered) */
    PowerSystemSleeping2    = 2,    /**< S2 - CPU powered off, deeper than S1 */
    PowerSystemSleeping3    = 3,    /**< S3 - Suspend to RAM (STR) */
    PowerSystemHibernate    = 4,    /**< S4 - Suspend to Disk (STD/Hibernate) */
    PowerSystemSoftOff      = 5,    /**< S5 - Soft Off (OS shutdown, wake events work) */
    PowerSystemMechanicalOff = 6,   /**< G3 - Mechanical Off (no power, must use power button) */
    PowerSystemShutdown     = 7,    /**< Forced shutdown */
    PowerSystemMaximum      = 8     /**< Maximum value */
} SYSTEM_POWER_STATE;

//
// Device Power States (ACPI Device States)
//

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceD0           = 0,    /**< D0 - Fully On */
    PowerDeviceD1           = 1,    /**< D1 - Intermediate state (device-specific) */
    PowerDeviceD2           = 2,    /**< D2 - Intermediate state (device-specific) */
    PowerDeviceD3Hot        = 3,    /**< D3hot - Off but can wake system */
    PowerDeviceD3Cold       = 4,    /**< D3cold - Off, no power to device */
    PowerDeviceMaximum      = 5     /**< Maximum value */
} DEVICE_POWER_STATE;

//
// CPU Power States (ACPI Processor States)
//

typedef enum _CPU_POWER_STATE {
    CpuPowerC0              = 0,    /**< C0 - Active (executing) */
    CpuPowerC1              = 1,    /**< C1 - Halt (CPU stopped, caches maintained) */
    CpuPowerC2              = 2,    /**< C2 - Stop Clock (deeper than C1) */
    CpuPowerC3              = 3,    /**< C3 - Deep Sleep (caches may be flushed) */
    CpuPowerC6              = 6,    /**< C6 - Deep Power Down */
    CpuPowerC7              = 7,    /**< C7 - Deeper sleep */
    CpuPowerC8              = 8,    /**< C8 - Even deeper */
    CpuPowerC9              = 9,    /**< C9 - Extended state */
    CpuPowerC10             = 10,   /**< C10 - Maximum power savings */
} CPU_POWER_STATE;

//
// ACPI Capability Flags
//

#define ACPI_CAP_S1_SUPPORTED           0x00000001  /**< S1 sleep state supported */
#define ACPI_CAP_S2_SUPPORTED           0x00000002  /**< S2 sleep state supported */
#define ACPI_CAP_S3_SUPPORTED           0x00000004  /**< S3 sleep state supported */
#define ACPI_CAP_S4_SUPPORTED           0x00000008  /**< S4 hibernate supported */
#define ACPI_CAP_S5_SUPPORTED           0x00000010  /**< S5 soft off supported */
#define ACPI_CAP_C1_SUPPORTED           0x00000020  /**< C1 C-state supported */
#define ACPI_CAP_C2_SUPPORTED           0x00000040  /**< C2 C-state supported */
#define ACPI_CAP_C3_SUPPORTED           0x00000080  /**< C3 C-state supported */
#define ACPI_CAP_PSTATES_SUPPORTED      0x00000100  /**< P-states (freq scaling) supported */
#define ACPI_CAP_FAN_CONTROL            0x00000200  /**< Fan control available */
#define ACPI_CAP_LID_SWITCH             0x00000400  /**< Lid switch present */
#define ACPI_CAP_POWER_BUTTON           0x00000800  /**< Power button present */
#define ACPI_CAP_SLEEP_BUTTON           0x00001000  /**< Sleep button present */
#define ACPI_CAP_THERMAL_ZONES          0x00002000  /**< Thermal zones present */
#define ACPI_CAP_BATTERY_PRESENT        0x00004000  /**< Battery present */
#define ACPI_CAP_AC_ADAPTER             0x00008000  /**< AC adapter present */
#define ACPI_CAP_RTC_WAKE               0x00010000  /**< RTC wake supported */
#define ACPI_CAP_PCI_EXPRESS_WAKE       0x00020000  /**< PCIe wake supported */

//
// Battery Types
//

typedef enum _BATTERY_TYPE {
    BatteryTypeUnknown      = 0,    /**< Unknown battery type */
    BatteryTypeLiIon        = 1,    /**< Lithium-Ion */
    BatteryTypeLiPoly       = 2,    /**< Lithium-Polymer */
    BatteryTypeNiMH         = 3,    /**< Nickel-Metal Hydride */
    BatteryTypeNiCd         = 4,    /**< Nickel-Cadmium (legacy) */
    BatteryTypeLiFePO4      = 5,    /**< Lithium Iron Phosphate */
} BATTERY_TYPE;

//
// Battery States
//

typedef enum _BATTERY_STATE {
    BatteryStateUnknown     = 0,    /**< Unknown state */
    BatteryStateCharging    = 1,    /**< Battery charging */
    BatteryStateDischarging = 2,    /**< Battery discharging */
    BatteryStateFull        = 3,    /**< Battery fully charged */
    BatteryStateNotPresent  = 4,    /**< Battery not present */
    BatteryStateError       = 5,    /**< Battery error */
} BATTERY_STATE;

//
// Battery Health Status
//

typedef enum _BATTERY_HEALTH {
    BatteryHealthGood       = 0,    /**< Battery health is good */
    BatteryHealthWeak       = 1,    /**< Battery is weak/degraded */
    BatteryHealthReplace    = 2,    /**< Battery should be replaced */
    BatteryHealthFailed     = 3,    /**< Battery has failed */
    BatteryHealthUnknown    = 4,    /**< Battery health unknown */
} BATTERY_HEALTH;

//
// Thermal State
//

typedef enum _THERMAL_STATE {
    ThermalStateNormal      = 0,    /**< Temperature normal */
    ThermalStateWarning     = 1,    /**< Temperature elevated */
    ThermalStateCritical    = 2,    /**< Temperature critical */
    ThermalStateEmergency   = 3,    /**< Emergency shutdown required */
} THERMAL_STATE;

//
// Cooling Policy
//

typedef enum _COOLING_POLICY {
    CoolingPolicyPassive    = 0,    /**< Passive cooling (throttle CPU) */
    CoolingPolicyActive     = 1,    /**< Active cooling (run fans) */
    CoolingPolicyAggressive = 2,    /**< Aggressive cooling (max fans) */
} COOLING_POLICY;

/**
 * @brief ACPI Capability Information
 *
 * Describes the power management capabilities supported by the system's
 * ACPI implementation.
 */
typedef struct _ACPI_CAPABILITY {
    UINT32              ACPIVersion;        /**< ACPI version (e.g., 0x0605 for 6.5) */
    UINT32              Capabilities;       /**< Capability flags (ACPI_CAP_*) */
    BOOLEAN             bACPIEnabled;       /**< ACPI is enabled */
    BOOLEAN             bAPMPresent;        /**< APM BIOS present (legacy) */
    UINT8               NumProcessors;      /**< Number of processors */
    UINT8               NumBatteries;       /**< Number of batteries */
    UINT8               NumThermalZones;    /**< Number of thermal zones */
    UINT8               NumFans;            /**< Number of fans */
} ACPI_CAPABILITY;

/**
 * @brief Battery Information
 *
 * Comprehensive information about a battery device including capacity,
 * voltage, current, health status, and charge state.
 */
typedef struct _BATTERY_INFO {
    // Type and Identification
    BATTERY_TYPE        Type;               /**< Battery chemistry type */
    CHAR8               Manufacturer[32];   /**< Manufacturer name */
    CHAR8               Model[32];          /**< Model number */
    CHAR8               SerialNumber[32];   /**< Serial number */

    // Capacity (in mWh or mAh)
    UINT32              DesignCapacity;     /**< Design capacity */
    UINT32              FullChargeCapacity; /**< Full charge capacity (current max) */
    UINT32              RemainingCapacity;  /**< Current remaining capacity */

    // Voltage (in millivolts)
    UINT32              DesignVoltage;      /**< Design voltage (mV) */
    UINT32              CurrentVoltage;     /**< Current voltage (mV) */

    // Current (in milliamps)
    INT32               Current;            /**< Current charge/discharge rate (mA, +/- for charge/discharge) */

    // Percentage and Time
    UINT8               PercentRemaining;   /**< Percentage remaining (0-100) */
    UINT32              TimeToEmpty;        /**< Estimated time to empty (minutes, 0xFFFFFFFF=unknown) */
    UINT32              TimeToFull;         /**< Estimated time to full charge (minutes, 0xFFFFFFFF=unknown) */

    // Health and Status
    BATTERY_STATE       State;              /**< Current battery state */
    BATTERY_HEALTH      Health;             /**< Battery health status */
    UINT32              CycleCount;         /**< Charge cycle count */
    UINT16              Temperature;        /**< Battery temperature (Celsius * 10) */

    // Flags
    BOOLEAN             bPresent;           /**< Battery is present */
    BOOLEAN             bACConnected;       /**< AC adapter connected */
    BOOLEAN             bCharging;          /**< Battery is charging */
    BOOLEAN             bDischarging;       /**< Battery is discharging */
    BOOLEAN             bCritical;          /**< Battery critically low */
} BATTERY_INFO;

/**
 * @brief Thermal Zone Information
 *
 * Information about a thermal zone including current temperature, thresholds,
 * and cooling state.
 */
typedef struct _THERMAL_INFO {
    // Temperature (in Celsius * 10, e.g., 450 = 45.0°C)
    UINT16              CurrentTemperature; /**< Current temperature */
    UINT16              CriticalTemperature;/**< Critical shutdown temperature */
    UINT16              HotTemperature;     /**< Hot temperature threshold */
    UINT16              PassiveTemperature; /**< Passive cooling threshold */

    // Active Cooling Thresholds (up to 10 levels)
    UINT16              ActiveTemperature[10]; /**< Active cooling thresholds */

    // State
    THERMAL_STATE       State;              /**< Current thermal state */
    COOLING_POLICY      Policy;             /**< Current cooling policy */

    // Fan Information
    UINT8               NumFans;            /**< Number of fans in zone */
    UINT16              FanSpeed[4];        /**< Fan speeds in RPM (up to 4 fans) */
    UINT8               FanLevel[4];        /**< Fan levels 0-100% (up to 4 fans) */
} THERMAL_INFO;

/**
 * @brief P-State (Performance State) Information
 *
 * CPU frequency and voltage scaling state.
 */
typedef struct _PSTATE_INFO {
    UINT32              Frequency;          /**< Frequency in MHz */
    UINT32              Voltage;            /**< Voltage in millivolts */
    UINT32              Power;              /**< Power consumption in milliwatts */
    UINT32              Latency;            /**< Transition latency in microseconds */
} PSTATE_INFO;

/**
 * @brief C-State (CPU Idle State) Information
 *
 * CPU idle/sleep state information.
 */
typedef struct _CSTATE_INFO {
    CPU_POWER_STATE     State;              /**< C-state number */
    UINT32              Latency;            /**< Exit latency in microseconds */
    UINT32              Power;              /**< Power consumption in milliwatts */
    CHAR8               Description[64];    /**< State description */
} CSTATE_INFO;

/**
 * @brief ACPI Table Header
 *
 * Standard header present in all ACPI tables except RSDP.
 */
typedef struct _ACPI_TABLE_HEADER {
    CHAR8               Signature[4];       /**< Table signature (e.g., "FACP") */
    UINT32              Length;             /**< Length of table including header */
    UINT8               Revision;           /**< Revision of table */
    UINT8               Checksum;           /**< Checksum of entire table */
    CHAR8               OEMID[6];           /**< OEM identification */
    CHAR8               OEMTableID[8];      /**< OEM table identification */
    UINT32              OEMRevision;        /**< OEM revision */
    CHAR8               CreatorID[4];       /**< ASL compiler ID */
    UINT32              CreatorRevision;    /**< ASL compiler revision */
} ACPI_TABLE_HEADER;

/**
 * @brief ACPI Root System Description Pointer (RSDP)
 *
 * The RSDP structure for ACPI 1.0 and 2.0+.
 */
typedef struct _ACPI_RSDP {
    CHAR8               Signature[8];       /**< "RSD PTR " */
    UINT8               Checksum;           /**< Checksum of first 20 bytes */
    CHAR8               OEMID[6];           /**< OEM ID */
    UINT8               Revision;           /**< 0=ACPI 1.0, 2=ACPI 2.0+ */
    UINT32              RsdtAddress;        /**< Physical address of RSDT */

    // ACPI 2.0+ fields
    UINT32              Length;             /**< Length of table (36 for 2.0) */
    UINT64              XsdtAddress;        /**< Physical address of XSDT (64-bit) */
    UINT8               ExtendedChecksum;   /**< Checksum of entire table */
    UINT8               Reserved[3];        /**< Reserved */
} ACPI_RSDP;

/**
 * @brief Power Event Types
 *
 * Events that can be registered for notification.
 */
typedef enum _POWER_EVENT_TYPE {
    PowerEventSleep         = 0x01,     /**< System entering sleep */
    PowerEventWake          = 0x02,     /**< System waking from sleep */
    PowerEventBatteryLow    = 0x03,     /**< Battery critically low */
    PowerEventACChanged     = 0x04,     /**< AC adapter state changed */
    PowerEventThermalHigh   = 0x05,     /**< Temperature high */
    PowerEventLidClosed     = 0x06,     /**< Lid closed */
    PowerEventLidOpened     = 0x07,     /**< Lid opened */
    PowerEventPowerButton   = 0x08,     /**< Power button pressed */
    PowerEventSleepButton   = 0x09,     /**< Sleep button pressed */
} POWER_EVENT_TYPE;

/**
 * @brief Power Event Callback
 *
 * Callback function invoked when a power event occurs.
 */
typedef VOID (*POWER_EVENT_CALLBACK)(
    POWER_EVENT_TYPE    EventType,
    VOID               *pContext
    );

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOPowerManager, IIOService);
DECLARE_INTERFACE_(IIOACPIController, IIOService);
DECLARE_INTERFACE_(IIOBattery, IIOService);
DECLARE_INTERFACE_(IIOThermal, IIOService);

/**
 * @brief IIOPowerManager - System Power Management Interface
 *
 * This interface provides system-level power management including power state
 * transitions, battery management, and power event notifications.
 */
#undef INTERFACE
#define INTERFACE IIOPowerManager

DECLARE_INTERFACE_(IIOPowerManager, IIOService)
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

    // IIOPowerManager methods

    /**
     * @brief Get system power capabilities
     *
     * Retrieves the power management capabilities supported by the system.
     *
     * @param pCapability   Receives capability information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        ACPI_CAPABILITY *pCapability
        ) PURE;

    /**
     * @brief Set system power state
     *
     * Transitions the system to the specified power state (sleep, hibernate, shutdown).
     *
     * @param State         Desired power state
     *
     * @retval IO_SUCCESS       State transition initiated
     * @retval IO_UNSUPPORTED   Power state not supported
     * @retval IO_ERROR         Transition failed
     */
    STDMETHOD_(IO_RETURN, SetSystemState)(THIS_
        SYSTEM_POWER_STATE State
        ) PURE;

    /**
     * @brief Get current system power state
     *
     * Returns the current system power state.
     *
     * @param pState        Receives current power state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetSystemState)(THIS_
        SYSTEM_POWER_STATE *pState
        ) PURE;

    /**
     * @brief Get battery information
     *
     * Retrieves information about the specified battery.
     *
     * @param uBatteryIndex Battery index (0-based)
     * @param pInfo         Receives battery information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_NO_DEVICE     Battery not present
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetBatteryInfo)(THIS_
        UINT32 uBatteryIndex,
        BATTERY_INFO *pInfo
        ) PURE;

    /**
     * @brief Get AC adapter status
     *
     * Checks if an AC adapter is connected and providing power.
     *
     * @param pbConnected   Receives TRUE if AC connected, FALSE otherwise
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetACStatus)(THIS_
        BOOLEAN *pbConnected
        ) PURE;

    /**
     * @brief Register power event callback
     *
     * Registers a callback to be invoked when power events occur.
     *
     * @param EventType     Event type to monitor
     * @param pfnCallback   Callback function
     * @param pContext      Context pointer passed to callback
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_NO_MEMORY     Insufficient memory
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, RegisterPowerCallback)(THIS_
        POWER_EVENT_TYPE EventType,
        POWER_EVENT_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Unregister power event callback
     *
     * Unregisters a previously registered power event callback.
     *
     * @param EventType     Event type
     * @param pfnCallback   Callback function to remove
     *
     * @retval IO_SUCCESS       Callback unregistered successfully
     * @retval IO_NO_MATCH      Callback not found
     */
    STDMETHOD_(IO_RETURN, UnregisterPowerCallback)(THIS_
        POWER_EVENT_TYPE EventType,
        POWER_EVENT_CALLBACK pfnCallback
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOACPIController - ACPI Controller Interface
 *
 * This interface provides access to ACPI tables and AML method execution.
 */
#undef INTERFACE
#define INTERFACE IIOACPIController

DECLARE_INTERFACE_(IIOACPIController, IIOService)
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

    // IIOACPIController methods

    /**
     * @brief Get ACPI version
     *
     * Returns the ACPI specification version supported.
     *
     * @param puVersion     Receives ACPI version (e.g., 0x0605 for 6.5)
     *
     * @retval IO_SUCCESS       Version retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetACPIVersion)(THIS_
        UINT32 *puVersion
        ) PURE;

    /**
     * @brief Find ACPI table by signature
     *
     * Locates an ACPI table by its 4-character signature.
     *
     * @param pszSignature  Table signature (e.g., "FACP", "DSDT")
     * @param ppTable       Receives pointer to table
     * @param puLength      Receives table length
     *
     * @retval IO_SUCCESS       Table found successfully
     * @retval IO_NO_MATCH      Table not found
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, FindTable)(THIS_
        CONST CHAR8 *pszSignature,
        VOID **ppTable,
        UINT32 *puLength
        ) PURE;

    /**
     * @brief Execute ACPI AML method
     *
     * Executes an ACPI AML (ACPI Machine Language) method.
     *
     * @param pszPath       Method path (e.g., "\\_SB.PCI0._CRS")
     * @param pArgs         Method arguments (may be NULL)
     * @param uNumArgs      Number of arguments
     * @param pResult       Receives result (may be NULL)
     *
     * @retval IO_SUCCESS       Method executed successfully
     * @retval IO_NO_MATCH      Method not found
     * @retval IO_ERROR         Execution failed
     */
    STDMETHOD_(IO_RETURN, ExecuteMethod)(THIS_
        CONST CHAR8 *pszPath,
        VOID *pArgs,
        UINT32 uNumArgs,
        VOID *pResult
        ) PURE;

    /**
     * @brief Get ACPI device status
     *
     * Retrieves the status of an ACPI device (_STA method).
     *
     * @param pszDevicePath Device path
     * @param puStatus      Receives device status flags
     *
     * @retval IO_SUCCESS       Status retrieved successfully
     * @retval IO_NO_MATCH      Device not found
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceStatus)(THIS_
        CONST CHAR8 *pszDevicePath,
        UINT32 *puStatus
        ) PURE;

    /**
     * @brief Enable ACPI event
     *
     * Enables notification for a specific ACPI event.
     *
     * @param EventType     Event type to enable
     *
     * @retval IO_SUCCESS       Event enabled successfully
     * @retval IO_UNSUPPORTED   Event not supported
     */
    STDMETHOD_(IO_RETURN, EnableEvent)(THIS_
        UINT32 EventType
        ) PURE;

    /**
     * @brief Disable ACPI event
     *
     * Disables notification for a specific ACPI event.
     *
     * @param EventType     Event type to disable
     *
     * @retval IO_SUCCESS       Event disabled successfully
     * @retval IO_UNSUPPORTED   Event not supported
     */
    STDMETHOD_(IO_RETURN, DisableEvent)(THIS_
        UINT32 EventType
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOBattery - Battery Device Interface
 *
 * This interface represents a battery device and provides methods for
 * querying battery status, capacity, and health.
 */
#undef INTERFACE
#define INTERFACE IIOBattery

DECLARE_INTERFACE_(IIOBattery, IIOService)
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

    // IIOBattery methods

    /**
     * @brief Get battery information
     *
     * Retrieves comprehensive battery information.
     *
     * @param pInfo         Receives battery information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_NOT_READY     Battery not ready
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetBatteryInfo)(THIS_
        BATTERY_INFO *pInfo
        ) PURE;

    /**
     * @brief Get battery state
     *
     * Returns the current battery state (charging, discharging, full).
     *
     * @param pState        Receives battery state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetState)(THIS_
        BATTERY_STATE *pState
        ) PURE;

    /**
     * @brief Get battery percentage
     *
     * Returns the battery charge percentage (0-100).
     *
     * @param puPercentage  Receives percentage remaining
     *
     * @retval IO_SUCCESS       Percentage retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetPercentage)(THIS_
        UINT8 *puPercentage
        ) PURE;

    /**
     * @brief Get time remaining
     *
     * Returns estimated time remaining (to empty or to full charge).
     *
     * @param pbCharging    TRUE if charging, FALSE if discharging
     * @param puMinutes     Receives time remaining in minutes (0xFFFFFFFF=unknown)
     *
     * @retval IO_SUCCESS       Time retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetTimeRemaining)(THIS_
        BOOLEAN *pbCharging,
        UINT32 *puMinutes
        ) PURE;

    /**
     * @brief Register battery status callback
     *
     * Registers a callback to be invoked when battery status changes.
     *
     * @param pfnCallback   Callback function
     * @param pContext      Context pointer passed to callback
     *
     * @retval IO_SUCCESS       Callback registered successfully
     * @retval IO_NO_MEMORY     Insufficient memory
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, RegisterStatusCallback)(THIS_
        POWER_EVENT_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOThermal - Thermal Zone Interface
 *
 * This interface represents a thermal zone and provides methods for
 * temperature monitoring, fan control, and cooling policy management.
 */
#undef INTERFACE
#define INTERFACE IIOThermal

DECLARE_INTERFACE_(IIOThermal, IIOService)
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

    // IIOThermal methods

    /**
     * @brief Get thermal zone information
     *
     * Retrieves comprehensive thermal zone information.
     *
     * @param pInfo         Receives thermal information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetThermalInfo)(THIS_
        THERMAL_INFO *pInfo
        ) PURE;

    /**
     * @brief Get current temperature
     *
     * Returns the current temperature of the thermal zone.
     *
     * @param puTemperature Receives temperature (Celsius * 10)
     *
     * @retval IO_SUCCESS       Temperature retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     Sensor not ready
     */
    STDMETHOD_(IO_RETURN, GetTemperature)(THIS_
        UINT16 *puTemperature
        ) PURE;

    /**
     * @brief Get thermal state
     *
     * Returns the current thermal state (normal, warning, critical).
     *
     * @param pState        Receives thermal state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetThermalState)(THIS_
        THERMAL_STATE *pState
        ) PURE;

    /**
     * @brief Set fan speed
     *
     * Sets the speed of a specific fan.
     *
     * @param uFanIndex     Fan index (0-based)
     * @param uLevel        Fan level (0-100%, 255=auto)
     *
     * @retval IO_SUCCESS       Fan speed set successfully
     * @retval IO_NO_DEVICE     Fan not present
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, SetFanSpeed)(THIS_
        UINT8 uFanIndex,
        UINT8 uLevel
        ) PURE;

    /**
     * @brief Get fan speed
     *
     * Returns the current speed of a specific fan.
     *
     * @param uFanIndex     Fan index (0-based)
     * @param puRPM         Receives fan speed in RPM
     * @param puLevel       Receives fan level (0-100%)
     *
     * @retval IO_SUCCESS       Fan speed retrieved successfully
     * @retval IO_NO_DEVICE     Fan not present
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetFanSpeed)(THIS_
        UINT8 uFanIndex,
        UINT16 *puRPM,
        UINT8 *puLevel
        ) PURE;

    /**
     * @brief Set cooling policy
     *
     * Sets the cooling policy for the thermal zone.
     *
     * @param Policy        Desired cooling policy
     *
     * @retval IO_SUCCESS       Policy set successfully
     * @retval IO_UNSUPPORTED   Policy not supported
     */
    STDMETHOD_(IO_RETURN, SetCoolingPolicy)(THIS_
        COOLING_POLICY Policy
        ) PURE;

    /**
     * @brief Get cooling policy
     *
     * Returns the current cooling policy.
     *
     * @param pPolicy       Receives cooling policy
     *
     * @retval IO_SUCCESS       Policy retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCoolingPolicy)(THIS_
        COOLING_POLICY *pPolicy
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOPowerManager methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOPowerManager_GetCapabilities(p,a)        (p)->lpVtbl->GetCapabilities(p,a)
#define IIOPowerManager_SetSystemState(p,a)         (p)->lpVtbl->SetSystemState(p,a)
#define IIOPowerManager_GetSystemState(p,a)         (p)->lpVtbl->GetSystemState(p,a)
#define IIOPowerManager_GetBatteryInfo(p,a,b)       (p)->lpVtbl->GetBatteryInfo(p,a,b)
#define IIOPowerManager_GetACStatus(p,a)            (p)->lpVtbl->GetACStatus(p,a)
#define IIOPowerManager_RegisterPowerCallback(p,a,b,c) (p)->lpVtbl->RegisterPowerCallback(p,a,b,c)
#define IIOPowerManager_UnregisterPowerCallback(p,a,b) (p)->lpVtbl->UnregisterPowerCallback(p,a,b)

#define IIOACPIController_GetACPIVersion(p,a)       (p)->lpVtbl->GetACPIVersion(p,a)
#define IIOACPIController_FindTable(p,a,b,c)        (p)->lpVtbl->FindTable(p,a,b,c)
#define IIOACPIController_ExecuteMethod(p,a,b,c,d)  (p)->lpVtbl->ExecuteMethod(p,a,b,c,d)
#define IIOACPIController_GetDeviceStatus(p,a,b)    (p)->lpVtbl->GetDeviceStatus(p,a,b)
#define IIOACPIController_EnableEvent(p,a)          (p)->lpVtbl->EnableEvent(p,a)
#define IIOACPIController_DisableEvent(p,a)         (p)->lpVtbl->DisableEvent(p,a)

#define IIOBattery_GetBatteryInfo(p,a)              (p)->lpVtbl->GetBatteryInfo(p,a)
#define IIOBattery_GetState(p,a)                    (p)->lpVtbl->GetState(p,a)
#define IIOBattery_GetPercentage(p,a)               (p)->lpVtbl->GetPercentage(p,a)
#define IIOBattery_GetTimeRemaining(p,a,b)          (p)->lpVtbl->GetTimeRemaining(p,a,b)
#define IIOBattery_RegisterStatusCallback(p,a,b)    (p)->lpVtbl->RegisterStatusCallback(p,a,b)

#define IIOThermal_GetThermalInfo(p,a)              (p)->lpVtbl->GetThermalInfo(p,a)
#define IIOThermal_GetTemperature(p,a)              (p)->lpVtbl->GetTemperature(p,a)
#define IIOThermal_GetThermalState(p,a)             (p)->lpVtbl->GetThermalState(p,a)
#define IIOThermal_SetFanSpeed(p,a,b)               (p)->lpVtbl->SetFanSpeed(p,a,b)
#define IIOThermal_GetFanSpeed(p,a,b,c)             (p)->lpVtbl->GetFanSpeed(p,a,b,c)
#define IIOThermal_SetCoolingPolicy(p,a)            (p)->lpVtbl->SetCoolingPolicy(p,a)
#define IIOThermal_GetCoolingPolicy(p,a)            (p)->lpVtbl->GetCoolingPolicy(p,a)

#endif

/**
 * @brief Initialize Power Management family subsystem
 *
 * Initializes the power management subsystem, enumerates ACPI tables,
 * discovers batteries and thermal zones.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
PowerInitialize(
    VOID
    );

/**
 * @brief Shutdown Power Management family subsystem
 *
 * Shuts down the power management subsystem and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
PowerShutdown(
    VOID
    );

/**
 * @brief Create a power manager instance
 *
 * Creates the system power manager interface.
 *
 * @param ppPowerManager    Receives power manager interface
 *
 * @retval IO_SUCCESS           Power manager created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
PowerManagerCreate(
    IIOPowerManager **ppPowerManager
    );

/**
 * @brief Create an ACPI controller instance
 *
 * Creates an ACPI controller interface for ACPI table access and AML execution.
 *
 * @param ppACPIController  Receives ACPI controller interface
 *
 * @retval IO_SUCCESS           ACPI controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 * @retval IO_UNSUPPORTED       ACPI not available
 */
IO_RETURN
ACPIControllerCreate(
    IIOACPIController **ppACPIController
    );

/**
 * @brief Create a battery device instance
 *
 * Creates a battery device interface.
 *
 * @param uBatteryIndex     Battery index (0-based)
 * @param ppBattery         Receives battery interface
 *
 * @retval IO_SUCCESS           Battery created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_NO_DEVICE         Battery not present
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
BatteryCreate(
    UINT32 uBatteryIndex,
    IIOBattery **ppBattery
    );

/**
 * @brief Create a thermal zone instance
 *
 * Creates a thermal zone interface.
 *
 * @param uZoneIndex        Thermal zone index (0-based)
 * @param ppThermal         Receives thermal interface
 *
 * @retval IO_SUCCESS           Thermal zone created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_NO_DEVICE         Thermal zone not present
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
ThermalCreate(
    UINT32 uZoneIndex,
    IIOThermal **ppThermal
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_POWER_H */
