/**
 * @file audio.h
 * @brief Audio Family Interface - Comprehensive Audio Device Management
 *
 * This header defines the Audio family interface for managing all types of audio
 * devices including sound cards, integrated audio, HDMI/DisplayPort audio, USB audio,
 * and Bluetooth audio devices.
 *
 * Supports:
 * - Sound cards (PCIe, ISA, USB)
 * - Integrated audio (motherboard)
 * - HDMI/DisplayPort audio
 * - USB Audio Class (UAC 1.0/2.0/3.0)
 * - Bluetooth audio (A2DP, aptX, LDAC)
 * - Professional audio interfaces
 *
 * Audio Technologies:
 * - AC'97 (Audio Codec '97)
 * - HD Audio (Intel High Definition Audio)
 * - Sound Blaster (ISA legacy)
 * - I2S (Inter-IC Sound)
 * - USB Audio Class
 * - Bluetooth audio profiles
 *
 * Supported Codecs:
 * - Realtek: ALC88x, ALC89x, ALC662, ALC892, ALC1220
 * - Creative: EMU10K1 (Sound Blaster Live/Audigy), CA0106
 * - VIA: VT1708, VT1716, VT2020
 * - Cirrus Logic: CS42xx series
 * - Wolfson/Cirrus: WM8xxx series
 * - ESS Technology: ES1371, ES1373, ES1938
 * - C-Media: CMI8738, CMI8788
 * - Analog Devices: AD1986A, AD1988
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_AUDIO_H
#define IOKIT_AUDIO_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOAudioController interface GUID
 * {A7D3E8F1-2B4C-4D6E-9F8A-1C3E5F7B9D2A}
 */
DEFINE_GUID(IID_IIOAudioController,
    0xA7D3E8F1, 0x2B4C, 0x4D6E, 0x9F, 0x8A, 0x1C, 0x3E, 0x5F, 0x7B, 0x9D, 0x2A);

/**
 * @brief IIOAudioDevice interface GUID
 * {B8E4F9A2-3C5D-4E7F-8A9B-2D4F6A8C0E3B}
 */
DEFINE_GUID(IID_IIOAudioDevice,
    0xB8E4F9A2, 0x3C5D, 0x4E7F, 0x8A, 0x9B, 0x2D, 0x4F, 0x6A, 0x8C, 0x0E, 0x3B);

/**
 * @brief IIOAudioStream interface GUID
 * {C9F5A0B3-4D6E-4F8A-9B0C-3E5A7B9D1F4C}
 */
DEFINE_GUID(IID_IIOAudioStream,
    0xC9F5A0B3, 0x4D6E, 0x4F8A, 0x9B, 0x0C, 0x3E, 0x5A, 0x7B, 0x9D, 0x1F, 0x4C);

/**
 * @brief IIOAudioMixer interface GUID
 * {D0A6B1C4-5E7F-4A9B-8C0D-4F6B8D0A2E5D}
 */
DEFINE_GUID(IID_IIOAudioMixer,
    0xD0A6B1C4, 0x5E7F, 0x4A9B, 0x8C, 0x0D, 0x4F, 0x6B, 0x8D, 0x0A, 0x2E, 0x5D);

/**
 * @brief Audio Device Types
 */
typedef enum _AUDIO_DEVICE_TYPE {
    AUDIO_TYPE_UNKNOWN          = 0x00,     /**< Unknown device type */
    AUDIO_TYPE_PCI_SOUND_CARD   = 0x01,     /**< PCI/PCIe sound card */
    AUDIO_TYPE_ISA_SOUND_CARD   = 0x02,     /**< ISA sound card (legacy) */
    AUDIO_TYPE_INTEGRATED       = 0x03,     /**< Integrated audio (motherboard) */
    AUDIO_TYPE_HDMI             = 0x04,     /**< HDMI audio */
    AUDIO_TYPE_DISPLAYPORT      = 0x05,     /**< DisplayPort audio */
    AUDIO_TYPE_USB_AUDIO        = 0x06,     /**< USB audio device */
    AUDIO_TYPE_BLUETOOTH        = 0x07,     /**< Bluetooth audio */
    AUDIO_TYPE_PROFESSIONAL     = 0x08,     /**< Professional audio interface */
    AUDIO_TYPE_VIRTUAL          = 0x09,     /**< Virtual audio device */
} AUDIO_DEVICE_TYPE;

/**
 * @brief Audio Technology Standards
 */
