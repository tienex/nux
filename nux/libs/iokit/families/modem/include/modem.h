/**
 * @file modem.h
 * @brief Modem Family Interface - Universal Modem Device Abstraction
 *
 * This header defines the Modem family interface providing a unified abstraction
 * layer for all modem devices including hardware modems, WinModems/Softmodems,
 * and controller-based modems.
 *
 * The Modem family sits ABOVE the serial port layer and provides:
 * - Protocol-agnostic modem command interface (AT commands)
 * - Unified device enumeration and capabilities reporting
 * - Support for dial-up, data, fax, and voice operations
 * - Modem standard detection and configuration
 * - Error correction and data compression management
 * - Call progress detection and caller ID support
 *
 * This family supports a comprehensive range of modems:
 *
 * HARDWARE MODEMS (Traditional with DSP chips):
 * - Hayes Smartmodem series (300-56K)
 * - USRobotics Sportster, Courier
 * - Zoom modems (V.32bis through V.92)
 * - Practical Peripherals
 * - Multi-Tech Systems
 * - External (Serial/USB), Internal (ISA/PCI/PCIe), PCMCIA/CardBus
 *
 * WINMODEMS/SOFTMODEMS (CPU-based DSP):
 * - Lucent/Agere Venus, Apollo, Mars, Scorpio chipsets
 * - Conexant HSF (HCF) SmartModem, AccessRunner
 * - Motorola SM56 chipsets
 * - ESS Technology modems
 * - Intel Hammerhead, Ambient MD3200
 * - 3Com Mini-PCI modems
 * - Broadcom BCM4212, BCM4213
 *
 * MODEM STANDARDS:
 * - V.21 (300 bps), V.22 (1200 bps), V.22bis (2400 bps)
 * - V.32 (9600 bps), V.32bis (14400 bps)
 * - V.34 (28800/33600 bps), V.90 (56K download), V.92 (48K upload)
 * - K56flex, X2 (proprietary 56K)
 * - Error correction: V.42 (LAPM), MNP 2-4
 * - Compression: V.42bis, V.44, MNP 5
 *
 * FAX SUPPORT:
 * - V.17 (14400 bps), V.29 (9600 bps), V.27ter (4800 bps)
 * - Class 1, Class 2, Class 2.0 fax protocols
 *
 * VOICE SUPPORT:
 * - Voice mail, speakerphone
 * - Call progress detection
 * - Caller ID (FSK and DTMF)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_MODEM_H
#define IOKIT_MODEM_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOModemController interface GUID
 * {F1E2D3C4-A5B6-4C7D-8E9F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIOModemController,
    0xF1E2D3C4, 0xA5B6, 0x4C7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIOModemDevice interface GUID
 * {E2D3C4B5-A6C7-4D8E-9F0A-1B2C3D4E5F6A}
 */
DEFINE_GUID(IID_IIOModemDevice,
    0xE2D3C4B5, 0xA6C7, 0x4D8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A);

/**
 * @brief Modem Types
 */
typedef enum _MODEM_TYPE {
    MODEM_TYPE_UNKNOWN          = 0x00,     /**< Unknown modem type */
    MODEM_TYPE_HARDWARE         = 0x01,     /**< Hardware modem (with DSP) */
    MODEM_TYPE_WINMODEM         = 0x02,     /**< WinModem/Softmodem (software DSP) */
    MODEM_TYPE_CONTROLLER       = 0x03,     /**< Controller-based modem */
    MODEM_TYPE_CABLE            = 0x04,     /**< Cable modem */
    MODEM_TYPE_DSL              = 0x05,     /**< DSL modem */
    MODEM_TYPE_CELLULAR         = 0x06,     /**< Cellular modem (3G/4G/5G) */
    MODEM_TYPE_SATELLITE        = 0x07,     /**< Satellite modem */
} MODEM_TYPE;

/**
 * @brief Modem Form Factors
 */
typedef enum _MODEM_FORM_FACTOR {
    MODEM_FORM_UNKNOWN          = 0x00,     /**< Unknown form factor */
    MODEM_FORM_EXTERNAL_SERIAL  = 0x01,     /**< External serial modem */
    MODEM_FORM_EXTERNAL_USB     = 0x02,     /**< External USB modem */
    MODEM_FORM_INTERNAL_ISA     = 0x03,     /**< Internal ISA modem */
    MODEM_FORM_INTERNAL_PCI     = 0x04,     /**< Internal PCI modem */
    MODEM_FORM_INTERNAL_PCIE    = 0x05,     /**< Internal PCIe modem */
    MODEM_FORM_PCMCIA           = 0x06,     /**< PCMCIA card modem */
    MODEM_FORM_CARDBUS          = 0x07,     /**< CardBus modem */
    MODEM_FORM_EXPRESSCARD      = 0x08,     /**< ExpressCard modem */
    MODEM_FORM_MINI_PCI         = 0x09,     /**< Mini-PCI modem */
    MODEM_FORM_M2               = 0x0A,     /**< M.2 modem */
} MODEM_FORM_FACTOR;

