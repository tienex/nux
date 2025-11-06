/**
 * @file hid.h
 * @brief HID (Human Interface Device) Family Interface - Comprehensive Input Bus Support
 *
 * This header defines the HID family interface for managing human interface devices
 * across multiple bus types and protocols. Provides unified interface for keyboards,
 * mice, joysticks, touchpads, touchscreens, and other input devices.
 *
 * Supported Bus Types:
 * - PS/2 (Personal System/2) - Keyboards and mice via 8042 controller
 * - ADB (Apple Desktop Bus) - Classic Macintosh input devices
 * - Serial - Serial mice (Microsoft, Logitech, MouseSystems protocols)
 * - Game Port - Analog joysticks and game controllers
 * - USB HID - Modern USB human interface devices (USB 1.1+)
 * - Bluetooth HID - Wireless HID over Bluetooth
 * - I2C HID - Touchpads and touchscreens on I2C bus
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_HID_H
#define IOKIT_HID_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOHIDController interface GUID
 * {C4D5E6F7-8A9B-4C3D-9E2F-1A0B8C7D6E5F}
 */
DEFINE_GUID(IID_IIOHIDController,
    0xC4D5E6F7, 0x8A9B, 0x4C3D, 0x9E, 0x2F, 0x1A, 0x0B, 0x8C, 0x7D, 0x6E, 0x5F);

/**
 * @brief IIOHIDDevice interface GUID
 * {D5E6F7A8-9B0C-4D3E-AF2B-1C0D8E7F6A5B}
 */
DEFINE_GUID(IID_IIOHIDDevice,
    0xD5E6F7A8, 0x9B0C, 0x4D3E, 0xAF, 0x2B, 0x1C, 0x0D, 0x8E, 0x7F, 0x6A, 0x5B);

//=============================================================================
// HID Bus Types and Device Types
//=============================================================================

/**
 * @brief HID bus types
 */
typedef enum _HID_BUS_TYPE {
    HID_BUS_UNKNOWN         = 0,    /**< Unknown bus */
    HID_BUS_PS2             = 1,    /**< PS/2 (8042 controller) */
    HID_BUS_ADB             = 2,    /**< Apple Desktop Bus */
    HID_BUS_SERIAL          = 3,    /**< Serial (RS-232) */
    HID_BUS_GAMEPORT        = 4,    /**< Game port (analog joysticks) */
    HID_BUS_USB             = 5,    /**< USB HID */
    HID_BUS_BLUETOOTH       = 6,    /**< Bluetooth HID */
    HID_BUS_I2C             = 7,    /**< I2C HID */
    HID_BUS_SPI             = 8,    /**< SPI HID (rare) */
} HID_BUS_TYPE;

/**
 * @brief HID device types
 */
typedef enum _HID_DEVICE_TYPE {
    HID_DEVICE_UNKNOWN      = 0,    /**< Unknown device */
    HID_DEVICE_KEYBOARD     = 1,    /**< Keyboard */
    HID_DEVICE_MOUSE        = 2,    /**< Mouse */
    HID_DEVICE_JOYSTICK     = 3,    /**< Joystick */
    HID_DEVICE_GAMEPAD      = 4,    /**< Gamepad/controller */
    HID_DEVICE_TOUCHPAD     = 5,    /**< Touchpad */
    HID_DEVICE_TOUCHSCREEN  = 6,    /**< Touchscreen */
    HID_DEVICE_DIGITIZER    = 7,    /**< Graphics tablet/digitizer */
    HID_DEVICE_BARCODE      = 8,    /**< Barcode scanner */
    HID_DEVICE_SENSOR       = 9,    /**< Generic sensor */
    HID_DEVICE_MULTITOUCH   = 10,   /**< Multi-touch device */
    HID_DEVICE_STYLUS       = 11,   /**< Stylus/pen */
    HID_DEVICE_TRACKBALL    = 12,   /**< Trackball */
    HID_DEVICE_TRACKPOINT   = 13,   /**< TrackPoint/pointing stick */
} HID_DEVICE_TYPE;

/**
 * @brief HID protocol types
 */
typedef enum _HID_PROTOCOL_TYPE {
    HID_PROTOCOL_UNKNOWN    = 0,    /**< Unknown protocol */
    HID_PROTOCOL_BOOT       = 1,    /**< Boot protocol (simplified) */
    HID_PROTOCOL_REPORT     = 2,    /**< Report protocol (full featured) */
    HID_PROTOCOL_LEGACY     = 3,    /**< Legacy protocol (non-HID) */
} HID_PROTOCOL_TYPE;

//=============================================================================
// PS/2 Specific Definitions
//=============================================================================

/**
 * @brief PS/2 controller ports
 */
#define PS2_DATA_PORT           0x60    /**< Data port */
#define PS2_STATUS_PORT         0x64    /**< Status register (read) */
#define PS2_COMMAND_PORT        0x64    /**< Command register (write) */

/**
 * @brief PS/2 controller status register bits
 */
#define PS2_STATUS_OUTPUT_FULL  0x01    /**< Output buffer full */
#define PS2_STATUS_INPUT_FULL   0x02    /**< Input buffer full */
#define PS2_STATUS_SYSTEM       0x04    /**< System flag */
#define PS2_STATUS_COMMAND      0x08    /**< Command/data flag */
#define PS2_STATUS_TIMEOUT      0x40    /**< Timeout error */
#define PS2_STATUS_PARITY       0x80    /**< Parity error */

/**
 * @brief PS/2 controller commands (sent to port 0x64)
 */