typedef enum _AUDIO_TECHNOLOGY {
    AUDIO_TECH_UNKNOWN          = 0x00,     /**< Unknown technology */
    AUDIO_TECH_AC97             = 0x01,     /**< AC'97 (Audio Codec '97) */
    AUDIO_TECH_HD_AUDIO         = 0x02,     /**< Intel HD Audio */
    AUDIO_TECH_SOUND_BLASTER    = 0x03,     /**< Sound Blaster (ISA) */
    AUDIO_TECH_I2S              = 0x04,     /**< I2S (Inter-IC Sound) */
    AUDIO_TECH_USB_AUDIO_1_0    = 0x05,     /**< USB Audio Class 1.0 */
    AUDIO_TECH_USB_AUDIO_2_0    = 0x06,     /**< USB Audio Class 2.0 */
    AUDIO_TECH_USB_AUDIO_3_0    = 0x07,     /**< USB Audio Class 3.0 */
    AUDIO_TECH_BLUETOOTH_A2DP   = 0x08,     /**< Bluetooth A2DP */
    AUDIO_TECH_BLUETOOTH_APTX   = 0x09,     /**< Bluetooth aptX */
    AUDIO_TECH_BLUETOOTH_LDAC   = 0x0A,     /**< Bluetooth LDAC */
} AUDIO_TECHNOLOGY;

/**
 * @brief Audio Codec Vendors
 */
typedef enum _AUDIO_CODEC_VENDOR {
    CODEC_VENDOR_UNKNOWN        = 0x0000,   /**< Unknown vendor */
    CODEC_VENDOR_REALTEK        = 0x10EC,   /**< Realtek Semiconductor */
    CODEC_VENDOR_CREATIVE       = 0x1102,   /**< Creative Labs */
    CODEC_VENDOR_VIA            = 0x1106,   /**< VIA Technologies */
    CODEC_VENDOR_CIRRUS         = 0x1013,   /**< Cirrus Logic */
    CODEC_VENDOR_WOLFSON        = 0x14A0,   /**< Wolfson Microelectronics */
    CODEC_VENDOR_ESS            = 0x125D,   /**< ESS Technology */
    CODEC_VENDOR_CMEDIA         = 0x13F6,   /**< C-Media Electronics */
    CODEC_VENDOR_ANALOG_DEVICES = 0x11D4,   /**< Analog Devices */
    CODEC_VENDOR_INTEL          = 0x8086,   /**< Intel Corporation */
    CODEC_VENDOR_NVIDIA         = 0x10DE,   /**< NVIDIA Corporation */
    CODEC_VENDOR_AMD            = 0x1002,   /**< AMD/ATI Technologies */
    CODEC_VENDOR_ENSONIQ        = 0x1274,   /**< Ensoniq/Creative */
    CODEC_VENDOR_YAMAHA         = 0x1073,   /**< Yamaha Corporation */
    CODEC_VENDOR_CONEXANT       = 0x14F1,   /**< Conexant Systems */
    CODEC_VENDOR_IDT            = 0x111D,   /**< IDT (Integrated Device Technology) */
    CODEC_VENDOR_SIGMATEL       = 0x8384,   /**< SigmaTel (now IDT) */
} AUDIO_CODEC_VENDOR;

/**
 * @brief Realtek HD Audio Codec Models
 */
typedef enum _REALTEK_CODEC_MODEL {
    REALTEK_ALC880              = 0x0880,   /**< ALC880 (8-channel) */
    REALTEK_ALC882              = 0x0882,   /**< ALC882 (8-channel) */
    REALTEK_ALC883              = 0x0883,   /**< ALC883 (8-channel) */
    REALTEK_ALC885              = 0x0885,   /**< ALC885 (8-channel) */
    REALTEK_ALC888              = 0x0888,   /**< ALC888 (8-channel) */
    REALTEK_ALC889              = 0x0889,   /**< ALC889 (8-channel, improved) */
    REALTEK_ALC892              = 0x0892,   /**< ALC892 (8-channel, popular) */
    REALTEK_ALC662              = 0x0662,   /**< ALC662 (6-channel) */
    REALTEK_ALC663              = 0x0663,   /**< ALC663 (6-channel) */
    REALTEK_ALC665              = 0x0665,   /**< ALC665 (6-channel) */
    REALTEK_ALC668              = 0x0668,   /**< ALC668 (8-channel) */
    REALTEK_ALC861              = 0x0861,   /**< ALC861 (6-channel) */
    REALTEK_ALC867              = 0x0867,   /**< ALC867 (8-channel) */
    REALTEK_ALC1150             = 0x1150,   /**< ALC1150 (high-end) */
    REALTEK_ALC1220             = 0x1220,   /**< ALC1220 (flagship, 120dB SNR) */
    REALTEK_ALC269              = 0x0269,   /**< ALC269 (laptop, low-power) */
    REALTEK_ALC271X             = 0x0271,   /**< ALC271X (laptop) */
    REALTEK_ALC283              = 0x0283,   /**< ALC283 (laptop) */
    REALTEK_ALC293              = 0x0293,   /**< ALC293 (laptop) */
    REALTEK_ALC298              = 0x0298,   /**< ALC298 (laptop) */
} REALTEK_CODEC_MODEL;

