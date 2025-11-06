/**
 * @file display.h
 * @brief Display/Graphics Family Interface - Unified GPU and Display Management
 *
 * This header defines the Display/Graphics family interface for GPU controllers,
 * display devices, framebuffer management, and graphics acceleration.
 *
 * Supports:
 * - GPU Types: Discrete, Integrated, Hybrid, Virtual (SR-IOV)
 * - Display Technologies: VGA, SVGA, DVI, HDMI (1.0-2.1), DisplayPort (1.0-2.1),
 *   eDP, LVDS, DSI, Thunderbolt/USB-C DP Alt Mode
 * - GPU Vendors: NVIDIA (GeForce, Quadro, Tesla, A100/H100), AMD (Radeon, Instinct),
 *   Intel (UHD, Iris, Arc), Apple (M1/M2/M3), ARM Mali, Qualcomm Adreno, PowerVR,
 *   Matrox, S3, 3dfx (legacy)
 * - Modern Features: HDR, VRR (FreeSync/G-Sync), Ray Tracing, AI/Tensor Cores,
 *   Video Encode/Decode (H.264/H.265/AV1/VP9)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_DISPLAY_H
#define IOKIT_DISPLAY_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIODisplayController interface GUID
 * {D7A3E4B2-9F1C-4D8A-B6E5-8C3D7F2A1E9B}
 */
DEFINE_GUID(IID_IIODisplayController,
    0xD7A3E4B2, 0x9F1C, 0x4D8A, 0xB6, 0xE5, 0x8C, 0x3D, 0x7F, 0x2A, 0x1E, 0x9B);

/**
 * @brief IIODisplayDevice interface GUID
 * {E8B4F5C3-AF2D-4E9B-C7F6-9D4E8F3B2FAC}
 */
DEFINE_GUID(IID_IIODisplayDevice,
    0xE8B4F5C3, 0xAF2D, 0x4E9B, 0xC7, 0xF6, 0x9D, 0x4E, 0x8F, 0x3B, 0x2F, 0xAC);

/**
 * @brief IIOFramebuffer interface GUID
 * {F9C5D6E4-B03E-5FAC-D807-AE5F904C30BD}
 */
DEFINE_GUID(IID_IIOFramebuffer,
    0xF9C5D6E4, 0xB03E, 0x5FAC, 0xD8, 0x07, 0xAE, 0x5F, 0x90, 0x4C, 0x30, 0xBD);

/**
 * @brief IIOAccelerator interface GUID
 * {A1D2E3F4-B5C6-D7E8-F9A0-B1C2D3E4F5A6}
 */
DEFINE_GUID(IID_IIOAccelerator,
    0xA1D2E3F4, 0xB5C6, 0xD7E8, 0xF9, 0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5, 0xA6);

/**
 * @brief GPU types
 */
typedef enum _GPU_TYPE {
    GPU_TYPE_DISCRETE       = 1,    /**< Discrete GPU (PCIe add-in card) */
    GPU_TYPE_INTEGRATED     = 2,    /**< Integrated GPU (CPU-integrated, IGP) */
    GPU_TYPE_HYBRID         = 3,    /**< Hybrid GPU (switchable graphics) */
    GPU_TYPE_VIRTUAL        = 4,    /**< Virtual GPU (SR-IOV, vGPU) */
} GPU_TYPE;

/**
 * @brief GPU vendors
 */
typedef enum _GPU_VENDOR {
    GPU_VENDOR_NVIDIA       = 0x10DE,   /**< NVIDIA Corporation */
    GPU_VENDOR_AMD          = 0x1002,   /**< AMD (ATI Technologies) */
    GPU_VENDOR_INTEL        = 0x8086,   /**< Intel Corporation */
    GPU_VENDOR_APPLE        = 0x106B,   /**< Apple Inc. */
    GPU_VENDOR_ARM          = 0x13B5,   /**< ARM Limited */
    GPU_VENDOR_QUALCOMM     = 0x5143,   /**< Qualcomm */
    GPU_VENDOR_POWERVR      = 0x1010,   /**< PowerVR (Imagination Technologies) */
    GPU_VENDOR_MATROX       = 0x102B,   /**< Matrox Graphics */
    GPU_VENDOR_S3           = 0x5333,   /**< S3 Graphics */
    GPU_VENDOR_3DFX         = 0x121A,   /**< 3dfx Interactive (legacy) */
    GPU_VENDOR_VIA          = 0x1106,   /**< VIA Technologies */
    GPU_VENDOR_SIS          = 0x1039,   /**< Silicon Integrated Systems */
    GPU_VENDOR_VMWARE       = 0x15AD,   /**< VMware SVGA */
    GPU_VENDOR_QEMU         = 0x1234,   /**< QEMU VGA */
} GPU_VENDOR;