#define PS2_CMD_READ_CONFIG     0x20    /**< Read controller configuration byte */
#define PS2_CMD_WRITE_CONFIG    0x60    /**< Write controller configuration byte */
#define PS2_CMD_DISABLE_PORT2   0xA7    /**< Disable second PS/2 port (mouse) */
#define PS2_CMD_ENABLE_PORT2    0xA8    /**< Enable second PS/2 port */
#define PS2_CMD_TEST_PORT2      0xA9    /**< Test second PS/2 port */
#define PS2_CMD_TEST_CTRL       0xAA    /**< Test PS/2 controller */
#define PS2_CMD_TEST_PORT1      0xAB    /**< Test first PS/2 port (keyboard) */
#define PS2_CMD_DISABLE_PORT1   0xAD    /**< Disable first PS/2 port */
#define PS2_CMD_ENABLE_PORT1    0xAE    /**< Enable first PS/2 port */
#define PS2_CMD_WRITE_PORT2     0xD4    /**< Write to second PS/2 port */

/**
 * @brief PS/2 device commands (sent to port 0x60)
 */
#define PS2_DEV_CMD_LED         0xED    /**< Set/reset status indicators (LEDs) */
#define PS2_DEV_CMD_ECHO        0xEE    /**< Echo (diagnostic) */
#define PS2_DEV_CMD_SCAN_CODE   0xF0    /**< Get/set scan code set */
#define PS2_DEV_CMD_IDENTIFY    0xF2    /**< Identify keyboard/mouse */
#define PS2_DEV_CMD_RATE        0xF3    /**< Set typematic rate/delay */
#define PS2_DEV_CMD_ENABLE      0xF4    /**< Enable scanning */
#define PS2_DEV_CMD_DISABLE     0xF5    /**< Disable scanning (default) */
#define PS2_DEV_CMD_DEFAULT     0xF6    /**< Set default parameters */
#define PS2_DEV_CMD_RESEND      0xFE    /**< Resend last byte */
#define PS2_DEV_CMD_RESET       0xFF    /**< Reset and self-test */

/**
 * @brief PS/2 mouse commands
 */
#define PS2_MOUSE_CMD_SAMPLE    0xF3    /**< Set sample rate */
#define PS2_MOUSE_CMD_RESOLUTION 0xE8   /**< Set resolution */
#define PS2_MOUSE_CMD_STATUS    0xE9    /**< Request status */
#define PS2_MOUSE_CMD_STREAM    0xEA    /**< Set stream mode */
#define PS2_MOUSE_CMD_READ      0xEB    /**< Read data */
#define PS2_MOUSE_CMD_WRAP      0xEE    /**< Set wrap mode */
#define PS2_MOUSE_CMD_REMOTE    0xF0    /**< Set remote mode */
#define PS2_MOUSE_CMD_GET_ID    0xF2    /**< Get device ID */
#define PS2_MOUSE_CMD_SCALING_2 0xE7    /**< Set scaling 2:1 */
#define PS2_MOUSE_CMD_SCALING_1 0xE6    /**< Set scaling 1:1 */

/**
 * @brief PS/2 device responses
 */
#define PS2_RESPONSE_ACK        0xFA    /**< Acknowledge */
#define PS2_RESPONSE_RESEND     0xFE    /**< Resend request */
#define PS2_RESPONSE_ERROR      0xFC    /**< Error */
#define PS2_RESPONSE_ECHO       0xEE    /**< Echo response */
#define PS2_RESPONSE_BAT_OK     0xAA    /**< Self-test passed */
#define PS2_RESPONSE_BAT_FAIL   0xFC    /**< Self-test failed */

/**
 * @brief PS/2 keyboard scan code sets
 */
typedef enum _PS2_SCAN_CODE_SET {
    PS2_SCAN_SET_1          = 1,    /**< Scan code set 1 (XT, default) */
    PS2_SCAN_SET_2          = 2,    /**< Scan code set 2 (AT) */
    PS2_SCAN_SET_3          = 3,    /**< Scan code set 3 (PS/2) */
} PS2_SCAN_CODE_SET;

/**
 * @brief PS/2 mouse types (identified by device ID)
 */
typedef enum _PS2_MOUSE_TYPE {
    PS2_MOUSE_STANDARD      = 0x00, /**< Standard PS/2 mouse (2 buttons) */
    PS2_MOUSE_WHEEL         = 0x03, /**< IntelliMouse (3 buttons + wheel) */
    PS2_MOUSE_5BUTTON       = 0x04, /**< IntelliMouse Explorer (5 buttons + wheel) */
} PS2_MOUSE_TYPE;

/**
 * @brief PS/2 mouse packet flags (first byte)
 */
#define PS2_MOUSE_LEFT_BTN      0x01    /**< Left button */
#define PS2_MOUSE_RIGHT_BTN     0x02    /**< Right button */
#define PS2_MOUSE_MIDDLE_BTN    0x04    /**< Middle button */
#define PS2_MOUSE_ALWAYS_1      0x08    /**< Always 1 (validity bit) */
#define PS2_MOUSE_X_SIGN        0x10    /**< X movement sign */
#define PS2_MOUSE_Y_SIGN        0x20    /**< Y movement sign */
#define PS2_MOUSE_X_OVERFLOW    0x40    /**< X overflow */
#define PS2_MOUSE_Y_OVERFLOW    0x80    /**< Y overflow */

/**
 * @brief PS/2 device information
 */
typedef struct _PS2_DEVICE_INFO {
    PS2_MOUSE_TYPE      MouseType;          /**< Mouse type (if mouse) */
    PS2_SCAN_CODE_SET   ScanCodeSet;        /**< Scan code set (if keyboard) */
    UINT8               DeviceID;           /**< Device ID */
    UINT8               Resolution;         /**< Mouse resolution (counts/mm) */
    UINT8               SampleRate;         /**< Mouse sample rate (Hz) */
    BOOLEAN             bHasWheel;          /**< Mouse has scroll wheel */
    BOOLEAN             bHas5Buttons;       /**< Mouse has 5 buttons */
    BOOLEAN             bIntelliMouse;      /**< IntelliMouse protocol */
    UINT32              Capabilities;       /**< Device capabilities */
} PS2_DEVICE_INFO;