/**
 * @brief Audio Sample Rates
 */
typedef enum _AUDIO_SAMPLE_RATE {
    AUDIO_RATE_8000             = 8000,     /**< 8 kHz (telephony) */
    AUDIO_RATE_11025            = 11025,    /**< 11.025 kHz */
    AUDIO_RATE_16000            = 16000,    /**< 16 kHz (wideband) */
    AUDIO_RATE_22050            = 22050,    /**< 22.05 kHz */
    AUDIO_RATE_32000            = 32000,    /**< 32 kHz */
    AUDIO_RATE_44100            = 44100,    /**< 44.1 kHz (CD quality) */
    AUDIO_RATE_48000            = 48000,    /**< 48 kHz (DVD/DAT) */
    AUDIO_RATE_88200            = 88200,    /**< 88.2 kHz (high-res) */
    AUDIO_RATE_96000            = 96000,    /**< 96 kHz (high-res) */
    AUDIO_RATE_176400           = 176400,   /**< 176.4 kHz (high-res) */
    AUDIO_RATE_192000           = 192000,   /**< 192 kHz (high-res) */
    AUDIO_RATE_352800           = 352800,   /**< 352.8 kHz (ultra high-res) */
    AUDIO_RATE_384000           = 384000,   /**< 384 kHz (ultra high-res) */
} AUDIO_SAMPLE_RATE;

/**
 * @brief Audio Bit Depths
 */
typedef enum _AUDIO_BIT_DEPTH {
    AUDIO_BITS_8                = 8,        /**< 8-bit audio */
    AUDIO_BITS_16               = 16,       /**< 16-bit audio (CD quality) */
    AUDIO_BITS_20               = 20,       /**< 20-bit audio */
    AUDIO_BITS_24               = 24,       /**< 24-bit audio (professional) */
    AUDIO_BITS_32               = 32,       /**< 32-bit audio (floating point) */
} AUDIO_BIT_DEPTH;

/**
 * @brief Audio Channel Configurations
 */
typedef enum _AUDIO_CHANNEL_CONFIG {
    AUDIO_CHANNELS_MONO         = 1,        /**< Mono (1.0) */
    AUDIO_CHANNELS_STEREO       = 2,        /**< Stereo (2.0) */
    AUDIO_CHANNELS_2_1          = 3,        /**< 2.1 (stereo + subwoofer) */
    AUDIO_CHANNELS_QUAD         = 4,        /**< Quadraphonic (4.0) */
    AUDIO_CHANNELS_4_1          = 5,        /**< 4.1 (quad + subwoofer) */
    AUDIO_CHANNELS_5_1          = 6,        /**< 5.1 surround */
    AUDIO_CHANNELS_6_1          = 7,        /**< 6.1 surround */
    AUDIO_CHANNELS_7_1          = 8,        /**< 7.1 surround */
    AUDIO_CHANNELS_7_1_4        = 12,       /**< 7.1.4 Dolby Atmos */
    AUDIO_CHANNELS_9_1_6        = 16,       /**< 9.1.6 Dolby Atmos */
} AUDIO_CHANNEL_CONFIG;

/**
 * @brief Audio Encoding Formats
 */
typedef enum _AUDIO_ENCODING {
    AUDIO_ENCODING_PCM          = 0x01,     /**< PCM (uncompressed) */
    AUDIO_ENCODING_ADPCM        = 0x02,     /**< ADPCM (compressed) */
    AUDIO_ENCODING_AC3          = 0x03,     /**< Dolby Digital (AC-3) */
    AUDIO_ENCODING_DTS          = 0x04,     /**< DTS */
    AUDIO_ENCODING_DOLBY_ATMOS  = 0x05,     /**< Dolby Atmos */
    AUDIO_ENCODING_DTS_X        = 0x06,     /**< DTS:X */
    AUDIO_ENCODING_MPEG1_L1     = 0x07,     /**< MPEG-1 Layer I */
    AUDIO_ENCODING_MPEG1_L2     = 0x08,     /**< MPEG-1 Layer II */
    AUDIO_ENCODING_MPEG1_L3     = 0x09,     /**< MPEG-1 Layer III (MP3) */
    AUDIO_ENCODING_AAC          = 0x0A,     /**< AAC */
    AUDIO_ENCODING_WMA          = 0x0B,     /**< Windows Media Audio */
    AUDIO_ENCODING_FLAC         = 0x0C,     /**< FLAC (lossless) */
    AUDIO_ENCODING_ALAC         = 0x0D,     /**< ALAC (Apple Lossless) */
    AUDIO_ENCODING_OPUS         = 0x0E,     /**< Opus */
    AUDIO_ENCODING_VORBIS       = 0x0F,     /**< Vorbis */
    AUDIO_ENCODING_DSD          = 0x10,     /**< DSD (Direct Stream Digital) */
} AUDIO_ENCODING;