/**
 * @brief Modem Standards (V-series)
 */
typedef enum _MODEM_STANDARD {
    MODEM_V21                   = 0x0001,   /**< V.21: 300 bps (FSK) */
    MODEM_V22                   = 0x0002,   /**< V.22: 1200 bps */
    MODEM_V22BIS                = 0x0004,   /**< V.22bis: 2400 bps */
    MODEM_V23                   = 0x0008,   /**< V.23: 1200 bps (FSK) */
    MODEM_V32                   = 0x0010,   /**< V.32: 9600 bps */
    MODEM_V32BIS                = 0x0020,   /**< V.32bis: 14400 bps */
    MODEM_V34                   = 0x0040,   /**< V.34: 28800/33600 bps */
    MODEM_V90                   = 0x0080,   /**< V.90: 56K download, 33.6K upload */
    MODEM_V92                   = 0x0100,   /**< V.92: 56K download, 48K upload */
    MODEM_K56FLEX               = 0x0200,   /**< K56flex: Rockwell 56K (pre-V.90) */
    MODEM_X2                    = 0x0400,   /**< X2: USR 56K (pre-V.90) */
    MODEM_V34BIS                = 0x0800,   /**< V.34bis (enhanced V.34) */
} MODEM_STANDARD;

/**
 * @brief Error Correction Protocols
 */
typedef enum _ERROR_CORRECTION {
    ERROR_CORR_NONE             = 0x00,     /**< No error correction */
    ERROR_CORR_V42              = 0x01,     /**< V.42 (LAPM) */
    ERROR_CORR_MNP2             = 0x02,     /**< MNP 2 */
    ERROR_CORR_MNP3             = 0x04,     /**< MNP 3 */
    ERROR_CORR_MNP4             = 0x08,     /**< MNP 4 */
} ERROR_CORRECTION;

/**
 * @brief Data Compression Protocols
 */
typedef enum _DATA_COMPRESSION {
    DATA_COMP_NONE              = 0x00,     /**< No compression */
    DATA_COMP_V42BIS            = 0x01,     /**< V.42bis */
    DATA_COMP_V44               = 0x02,     /**< V.44 */
    DATA_COMP_MNP5              = 0x04,     /**< MNP 5 */
} DATA_COMPRESSION;

/**
 * @brief Fax Standards
 */
typedef enum _FAX_STANDARD {
    FAX_NONE                    = 0x00,     /**< No fax support */
    FAX_V17                     = 0x01,     /**< V.17: 14400 bps */
    FAX_V29                     = 0x02,     /**< V.29: 9600 bps */
    FAX_V27TER                  = 0x04,     /**< V.27ter: 4800 bps */
    FAX_CLASS1                  = 0x10,     /**< Fax Class 1 */
    FAX_CLASS2                  = 0x20,     /**< Fax Class 2 */
    FAX_CLASS2_0                = 0x40,     /**< Fax Class 2.0 */
} FAX_STANDARD;

/**
 * @brief Modem Capability Flags (Bitmask)
 */
#define MODEM_CAP_DATA              0x00000001  /**< Data mode */
#define MODEM_CAP_FAX               0x00000002  /**< Fax capability */
#define MODEM_CAP_VOICE             0x00000004  /**< Voice capability */
#define MODEM_CAP_CALLER_ID         0x00000008  /**< Caller ID support */
#define MODEM_CAP_DISTINCTIVE_RING  0x00000010  /**< Distinctive ring */
#define MODEM_CAP_CALL_WAITING      0x00000020  /**< Call waiting ID */
#define MODEM_CAP_SPEAKERPHONE      0x00000040  /**< Speakerphone mode */
#define MODEM_CAP_VOICE_MAIL        0x00000080  /**< Voice mail */
#define MODEM_CAP_TAM               0x00000100  /**< Telephone answering machine */
#define MODEM_CAP_CELLULAR          0x00000200  /**< Cellular support */
#define MODEM_CAP_V8                0x00000400  /**< V.8 call setup */
#define MODEM_CAP_V8BIS             0x00000800  /**< V.8bis negotiations */
#define MODEM_CAP_QUICK_CONNECT     0x00001000  /**< Quick Connect (V.92) */
#define MODEM_CAP_MODEM_ON_HOLD     0x00002000  /**< Modem on Hold (V.92) */
#define MODEM_CAP_PCM_UPSTREAM      0x00004000  /**< PCM upstream (V.92) */
#define MODEM_CAP_DSVD              0x00008000  /**< DSVD (simultaneous voice/data) */