/**
 * @brief Display interface types
 */
typedef enum _DISPLAY_INTERFACE_TYPE {
    DISPLAY_INTERFACE_VGA           = 0x01,     /**< VGA (legacy analog, 640x480+) */
    DISPLAY_INTERFACE_SVGA          = 0x02,     /**< SVGA/VESA (800x600+) */
    DISPLAY_INTERFACE_DVI_SINGLE    = 0x03,     /**< DVI Single Link (1920x1200@60Hz) */
    DISPLAY_INTERFACE_DVI_DUAL      = 0x04,     /**< DVI Dual Link (2560x1600@60Hz) */
    DISPLAY_INTERFACE_HDMI_1_0      = 0x10,     /**< HDMI 1.0 (1080p@60Hz) */
    DISPLAY_INTERFACE_HDMI_1_4      = 0x14,     /**< HDMI 1.4 (4K@30Hz, ARC) */
    DISPLAY_INTERFACE_HDMI_2_0      = 0x20,     /**< HDMI 2.0 (4K@60Hz, HDR) */
    DISPLAY_INTERFACE_HDMI_2_1      = 0x21,     /**< HDMI 2.1 (8K@60Hz, 4K@120Hz, VRR) */
    DISPLAY_INTERFACE_DP_1_0        = 0x30,     /**< DisplayPort 1.0 (2560x1600@60Hz) */
    DISPLAY_INTERFACE_DP_1_2        = 0x32,     /**< DisplayPort 1.2 (4K@60Hz, MST) */
    DISPLAY_INTERFACE_DP_1_4        = 0x34,     /**< DisplayPort 1.4 (8K@60Hz, HDR) */
    DISPLAY_INTERFACE_DP_2_0        = 0x40,     /**< DisplayPort 2.0 (16K@60Hz, 8K@120Hz) */
    DISPLAY_INTERFACE_DP_2_1        = 0x41,     /**< DisplayPort 2.1 (improved VRR) */
    DISPLAY_INTERFACE_EDP           = 0x50,     /**< Embedded DisplayPort (laptops) */
    DISPLAY_INTERFACE_LVDS          = 0x60,     /**< LVDS (laptop panels) */
    DISPLAY_INTERFACE_DSI           = 0x70,     /**< Mobile Display Serial Interface */
    DISPLAY_INTERFACE_THUNDERBOLT   = 0x80,     /**< Thunderbolt DisplayPort */
    DISPLAY_INTERFACE_USB_C_DP      = 0x90,     /**< USB-C DisplayPort Alt Mode */
} DISPLAY_INTERFACE_TYPE;

/**
 * @brief VRAM types
 */
typedef enum _VRAM_TYPE {
    VRAM_TYPE_UNKNOWN       = 0,
    VRAM_TYPE_SDRAM         = 1,    /**< Synchronous DRAM (legacy) */
    VRAM_TYPE_DDR           = 2,    /**< DDR (legacy) */
    VRAM_TYPE_DDR2          = 3,    /**< DDR2 (legacy) */
    VRAM_TYPE_DDR3          = 4,    /**< DDR3 (older GPUs) */
    VRAM_TYPE_GDDR3         = 5,    /**< GDDR3 (older GPUs) */
    VRAM_TYPE_GDDR4         = 6,    /**< GDDR4 (rare) */
    VRAM_TYPE_GDDR5         = 7,    /**< GDDR5 (common in older GPUs) */
    VRAM_TYPE_GDDR5X        = 8,    /**< GDDR5X (GTX 1080/Ti) */
    VRAM_TYPE_GDDR6         = 9,    /**< GDDR6 (modern GPUs) */
    VRAM_TYPE_GDDR6X        = 10,   /**< GDDR6X (RTX 30/40 series) */
    VRAM_TYPE_HBM           = 11,   /**< High Bandwidth Memory (Fiji, Vega) */
    VRAM_TYPE_HBM2          = 12,   /**< HBM2 (Vega, Radeon VII, Tesla V100) */
    VRAM_TYPE_HBM2E         = 13,   /**< HBM2E (A100) */
    VRAM_TYPE_HBM3          = 14,   /**< HBM3 (H100, MI300) */
} VRAM_TYPE;

/**
 * @brief Pixel formats
 */