/**
 * @brief Audio Stream Direction
 */
typedef enum _AUDIO_STREAM_DIRECTION {
    AUDIO_DIRECTION_PLAYBACK    = 0x01,     /**< Playback (output) */
    AUDIO_DIRECTION_CAPTURE     = 0x02,     /**< Capture/recording (input) */
    AUDIO_DIRECTION_DUPLEX      = 0x03,     /**< Bidirectional (both) */
} AUDIO_STREAM_DIRECTION;

/**
 * @brief Audio Capability Flags
 */
#define AUDIO_CAP_PLAYBACK              0x00000001  /**< Supports playback */
#define AUDIO_CAP_CAPTURE               0x00000002  /**< Supports capture/recording */
#define AUDIO_CAP_DUPLEX                0x00000004  /**< Simultaneous play/record */
#define AUDIO_CAP_MULTI_CHANNEL         0x00000008  /**< Multi-channel (>2) support */
#define AUDIO_CAP_HARDWARE_MIXING       0x00000010  /**< Hardware mixing */
#define AUDIO_CAP_SAMPLE_RATE_CONVERT   0x00000020  /**< Sample rate conversion */
#define AUDIO_CAP_BIT_DEPTH_CONVERT     0x00000040  /**< Bit depth conversion */
#define AUDIO_CAP_SPDIF_OUT             0x00000080  /**< S/PDIF digital output */
#define AUDIO_CAP_SPDIF_IN              0x00000100  /**< S/PDIF digital input */
#define AUDIO_CAP_HDMI_AUDIO            0x00000200  /**< HDMI audio output */
#define AUDIO_CAP_DISPLAYPORT_AUDIO     0x00000400  /**< DisplayPort audio */
#define AUDIO_CAP_DSD                   0x00000800  /**< DSD support */
#define AUDIO_CAP_HIGH_RES_AUDIO        0x00001000  /**< High-res audio (>192kHz) */
#define AUDIO_CAP_DOLBY_DIGITAL         0x00002000  /**< Dolby Digital (AC-3) */
#define AUDIO_CAP_DOLBY_ATMOS           0x00004000  /**< Dolby Atmos */
#define AUDIO_CAP_DTS                   0x00008000  /**< DTS */
#define AUDIO_CAP_DTS_X                 0x00010000  /**< DTS:X */
#define AUDIO_CAP_HEADPHONE_AMP         0x00020000  /**< Integrated headphone amp */
#define AUDIO_CAP_MIC_BOOST             0x00040000  /**< Microphone boost */
#define AUDIO_CAP_LOOPBACK              0x00080000  /**< Loopback recording */
#define AUDIO_CAP_EFFECTS_PROCESSING    0x00100000  /**< Effects processing (EQ, etc.) */
#define AUDIO_CAP_3D_AUDIO              0x00200000  /**< 3D audio positioning */
#define AUDIO_CAP_JACK_DETECTION        0x00400000  /**< Jack detection/sensing */
#define AUDIO_CAP_HOTPLUG               0x00800000  /**< Hot-plug support */
#define AUDIO_CAP_POWER_MANAGEMENT      0x01000000  /**< Power management */
#define AUDIO_CAP_VIRTUAL_SURROUND      0x02000000  /**< Virtual surround sound */

/**
 * @brief Audio Format Specification
 */
typedef struct _AUDIO_FORMAT {
    AUDIO_SAMPLE_RATE       SampleRate;     /**< Sample rate in Hz */
    AUDIO_BIT_DEPTH         BitDepth;       /**< Bit depth */
    AUDIO_CHANNEL_CONFIG    Channels;       /**< Number of channels */
    AUDIO_ENCODING          Encoding;       /**< Audio encoding format */
    UINT32                  FrameSize;      /**< Frame size in bytes */
    UINT32                  BufferSize;     /**< Buffer size in frames */
    BOOLEAN                 bInterleaved;   /**< Interleaved channels */
    BOOLEAN                 bSignedSamples; /**< Signed samples */
    BOOLEAN                 bBigEndian;     /**< Big endian byte order */
} AUDIO_FORMAT;