/**
 * @brief Modem States
 */
typedef enum _MODEM_STATE {
    MODEM_STATE_IDLE            = 0x00,     /**< Idle, not connected */
    MODEM_STATE_DIALING         = 0x01,     /**< Dialing */
    MODEM_STATE_ANSWERING       = 0x02,     /**< Answering incoming call */
    MODEM_STATE_CONNECTING      = 0x03,     /**< Connecting/negotiating */
    MODEM_STATE_CONNECTED       = 0x04,     /**< Connected, data mode */
    MODEM_STATE_DISCONNECTING   = 0x05,     /**< Disconnecting */
    MODEM_STATE_COMMAND         = 0x06,     /**< Command mode (online) */
    MODEM_STATE_FAX             = 0x07,     /**< Fax mode */
    MODEM_STATE_VOICE           = 0x08,     /**< Voice mode */
    MODEM_STATE_ERROR           = 0xFF,     /**< Error state */
} MODEM_STATE;

/**
 * @brief AT Command Result Codes
 */
typedef enum _AT_RESULT {
    AT_OK                       = 0,        /**< OK */
    AT_CONNECT                  = 1,        /**< CONNECT */
    AT_RING                     = 2,        /**< RING */
    AT_NO_CARRIER               = 3,        /**< NO CARRIER */
    AT_ERROR                    = 4,        /**< ERROR */
    AT_NO_DIALTONE              = 6,        /**< NO DIALTONE */
    AT_BUSY                     = 7,        /**< BUSY */
    AT_NO_ANSWER                = 8,        /**< NO ANSWER */
    AT_CONNECT_1200             = 10,       /**< CONNECT 1200 */
    AT_CONNECT_2400             = 11,       /**< CONNECT 2400 */
    AT_CONNECT_4800             = 12,       /**< CONNECT 4800 */
    AT_CONNECT_9600             = 13,       /**< CONNECT 9600 */
    AT_CONNECT_14400            = 14,       /**< CONNECT 14400 */
    AT_CONNECT_19200            = 15,       /**< CONNECT 19200 */
    AT_CONNECT_28800            = 16,       /**< CONNECT 28800 */
    AT_CONNECT_33600            = 17,       /**< CONNECT 33600 */
    AT_CONNECT_56000            = 18,       /**< CONNECT 56000 */
} AT_RESULT;

/**
 * @brief Modem Chipset Manufacturers
 */
typedef enum _MODEM_CHIPSET_VENDOR {
    CHIPSET_UNKNOWN             = 0x00,     /**< Unknown chipset */
    CHIPSET_LUCENT              = 0x01,     /**< Lucent/Agere */
    CHIPSET_CONEXANT            = 0x02,     /**< Conexant */
    CHIPSET_MOTOROLA            = 0x03,     /**< Motorola */
    CHIPSET_ESS                 = 0x04,     /**< ESS Technology */
    CHIPSET_INTEL               = 0x05,     /**< Intel */
    CHIPSET_AMBIENT             = 0x06,     /**< Ambient Technologies */
    CHIPSET_BROADCOM            = 0x07,     /**< Broadcom */
    CHIPSET_3COM                = 0x08,     /**< 3Com */
    CHIPSET_ROCKWELL            = 0x09,     /**< Rockwell */
    CHIPSET_SMARTLINK           = 0x0A,     /**< SmartLink */
    CHIPSET_VIA                 = 0x0B,     /**< VIA */
    CHIPSET_TOPIC               = 0x0C,     /**< TOPIC Semiconductor */
    CHIPSET_ALI                 = 0x0D,     /**< ALi */
} MODEM_CHIPSET_VENDOR;

/**
 * @brief Call Progress Tones
 */
typedef enum _CALL_PROGRESS {
    CALL_PROGRESS_NONE          = 0x00,     /**< No tone detected */
    CALL_PROGRESS_DIALTONE      = 0x01,     /**< Dial tone */
    CALL_PROGRESS_BUSY          = 0x02,     /**< Busy signal */
    CALL_PROGRESS_RINGBACK      = 0x03,     /**< Ringback tone */
    CALL_PROGRESS_REORDER       = 0x04,     /**< Reorder/fast busy */
    CALL_PROGRESS_SILENCE       = 0x05,     /**< Silence */
    CALL_PROGRESS_VOICE         = 0x06,     /**< Voice detected */
    CALL_PROGRESS_FAX           = 0x07,     /**< Fax tone (CNG) */
    CALL_PROGRESS_DATA          = 0x08,     /**< Data answer tone */
} CALL_PROGRESS;

/**
 * @brief Maximum sizes
 */
#define MAX_AT_COMMAND_LENGTH       256
#define MAX_AT_RESPONSE_LENGTH      2048
#define MAX_PHONE_NUMBER_LENGTH     64
#define MAX_CALLER_ID_LENGTH        64