//=============================================================================
// ADB (Apple Desktop Bus) Specific Definitions
//=============================================================================

/**
 * @brief ADB addresses (device addresses on bus)
 */
#define ADB_ADDR_DONGLE         1       /**< Security dongle */
#define ADB_ADDR_KEYBOARD       2       /**< Keyboard (default) */
#define ADB_ADDR_MOUSE          3       /**< Mouse (default) */
#define ADB_ADDR_TABLET         4       /**< Graphics tablet */
#define ADB_ADDR_MODEM          5       /**< Modem */
#define ADB_ADDR_RESERVED       6       /**< Reserved */
#define ADB_ADDR_APPLIANCE      7       /**< Misc appliance */
#define ADB_ADDR_FREE_MIN       8       /**< Free addresses start */
#define ADB_ADDR_FREE_MAX       15      /**< Free addresses end */

/**
 * @brief ADB commands
 */
typedef enum _ADB_COMMAND {
    ADB_CMD_SEND_RESET      = 0x00, /**< Send reset */
    ADB_CMD_FLUSH           = 0x01, /**< Flush device buffers */
    ADB_CMD_LISTEN          = 0x08, /**< Listen (write to device) */
    ADB_CMD_TALK            = 0x0C, /**< Talk (read from device) */
} ADB_COMMAND;

/**
 * @brief ADB register numbers (0-3)
 */
#define ADB_REG_0               0       /**< Register 0 (device data) */
#define ADB_REG_1               1       /**< Register 1 (device data) */
#define ADB_REG_2               2       /**< Register 2 (device data) */
#define ADB_REG_3               3       /**< Register 3 (device info/handler) */

/**
 * @brief ADB handler IDs for different device types
 */
#define ADB_HANDLER_KEYBOARD    1       /**< Standard keyboard */
#define ADB_HANDLER_ISOKBD      2       /**< ISO keyboard */
#define ADB_HANDLER_EXTKBD      4       /**< Extended keyboard */
#define ADB_HANDLER_MOUSE       1       /**< Standard mouse */
#define ADB_HANDLER_EXTMOUSE    2       /**< Extended mouse */
#define ADB_HANDLER_TABLET      1       /**< Graphics tablet */

/**
 * @brief ADB controller types
 */
typedef enum _ADB_CONTROLLER_TYPE {
    ADB_CTRL_CUDA           = 1,    /**< CUDA (Macs with 68K/early PPC) */
    ADB_CTRL_PMU            = 2,    /**< PMU (PowerBooks) */
    ADB_CTRL_IOP            = 3,    /**< IOP-based (Mac II family) */
    ADB_CTRL_EGRET          = 4,    /**< Egret (Mac IIsi) */
} ADB_CONTROLLER_TYPE;

/**
 * @brief ADB device information
 */
typedef struct _ADB_DEVICE_INFO {
    UINT8                   Address;        /**< Device address (0-15) */
    UINT8                   HandlerID;      /**< Handler ID */
    UINT8                   DefaultAddress; /**< Default address */
    UINT16                  Features;       /**< Feature flags */
    ADB_CONTROLLER_TYPE     ControllerType; /**< Controller type */
    BOOLEAN                 bOriginalDevice;/**< Original Apple device */
    CHAR8                   DeviceName[64]; /**< Device name */
} ADB_DEVICE_INFO;

//=============================================================================
// Serial Mouse Specific Definitions
//=============================================================================

/**
 * @brief Serial mouse protocols
 */
typedef enum _SERIAL_MOUSE_PROTOCOL {
    SERIAL_MOUSE_MICROSOFT  = 1,    /**< Microsoft 2-button protocol */
    SERIAL_MOUSE_LOGITECH   = 2,    /**< Logitech 3-button protocol */
    SERIAL_MOUSE_MOUSESYS   = 3,    /**< MouseSystems 3-button protocol */
    SERIAL_MOUSE_MM         = 4,    /**< MM series protocol */
    SERIAL_MOUSE_LOGITECH_WHEEL = 5,/**< Logitech with wheel */
} SERIAL_MOUSE_PROTOCOL;

/**
 * @brief Serial mouse baud rates
 */
typedef enum _SERIAL_MOUSE_BAUD {
    SERIAL_BAUD_300         = 300,      /**< 300 baud */
    SERIAL_BAUD_1200        = 1200,     /**< 1200 baud (most common) */
    SERIAL_BAUD_2400        = 2400,     /**< 2400 baud */
    SERIAL_BAUD_4800        = 4800,     /**< 4800 baud */
    SERIAL_BAUD_9600        = 9600,     /**< 9600 baud */
} SERIAL_MOUSE_BAUD;

/**
 * @brief Serial port data format
 */
typedef struct _SERIAL_DATA_FORMAT {
    UINT8   DataBits;           /**< Data bits (7 or 8) */
    UINT8   StopBits;           /**< Stop bits (1 or 2) */
    UINT8   Parity;             /**< Parity (0=none, 1=odd, 2=even) */
} SERIAL_DATA_FORMAT;

/**
 * @brief Serial mouse information
 */
typedef struct _SERIAL_MOUSE_INFO {
    SERIAL_MOUSE_PROTOCOL   Protocol;       /**< Mouse protocol */
    SERIAL_MOUSE_BAUD       BaudRate;       /**< Baud rate */
    SERIAL_DATA_FORMAT      DataFormat;     /**< Data format */
    UINT8                   NumButtons;     /**< Number of buttons */
    BOOLEAN                 bHasWheel;      /**< Has scroll wheel */
    UINT32                  PortNumber;     /**< Serial port number (COM1=1, etc.) */
    UINT16                  PortBase;       /**< I/O port base address */
} SERIAL_MOUSE_INFO;

