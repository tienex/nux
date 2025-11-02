/**
 * @file usb.h
 * @brief USB Family Interface - USB 1.0/1.1/2.0/3.x/4.0 Support
 *
 * This header defines the USB (Universal Serial Bus) family interface for
 * managing USB host controllers, hubs, and devices.
 *
 * Supports:
 * - USB 1.0 (Low Speed): 1.5 Mbps
 * - USB 1.1 (Full Speed): 12 Mbps
 * - USB 2.0 (High Speed): 480 Mbps
 * - USB 3.0 (SuperSpeed): 5 Gbps
 * - USB 3.1 (SuperSpeed+): 10 Gbps
 * - USB 3.2 (SuperSpeed+ Gen 2x2): 20 Gbps
 * - USB 4.0 (based on Thunderbolt 3): 40 Gbps
 *
 * Controller Types:
 * - UHCI (USB 1.x, Intel)
 * - OHCI (USB 1.x, Compaq/MS/National Semi)
 * - EHCI (USB 2.0)
 * - xHCI (USB 3.x/4.0)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_USB_H
#define IOKIT_USB_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB specifications/versions
 */
typedef enum _USB_VERSION {
    USB_VERSION_UNKNOWN     = 0x0000,
    USB_VERSION_1_0         = 0x0100,   /**< USB 1.0 (1.5 Mbps LS) */
    USB_VERSION_1_1         = 0x0110,   /**< USB 1.1 (12 Mbps FS) */
    USB_VERSION_2_0         = 0x0200,   /**< USB 2.0 (480 Mbps HS) */
    USB_VERSION_3_0         = 0x0300,   /**< USB 3.0 (5 Gbps SS) */
    USB_VERSION_3_1         = 0x0310,   /**< USB 3.1 (10 Gbps SS+) */
    USB_VERSION_3_2         = 0x0320,   /**< USB 3.2 (20 Gbps SS+ Gen2x2) */
    USB_VERSION_4_0         = 0x0400,   /**< USB 4.0 (40 Gbps) */
} USB_VERSION;

/**
 * @brief USB device speeds
 */
typedef enum _USB_SPEED {
    USB_SPEED_LOW           = 0,        /**< Low Speed (1.5 Mbps) */
    USB_SPEED_FULL          = 1,        /**< Full Speed (12 Mbps) */
    USB_SPEED_HIGH          = 2,        /**< High Speed (480 Mbps) */
    USB_SPEED_SUPER         = 3,        /**< SuperSpeed (5 Gbps) */
    USB_SPEED_SUPER_PLUS    = 4,        /**< SuperSpeed+ (10/20 Gbps) */
    USB_SPEED_SUPER_PLUS_GEN2X2 = 5,   /**< SuperSpeed+ Gen2x2 (20 Gbps) */
    USB_SPEED_USB4          = 6,        /**< USB4 (40 Gbps) */
} USB_SPEED;

/**
 * @brief USB controller types
 */
typedef enum _USB_CONTROLLER_TYPE {
    USB_CONTROLLER_UHCI     = 1,        /**< UHCI (USB 1.x, Intel) */
    USB_CONTROLLER_OHCI     = 2,        /**< OHCI (USB 1.x, Compaq/MS/NatSemi) */
    USB_CONTROLLER_EHCI     = 3,        /**< EHCI (USB 2.0) */
    USB_CONTROLLER_XHCI     = 4,        /**< xHCI (USB 3.x/4.0) */
} USB_CONTROLLER_TYPE;

/**
 * @brief USB device classes (from USB-IF)
 */