/**
 * @brief Modem Controller Information
 */
typedef struct _MODEM_CONTROLLER_INFO {
    // Device Identity
    CHAR8               ControllerName[64]; /**< Controller name */
    CHAR8               Vendor[40];         /**< Vendor/manufacturer */
    CHAR8               Model[40];          /**< Model name */
    CHAR8               ChipsetName[40];    /**< Chipset name */
    MODEM_TYPE          ModemType;          /**< Modem type */
    MODEM_FORM_FACTOR   FormFactor;         /**< Form factor */
    MODEM_CHIPSET_VENDOR ChipsetVendor;     /**< Chipset vendor */

    // Hardware Information
    UINT16              VendorID;           /**< PCI/USB Vendor ID */
    UINT16              DeviceID;           /**< PCI/USB Device ID */
    UINT16              SubsystemVendorID;  /**< Subsystem Vendor ID */
    UINT16              SubsystemDeviceID;  /**< Subsystem Device ID */
    CHAR8               FirmwareVersion[16];/**< Firmware version */

    // Capabilities
    UINT32              Capabilities;       /**< Capability flags */
    UINT32              SupportedStandards; /**< Supported modem standards */
    UINT32              MaxSpeed;           /**< Maximum speed (bps) */
    UINT32              MaxDownload;        /**< Max download speed (bps) */
    UINT32              MaxUpload;          /**< Max upload speed (bps) */

    // Protocol Support
    UINT16              ErrorCorrection;    /**< Error correction protocols */
    UINT16              DataCompression;    /**< Data compression protocols */
    UINT16              FaxSupport;         /**< Fax standards supported */

    // Configuration
    UINT16              COMPort;            /**< Assigned COM port */
    UINT32              BaudRate;           /**< Serial port baud rate */
    BOOLEAN             bATCommandSet;      /**< Supports AT commands */

    // Driver Information
    CHAR8               DriverName[32];     /**< Driver name */
    CHAR8               DriverVersion[16];  /**< Driver version */
} MODEM_CONTROLLER_INFO;

/**
 * @brief Modem Device Information
 */
typedef struct _MODEM_DEVICE_INFO {
    // Identity
    CHAR8               DeviceName[64];     /**< Device name */
    CHAR8               Manufacturer[40];   /**< Manufacturer */
    CHAR8               Model[40];          /**< Model name */
    CHAR8               SerialNumber[32];   /**< Serial number */

    // Current State
    MODEM_STATE         State;              /**< Current state */
    UINT32              ConnectedSpeed;     /**< Connected speed (bps) */
    ERROR_CORRECTION    ActiveErrorCorr;    /**< Active error correction */
    DATA_COMPRESSION    ActiveCompression;  /**< Active compression */

    // Call Information
    CHAR8               PhoneNumber[MAX_PHONE_NUMBER_LENGTH]; /**< Connected number */
    CHAR8               CallerID[MAX_CALLER_ID_LENGTH];       /**< Caller ID */
    UINT32              CallDuration;       /**< Call duration (seconds) */

    // Statistics
    UINT64              BytesSent;          /**< Total bytes sent */
    UINT64              BytesReceived;      /**< Total bytes received */
    UINT32              TotalCalls;         /**< Total calls made */
    UINT32              SuccessfulCalls;    /**< Successful connections */
    UINT32              FailedCalls;        /**< Failed connections */

    // Signal Quality
    INT32               SignalStrength;     /**< Signal strength (dBm) */
    UINT32              LineQuality;        /**< Line quality (0-100%) */
    UINT32              RetransmitRate;     /**< Retransmit rate (%) */
} MODEM_DEVICE_INFO;

/**
 * @brief Connection Parameters
 */
typedef struct _MODEM_CONNECTION_PARAMS {
    CHAR8               PhoneNumber[MAX_PHONE_NUMBER_LENGTH];
    UINT32              Timeout;            /**< Dial timeout (seconds) */
    UINT32              Retries;            /**< Number of retries */
    BOOLEAN             bBlindDial;         /**< Blind dial (no dialtone check) */
    BOOLEAN             bPulseDialing;      /**< Pulse dialing (vs tone) */
    UINT32              DialPrefix;         /**< Dial prefix (outside line) */
} MODEM_CONNECTION_PARAMS;

/**
 * @brief Modem Statistics
 */