/**
 * @brief Microsoft serial mouse packet format
 * 3 bytes: [0][1][2]
 * [0]: 01LRYYXX (L=left, R=right, YY=Y sign/MSB, XX=X sign/MSB)
 * [1]: 00XXXXXX (X movement low 6 bits)
 * [2]: 00YYYYYY (Y movement low 6 bits)
 */
#define SERIAL_MS_SYNC_BIT      0x40    /**< Sync bit (byte 0) */
#define SERIAL_MS_LEFT_BTN      0x20    /**< Left button */
#define SERIAL_MS_RIGHT_BTN     0x10    /**< Right button */

/**
 * @brief MouseSystems serial mouse packet format
 * 5 bytes: [0][1][2][3][4]
 * [0]: 10000LMR (L=left, M=middle, R=right)
 * [1]: X movement byte 1
 * [2]: Y movement byte 1
 * [3]: X movement byte 2
 * [4]: Y movement byte 2
 */
#define SERIAL_MSYS_SYNC        0x80    /**< Sync byte */
#define SERIAL_MSYS_LEFT_BTN    0x04    /**< Left button */
#define SERIAL_MSYS_MIDDLE_BTN  0x02    /**< Middle button */
#define SERIAL_MSYS_RIGHT_BTN   0x01    /**< Right button */

//=============================================================================
// Game Port Specific Definitions
//=============================================================================

/**
 * @brief Game port I/O address (standard)
 */
#define GAMEPORT_IO_ADDR        0x201   /**< Game port base address */

/**
 * @brief Game port data register bits
 */
#define GAMEPORT_BTN_A1         0x10    /**< Joystick A button 1 */
#define GAMEPORT_BTN_A2         0x20    /**< Joystick A button 2 */
#define GAMEPORT_BTN_B1         0x40    /**< Joystick B button 1 */
#define GAMEPORT_BTN_B2         0x80    /**< Joystick B button 2 */
#define GAMEPORT_AXIS_AX        0x01    /**< Joystick A X-axis */
#define GAMEPORT_AXIS_AY        0x02    /**< Joystick A Y-axis */
#define GAMEPORT_AXIS_BX        0x04    /**< Joystick B X-axis */
#define GAMEPORT_AXIS_BY        0x08    /**< Joystick B Y-axis */

/**
 * @brief Game port device types
 */
typedef enum _GAMEPORT_DEVICE_TYPE {
    GAMEPORT_JOYSTICK_ANALOG    = 1,    /**< Analog joystick */
    GAMEPORT_GAMEPAD            = 2,    /**< Gamepad */
    GAMEPORT_RACING_WHEEL       = 3,    /**< Racing wheel */
    GAMEPORT_FLIGHT_STICK       = 4,    /**< Flight stick */
} GAMEPORT_DEVICE_TYPE;

/**
 * @brief Game port device information
 */
typedef struct _GAMEPORT_DEVICE_INFO {
    GAMEPORT_DEVICE_TYPE    Type;           /**< Device type */
    UINT8                   NumButtons;     /**< Number of buttons */
    UINT8                   NumAxes;        /**< Number of axes */
    BOOLEAN                 bHasPOV;        /**< Has POV hat switch */
    UINT16                  CalibMin[4];    /**< Calibration minimum per axis */
    UINT16                  CalibMax[4];    /**< Calibration maximum per axis */
    UINT16                  CalibCenter[4]; /**< Calibration center per axis */
} GAMEPORT_DEVICE_INFO;

//=============================================================================
// USB HID Specific Definitions
//=============================================================================

/**
 * @brief USB HID descriptor types
 */
#define USB_HID_DESC_HID        0x21    /**< HID descriptor */
#define USB_HID_DESC_REPORT     0x22    /**< Report descriptor */
#define USB_HID_DESC_PHYSICAL   0x23    /**< Physical descriptor */

/**
 * @brief USB HID subclass codes
 */
#define USB_HID_SUBCLASS_NONE   0x00    /**< No subclass */
#define USB_HID_SUBCLASS_BOOT   0x01    /**< Boot interface */

/**
 * @brief USB HID protocol codes
 */
#define USB_HID_PROTOCOL_NONE   0x00    /**< No protocol */
#define USB_HID_PROTOCOL_KBD    0x01    /**< Keyboard protocol */
#define USB_HID_PROTOCOL_MOUSE  0x02    /**< Mouse protocol */

/**
 * @brief USB HID class-specific requests
 */
#define USB_HID_REQ_GET_REPORT      0x01    /**< Get report */
#define USB_HID_REQ_GET_IDLE        0x02    /**< Get idle rate */
#define USB_HID_REQ_GET_PROTOCOL    0x03    /**< Get protocol */
#define USB_HID_REQ_SET_REPORT      0x09    /**< Set report */
#define USB_HID_REQ_SET_IDLE        0x0A    /**< Set idle rate */
#define USB_HID_REQ_SET_PROTOCOL    0x0B    /**< Set protocol */

/**
 * @brief USB HID report types
 */
#define USB_HID_REPORT_INPUT    0x01    /**< Input report */
#define USB_HID_REPORT_OUTPUT   0x02    /**< Output report */
#define USB_HID_REPORT_FEATURE  0x03    /**< Feature report */

/**
 * @brief USB HID descriptor
 */