/**
 * @brief Audio Controller Information
 */
typedef struct _AUDIO_CONTROLLER_INFO {
    AUDIO_DEVICE_TYPE       DeviceType;     /**< Device type */
    AUDIO_TECHNOLOGY        Technology;     /**< Audio technology */
    AUDIO_CODEC_VENDOR      Vendor;         /**< Codec vendor */
    UINT16                  DeviceID;       /**< Device/codec ID */
    UINT16                  SubsystemVendor;/**< Subsystem vendor ID */
    UINT16                  SubsystemDevice;/**< Subsystem device ID */
    UINT8                   RevisionID;     /**< Revision ID */
    UINT32                  Capabilities;   /**< Capability flags */
    UINT8                   NumDACs;        /**< Number of DACs */
    UINT8                   NumADCs;        /**< Number of ADCs */
    UINT32                  MaxSampleRate;  /**< Maximum sample rate */
    UINT8                   MaxChannels;    /**< Maximum channels */
    CHAR8                   ControllerName[128]; /**< Controller name */
    CHAR8                   CodecName[128]; /**< Codec name */
} AUDIO_CONTROLLER_INFO;

/**
 * @brief Audio Device Information
 */
typedef struct _AUDIO_DEVICE_INFO {
    UINT32                  DeviceIndex;    /**< Device index */
    AUDIO_STREAM_DIRECTION  Direction;      /**< Stream direction */
    UINT32                  Capabilities;   /**< Device capabilities */
    UINT8                   NumFormats;     /**< Number of supported formats */
    AUDIO_FORMAT            SupportedFormats[32]; /**< Supported formats */
    CHAR8                   DeviceName[64]; /**< Device name */
    CHAR8                   Description[128]; /**< Description */
} AUDIO_DEVICE_INFO;

/**
 * @brief Audio Stream Information
 */
typedef struct _AUDIO_STREAM_INFO {
    UINT32                  StreamID;       /**< Stream identifier */
    AUDIO_STREAM_DIRECTION  Direction;      /**< Stream direction */
    AUDIO_FORMAT            Format;         /**< Current format */
    UINT32                  BufferSize;     /**< Buffer size in bytes */
    UINT32                  Period;         /**< Period size in frames */
    UINT32                  Latency;        /**< Latency in microseconds */
    BOOLEAN                 bIsActive;      /**< Stream is active */
    BOOLEAN                 bIsPaused;      /**< Stream is paused */
    UINT64                  TotalFrames;    /**< Total frames processed */
    UINT64                  Position;       /**< Current position in frames */
} AUDIO_STREAM_INFO;

/**
 * @brief Audio Mixer Control Types
 */
typedef enum _AUDIO_MIXER_CONTROL {
    MIXER_CONTROL_VOLUME        = 0x01,     /**< Volume control */
    MIXER_CONTROL_MUTE          = 0x02,     /**< Mute control */
    MIXER_CONTROL_BALANCE       = 0x03,     /**< Balance control */
    MIXER_CONTROL_BASS          = 0x04,     /**< Bass control */
    MIXER_CONTROL_TREBLE        = 0x05,     /**< Treble control */
    MIXER_CONTROL_GAIN          = 0x06,     /**< Gain control */
    MIXER_CONTROL_SELECT        = 0x07,     /**< Input/output select */
    MIXER_CONTROL_3D_EFFECT     = 0x08,     /**< 3D effect control */
    MIXER_CONTROL_TONE          = 0x09,     /**< Tone control */
    MIXER_CONTROL_EQUALIZER     = 0x0A,     /**< Equalizer */
} AUDIO_MIXER_CONTROL;

/**
 * @brief Audio Codec Database Entry
 */
typedef struct _AUDIO_CODEC_DATABASE_ENTRY {
    AUDIO_CODEC_VENDOR      Vendor;         /**< Vendor ID */
    UINT16                  DeviceID;       /**< Device/codec ID */
    CONST CHAR8            *pszVendorName;  /**< Vendor name */
    CONST CHAR8            *pszCodecName;   /**< Codec name */
    AUDIO_TECHNOLOGY        Technology;     /**< Technology */
    UINT32                  Capabilities;   /**< Default capabilities */
    UINT8                   MaxDACs;        /**< Maximum DACs */
    UINT8                   MaxADCs;        /**< Maximum ADCs */
    UINT32                  MaxSampleRate;  /**< Maximum sample rate */
    UINT8                   MaxChannels;    /**< Maximum channels */
} AUDIO_CODEC_DATABASE_ENTRY;