typedef enum _USB_DEVICE_CLASS {
    USB_CLASS_DEVICE                = 0x00,     /**< Device class in interface */
    USB_CLASS_AUDIO                 = 0x01,     /**< Audio device */
    USB_CLASS_COMMUNICATIONS        = 0x02,     /**< CDC (modem, ethernet) */
    USB_CLASS_HID                   = 0x03,     /**< Human Interface Device */
    USB_CLASS_PHYSICAL              = 0x05,     /**< Physical device */
    USB_CLASS_IMAGE                 = 0x06,     /**< Still imaging (camera) */
    USB_CLASS_PRINTER               = 0x07,     /**< Printer */
    USB_CLASS_MASS_STORAGE          = 0x08,     /**< Mass storage (USB drives) */
    USB_CLASS_HUB                   = 0x09,     /**< USB hub */
    USB_CLASS_CDC_DATA              = 0x0A,     /**< CDC data */
    USB_CLASS_SMART_CARD            = 0x0B,     /**< Smart card reader */
    USB_CLASS_CONTENT_SECURITY      = 0x0D,     /**< Content security */
    USB_CLASS_VIDEO                 = 0x0E,     /**< Video device (webcam) */
    USB_CLASS_PERSONAL_HEALTHCARE   = 0x0F,     /**< Personal healthcare */
    USB_CLASS_AUDIO_VIDEO           = 0x10,     /**< Audio/Video devices */
    USB_CLASS_BILLBOARD             = 0x11,     /**< Billboard device */
    USB_CLASS_TYPE_C_BRIDGE         = 0x12,     /**< USB Type-C bridge */
    USB_CLASS_DIAGNOSTIC            = 0xDC,     /**< Diagnostic device */
    USB_CLASS_WIRELESS              = 0xE0,     /**< Wireless controller */
    USB_CLASS_MISCELLANEOUS         = 0xEF,     /**< Miscellaneous */
    USB_CLASS_APPLICATION_SPECIFIC  = 0xFE,     /**< Application specific */
    USB_CLASS_VENDOR_SPECIFIC       = 0xFF,     /**< Vendor specific */
} USB_DEVICE_CLASS;

/**
 * @brief USB transfer types
 */
typedef enum _USB_TRANSFER_TYPE {
    USB_TRANSFER_CONTROL        = 0,    /**< Control transfer */
    USB_TRANSFER_ISOCHRONOUS    = 1,    /**< Isochronous transfer */
    USB_TRANSFER_BULK           = 2,    /**< Bulk transfer */
    USB_TRANSFER_INTERRUPT      = 3,    /**< Interrupt transfer */
} USB_TRANSFER_TYPE;

/**
 * @brief USB power delivery profiles (USB-C PD)
 */
typedef enum _USB_PD_PROFILE {
    USB_PD_NONE             = 0,        /**< No power delivery */
    USB_PD_5V_3A            = 1,        /**< 5V @ 3A (15W) */
    USB_PD_9V_3A            = 2,        /**< 9V @ 3A (27W) */
    USB_PD_15V_3A           = 3,        /**< 15V @ 3A (45W) */
    USB_PD_20V_3A           = 4,        /**< 20V @ 3A (60W) */
    USB_PD_20V_5A           = 5,        /**< 20V @ 5A (100W) */
    USB_PD_28V_5A           = 6,        /**< 28V @ 5A (140W, USB PD 3.1) */
    USB_PD_48V_5A           = 7,        /**< 48V @ 5A (240W, USB PD 3.1) */
} USB_PD_PROFILE;

/**
 * @brief USB device descriptor (standard)
 */
typedef struct _USB_DEVICE_DESCRIPTOR {
    UINT8   bLength;                    /**< Descriptor length (18 bytes) */
    UINT8   bDescriptorType;            /**< Descriptor type (0x01 = device) */
    UINT16  bcdUSB;                     /**< USB specification (BCD) */
    UINT8   bDeviceClass;               /**< Device class code */
    UINT8   bDeviceSubClass;            /**< Device subclass code */
    UINT8   bDeviceProtocol;            /**< Device protocol code */
    UINT8   bMaxPacketSize0;            /**< Max packet size for EP0 */
    UINT16  idVendor;                   /**< Vendor ID */
    UINT16  idProduct;                  /**< Product ID */
    UINT16  bcdDevice;                  /**< Device release number (BCD) */
    UINT8   iManufacturer;              /**< Manufacturer string index */
    UINT8   iProduct;                   /**< Product string index */
    UINT8   iSerialNumber;              /**< Serial number string index */
    UINT8   bNumConfigurations;         /**< Number of configurations */
} USB_DEVICE_DESCRIPTOR;