typedef struct _MODEM_STATISTICS {
    // Connection Statistics
    UINT32              TotalConnections;   /**< Total connection attempts */
    UINT32              SuccessfulConnections; /**< Successful connections */
    UINT32              FailedConnections;  /**< Failed connections */
    UINT32              DroppedConnections; /**< Dropped connections */

    // Data Transfer
    UINT64              TotalBytesSent;     /**< Total bytes transmitted */
    UINT64              TotalBytesReceived; /**< Total bytes received */
    UINT32              AverageSpeed;       /**< Average connection speed */
    UINT32              PeakSpeed;          /**< Peak connection speed */

    // Error Statistics
    UINT32              FrameErrors;        /**< Frame errors */
    UINT32              OverrunErrors;      /**< Overrun errors */
    UINT32              ParityErrors;       /**< Parity errors */
    UINT32              ProtocolErrors;     /**< Protocol errors */
    UINT32              Retransmissions;    /**< Retransmissions */

    // Time Statistics
    UINT64              TotalConnectTime;   /**< Total connect time (seconds) */
    UINT32              AverageCallLength;  /**< Average call length (seconds) */
    UINT32              LongestCall;        /**< Longest call (seconds) */
} MODEM_STATISTICS;

/**
 * @brief AT Command Callback
 *
 * Called when a response is received from an AT command.
 *
 * @param pContext      User context pointer
 * @param pszResponse   Response string
 * @param Result        Result code
 *
 * @retval IO_SUCCESS   Response processed successfully
 */
typedef IO_RETURN (*PFN_AT_COMMAND_CALLBACK)(
    VOID *pContext,
    CONST CHAR8 *pszResponse,
    AT_RESULT Result
    );

/**
 * @brief Ring Callback
 *
 * Called when an incoming call is detected (RING).
 *
 * @param pContext      User context pointer
 * @param pszCallerID   Caller ID (may be NULL)
 *
 * @retval IO_SUCCESS   Ring processed successfully
 */
typedef IO_RETURN (*PFN_RING_CALLBACK)(
    VOID *pContext,
    CONST CHAR8 *pszCallerID
    );

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOModemController, IIOService);
DECLARE_INTERFACE_(IIOModemDevice, IIOService);

/**
 * @brief IIOModemController - Modem Controller Interface
 *
 * This interface represents a modem controller and provides methods for
 * modem-level operations including configuration, dialing, and status.
 */
#undef INTERFACE
#define INTERFACE IIOModemController