typedef enum _PIXEL_FORMAT {
    PIXEL_FORMAT_RGB332         = 0x01,     /**< 8-bit RGB (3:3:2) */
    PIXEL_FORMAT_RGB565         = 0x02,     /**< 16-bit RGB (5:6:5) */
    PIXEL_FORMAT_RGB555         = 0x03,     /**< 15-bit RGB (5:5:5) */
    PIXEL_FORMAT_RGB888         = 0x04,     /**< 24-bit RGB (8:8:8) */
    PIXEL_FORMAT_RGBX8888       = 0x05,     /**< 32-bit RGB (8:8:8:X) */
    PIXEL_FORMAT_RGBA8888       = 0x06,     /**< 32-bit RGBA (8:8:8:8) */
    PIXEL_FORMAT_BGR888         = 0x07,     /**< 24-bit BGR (8:8:8) */
    PIXEL_FORMAT_BGRX8888       = 0x08,     /**< 32-bit BGR (8:8:8:X) */
    PIXEL_FORMAT_BGRA8888       = 0x09,     /**< 32-bit BGRA (8:8:8:8) */
    PIXEL_FORMAT_RGB101010      = 0x0A,     /**< 30-bit RGB (10:10:10) */
    PIXEL_FORMAT_RGBA1010102    = 0x0B,     /**< 32-bit RGBA (10:10:10:2) */
    PIXEL_FORMAT_RGB161616      = 0x0C,     /**< 48-bit RGB (16:16:16) */
    PIXEL_FORMAT_RGBA16161616   = 0x0D,     /**< 64-bit RGBA (16:16:16:16) */
    PIXEL_FORMAT_YUV422         = 0x10,     /**< YUV 4:2:2 */
    PIXEL_FORMAT_YUV420         = 0x11,     /**< YUV 4:2:0 */
    PIXEL_FORMAT_YUV444         = 0x12,     /**< YUV 4:4:4 */
} PIXEL_FORMAT;

/**
 * @brief Color spaces
 */
typedef enum _COLOR_SPACE {
    COLOR_SPACE_SRGB            = 0x01,     /**< sRGB (standard RGB) */
    COLOR_SPACE_ADOBE_RGB       = 0x02,     /**< Adobe RGB (wider gamut) */
    COLOR_SPACE_DCI_P3          = 0x03,     /**< DCI-P3 (digital cinema) */
    COLOR_SPACE_REC_709         = 0x04,     /**< Rec. 709 (HDTV) */
    COLOR_SPACE_REC_2020        = 0x05,     /**< Rec. 2020 (UHDTV, HDR) */
    COLOR_SPACE_BT_2100_PQ      = 0x06,     /**< BT.2100 PQ (HDR10) */
    COLOR_SPACE_BT_2100_HLG     = 0x07,     /**< BT.2100 HLG (Hybrid Log-Gamma) */
} COLOR_SPACE;

/**
 * @brief GPU capability flags
 */
#define GPU_CAP_2D_ACCEL            0x00000001  /**< 2D acceleration */
#define GPU_CAP_3D_ACCEL            0x00000002  /**< 3D acceleration */
#define GPU_CAP_VIDEO_DECODE        0x00000004  /**< Video decode (H.264/H.265/etc) */
#define GPU_CAP_VIDEO_ENCODE        0x00000008  /**< Video encode */
#define GPU_CAP_H264                0x00000010  /**< H.264/AVC codec */
#define GPU_CAP_H265                0x00000020  /**< H.265/HEVC codec */
#define GPU_CAP_AV1                 0x00000040  /**< AV1 codec */
#define GPU_CAP_VP9                 0x00000080  /**< VP9 codec */
#define GPU_CAP_MULTI_MONITOR       0x00000100  /**< Multi-monitor support */
#define GPU_CAP_HDR10               0x00000200  /**< HDR10 support */
#define GPU_CAP_HDR10_PLUS          0x00000400  /**< HDR10+ support */
#define GPU_CAP_DOLBY_VISION        0x00000800  /**< Dolby Vision support */
#define GPU_CAP_VRR                 0x00001000  /**< Variable refresh rate */
#define GPU_CAP_FREESYNC            0x00002000  /**< AMD FreeSync */
#define GPU_CAP_GSYNC               0x00004000  /**< NVIDIA G-Sync */
#define GPU_CAP_CUDA                0x00010000  /**< NVIDIA CUDA compute */
#define GPU_CAP_OPENCL              0x00020000  /**< OpenCL compute */
#define GPU_CAP_ROCM                0x00040000  /**< AMD ROCm compute */
#define GPU_CAP_ONEAPI              0x00080000  /**< Intel oneAPI compute */
#define GPU_CAP_RAYTRACING          0x00100000  /**< Hardware ray tracing */
#define GPU_CAP_TENSOR_CORES        0x00200000  /**< AI/Tensor cores */
#define GPU_CAP_DLSS                0x00400000  /**< NVIDIA DLSS */
#define GPU_CAP_FSR                 0x00800000  /**< AMD FidelityFX Super Resolution */
#define GPU_CAP_XESS                0x01000000  /**< Intel Xe Super Sampling */
#define GPU_CAP_VGA_OUTPUT          0x02000000  /**< VGA output */
#define GPU_CAP_DVI_OUTPUT          0x04000000  /**< DVI output */
#define GPU_CAP_HDMI_OUTPUT         0x08000000  /**< HDMI output */
#define GPU_CAP_DP_OUTPUT           0x10000000  /**< DisplayPort output */
#define GPU_CAP_USB_C_DP            0x20000000  /**< USB-C DisplayPort */
#define GPU_CAP_THUNDERBOLT         0x40000000  /**< Thunderbolt display */