//
// Forward declarations
//
DECLARE_INTERFACE_(IIOAudioController, IIOService);
DECLARE_INTERFACE_(IIOAudioDevice, IIOService);
DECLARE_INTERFACE_(IIOAudioStream, IIOService);
DECLARE_INTERFACE_(IIOAudioMixer, IIOService);

/**
 * @brief IIOAudioController - Audio Controller Interface
 *
 * Represents an audio controller/codec (e.g., HD Audio controller, AC'97 codec).
 */
#undef INTERFACE
#define INTERFACE IIOAudioController

DECLARE_INTERFACE_(IIOAudioController, IIOService)
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

    // IIOAudioController specific methods

    /**
     * @brief Get audio controller information
     *
     * Retrieves comprehensive controller/codec information.
     *
     * @param pInfo         Receives controller information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetControllerInfo)(THIS_
        AUDIO_CONTROLLER_INFO *pInfo
        ) PURE;

    /**
     * @brief Enumerate audio devices
     *
     * Enumerates all audio devices (playback/capture) on this controller.
     *
     * @param ppDevices     Array to receive device pointers
     * @param puCount       On input: max devices; On output: actual count
     *
     * @retval IO_SUCCESS       Enumeration successful
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, EnumerateDevices)(THIS_
        IIOAudioDevice **ppDevices,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Get codec information
     *
     * Retrieves detailed codec information.
     *
     * @param pszCodecInfo  Buffer to receive codec info string
     * @param cbSize        Buffer size
     *
     * @retval IO_SUCCESS       Information retrieved
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCodecInfo)(THIS_
        CHAR8 *pszCodecInfo,
        UINTN cbSize
        ) PURE;

    /**
     * @brief Set master volume
     *
     * Sets the master volume level.
     *
     * @param uVolume       Volume level (0-100)
     *
     * @retval IO_SUCCESS   Volume set successfully
     */
    STDMETHOD_(IO_RETURN, SetMasterVolume)(THIS_
        UINT32 uVolume
        ) PURE;

    /**
     * @brief Get master volume
     *
     * Gets the current master volume level.
     *
     * @param puVolume      Receives volume level (0-100)
     *
     * @retval IO_SUCCESS       Volume retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetMasterVolume)(THIS_
        UINT32 *puVolume
        ) PURE;

    /**
     * @brief Mute audio output
     *
     * @param bMute         TRUE to mute, FALSE to unmute
     *
     * @retval IO_SUCCESS   Mute state changed
     */
    STDMETHOD_(IO_RETURN, Mute)(THIS_
        BOOLEAN bMute
        ) PURE;

    /**
     * @brief Get mute state
     *
     * @param pbMuted       Receives mute state
     *
     * @retval IO_SUCCESS       State retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetMuteState)(THIS_
        BOOLEAN *pbMuted
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOAudioDevice - Audio Device Interface
 *
 * Represents a single audio device (playback or capture).
 */
#undef INTERFACE
#define INTERFACE IIOAudioDevice