DECLARE_INTERFACE_(IIOModemController, IIOService)
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

    // IIOModemController methods

    /**
     * @brief Get controller information
     *
     * Retrieves comprehensive information about the modem controller.
     *
     * @param pControllerInfo   Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        MODEM_CONTROLLER_INFO *pControllerInfo
        ) PURE;

    /**
     * @brief Initialize modem
     *
     * Initializes the modem with default AT commands.
     *
     * @retval IO_SUCCESS       Modem initialized successfully
     * @retval IO_ERROR         Initialization failed
     */
    STDMETHOD_(IO_RETURN, Initialize)(THIS) PURE;

    /**
     * @brief Reset modem
     *
     * Resets the modem to factory defaults (ATZ).
     *
     * @retval IO_SUCCESS       Modem reset successfully
     * @retval IO_ERROR         Reset failed
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;

    /**
     * @brief Send AT command
     *
     * Sends an AT command to the modem.
     *
     * @param pszCommand        AT command string (without "AT" prefix)
     * @param pszResponse       Buffer to receive response
     * @param cbResponseSize    Size of response buffer
     * @param uTimeout          Timeout in milliseconds
     *
     * @retval IO_SUCCESS       Command sent and response received
     * @retval IO_TIMEOUT       Command timeout
     * @retval IO_ERROR         Command failed
     */
    STDMETHOD_(IO_RETURN, SendATCommand)(THIS_
        CONST CHAR8 *pszCommand,
        CHAR8 *pszResponse,
        UINTN cbResponseSize,
        UINT32 uTimeout
        ) PURE;

    /**
     * @brief Dial number
     *
     * Dials a phone number (ATDT or ATDP).
     *
     * @param pParams           Connection parameters
     *
     * @retval IO_SUCCESS       Dialing initiated
     * @retval IO_BUSY          Line busy
     * @retval IO_NO_CARRIER    No carrier
     * @retval IO_ERROR         Dial failed
     */
    STDMETHOD_(IO_RETURN, Dial)(THIS_
        CONST MODEM_CONNECTION_PARAMS *pParams
        ) PURE;

    /**
     * @brief Answer incoming call
     *
     * Answers an incoming call (ATA).
     *
     * @retval IO_SUCCESS       Call answered successfully
     * @retval IO_ERROR         Failed to answer
     */
    STDMETHOD_(IO_RETURN, Answer)(THIS) PURE;

    /**
     * @brief Hang up
     *
     * Terminates the current connection (ATH).
     *
     * @retval IO_SUCCESS       Hung up successfully
     */
    STDMETHOD_(IO_RETURN, Hangup)(THIS) PURE;

    /**
     * @brief Get modem state
     *
     * Retrieves the current modem state.
     *
     * @param pState            Receives modem state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetState)(THIS_
        MODEM_STATE *pState
        ) PURE;

    /**
     * @brief Get connection speed
     *
     * Retrieves the current connection speed.
     *
     * @param puSpeed           Receives speed in bps
     *
     * @retval IO_SUCCESS       Speed retrieved successfully
     * @retval IO_NOT_READY     Not connected
     */
    STDMETHOD_(IO_RETURN, GetConnectionSpeed)(THIS_
        UINT32 *puSpeed
        ) PURE;

    /**
     * @brief Set speaker volume
     *
     * Sets the modem speaker volume (ATL command).
     *
     * @param uVolume           Volume level (0-3)
     *
     * @retval IO_SUCCESS       Volume set successfully
     */
    STDMETHOD_(IO_RETURN, SetSpeakerVolume)(THIS_
        UINT8 uVolume
        ) PURE;

    /**
     * @brief Set speaker mode
     *
     * Sets the modem speaker mode (ATM command).
     *
     * @param uMode             Speaker mode (0=off, 1=on until carrier, 2=always on)
     *
     * @retval IO_SUCCESS       Mode set successfully
     */
    STDMETHOD_(IO_RETURN, SetSpeakerMode)(THIS_
        UINT8 uMode
        ) PURE;

    /**
     * @brief Enable echo
     *
     * Enables command echo (ATE1).
     *
     * @retval IO_SUCCESS       Echo enabled
     */
    STDMETHOD_(IO_RETURN, EnableEcho)(THIS) PURE;

    /**
     * @brief Disable echo
     *
     * Disables command echo (ATE0).
     *
     * @retval IO_SUCCESS       Echo disabled
     */
    STDMETHOD_(IO_RETURN, DisableEcho)(THIS) PURE;

    /**
     * @brief Set result code mode
     *
     * Sets the result code mode (ATV - verbose/numeric).
     *
     * @param bVerbose          TRUE for verbose mode, FALSE for numeric
     *
     * @retval IO_SUCCESS       Mode set successfully
     */
    STDMETHOD_(IO_RETURN, SetResultCodeMode)(THIS_
        BOOLEAN bVerbose
        ) PURE;

    /**
     * @brief Enable caller ID
     *
     * Enables caller ID reporting.
     *
     * @retval IO_SUCCESS       Caller ID enabled
     * @retval IO_UNSUPPORTED   Caller ID not supported
     */
    STDMETHOD_(IO_RETURN, EnableCallerID)(THIS) PURE;

    /**
     * @brief Disable caller ID
     *
     * Disables caller ID reporting.
     *
     * @retval IO_SUCCESS       Caller ID disabled
     */
    STDMETHOD_(IO_RETURN, DisableCallerID)(THIS) PURE;

    /**
     * @brief Set error correction mode
     *
     * Enables/disables error correction protocols.
     *
     * @param ErrorCorr         Error correction protocol
     * @param bEnable           Enable or disable
     *
     * @retval IO_SUCCESS       Setting applied
     * @retval IO_UNSUPPORTED   Protocol not supported
     */
    STDMETHOD_(IO_RETURN, SetErrorCorrection)(THIS_
        ERROR_CORRECTION ErrorCorr,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Set data compression
     *
     * Enables/disables data compression.
     *
     * @param Compression       Compression protocol
     * @param bEnable           Enable or disable
     *
     * @retval IO_SUCCESS       Setting applied
     * @retval IO_UNSUPPORTED   Protocol not supported
     */
    STDMETHOD_(IO_RETURN, SetDataCompression)(THIS_
        DATA_COMPRESSION Compression,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Enter fax mode
     *
     * Switches modem to fax mode.
     *
     * @param FaxClass          Fax class (1, 2, or 2.0)
     *
     * @retval IO_SUCCESS       Fax mode entered
     * @retval IO_UNSUPPORTED   Fax not supported
     */
    STDMETHOD_(IO_RETURN, EnterFaxMode)(THIS_
        UINT8 FaxClass
        ) PURE;

    /**
     * @brief Enter voice mode
     *
     * Switches modem to voice mode.
     *
     * @retval IO_SUCCESS       Voice mode entered
     * @retval IO_UNSUPPORTED   Voice not supported
     */
    STDMETHOD_(IO_RETURN, EnterVoiceMode)(THIS) PURE;

    /**
     * @brief Exit to data mode
     *
     * Returns modem to data mode.
     *
     * @retval IO_SUCCESS       Data mode entered
     */
    STDMETHOD_(IO_RETURN, EnterDataMode)(THIS) PURE;

    /**
     * @brief Get statistics
     *
     * Retrieves modem statistics.
     *
     * @param pStats            Receives statistics
     * @param bReset            Reset statistics after reading
     *
     * @retval IO_SUCCESS       Statistics retrieved
     */
    STDMETHOD_(IO_RETURN, GetStatistics)(THIS_
        MODEM_STATISTICS *pStats,
        BOOLEAN bReset
        ) PURE;

    /**
     * @brief Register AT command callback
     *
     * Registers a callback for asynchronous AT responses.
     *
     * @param pfnCallback       Callback function
     * @param pContext          User context
     *
     * @retval IO_SUCCESS       Callback registered
     */
    STDMETHOD_(IO_RETURN, RegisterATCallback)(THIS_
        PFN_AT_COMMAND_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Register ring callback
     *
     * Registers a callback for incoming calls (RING).
     *
     * @param pfnCallback       Callback function
     * @param pContext          User context
     *
     * @retval IO_SUCCESS       Callback registered
     */
    STDMETHOD_(IO_RETURN, RegisterRingCallback)(THIS_
        PFN_RING_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Detect call progress
     *
     * Detects call progress tones.
     *
     * @param pProgress         Receives detected tone
     *
     * @retval IO_SUCCESS       Tone detected
     * @retval IO_TIMEOUT       No tone detected
     */
    STDMETHOD_(IO_RETURN, DetectCallProgress)(THIS_
        CALL_PROGRESS *pProgress
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOModemDevice - Modem Device Interface
 *
 * This interface represents a modem device and provides methods for
 * device-specific operations and information retrieval.
 */
#undef INTERFACE
#define INTERFACE IIOModemDevice

DECLARE_INTERFACE_(IIOModemDevice, IIOService)
{
    // IUnknown and IIOService methods inherited...

    /**
     * @brief Get device information
     *
     * Retrieves information about the modem device.
     *
     * @param pDeviceInfo       Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        MODEM_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Query capabilities
     *
     * Queries modem capabilities.
     *
     * @param puCapabilities    Receives capability flags
     *
     * @retval IO_SUCCESS       Capabilities retrieved
     */
    STDMETHOD_(IO_RETURN, QueryCapabilities)(THIS_
        UINT32 *puCapabilities
        ) PURE;

    /**
     * @brief Get signal strength
     *
     * Retrieves signal strength (for cellular modems).
     *
     * @param pStrength         Receives signal strength (dBm)
     *
     * @retval IO_SUCCESS       Signal strength retrieved
     * @retval IO_UNSUPPORTED   Not applicable
     */
    STDMETHOD_(IO_RETURN, GetSignalStrength)(THIS_
        INT32 *pStrength
        ) PURE;

    /**
     * @brief Get line quality
     *
     * Retrieves line quality percentage.
     *
     * @param puQuality         Receives quality (0-100%)
     *
     * @retval IO_SUCCESS       Quality retrieved
     * @retval IO_NOT_READY     Not connected
     */
    STDMETHOD_(IO_RETURN, GetLineQuality)(THIS_
        UINT32 *puQuality
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOModemController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOModemController_GetControllerInfo(p,a)      (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOModemController_Initialize(p)               (p)->lpVtbl->Initialize(p)
#define IIOModemController_Reset(p)                    (p)->lpVtbl->Reset(p)
#define IIOModemController_SendATCommand(p,a,b,c,d)    (p)->lpVtbl->SendATCommand(p,a,b,c,d)
#define IIOModemController_Dial(p,a)                   (p)->lpVtbl->Dial(p,a)
#define IIOModemController_Answer(p)                   (p)->lpVtbl->Answer(p)
#define IIOModemController_Hangup(p)                   (p)->lpVtbl->Hangup(p)
#define IIOModemController_GetState(p,a)               (p)->lpVtbl->GetState(p,a)
#define IIOModemController_GetConnectionSpeed(p,a)     (p)->lpVtbl->GetConnectionSpeed(p,a)
#define IIOModemController_SetSpeakerVolume(p,a)       (p)->lpVtbl->SetSpeakerVolume(p,a)
#define IIOModemController_SetSpeakerMode(p,a)         (p)->lpVtbl->SetSpeakerMode(p,a)
#define IIOModemController_EnableEcho(p)               (p)->lpVtbl->EnableEcho(p)
#define IIOModemController_DisableEcho(p)              (p)->lpVtbl->DisableEcho(p)
#define IIOModemController_SetResultCodeMode(p,a)      (p)->lpVtbl->SetResultCodeMode(p,a)
#define IIOModemController_EnableCallerID(p)           (p)->lpVtbl->EnableCallerID(p)
#define IIOModemController_DisableCallerID(p)          (p)->lpVtbl->DisableCallerID(p)
#define IIOModemController_SetErrorCorrection(p,a,b)   (p)->lpVtbl->SetErrorCorrection(p,a,b)
#define IIOModemController_SetDataCompression(p,a,b)   (p)->lpVtbl->SetDataCompression(p,a,b)
#define IIOModemController_EnterFaxMode(p,a)           (p)->lpVtbl->EnterFaxMode(p,a)
#define IIOModemController_EnterVoiceMode(p)           (p)->lpVtbl->EnterVoiceMode(p)
#define IIOModemController_EnterDataMode(p)            (p)->lpVtbl->EnterDataMode(p)
#define IIOModemController_GetStatistics(p,a,b)        (p)->lpVtbl->GetStatistics(p,a,b)
#define IIOModemController_RegisterATCallback(p,a,b)   (p)->lpVtbl->RegisterATCallback(p,a,b)
#define IIOModemController_RegisterRingCallback(p,a,b) (p)->lpVtbl->RegisterRingCallback(p,a,b)
#define IIOModemController_DetectCallProgress(p,a)     (p)->lpVtbl->DetectCallProgress(p,a)

#define IIOModemDevice_GetDeviceInfo(p,a)              (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOModemDevice_QueryCapabilities(p,a)          (p)->lpVtbl->QueryCapabilities(p,a)
#define IIOModemDevice_GetSignalStrength(p,a)          (p)->lpVtbl->GetSignalStrength(p,a)
#define IIOModemDevice_GetLineQuality(p,a)             (p)->lpVtbl->GetLineQuality(p,a)

#endif

/**
 * @brief Initialize Modem family subsystem
 *
 * Initializes the modem abstraction layer and registers it with IOKit.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
ModemInitialize(
    VOID
    );

/**
 * @brief Shutdown Modem family subsystem
 *
 * Shuts down the modem abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
ModemShutdown(
    VOID
    );

/**
 * @brief Create a modem controller instance
 *
 * Creates a modem controller interface for the specified serial port.
 *
 * @param uCOMPort          COM port number
 * @param ppController      Receives modem controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 * @retval IO_NO_DEVICE         Port does not exist
 */
IO_RETURN
ModemControllerCreate(
    UINT16 uCOMPort,
    IIOModemController **ppController
    );

/**
 * @brief Enumerate modems
 *
 * Enumerates all available modems in the system.
 *
 * @param ppControllers     Array to receive controller interfaces
 * @param puCount           On input: max controllers; On output: actual count
 *
 * @retval IO_SUCCESS       Enumeration successful
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
ModemEnumerateControllers(
    IIOModemController **ppControllers,
    UINT32 *puCount
    );

/**
 * @brief Detect modem type
 *
 * Detects the modem type by examining PCI IDs and querying the device.
 *
 * @param uVendorID         PCI/USB Vendor ID
 * @param uDeviceID         PCI/USB Device ID
 * @param pModemType        Receives modem type
 *
 * @retval IO_SUCCESS       Type detected successfully
 * @retval IO_NO_MATCH      Modem not in database
 */
IO_RETURN
ModemDetectType(
    UINT16 uVendorID,
    UINT16 uDeviceID,
    MODEM_TYPE *pModemType
    );

/**
 * @brief Get modem name from PCI IDs
 *
 * Looks up the modem name based on PCI Vendor ID and Device ID.
 *
 * @param uVendorID         PCI/USB Vendor ID
 * @param uDeviceID         PCI/USB Device ID
 * @param pszName           Buffer to receive modem name
 * @param cbSize            Size of buffer
 *
 * @retval IO_SUCCESS       Modem name found
 * @retval IO_NO_MATCH      Modem not in database
 */
IO_RETURN
ModemGetDeviceName(
    UINT16 uVendorID,
    UINT16 uDeviceID,
    CHAR8 *pszName,
    UINTN cbSize
    );

/**
 * @brief Parse AT response
 *
 * Parses an AT command response and extracts the result code.
 *
 * @param pszResponse       Response string
 * @param pResult           Receives result code
 *
 * @retval IO_SUCCESS       Response parsed successfully
 * @retval IO_BAD_ARGUMENT  Invalid response
 */
IO_RETURN
ModemParseATResponse(
    CONST CHAR8 *pszResponse,
    AT_RESULT *pResult
    );

/**
 * @brief Get modem database count
 *
 * Returns the number of modems in the database.
 *
 * @return Number of modem entries
 */
UINT32
ModemGetDatabaseCount(
    VOID
    );

/**
 * @brief Get modem by index
 *
 * Retrieves modem information by database index.
 *
 * @param uIndex            Database index
 * @param pControllerInfo   Receives controller information
 *
 * @retval IO_SUCCESS       Information retrieved
 * @retval IO_BAD_ARGUMENT  Invalid index
 */
IO_RETURN
ModemGetByIndex(
    UINT32 uIndex,
    MODEM_CONTROLLER_INFO *pControllerInfo
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_MODEM_H */