/**
 * @brief Display mode structure
 */
typedef struct _DISPLAY_MODE {
    UINT32              Width;              /**< Horizontal resolution (pixels) */
    UINT32              Height;             /**< Vertical resolution (pixels) */
    UINT32              RefreshRate;        /**< Refresh rate (Hz) */
    UINT32              BitDepth;           /**< Bits per pixel (8/10/12/16 bpc) */
    PIXEL_FORMAT        PixelFormat;        /**< Pixel format */
    COLOR_SPACE         ColorSpace;         /**< Color space */
    BOOLEAN             bInterlaced;        /**< Interlaced mode */
    BOOLEAN             bHDRSupported;      /**< HDR support */
    UINT32              Flags;              /**< Additional flags */
} DISPLAY_MODE;

/**
 * @brief GPU device database entry
 */
typedef struct _GPU_DEVICE_ENTRY {
    UINT16              VendorID;           /**< PCI Vendor ID */
    UINT16              DeviceID;           /**< PCI Device ID */
    GPU_TYPE            Type;               /**< GPU type */
    CONST CHAR8        *pszName;            /**< Device name */
    UINT32              DefaultVRAM;        /**< Default VRAM size (MB) */
    VRAM_TYPE           VRAMType;           /**< VRAM type */
    UINT32              Capabilities;       /**< Capability flags */
} GPU_DEVICE_ENTRY;

/**
 * @brief Display controller information
 */
typedef struct _DISPLAY_CONTROLLER_INFO {
    UINT16              VendorID;           /**< GPU vendor ID */
    UINT16              DeviceID;           /**< GPU device ID */
    CHAR8               DeviceName[128];    /**< GPU name */
    GPU_TYPE            Type;               /**< GPU type */
    UINT64              VRAMSize;           /**< VRAM size (bytes) */
    VRAM_TYPE           VRAMType;           /**< VRAM type */
    UINT32              CoreClock;          /**< Core clock (MHz) */
    UINT32              MemoryClock;        /**< Memory clock (MHz) */
    UINT32              PowerLimit;         /**< Power limit (watts) */
    CHAR8               DriverVersion[64];  /**< Driver version */
    UINT32              Capabilities;       /**< Capability flags */
    UINT32              NumOutputs;         /**< Number of display outputs */
    UINT32              MaxResolutionX;     /**< Maximum horizontal resolution */
    UINT32              MaxResolutionY;     /**< Maximum vertical resolution */
} DISPLAY_CONTROLLER_INFO;

/**
 * @brief Framebuffer information
 */
typedef struct _FRAMEBUFFER_INFO {
    UINT64              PhysicalAddress;    /**< Physical address */
    UINT64              Size;               /**< Size in bytes */
    UINT32              Width;              /**< Width in pixels */
    UINT32              Height;             /**< Height in pixels */
    UINT32              Pitch;              /**< Bytes per scanline */
    UINT32              BitsPerPixel;       /**< Bits per pixel */
    PIXEL_FORMAT        PixelFormat;        /**< Pixel format */
    UINT32              RefreshRate;        /**< Refresh rate (Hz) */
} FRAMEBUFFER_INFO;

/**
 * @brief EDID (Extended Display Identification Data) structure
 */