/**
 * @brief USB configuration descriptor
 */
typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    UINT8   bLength;                    /**< Descriptor length (9 bytes) */
    UINT8   bDescriptorType;            /**< Descriptor type (0x02 = config) */
    UINT16  wTotalLength;               /**< Total length of data */
    UINT8   bNumInterfaces;             /**< Number of interfaces */
    UINT8   bConfigurationValue;        /**< Configuration value */
    UINT8   iConfiguration;             /**< Configuration string index */
    UINT8   bmAttributes;               /**< Attributes (self-powered, etc.) */
    UINT8   bMaxPower;                  /**< Max power in 2mA units */
} USB_CONFIGURATION_DESCRIPTOR;

/**
 * @brief USB interface descriptor
 */
typedef struct _USB_INTERFACE_DESCRIPTOR {
    UINT8   bLength;                    /**< Descriptor length (9 bytes) */
    UINT8   bDescriptorType;            /**< Descriptor type (0x04 = interface) */
    UINT8   bInterfaceNumber;           /**< Interface number */
    UINT8   bAlternateSetting;          /**< Alternate setting */
    UINT8   bNumEndpoints;              /**< Number of endpoints */
    UINT8   bInterfaceClass;            /**< Interface class code */
    UINT8   bInterfaceSubClass;         /**< Interface subclass code */
    UINT8   bInterfaceProtocol;         /**< Interface protocol code */
    UINT8   iInterface;                 /**< Interface string index */
} USB_INTERFACE_DESCRIPTOR;

/**
 * @brief USB endpoint descriptor
 */
typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UINT8   bLength;                    /**< Descriptor length (7 bytes) */
    UINT8   bDescriptorType;            /**< Descriptor type (0x05 = endpoint) */
    UINT8   bEndpointAddress;           /**< Endpoint address */
    UINT8   bmAttributes;               /**< Transfer type and sync type */
    UINT16  wMaxPacketSize;             /**< Maximum packet size */
    UINT8   bInterval;                  /**< Polling interval */
} USB_ENDPOINT_DESCRIPTOR;

/**
 * @brief USB device information
 */
typedef struct _USB_DEVICE_INFO {
    UINT8               BusAddress;     /**< Bus address (1-127) */
    UINT8               PortNumber;     /**< Hub port number */
    USB_SPEED           Speed;          /**< Device speed */
    USB_DEVICE_CLASS    Class;          /**< Device class */
    UINT16              VendorID;       /**< Vendor ID */
    UINT16              ProductID;      /**< Product ID */
    UINT16              Version;        /**< Device version (BCD) */
    CHAR8               Manufacturer[128];  /**< Manufacturer string */
    CHAR8               Product[128];       /**< Product string */
    CHAR8               SerialNumber[128];  /**< Serial number string */
    UINT32              Capabilities;   /**< Capability flags */
    USB_PD_PROFILE      PowerProfile;   /**< Power delivery profile */
} USB_DEVICE_INFO;

/**
 * @brief USB controller information
 */
typedef struct _USB_CONTROLLER_INFO {
    USB_CONTROLLER_TYPE Type;           /**< Controller type */
    USB_VERSION         Version;        /**< USB version */
    UINT16              VendorID;       /**< Vendor ID */
    UINT16              DeviceID;       /**< Device ID */
    UINT8               NumPorts;       /**< Number of root hub ports */
    UINT32              Capabilities;   /**< Capability flags */
    UINT64              MMIOBase;       /**< Memory-mapped I/O base */
    UINT32              MMIOSize;       /**< MMIO region size */
    CHAR8               ControllerName[64];  /**< Controller name */
} USB_CONTROLLER_INFO;