typedef struct _USB_HID_DESCRIPTOR {
    UINT8   bLength;                /**< Descriptor length */
    UINT8   bDescriptorType;        /**< Descriptor type (0x21) */
    UINT16  bcdHID;                 /**< HID specification release (BCD) */
    UINT8   bCountryCode;           /**< Country code */
    UINT8   bNumDescriptors;        /**< Number of class descriptors */
    UINT8   bReportDescriptorType;  /**< Report descriptor type */
    UINT16  wReportDescriptorLength;/**< Report descriptor length */
} USB_HID_DESCRIPTOR;

/**
 * @brief USB HID device information
 */
typedef struct _USB_HID_DEVICE_INFO {
    UINT16  VendorID;               /**< USB vendor ID */
    UINT16  ProductID;              /**< USB product ID */
    UINT16  VersionNumber;          /**< Device version (BCD) */
    UINT8   InterfaceNumber;        /**< Interface number */
    UINT8   InterfaceSubclass;      /**< Interface subclass */
    UINT8   InterfaceProtocol;      /**< Interface protocol */
    UINT16  ReportDescriptorSize;   /**< Report descriptor size */
    UINT8   NumEndpoints;           /**< Number of endpoints */
    UINT8   InputEndpoint;          /**< Input endpoint address */
    UINT8   OutputEndpoint;         /**< Output endpoint address */
} USB_HID_DEVICE_INFO;

//=============================================================================
// Bluetooth HID Specific Definitions
//=============================================================================

/**
 * @brief Bluetooth HID service UUIDs
 */
#define BT_HID_SERVICE_UUID     0x1124  /**< Human Interface Device service */
#define BT_HID_DEVICE_UUID      0x0124  /**< HID Device profile UUID */

/**
 * @brief Bluetooth HID message types
 */
typedef enum _BT_HID_MESSAGE_TYPE {
    BT_HID_MSG_HANDSHAKE        = 0x00, /**< Handshake */
    BT_HID_MSG_CONTROL          = 0x01, /**< HID control */
    BT_HID_MSG_GET_REPORT       = 0x04, /**< Get report */
    BT_HID_MSG_SET_REPORT       = 0x05, /**< Set report */
    BT_HID_MSG_GET_PROTOCOL     = 0x06, /**< Get protocol */
    BT_HID_MSG_SET_PROTOCOL     = 0x07, /**< Set protocol */
    BT_HID_MSG_DATA             = 0x0A, /**< Data */
} BT_HID_MESSAGE_TYPE;

/**
 * @brief Bluetooth HID device information
 */
typedef struct _BT_HID_DEVICE_INFO {
    UINT8   BDAddr[6];              /**< Bluetooth device address */
    UINT16  VendorID;               /**< Vendor ID */
    UINT16  ProductID;              /**< Product ID */
    UINT16  Version;                /**< Version */
    UINT8   DeviceSubclass;         /**< Device subclass */
    UINT8   CountryCode;            /**< Country code */
    BOOLEAN bVirtualCable;          /**< Virtual cable enabled */
    BOOLEAN bReconnectInit;         /**< Reconnect initiate */
    UINT16  ReportDescriptorSize;   /**< Report descriptor size */
    CHAR8   DeviceName[64];         /**< Device name */
} BT_HID_DEVICE_INFO;

//=============================================================================
// I2C HID Specific Definitions
//=============================================================================

/**
 * @brief I2C HID descriptor register (fixed address)
 */
#define I2C_HID_DESC_REG        0x0001  /**< Descriptor register */

/**
 * @brief I2C HID commands
 */
#define I2C_HID_CMD_RESET       0x01    /**< Reset command */
#define I2C_HID_CMD_GET_REPORT  0x02    /**< Get report */
#define I2C_HID_CMD_SET_REPORT  0x03    /**< Set report */
#define I2C_HID_CMD_GET_IDLE    0x04    /**< Get idle */
#define I2C_HID_CMD_SET_IDLE    0x05    /**< Set idle */
#define I2C_HID_CMD_GET_PROTOCOL 0x06   /**< Get protocol */
#define I2C_HID_CMD_SET_PROTOCOL 0x07   /**< Set protocol */
#define I2C_HID_CMD_SET_POWER   0x08    /**< Set power */

/**
 * @brief I2C HID power states
 */
#define I2C_HID_POWER_ON        0x00    /**< Power on */
#define I2C_HID_POWER_SLEEP     0x01    /**< Sleep mode */

/**
 * @brief I2C HID descriptor
 */
typedef struct _I2C_HID_DESCRIPTOR {
    UINT16  wHIDDescLength;         /**< Descriptor length */
    UINT16  bcdVersion;             /**< I2C HID version (BCD) */
    UINT16  wReportDescLength;      /**< Report descriptor length */
    UINT16  wReportDescRegister;    /**< Report descriptor register */
    UINT16  wInputRegister;         /**< Input register */
    UINT16  wMaxInputLength;        /**< Max input length */
    UINT16  wOutputRegister;        /**< Output register */
    UINT16  wMaxOutputLength;       /**< Max output length */
    UINT16  wCommandRegister;       /**< Command register */
    UINT16  wDataRegister;          /**< Data register */
    UINT16  wVendorID;              /**< Vendor ID */
    UINT16  wProductID;             /**< Product ID */
    UINT16  wVersionID;             /**< Version ID */
    UINT32  dwReserved;             /**< Reserved */
} I2C_HID_DESCRIPTOR;

/**
 * @brief I2C HID device information
 */
typedef struct _I2C_HID_DEVICE_INFO {
    UINT8   I2CAddress;             /**< I2C device address */
    UINT8   I2CBusNumber;           /**< I2C bus number */
    UINT16  VendorID;               /**< Vendor ID */
    UINT16  ProductID;              /**< Product ID */
    UINT16  VersionID;              /**< Version ID */
    UINT16  MaxInputLength;         /**< Maximum input report length */
    UINT16  MaxOutputLength;        /**< Maximum output report length */
    I2C_HID_DESCRIPTOR Descriptor;  /**< I2C HID descriptor */
} I2C_HID_DEVICE_INFO;