typedef struct _EDID_DATA {
    UINT8               Header[8];          /**< EDID header (00 FF FF FF FF FF FF 00) */
    UINT16              ManufacturerID;     /**< Manufacturer ID */
    UINT16              ProductCode;        /**< Product code */
    UINT32              SerialNumber;       /**< Serial number */
    UINT8               WeekOfManufacture;  /**< Week of manufacture */
    UINT8               YearOfManufacture;  /**< Year of manufacture */
    UINT8               EDIDVersion;        /**< EDID version */
    UINT8               EDIDRevision;       /**< EDID revision */
    UINT8               VideoInputDef;      /**< Video input definition */
    UINT8               MaxHorizSize;       /**< Max horizontal size (cm) */
    UINT8               MaxVertSize;        /**< Max vertical size (cm) */
    UINT8               DisplayGamma;       /**< Display gamma */
    UINT8               Features;           /**< Feature support */
    UINT8               ChromaData[10];     /**< Chromaticity data */
    UINT8               TimingBitmap1;      /**< Established timing bitmap */
    UINT8               TimingBitmap2;      /**< Established timing bitmap */
    UINT8               ManufTimingBitmap;  /**< Manufacturer timing bitmap */
    UINT8               StandardTiming[16]; /**< Standard timing identification */
    UINT8               DetailedTiming[72]; /**< Detailed timing descriptors */
    UINT8               ExtensionFlag;      /**< Extension flag */
    UINT8               Checksum;           /**< Checksum */
} EDID_DATA;

/**
 * @brief Forward declarations
 */
DECLARE_INTERFACE_(IIODisplayController, IIOService);
DECLARE_INTERFACE_(IIODisplayDevice, IIOService);
DECLARE_INTERFACE_(IIOFramebuffer, IIOService);
DECLARE_INTERFACE_(IIOAccelerator, IIOService);

/**
 * @brief IIODisplayController - GPU/Display Controller Interface
 *
 * Represents a GPU/display controller and provides methods for mode enumeration,
 * framebuffer allocation, output management, and display configuration.
 */
#undef INTERFACE
#define INTERFACE IIODisplayController