/**
 * @brief USB transfer request
 */
typedef struct _USB_TRANSFER_REQUEST {
    UINT8               DeviceAddress;  /**< Target device address */
    UINT8               EndpointAddress;/**< Target endpoint */
    USB_TRANSFER_TYPE   TransferType;   /**< Transfer type */
    VOID               *pBuffer;        /**< Data buffer */
    UINT32              BufferLength;   /**< Buffer length */
    UINT32              Timeout;        /**< Timeout in ms */
    UINT32              Flags;          /**< Transfer flags */
} USB_TRANSFER_REQUEST;

/**
 * @brief USB capability flags
 */
#define USB_CAP_LOW_SPEED           0x00000001  /**< Low speed support */
#define USB_CAP_FULL_SPEED          0x00000002  /**< Full speed support */
#define USB_CAP_HIGH_SPEED          0x00000004  /**< High speed support */
#define USB_CAP_SUPER_SPEED         0x00000008  /**< SuperSpeed support */
#define USB_CAP_SUPER_SPEED_PLUS    0x00000010  /**< SuperSpeed+ support */
#define USB_CAP_USB4                0x00000020  /**< USB4 support */
#define USB_CAP_POWER_DELIVERY      0x00000040  /**< USB-C PD support */
#define USB_CAP_DISPLAYPORT         0x00000080  /**< DisplayPort alt mode */
#define USB_CAP_THUNDERBOLT         0x00000100  /**< Thunderbolt alt mode */
#define USB_CAP_OTG                 0x00000200  /**< USB OTG support */
#define USB_CAP_BATTERY_CHARGING    0x00000400  /**< Battery charging */
#define USB_CAP_LPM                 0x00000800  /**< Link power management */
#define USB_CAP_STREAMS             0x00001000  /**< Bulk streams */
#define USB_CAP_BURST               0x00002000  /**< Burst transfers */
#define USB_CAP_REMOTE_WAKEUP       0x00004000  /**< Remote wakeup */

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOUSBController, IIOService);
DECLARE_INTERFACE_(IIOUSBDevice, IIOService);
DECLARE_INTERFACE_(IIOUSBInterface, IIOService);

/**
 * @brief USB Controller Interface
 *
 * Represents a USB host controller (UHCI/OHCI/EHCI/xHCI).
 */
#undef INTERFACE
#define INTERFACE IIOUSBController

DECLARE_INTERFACE_(IIOUSBController, IIOService)
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

    // USB Controller specific methods

    /**
     * @brief Get controller information
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        USB_CONTROLLER_INFO *pInfo
        ) PURE;

    /**
     * @brief Enumerate USB devices on bus
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOUSBDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Reset the controller
     */
    STDMETHOD_(IO_RETURN, ResetController)(THIS) PURE;

    /**
     * @brief Get root hub port status
     */
    STDMETHOD_(IO_RETURN, GetPortStatus)(THIS_
        UINT8 uPort,
        UINT32 *puStatus
        ) PURE;

    /**
     * @brief Reset a root hub port
     */
    STDMETHOD_(IO_RETURN, ResetPort)(THIS_
        UINT8 uPort
        ) PURE;

    /**
     * @brief Enable/disable a root hub port
     */
    STDMETHOD_(IO_RETURN, SetPortPower)(THIS_
        UINT8 uPort,
        BOOLEAN bEnable
        ) PURE;

    /**
     * @brief Submit a transfer request
     */
    STDMETHOD_(IO_RETURN, SubmitTransfer)(THIS_
        USB_TRANSFER_REQUEST *pRequest,
        UINT32 *puBytesTransferred
        ) PURE;

    /**
     * @brief Abort pending transfers for a device
     */
    STDMETHOD_(IO_RETURN, AbortTransfers)(THIS_
        UINT8 uDeviceAddress
        ) PURE;

    /**
     * @brief Set power delivery profile (USB-C PD)
     */
    STDMETHOD_(IO_RETURN, SetPowerProfile)(THIS_
        USB_PD_PROFILE Profile
        ) PURE;
};