//=============================================================================
// Unified HID Device Information
//=============================================================================

/**
 * @brief HID input report
 */
typedef struct _HID_INPUT_REPORT {
    UINT8   ReportID;               /**< Report ID (0 if not used) */
    UINT8   *pData;                 /**< Report data */
    UINT32  cbDataSize;             /**< Data size in bytes */
    UINT64  Timestamp;              /**< Timestamp (microseconds) */
} HID_INPUT_REPORT;

/**
 * @brief HID output report
 */
typedef struct _HID_OUTPUT_REPORT {
    UINT8   ReportID;               /**< Report ID (0 if not used) */
    UINT8   *pData;                 /**< Report data */
    UINT32  cbDataSize;             /**< Data size in bytes */
} HID_OUTPUT_REPORT;

/**
 * @brief HID feature report
 */
typedef struct _HID_FEATURE_REPORT {
    UINT8   ReportID;               /**< Report ID (0 if not used) */
    UINT8   *pData;                 /**< Report data */
    UINT32  cbDataSize;             /**< Data size in bytes */
} HID_FEATURE_REPORT;

/**
 * @brief Unified HID device information
 */
typedef struct _HID_DEVICE_INFO {
    HID_BUS_TYPE            BusType;        /**< Bus type */
    HID_DEVICE_TYPE         DeviceType;     /**< Device type */
    HID_PROTOCOL_TYPE       ProtocolType;   /**< Protocol type */
    CHAR8                   DeviceName[128];/**< Device name */
    CHAR8                   Manufacturer[128];/**< Manufacturer */
    CHAR8                   SerialNumber[64];/**< Serial number */
    UINT32                  Capabilities;   /**< Capability flags */

    // Bus-specific information (union)
    union {
        PS2_DEVICE_INFO     PS2;            /**< PS/2 specific info */
        ADB_DEVICE_INFO     ADB;            /**< ADB specific info */
        SERIAL_MOUSE_INFO   Serial;         /**< Serial specific info */
        GAMEPORT_DEVICE_INFO GamePort;      /**< Game port specific info */
        USB_HID_DEVICE_INFO USB;            /**< USB HID specific info */
        BT_HID_DEVICE_INFO  Bluetooth;      /**< Bluetooth HID specific info */
        I2C_HID_DEVICE_INFO I2C;            /**< I2C HID specific info */
    } BusInfo;
} HID_DEVICE_INFO;

/**
 * @brief HID controller information
 */
typedef struct _HID_CONTROLLER_INFO {
    HID_BUS_TYPE        BusType;            /**< Bus type */
    CHAR8               ControllerName[64]; /**< Controller name */
    UINT32              Capabilities;       /**< Controller capabilities */
    UINT8               NumPorts;           /**< Number of ports */
    UINT64              BaseAddress;        /**< Base I/O or memory address */
    UINT32              IRQNumber;          /**< IRQ number */

    // Bus-specific information
    union {
        struct {
            BOOLEAN bDualChannel;           /**< Dual-channel 8042 */
            BOOLEAN bPort1Enabled;          /**< Port 1 (keyboard) enabled */
            BOOLEAN bPort2Enabled;          /**< Port 2 (mouse) enabled */
        } PS2;

        struct {
            ADB_CONTROLLER_TYPE Type;       /**< ADB controller type */
            UINT8   NumDevices;             /**< Number of devices on bus */
        } ADB;

        struct {
            UINT8   NumPorts;               /**< Number of serial ports */
            UINT16  PortBases[8];           /**< I/O port bases */
        } Serial;
    } CtrlInfo;
} HID_CONTROLLER_INFO;

/**
 * @brief HID capability flags
 */
#define HID_CAP_KEYBOARD        0x00000001  /**< Keyboard support */
#define HID_CAP_MOUSE           0x00000002  /**< Mouse support */
#define HID_CAP_JOYSTICK        0x00000004  /**< Joystick support */
#define HID_CAP_TOUCHPAD        0x00000008  /**< Touchpad support */
#define HID_CAP_TOUCHSCREEN     0x00000010  /**< Touchscreen support */
#define HID_CAP_DIGITIZER       0x00000020  /**< Digitizer support */
#define HID_CAP_MULTITOUCH      0x00000040  /**< Multi-touch support */
#define HID_CAP_FORCE_FEEDBACK  0x00000080  /**< Force feedback support */
#define HID_CAP_LEDS            0x00000100  /**< LED indicators */
#define HID_CAP_AUDIO           0x00000200  /**< Audio feedback (beep) */
#define HID_CAP_HOTPLUG         0x00000400  /**< Hot-plug capable */
#define HID_CAP_WIRELESS        0x00000800  /**< Wireless device */
#define HID_CAP_BATTERY         0x00001000  /**< Battery powered */

//=============================================================================
// HID Controller and Device Interfaces
//=============================================================================

// Forward declarations
DECLARE_INTERFACE_(IIOHIDController, IIOService);
DECLARE_INTERFACE_(IIOHIDDevice, IIOService);

/**
 * @brief HID input event callback
 *
 * @param pContext      User context
 * @param pReport       Input report
 */
typedef VOID (*HID_INPUT_CALLBACK)(
    VOID *pContext,
    CONST HID_INPUT_REPORT *pReport
    );

/**
 * @brief IIOHIDController - HID Controller/Bus Interface
 *
 * Represents a HID controller or bus (PS/2 8042, ADB controller, etc.)
 */
#undef INTERFACE
#define INTERFACE IIOHIDController