DECLARE_INTERFACE_(IIOAudioDevice, IIOService)
{
    // IUnknown and IIOService methods (same as above)...

    /**
     * @brief Get audio device information
     *
     * Retrieves device information including supported formats.
     *
     * @param pInfo         Receives device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDeviceInfo)(THIS_
        AUDIO_DEVICE_INFO *pInfo
        ) PURE;

    /**
     * @brief Open audio stream
     *
     * Opens an audio stream for playback or capture.
     *
     * @param pFormat       Desired audio format
     * @param ppStream      Receives stream interface
     *
     * @retval IO_SUCCESS       Stream opened successfully
     * @retval IO_BAD_ARGUMENT  Invalid format
     * @retval IO_BUSY          Device already in use
     */
    STDMETHOD_(IO_RETURN, OpenStream)(THIS_
        CONST AUDIO_FORMAT *pFormat,
        IIOAudioStream **ppStream
        ) PURE;

    /**
     * @brief Close audio stream
     *
     * @param pStream       Stream to close
     *
     * @retval IO_SUCCESS   Stream closed successfully
     */
    STDMETHOD_(IO_RETURN, CloseStream)(THIS_
        IIOAudioStream *pStream
        ) PURE;

    /**
     * @brief Get supported audio formats
     *
     * @param pFormats      Array to receive formats
     * @param puCount       On input: max formats; On output: actual count
     *
     * @retval IO_SUCCESS       Formats retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetSupportedFormats)(THIS_
        AUDIO_FORMAT *pFormats,
        UINT32 *puCount
        ) PURE;

    /**
     * @brief Set audio format
     *
     * @param pFormat       Desired audio format
     *
     * @retval IO_SUCCESS       Format set successfully
     * @retval IO_UNSUPPORTED   Format not supported
     */
    STDMETHOD_(IO_RETURN, SetFormat)(THIS_
        CONST AUDIO_FORMAT *pFormat
        ) PURE;

    /**
     * @brief Get current format
     *
     * @param pFormat       Receives current format
     *
     * @retval IO_SUCCESS       Format retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCurrentFormat)(THIS_
        AUDIO_FORMAT *pFormat
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOAudioStream - Audio Stream Interface
 *
 * Represents an active audio stream (playback or capture).
 */
#undef INTERFACE
#define INTERFACE IIOAudioStream

DECLARE_INTERFACE_(IIOAudioStream, IIOService)
{
    // IUnknown and IIOService methods (same as above)...

    /**
     * @brief Start audio stream
     *
     * Begins audio playback or capture.
     *
     * @retval IO_SUCCESS   Stream started successfully
     * @retval IO_ERROR     Failed to start stream
     */
    STDMETHOD_(IO_RETURN, Start)(THIS) PURE;

    /**
     * @brief Stop audio stream
     *
     * Stops audio playback or capture.
     *
     * @retval IO_SUCCESS   Stream stopped successfully
     */
    STDMETHOD_(IO_RETURN, Stop)(THIS) PURE;

    /**
     * @brief Pause audio stream
     *
     * @param bPause        TRUE to pause, FALSE to resume
     *
     * @retval IO_SUCCESS   Stream paused/resumed
     */
    STDMETHOD_(IO_RETURN, Pause)(THIS_
        BOOLEAN bPause
        ) PURE;

    /**
     * @brief Write audio data (playback)
     *
     * Writes audio samples to the playback buffer.
     *
     * @param pBuffer       Buffer containing audio data
     * @param cbSize        Size of data in bytes
     * @param pcbWritten    Receives bytes actually written
     *
     * @retval IO_SUCCESS       Data written successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_ERROR         Write failed
     */
    STDMETHOD_(IO_RETURN, Write)(THIS_
        CONST VOID *pBuffer,
        UINT32 cbSize,
        UINT32 *pcbWritten
        ) PURE;

    /**
     * @brief Read audio data (capture)
     *
     * Reads captured audio samples from the input buffer.
     *
     * @param pBuffer       Buffer to receive audio data
     * @param cbSize        Buffer size in bytes
     * @param pcbRead       Receives bytes actually read
     *
     * @retval IO_SUCCESS       Data read successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_ERROR         Read failed
     */
    STDMETHOD_(IO_RETURN, Read)(THIS_
        VOID *pBuffer,
        UINT32 cbSize,
        UINT32 *pcbRead
        ) PURE;

    /**
     * @brief Get stream position
     *
     * Returns current position in frames.
     *
     * @param puPosition    Receives position in frames
     *
     * @retval IO_SUCCESS       Position retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetPosition)(THIS_
        UINT64 *puPosition
        ) PURE;

    /**
     * @brief Get stream latency
     *
     * Returns current latency in microseconds.
     *
     * @param puLatency     Receives latency in microseconds
     *
     * @retval IO_SUCCESS       Latency retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetLatency)(THIS_
        UINT32 *puLatency
        ) PURE;

    /**
     * @brief Get stream information
     *
     * @param pInfo         Receives stream information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetStreamInfo)(THIS_
        AUDIO_STREAM_INFO *pInfo
        ) PURE;

    /**
     * @brief Set stream volume
     *
     * @param uVolume       Volume level (0-100)
     *
     * @retval IO_SUCCESS   Volume set successfully
     */
    STDMETHOD_(IO_RETURN, SetVolume)(THIS_
        UINT32 uVolume
        ) PURE;

    /**
     * @brief Get stream volume
     *
     * @param puVolume      Receives volume level (0-100)
     *
     * @retval IO_SUCCESS       Volume retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetVolume)(THIS_
        UINT32 *puVolume
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOAudioMixer - Audio Mixer Interface
 *
 * Provides audio mixing and level control.
 */
#undef INTERFACE
#define INTERFACE IIOAudioMixer

DECLARE_INTERFACE_(IIOAudioMixer, IIOService)
{
    // IUnknown and IIOService methods (same as above)...

    /**
     * @brief Get mixer control value
     *
     * @param Control       Control type
     * @param uChannel      Channel index (0 for all)
     * @param puValue       Receives control value
     *
     * @retval IO_SUCCESS       Value retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   Control not supported
     */
    STDMETHOD_(IO_RETURN, GetControlValue)(THIS_
        AUDIO_MIXER_CONTROL Control,
        UINT32 uChannel,
        UINT32 *puValue
        ) PURE;

    /**
     * @brief Set mixer control value
     *
     * @param Control       Control type
     * @param uChannel      Channel index (0 for all)
     * @param uValue        Control value
     *
     * @retval IO_SUCCESS       Value set successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   Control not supported
     */
    STDMETHOD_(IO_RETURN, SetControlValue)(THIS_
        AUDIO_MIXER_CONTROL Control,
        UINT32 uChannel,
        UINT32 uValue
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOAudioController methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOAudioController_GetControllerInfo(p,a)       (p)->lpVtbl->GetControllerInfo(p,a)
#define IIOAudioController_EnumerateDevices(p,a,b)      (p)->lpVtbl->EnumerateDevices(p,a,b)
#define IIOAudioController_GetCodecInfo(p,a,b)          (p)->lpVtbl->GetCodecInfo(p,a,b)
#define IIOAudioController_SetMasterVolume(p,a)         (p)->lpVtbl->SetMasterVolume(p,a)
#define IIOAudioController_GetMasterVolume(p,a)         (p)->lpVtbl->GetMasterVolume(p,a)
#define IIOAudioController_Mute(p,a)                    (p)->lpVtbl->Mute(p,a)
#define IIOAudioController_GetMuteState(p,a)            (p)->lpVtbl->GetMuteState(p,a)

#define IIOAudioDevice_GetDeviceInfo(p,a)               (p)->lpVtbl->GetDeviceInfo(p,a)
#define IIOAudioDevice_OpenStream(p,a,b)                (p)->lpVtbl->OpenStream(p,a,b)
#define IIOAudioDevice_CloseStream(p,a)                 (p)->lpVtbl->CloseStream(p,a)
#define IIOAudioDevice_GetSupportedFormats(p,a,b)       (p)->lpVtbl->GetSupportedFormats(p,a,b)
#define IIOAudioDevice_SetFormat(p,a)                   (p)->lpVtbl->SetFormat(p,a)
#define IIOAudioDevice_GetCurrentFormat(p,a)            (p)->lpVtbl->GetCurrentFormat(p,a)

#define IIOAudioStream_Start(p)                         (p)->lpVtbl->Start(p)
#define IIOAudioStream_Stop(p)                          (p)->lpVtbl->Stop(p)
#define IIOAudioStream_Pause(p,a)                       (p)->lpVtbl->Pause(p,a)
#define IIOAudioStream_Write(p,a,b,c)                   (p)->lpVtbl->Write(p,a,b,c)
#define IIOAudioStream_Read(p,a,b,c)                    (p)->lpVtbl->Read(p,a,b,c)
#define IIOAudioStream_GetPosition(p,a)                 (p)->lpVtbl->GetPosition(p,a)
#define IIOAudioStream_GetLatency(p,a)                  (p)->lpVtbl->GetLatency(p,a)
#define IIOAudioStream_GetStreamInfo(p,a)               (p)->lpVtbl->GetStreamInfo(p,a)
#define IIOAudioStream_SetVolume(p,a)                   (p)->lpVtbl->SetVolume(p,a)
#define IIOAudioStream_GetVolume(p,a)                   (p)->lpVtbl->GetVolume(p,a)

#define IIOAudioMixer_GetControlValue(p,a,b,c)          (p)->lpVtbl->GetControlValue(p,a,b,c)
#define IIOAudioMixer_SetControlValue(p,a,b,c)          (p)->lpVtbl->SetControlValue(p,a,b,c)

#endif

/**
 * @brief Create an audio controller instance
 *
 * @param VendorID      Vendor ID
 * @param DeviceID      Device ID
 * @param ppController  Receives pointer to controller interface
 *
 * @retval IO_SUCCESS   Controller created successfully
 * @retval IO_NO_MEMORY Insufficient memory
 */
IO_RETURN
AudioControllerCreate(
    UINT16 VendorID,
    UINT16 DeviceID,
    IIOAudioController **ppController
    );

/**
 * @brief Initialize audio subsystem
 *
 * @retval IO_SUCCESS   Audio subsystem initialized
 */
IO_RETURN
AudioSubsystemInit(
    VOID
    );

/**
 * @brief Shutdown audio subsystem
 *
 * @retval IO_SUCCESS   Audio subsystem shut down
 */
IO_RETURN
AudioSubsystemShutdown(
    VOID
    );

/**
 * @brief Lookup codec in database
 *
 * @param VendorID      Vendor ID
 * @param DeviceID      Device ID
 *
 * @return Pointer to codec database entry, or NULL if not found
 */
CONST AUDIO_CODEC_DATABASE_ENTRY *
AudioCodecLookup(
    UINT16 VendorID,
    UINT16 DeviceID
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_AUDIO_H */