/**
 * @brief USB Device Interface
 *
 * Represents a USB device on the bus.
 */
#undef INTERFACE
#define INTERFACE IIOUSBDevice

DECLARE_INTERFACE_(IIOUSBDevice, IIOService)
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

    // USB Device specific methods

    /**
     * @brief Get device information
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        USB_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Get device descriptor
     */
    STDMETHOD_(IO_RETURN, GetDeviceDescriptor)(THIS_
        USB_DEVICE_DESCRIPTOR *pDescriptor
        ) PURE;

    /**
     * @brief Get configuration descriptor
     */
    STDMETHOD_(IO_RETURN, GetConfigDescriptor)(THIS_
        UINT8 uConfigIndex,
        USB_CONFIGURATION_DESCRIPTOR *pDescriptor,
        VOID *pBuffer,
        UINT32 *puBufferSize
        ) PURE;

    /**
     * @brief Set device configuration
     */
    STDMETHOD_(IO_RETURN, SetConfiguration)(THIS_
        UINT8 uConfiguration
        ) PURE;

    /**
     * @brief Get string descriptor
     */
    STDMETHOD_(IO_RETURN, GetStringDescriptor)(THIS_
        UINT8 uIndex,
        UINT16 uLanguageID,
        CHAR8 *pszBuffer,
        UINT32 cbBufferSize
        ) PURE;

    /**
     * @brief Send control transfer
     */
    STDMETHOD_(IO_RETURN, ControlTransfer)(THIS_
        UINT8 bmRequestType,
        UINT8 bRequest,
        UINT16 wValue,
        UINT16 wIndex,
        VOID *pData,
        UINT16 wLength,
        UINT32 *puBytesTransferred
        ) PURE;

    /**
     * @brief Reset the device
     */
    STDMETHOD_(IO_RETURN, ResetDevice)(THIS) PURE;
};

/**
 * @brief USB Interface Interface
 *
 * Represents a USB interface (function) within a device.
 */
#undef INTERFACE
#define INTERFACE IIOUSBInterface

DECLARE_INTERFACE_(IIOUSBInterface, IIOService)
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

    // USB Interface specific methods

    /**
     * @brief Get interface descriptor
     */
    STDMETHOD_(IO_RETURN, GetInterfaceDescriptor)(THIS_
        USB_INTERFACE_DESCRIPTOR *pDescriptor
        ) PURE;

    /**
     * @brief Get endpoint descriptor
     */
    STDMETHOD_(IO_RETURN, GetEndpointDescriptor)(THIS_
        UINT8 uEndpointIndex,
        USB_ENDPOINT_DESCRIPTOR *pDescriptor
        ) PURE;

    /**
     * @brief Set alternate setting
     */
    STDMETHOD_(IO_RETURN, SetAlternateSetting)(THIS_
        UINT8 uAlternateSetting
        ) PURE;

    /**
     * @brief Perform bulk transfer
     */
    STDMETHOD_(IO_RETURN, BulkTransfer)(THIS_
        UINT8 uEndpoint,
        VOID *pBuffer,
        UINT32 uLength,
        UINT32 uTimeout,
        UINT32 *puBytesTransferred
        ) PURE;

    /**
     * @brief Perform interrupt transfer
     */
    STDMETHOD_(IO_RETURN, InterruptTransfer)(THIS_
        UINT8 uEndpoint,
        VOID *pBuffer,
        UINT32 uLength,
        UINT32 uTimeout,
        UINT32 *puBytesTransferred
        ) PURE;

    /**
     * @brief Perform isochronous transfer
     */
    STDMETHOD_(IO_RETURN, IsochronousTransfer)(THIS_
        UINT8 uEndpoint,
        VOID *pBuffer,
        UINT32 uLength,
        UINT32 uPackets,
        UINT32 *puBytesTransferred
        ) PURE;
};