DECLARE_INTERFACE_(IIOHIDController, IIOService)
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

    // IIOHIDController specific methods

    /**
     * @brief Get controller information
     *
     * @param pInfo         Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        HID_CONTROLLER_INFO *pInfo
        ) PURE;

    /**
     * @brief Enumerate HID devices on controller/bus
     *
     * @param ppDevices     Receives array of device interfaces
     * @param puCount       On input: max count; On output: actual count
     *
     * @retval IO_SUCCESS       Enumeration successful
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NO_MEMORY     Insufficient memory
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOHIDDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Reset the controller
     *
     * Performs a hardware reset of the HID controller.
     *
     * @retval IO_SUCCESS   Reset successful
     * @retval IO_ERROR     Reset failed
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;

    /**
     * @brief Enable a device port/channel
     *
     * @param uPort         Port/channel number
     *
     * @retval IO_SUCCESS       Port enabled
     * @retval IO_BAD_ARGUMENT  Invalid port number
     */
    STDMETHOD_(IO_RETURN, EnableDevice)(THIS_
        UINT8 uPort
        ) PURE;

    /**
     * @brief Disable a device port/channel
     *
     * @param uPort         Port/channel number
     *
     * @retval IO_SUCCESS       Port disabled
     * @retval IO_BAD_ARGUMENT  Invalid port number
     */
    STDMETHOD_(IO_RETURN, DisableDevice)(THIS_
        UINT8 uPort
        ) PURE;

    /**
     * @brief Set sample rate (PS/2 mouse specific)
     *
     * Sets the mouse sample rate in Hz.
     *
     * @param uPort         Port number
     * @param uRate         Sample rate (10, 20, 40, 60, 80, 100, 200 Hz)
     *
     * @retval IO_SUCCESS       Sample rate set
     * @retval IO_UNSUPPORTED   Not supported by device
     */
    STDMETHOD_(IO_RETURN, SetSampleRate)(THIS_
        UINT8 uPort,
        UINT8 uRate
        ) PURE;

    /**
     * @brief Set resolution (PS/2 mouse specific)
     *
     * Sets the mouse resolution in counts per mm.
     *
     * @param uPort         Port number
     * @param uResolution   Resolution (1, 2, 4, 8 counts/mm)
     *
     * @retval IO_SUCCESS       Resolution set
     * @retval IO_UNSUPPORTED   Not supported by device
     */
    STDMETHOD_(IO_RETURN, SetResolution)(THIS_
        UINT8 uPort,
        UINT8 uResolution
        ) PURE;

    /**
     * @brief Send raw command to device
     *
     * @param uPort         Port number
     * @param uCommand      Command byte
     * @param pResponse     Receives response byte(s)
     * @param pcbResponse   On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS       Command sent successfully
     * @retval IO_TIMEOUT       Command timeout
     */
    STDMETHOD_(IO_RETURN, SendCommand)(THIS_
        UINT8 uPort,
        UINT8 uCommand,
        UINT8 *pResponse,
        UINT32 *pcbResponse
        ) PURE;
};

/**
 * @brief IIOHIDDevice - HID Device Interface
 *
 * Represents an individual HID device (keyboard, mouse, etc.)
 */
#undef INTERFACE
#define INTERFACE IIOHIDDevice

DECLARE_INTERFACE_(IIOHIDDevice, IIOService)
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

    // IIOHIDDevice specific methods

    /**
     * @brief Get device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        HID_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Send command to device
     *
     * @param uCommand      Command byte
     * @param pParams       Command parameters
     * @param cbParamsSize  Parameter size
     * @param pResponse     Receives response
     * @param pcbResponse   On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS       Command sent successfully
     * @retval IO_TIMEOUT       Command timeout
     */
    STDMETHOD_(IO_RETURN, SendCommand)(THIS_
        UINT8 uCommand,
        CONST VOID *pParams,
        UINT32 cbParamsSize,
        VOID *pResponse,
        UINT32 *pcbResponse
        ) PURE;

    /**
     * @brief Receive data from device
     *
     * Reads raw data from the device (non-blocking).
     *
     * @param pBuffer       Receives data
     * @param pcbSize       On input: buffer size; On output: bytes read
     *
     * @retval IO_SUCCESS       Data received
     * @retval IO_NO_DATA       No data available
     */
    STDMETHOD_(IO_RETURN, ReceiveData)(THIS_
        VOID *pBuffer,
        UINT32 *pcbSize
        ) PURE;

    /**
     * @brief Get input report
     *
     * @param pReport       Receives input report
     *
     * @retval IO_SUCCESS       Report retrieved
     * @retval IO_NO_DATA       No report available
     */
    STDMETHOD_(IO_RETURN, GetInputReport)(THIS_
        HID_INPUT_REPORT *pReport
        ) PURE;

    /**
     * @brief Set feature report
     *
     * @param pReport       Feature report to set
     *
     * @retval IO_SUCCESS       Report set successfully
     * @retval IO_UNSUPPORTED   Feature reports not supported
     */
    STDMETHOD_(IO_RETURN, SetFeatureReport)(THIS_
        CONST HID_FEATURE_REPORT *pReport
        ) PURE;

    /**
     * @brief Get feature report
     *
     * @param pReport       Receives feature report
     *
     * @retval IO_SUCCESS       Report retrieved
     * @retval IO_UNSUPPORTED   Feature reports not supported
     */
    STDMETHOD_(IO_RETURN, GetFeatureReport)(THIS_
        HID_FEATURE_REPORT *pReport
        ) PURE;

    /**
     * @brief Set protocol (boot vs report)
     *
     * @param Protocol      Protocol type
     *
     * @retval IO_SUCCESS       Protocol set
     * @retval IO_UNSUPPORTED   Protocol not supported
     */
    STDMETHOD_(IO_RETURN, SetProtocol)(THIS_
        HID_PROTOCOL_TYPE Protocol
        ) PURE;

    /**
     * @brief Get HID descriptor (USB HID specific)
     *
     * @param pDescriptor   Receives HID descriptor
     * @param pBuffer       Buffer for descriptor data
     * @param pcbSize       On input: buffer size; On output: actual size
     *
     * @retval IO_SUCCESS       Descriptor retrieved
     * @retval IO_UNSUPPORTED   Not a USB HID device
     */
    STDMETHOD_(IO_RETURN, GetDescriptor)(THIS_
        USB_HID_DESCRIPTOR *pDescriptor,
        VOID *pBuffer,
        UINT32 *pcbSize
        ) PURE;

    /**
     * @brief Register input event callback
     *
     * @param pfnCallback   Callback function
     * @param pContext      User context
     *
     * @retval IO_SUCCESS   Callback registered
     */
    STDMETHOD_(IO_RETURN, RegisterInputCallback)(THIS_
        HID_INPUT_CALLBACK pfnCallback,
        VOID *pContext
        ) PURE;

    /**
     * @brief Enable device
     *
     * Enables the device for input.
     *
     * @retval IO_SUCCESS   Device enabled
     */
    STDMETHOD_(IO_RETURN, Enable)(THIS) PURE;

    /**
     * @brief Disable device
     *
     * Disables the device.
     *
     * @retval IO_SUCCESS   Device disabled
     */
    STDMETHOD_(IO_RETURN, Disable)(THIS) PURE;

    /**
     * @brief Reset device
     *
     * Performs a device reset and self-test.
     *
     * @retval IO_SUCCESS   Reset successful
     * @retval IO_ERROR     Reset failed
     */
    STDMETHOD_(IO_RETURN, Reset)(THIS) PURE;
};