DECLARE_INTERFACE_(IIODisplayController, IIOService)
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

    // IIODisplayController methods

    /**
     * @brief Get display controller information
     *
     * @param pInfo         Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        DISPLAY_CONTROLLER_INFO *pInfo
        ) PURE;

    /**
     * @brief Enumerate supported display modes
     *
     * @param pModes        Buffer to receive modes
     * @param puCount       On input: buffer size; On output: actual count
     *
     * @retval IO_SUCCESS       Modes enumerated successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NO_SPACE      Buffer too small
     */
    STDMETHOD_(IO_RETURN, EnumerateModes)(THIS_
        DISPLAY_MODE *pModes,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Set display mode
     *
     * @param pMode         Display mode to set
     *
     * @retval IO_SUCCESS       Mode set successfully
     * @retval IO_BAD_ARGUMENT  Invalid mode
     * @retval IO_UNSUPPORTED   Mode not supported
     */
    STDMETHOD_(IO_RETURN, SetMode)(THIS_
        CONST DISPLAY_MODE *pMode
        ) PURE;

    /**
     * @brief Get current display mode
     *
     * @param pMode         Receives current mode
     *
     * @retval IO_SUCCESS       Mode retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCurrentMode)(THIS_
        DISPLAY_MODE *pMode
        ) PURE;

    /**
     * @brief Allocate framebuffer
     *
     * @param uWidth        Width in pixels
     * @param uHeight       Height in pixels
     * @param Format        Pixel format
     * @param ppFramebuffer Receives framebuffer interface
     *
     * @retval IO_SUCCESS       Framebuffer allocated successfully
     * @retval IO_NO_MEMORY     Insufficient VRAM
     */
    STDMETHOD_(IO_RETURN, AllocateFramebuffer)(THIS_
        UINT32 uWidth,
        UINT32 uHeight,
        PIXEL_FORMAT Format,
        IIOFramebuffer **ppFramebuffer
        ) PURE;

    /**
     * @brief Free framebuffer
     *
     * @param pFramebuffer  Framebuffer to free
     *
     * @retval IO_SUCCESS   Framebuffer freed successfully
     */
    STDMETHOD_(IO_RETURN, FreeFramebuffer)(THIS_
        IIOFramebuffer *pFramebuffer
        ) PURE;

    /**
     * @brief Enable display output
     *
     * @param uOutputIndex  Output index
     *
     * @retval IO_SUCCESS       Output enabled successfully
     * @retval IO_BAD_ARGUMENT  Invalid output index
     */
    STDMETHOD_(IO_RETURN, EnableOutput)(THIS_
        UINT32 uOutputIndex
        ) PURE;

    /**
     * @brief Disable display output
     *
     * @param uOutputIndex  Output index
     *
     * @retval IO_SUCCESS       Output disabled successfully
     * @retval IO_BAD_ARGUMENT  Invalid output index
     */
    STDMETHOD_(IO_RETURN, DisableOutput)(THIS_
        UINT32 uOutputIndex
        ) PURE;

    /**
     * @brief Get EDID from display
     *
     * @param uOutputIndex  Output index
     * @param pEDID         Receives EDID data
     *
     * @retval IO_SUCCESS       EDID retrieved successfully
     * @retval IO_NO_DEVICE     No display connected
     * @retval IO_IO_ERROR      EDID read failed
     */
    STDMETHOD_(IO_RETURN, GetEDID)(THIS_
        UINT32 uOutputIndex,
        EDID_DATA *pEDID
        ) PURE;

    /**
     * @brief Set display brightness
     *
     * @param uOutputIndex  Output index
     * @param uBrightness   Brightness (0-100)
     *
     * @retval IO_SUCCESS       Brightness set successfully
     * @retval IO_UNSUPPORTED   Brightness control not supported
     */
    STDMETHOD_(IO_RETURN, SetBrightness)(THIS_
        UINT32 uOutputIndex,
        UINT32 uBrightness
        ) PURE;

    /**
     * @brief Get display brightness
     *
     * @param uOutputIndex  Output index
     * @param puBrightness  Receives brightness (0-100)
     *
     * @retval IO_SUCCESS       Brightness retrieved successfully
     * @retval IO_UNSUPPORTED   Brightness control not supported
     */
    STDMETHOD_(IO_RETURN, GetBrightness)(THIS_
        UINT32 uOutputIndex,
        UINT32 *puBrightness
        ) PURE;

    /**
     * @brief Enable HDR
     *
     * @param uOutputIndex  Output index
     *
     * @retval IO_SUCCESS       HDR enabled successfully
     * @retval IO_UNSUPPORTED   HDR not supported
     */
    STDMETHOD_(IO_RETURN, EnableHDR)(THIS_
        UINT32 uOutputIndex
        ) PURE;

    /**
     * @brief Disable HDR
     *
     * @param uOutputIndex  Output index
     *
     * @retval IO_SUCCESS       HDR disabled successfully
     */
    STDMETHOD_(IO_RETURN, DisableHDR)(THIS_
        UINT32 uOutputIndex
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIODisplayDevice - Display/Monitor Interface
 *
 * Represents a connected display device (monitor, TV, projector).
 */
#undef INTERFACE
#define INTERFACE IIODisplayDevice

DECLARE_INTERFACE_(IIODisplayDevice, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get display device information
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        VOID *pInfo
        ) PURE;

    /**
     * @brief Get EDID data
     *
     * @param pEDID         Receives EDID data
     *
     * @retval IO_SUCCESS   EDID retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetEDID)(THIS_
        EDID_DATA *pEDID
        ) PURE;

    /**
     * @brief Get supported modes
     *
     * @param pModes        Buffer to receive modes
     * @param puCount       On input: buffer size; On output: actual count
     *
     * @retval IO_SUCCESS   Modes retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetSupportedModes)(THIS_
        DISPLAY_MODE *pModes,
        UINT32 *puCount
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOFramebuffer - Framebuffer Management Interface
 *
 * Provides direct framebuffer access and 2D operations.
 */
#undef INTERFACE
#define INTERFACE IIOFramebuffer

DECLARE_INTERFACE_(IIOFramebuffer, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get framebuffer information
     *
     * @param pInfo         Receives framebuffer information
     *
     * @retval IO_SUCCESS   Information retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetInfo)(THIS_
        FRAMEBUFFER_INFO *pInfo
        ) PURE;

    /**
     * @brief Map framebuffer to virtual memory
     *
     * @param ppAddress     Receives mapped address
     *
     * @retval IO_SUCCESS   Framebuffer mapped successfully
     * @retval IO_VM_ERROR  Mapping failed
     */
    STDMETHOD_(IO_RETURN, Map)(THIS_
        VOID **ppAddress
        ) PURE;

    /**
     * @brief Unmap framebuffer
     *
     * @retval IO_SUCCESS   Framebuffer unmapped successfully
     */
    STDMETHOD_(IO_RETURN, Unmap)(THIS) PURE;

    /**
     * @brief Blit (copy) rectangle
     *
     * @param uSrcX         Source X
     * @param uSrcY         Source Y
     * @param uDstX         Destination X
     * @param uDstY         Destination Y
     * @param uWidth        Width
     * @param uHeight       Height
     *
     * @retval IO_SUCCESS   Blit completed successfully
     */
    STDMETHOD_(IO_RETURN, Blit)(THIS_
        UINT32 uSrcX,
        UINT32 uSrcY,
        UINT32 uDstX,
        UINT32 uDstY,
        UINT32 uWidth,
        UINT32 uHeight
        ) PURE;

    /**
     * @brief Fill rectangle
     *
     * @param uX            X coordinate
     * @param uY            Y coordinate
     * @param uWidth        Width
     * @param uHeight       Height
     * @param uColor        Fill color
     *
     * @retval IO_SUCCESS   Fill completed successfully
     */
    STDMETHOD_(IO_RETURN, Fill)(THIS_
        UINT32 uX,
        UINT32 uY,
        UINT32 uWidth,
        UINT32 uHeight,
        UINT32 uColor
        ) PURE;

    /**
     * @brief Set pixel
     *
     * @param uX            X coordinate
     * @param uY            Y coordinate
     * @param uColor        Pixel color
     *
     * @retval IO_SUCCESS   Pixel set successfully
     */
    STDMETHOD_(IO_RETURN, SetPixel)(THIS_
        UINT32 uX,
        UINT32 uY,
        UINT32 uColor
        ) PURE;

    /**
     * @brief Get pixel
     *
     * @param uX            X coordinate
     * @param uY            Y coordinate
     * @param puColor       Receives pixel color
     *
     * @retval IO_SUCCESS   Pixel retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetPixel)(THIS_
        UINT32 uX,
        UINT32 uY,
        UINT32 *puColor
        ) PURE;

    /**
     * @brief Pan display (set display offset)
     *
     * @param uX            X offset
     * @param uY            Y offset
     *
     * @retval IO_SUCCESS   Pan completed successfully
     */
    STDMETHOD_(IO_RETURN, Pan)(THIS_
        UINT32 uX,
        UINT32 uY
        ) PURE;

    /**
     * @brief Set framebuffer offset
     *
     * @param uOffset       Offset in bytes
     *
     * @retval IO_SUCCESS   Offset set successfully
     */
    STDMETHOD_(IO_RETURN, SetOffset)(THIS_
        UINT64 uOffset
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOAccelerator - GPU Compute/Acceleration Interface
 *
 * Provides access to GPU compute capabilities (CUDA, OpenCL, etc.).
 */
#undef INTERFACE
#define INTERFACE IIOAccelerator

DECLARE_INTERFACE_(IIOAccelerator, IIOService)
{
    // All IIOService methods inherited...

    /**
     * @brief Get accelerator capabilities
     *
     * @param puCapabilities    Receives capability flags
     *
     * @retval IO_SUCCESS       Capabilities retrieved successfully
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        UINT32 *puCapabilities
        ) PURE;

    /**
     * @brief Allocate GPU memory
     *
     * @param cbSize        Size in bytes
     * @param ppAddress     Receives GPU memory address
     *
     * @retval IO_SUCCESS       Memory allocated successfully
     * @retval IO_NO_MEMORY     Insufficient VRAM
     */
    STDMETHOD_(IO_RETURN, AllocateMemory)(THIS_
        UINT64 cbSize,
        UINT64 *ppAddress
        ) PURE;

    /**
     * @brief Free GPU memory
     *
     * @param pAddress      GPU memory address
     *
     * @retval IO_SUCCESS   Memory freed successfully
     */
    STDMETHOD_(IO_RETURN, FreeMemory)(THIS_
        UINT64 pAddress
        ) PURE;

    /**
     * @brief Submit compute command
     *
     * @param pCommandBuffer    Command buffer
     * @param cbSize            Command buffer size
     *
     * @retval IO_SUCCESS       Command submitted successfully
     */
    STDMETHOD_(IO_RETURN, SubmitCommand)(THIS_
        CONST VOID *pCommandBuffer,
        UINT32 cbSize
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIODisplayController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIODisplayController_GetControllerInfo(p,a)        (p)->lpVtbl->GetControllerInfo(p,a)
#define IIODisplayController_EnumerateModes(p,a,b)         (p)->lpVtbl->EnumerateModes(p,a,b)
#define IIODisplayController_SetMode(p,a)                  (p)->lpVtbl->SetMode(p,a)
#define IIODisplayController_GetCurrentMode(p,a)           (p)->lpVtbl->GetCurrentMode(p,a)
#define IIODisplayController_AllocateFramebuffer(p,a,b,c,d) (p)->lpVtbl->AllocateFramebuffer(p,a,b,c,d)
#define IIODisplayController_FreeFramebuffer(p,a)          (p)->lpVtbl->FreeFramebuffer(p,a)
#define IIODisplayController_EnableOutput(p,a)             (p)->lpVtbl->EnableOutput(p,a)
#define IIODisplayController_DisableOutput(p,a)            (p)->lpVtbl->DisableOutput(p,a)
#define IIODisplayController_GetEDID(p,a,b)                (p)->lpVtbl->GetEDID(p,a,b)
#define IIODisplayController_SetBrightness(p,a,b)          (p)->lpVtbl->SetBrightness(p,a,b)
#define IIODisplayController_GetBrightness(p,a,b)          (p)->lpVtbl->GetBrightness(p,a,b)
#define IIODisplayController_EnableHDR(p,a)                (p)->lpVtbl->EnableHDR(p,a)
#define IIODisplayController_DisableHDR(p,a)               (p)->lpVtbl->DisableHDR(p,a)

#define IIODisplayDevice_GetDeviceInfo(p,a)                (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIODisplayDevice_GetEDID(p,a)                      (p)->lpVtbl->GetEDID(p,a)
#define IIODisplayDevice_GetSupportedModes(p,a,b)          (p)->lpVtbl->GetSupportedModes(p,a,b)

#define IIOFramebuffer_GetInfo(p,a)                        (p)->lpVtbl->GetInfo(p,a)
#define IIOFramebuffer_Map(p,a)                            (p)->lpVtbl->Map(p,a)
#define IIOFramebuffer_Unmap(p)                            (p)->lpVtbl->Unmap(p)
#define IIOFramebuffer_Blit(p,a,b,c,d,e,f)                 (p)->lpVtbl->Blit(p,a,b,c,d,e,f)
#define IIOFramebuffer_Fill(p,a,b,c,d,e)                   (p)->lpVtbl->Fill(p,a,b,c,d,e)
#define IIOFramebuffer_SetPixel(p,a,b,c)                   (p)->lpVtbl->SetPixel(p,a,b,c)
#define IIOFramebuffer_GetPixel(p,a,b,c)                   (p)->lpVtbl->GetPixel(p,a,b,c)
#define IIOFramebuffer_Pan(p,a,b)                          (p)->lpVtbl->Pan(p,a,b)
#define IIOFramebuffer_SetOffset(p,a)                      (p)->lpVtbl->SetOffset(p,a)

#define IIOAccelerator_GetCapabilities(p,a)                (p)->lpVtbl->GetCapabilities(p,a)
#define IIOAccelerator_AllocateMemory(p,a,b)               (p)->lpVtbl->AllocateMemory(p,a,b)
#define IIOAccelerator_FreeMemory(p,a)                     (p)->lpVtbl->FreeMemory(p,a)
#define IIOAccelerator_SubmitCommand(p,a,b)                (p)->lpVtbl->SubmitCommand(p,a,b)

#endif

/**
 * @brief Create a display controller instance
 *
 * @param pszName           Controller name
 * @param ppController      Receives controller interface
 *
 * @retval IO_SUCCESS           Controller created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid parameter
 */
IO_RETURN
IODisplayControllerCreate(
    CONST CHAR8              *pszName,
    IIODisplayController    **ppController
    );

/**
 * @brief Initialize display subsystem
 *
 * @retval IO_SUCCESS   Subsystem initialized successfully
 */
IO_RETURN
DisplayInitialize(
    VOID
    );

/**
 * @brief Shutdown display subsystem
 *
 * @retval IO_SUCCESS   Subsystem shut down successfully
 */
IO_RETURN
DisplayShutdown(
    VOID
    );

/**
 * @brief Lookup GPU device by vendor/device ID
 *
 * @param uVendorID     Vendor ID
 * @param uDeviceID     Device ID
 * @param ppEntry       Receives device entry (NULL if not found)
 *
 * @retval IO_SUCCESS   Lookup completed
 */
IO_RETURN
DisplayLookupDevice(
    UINT16 uVendorID,
    UINT16 uDeviceID,
    CONST GPU_DEVICE_ENTRY **ppEntry
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_DISPLAY_H */