// Convenience macros for C code

#define IIOUSBController_GetControllerInfo(p,a)     (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOUSBController_EnumerateDevices(p,a,b)    (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOUSBController_ResetController(p)         (p)->lpVtbl->ResetController(p)
#define IIOUSBController_GetPortStatus(p,a,b)       (p)->lpVtbl->GetPortStatus(p,a,b)
#define IIOUSBController_ResetPort(p,a)             (p)->lpVtbl->ResetPort(p,a)
#define IIOUSBController_SetPortPower(p,a,b)        (p)->lpVtbl->SetPortPower(p,a,b)
#define IIOUSBController_SubmitTransfer(p,a,b)      (p)->lpVtbl->SubmitTransfer(p,a,b)
#define IIOUSBController_AbortTransfers(p,a)        (p)->lpVtbl->AbortTransfers(p,a)
#define IIOUSBController_SetPowerProfile(p,a)       (p)->lpVtbl->SetPowerProfile(p,a)

#define IIOUSBDevice_GetDeviceInfo(p,a)             (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOUSBDevice_GetDeviceDescriptor(p,a)       (p)->lpVtbl->GetDeviceDescriptor(p,a)
#define IIOUSBDevice_GetConfigDescriptor(p,a,b,c,d) (p)->lpVtbl->GetConfigDescriptor(p,a,b,c,d)
#define IIOUSBDevice_SetConfiguration(p,a)          (p)->lpVtbl->SetConfiguration(p,a)
#define IIOUSBDevice_GetStringDescriptor(p,a,b,c,d) (p)->lpVtbl->GetStringDescriptor(p,a,b,c,d)
#define IIOUSBDevice_ControlTransfer(p,a,b,c,d,e,f,g) (p)->lpVtbl->ControlTransfer(p,a,b,c,d,e,f,g)
#define IIOUSBDevice_ResetDevice(p)                 (p)->lpVtbl->ResetDevice(p)

#define IIOUSBInterface_GetInterfaceDescriptor(p,a) (p)->lpVtbl->GetInterfaceDescriptor(p,a)
#define IIOUSBInterface_GetEndpointDescriptor(p,a,b) (p)->lpVtbl->GetEndpointDescriptor(p,a,b)
#define IIOUSBInterface_SetAlternateSetting(p,a)    (p)->lpVtbl->SetAlternateSetting(p,a)
#define IIOUSBInterface_BulkTransfer(p,a,b,c,d,e)   (p)->lpVtbl->BulkTransfer(p,a,b,c,d,e)
#define IIOUSBInterface_InterruptTransfer(p,a,b,c,d,e) (p)->lpVtbl->InterruptTransfer(p,a,b,c,d,e)
#define IIOUSBInterface_IsochronousTransfer(p,a,b,c,d,e) (p)->lpVtbl->IsochronousTransfer(p,a,b,c,d,e)

/**
 * @brief Initialize USB subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
USBInitialize(
    VOID
    );

/**
 * @brief Shutdown USB subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
USBShutdown(
    VOID
    );

/**
 * @brief Create a USB controller instance
 *
 * @param pszName           Controller name
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_ERR_NO_MEMORY     Insufficient memory
 * @retval IO_ERR_INVALID_PARAM Invalid parameter
 */
IO_RETURN
IOUSBControllerCreate(
    CONST CHAR8        *pszName,
    IIOUSBController  **ppController
    );

/**
 * @brief Create a USB device instance
 *
 * @param pszName       Device name
 * @param ppDevice      Receives device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_ERR_NO_MEMORY     Insufficient memory
 * @retval IO_ERR_INVALID_PARAM Invalid parameter
 */
IO_RETURN
IOUSBDeviceCreate(
    CONST CHAR8    *pszName,
    IIOUSBDevice  **ppDevice
    );

#ifdef __cplusplus
}
#endif

#endif // IOKIT_USB_H