#undef INTERFACE

//=============================================================================
// Convenience Macros
//=============================================================================

#if !defined(__cplusplus) || defined(CINTERFACE)

// IIOHIDController macros
#define IIOHIDController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOHIDController_EnumerateDevices(p,a,b)    (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOHIDController_ResetController(p)         (p)->lpVtbl->ResetController(p)
#define IIOHIDController_EnableDevice(p,a)          (p)->lpVtbl->EnableDevice(p,a)
#define IIOHIDController_DisableDevice(p,a)         (p)->lpVtbl->DisableDevice(p,a)
#define IIOHIDController_SetSampleRate(p,a,b)       (p)->lpVtbl->SetSampleRate(p,a,b)
#define IIOHIDController_SetResolution(p,a,b)       (p)->lpVtbl->SetResolution(p,a,b)
#define IIOHIDController_SendCommand(p,a,b,c,d)     (p)->lpVtbl->SendCommand(p,a,b,c,d)

// IIOHIDDevice macros
#define IIOHIDDevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOHIDDevice_SendCommand(p,a,b,c,d,e)       (p)->lpVtbl->SendCommand(p,a,b,c,d,e)
#define IIOHIDDevice_ReceiveData(p,a,b)             (p)->lpVtbl->ReceiveData(p,a,b)
#define IIOHIDDevice_GetInputReport(p,a)            (p)->lpVtbl->GetInputReport(p,a)
#define IIOHIDDevice_SetFeatureReport(p,a)          (p)->lpVtbl->SetFeatureReport(p,a)
#define IIOHIDDevice_GetFeatureReport(p,a)          (p)->lpVtbl->GetFeatureReport(p,a)
#define IIOHIDDevice_SetProtocol(p,a)               (p)->lpVtbl->SetProtocol(p,a)
#define IIOHIDDevice_GetDescriptor(p,a,b,c)         (p)->lpVtbl->GetDescriptor(p,a,b,c)
#define IIOHIDDevice_RegisterInputCallback(p,a,b)   (p)->lpVtbl->RegisterInputCallback(p,a,b)
#define IIOHIDDevice_Enable(p)                      (p)->lpVtbl->Enable(p)
#define IIOHIDDevice_Disable(p)                     (p)->lpVtbl->Disable(p)
#define IIOHIDDevice_Reset(p)                       (p)->lpVtbl->Reset(p)

#endif

//=============================================================================
// HID Subsystem Functions
//=============================================================================

/**
 * @brief Initialize HID subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
HIDInitialize(
    VOID
    );

/**
 * @brief Shutdown HID subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
HIDShutdown(
    VOID
    );

/**
 * @brief Create a HID controller instance
 *
 * @param BusType           Bus type
 * @param pszName           Controller name
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid parameter
 */
IO_RETURN
IOHIDControllerCreate(
    HID_BUS_TYPE        BusType,
    CONST CHAR8        *pszName,
    IIOHIDController  **ppController
    );

/**
 * @brief Create a HID device instance
 *
 * @param pDeviceInfo       Device information
 * @param ppDevice          Receives device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid parameter
 */
IO_RETURN
IOHIDDeviceCreate(
    CONST HID_DEVICE_INFO *pDeviceInfo,
    IIOHIDDevice         **ppDevice
    );

/**
 * @brief Detect PS/2 controller (8042)
 *
 * Detects and initializes the PS/2 keyboard/mouse controller.
 *
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller detected
 * @retval IO_NOT_FOUND     No PS/2 controller found
 */
IO_RETURN
HIDDetectPS2Controller(
    IIOHIDController **ppController
    );

/**
 * @brief Detect ADB controller
 *
 * Detects and initializes the Apple Desktop Bus controller.
 *
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS       Controller detected
 * @retval IO_NOT_FOUND     No ADB controller found
 */
IO_RETURN
HIDDetectADBController(
    IIOHIDController **ppController
    );

#ifdef __cplusplus
}
#endif

#endif // IOKIT_HID_H
