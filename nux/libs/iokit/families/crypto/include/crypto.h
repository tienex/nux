/**
 * @file crypto.h
 * @brief Crypto/Security Family Interface - Hardware Crypto Acceleration and Security Devices
 *
 * This header defines the Crypto/Security family interface providing hardware-accelerated
 * cryptographic operations, TPM (Trusted Platform Module) support, random number generation,
 * and smart card reader abstraction. This family is critical for secure boot, key management,
 * encryption, and attestation services.
 *
 * The Crypto family supports:
 * - Hardware crypto accelerators (AES-NI, SHA-NI, QAT, etc.)
 * - Trusted Platform Modules (TPM 1.2, TPM 2.0)
 * - True Random Number Generators (TRNG)
 * - Smart card readers (ISO 7816)
 * - Hardware Security Modules (HSM)
 * - CPU security extensions (Intel SGX, AMD SEV, ARM TrustZone)
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_CRYPTO_H
#define IOKIT_CRYPTO_H

#include <iokit/iokit.h>
#include <iokit/ioservice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOCryptoDevice interface GUID
 * {C1D2E3F4-A5B6-4C7D-8E9F-0A1B2C3D4E5F}
 */
DEFINE_GUID(IID_IIOCryptoDevice,
    0xC1D2E3F4, 0xA5B6, 0x4C7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
 * @brief IIOTPMDevice interface GUID
 * {D2E3F4A5-B6C7-4D8E-9F0A-1B2C3D4E5F6A}
 */
DEFINE_GUID(IID_IIOTPMDevice,
    0xD2E3F4A5, 0xB6C7, 0x4D8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A);

/**
 * @brief IIORNGDevice interface GUID
 * {E3F4A5B6-C7D8-4E9F-0A1B-2C3D4E5F6A7B}
 */
DEFINE_GUID(IID_IIORNGDevice,
    0xE3F4A5B6, 0xC7D8, 0x4E9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B);

/**
 * @brief IIOSmartCardReader interface GUID
 * {F4A5B6C7-D8E9-4F0A-1B2C-3D4E5F6A7B8C}
 */
DEFINE_GUID(IID_IIOSmartCardReader,
    0xF4A5B6C7, 0xD8E9, 0x4F0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C);

//
// ============================================================================
// Crypto Device Types
// ============================================================================
//

/**
 * @brief Crypto Device Types
 */
typedef enum _CRYPTO_DEVICE_TYPE {
    CRYPTO_TYPE_UNKNOWN             = 0x00,     /**< Unknown device type */
    CRYPTO_TYPE_ACCELERATOR         = 0x01,     /**< Hardware crypto accelerator */
    CRYPTO_TYPE_TPM_1_2             = 0x02,     /**< Trusted Platform Module 1.2 */
    CRYPTO_TYPE_TPM_2_0             = 0x03,     /**< Trusted Platform Module 2.0 */
    CRYPTO_TYPE_SMARTCARD_READER    = 0x04,     /**< Smart card reader (ISO 7816) */
    CRYPTO_TYPE_HSM                 = 0x05,     /**< Hardware Security Module */
    CRYPTO_TYPE_TRNG                = 0x06,     /**< True Random Number Generator */
    CRYPTO_TYPE_COPROCESSOR         = 0x07,     /**< Dedicated crypto coprocessor */
    CRYPTO_TYPE_CPU_EXTENSION       = 0x08,     /**< CPU crypto extensions (AES-NI, SHA-NI) */
} CRYPTO_DEVICE_TYPE;

/**
 * @brief Crypto Vendor IDs
 */
typedef enum _CRYPTO_VENDOR {
    CRYPTO_VENDOR_UNKNOWN           = 0x0000,

    // CPU Vendors
    CRYPTO_VENDOR_INTEL             = 0x0001,   /**< Intel (AES-NI, SHA-NI, SGX, QAT) */
    CRYPTO_VENDOR_AMD               = 0x0002,   /**< AMD (PSP, SEV, SME) */
    CRYPTO_VENDOR_ARM               = 0x0003,   /**< ARM (TrustZone, CryptoCell) */

    // Crypto Accelerator Vendors
    CRYPTO_VENDOR_BROADCOM          = 0x0010,   /**< Broadcom BCM58xxx */
    CRYPTO_VENDOR_CAVIUM            = 0x0011,   /**< Cavium NITROX, OCTEON */
    CRYPTO_VENDOR_MARVELL           = 0x0012,   /**< Marvell (acquired Cavium) */
    CRYPTO_VENDOR_QUALCOMM          = 0x0013,   /**< Qualcomm Crypto Engine */
    CRYPTO_VENDOR_NXP               = 0x0014,   /**< NXP CAAM */
    CRYPTO_VENDOR_SAMSUNG           = 0x0015,   /**< Samsung SSS */
    CRYPTO_VENDOR_HISILICON         = 0x0016,   /**< HiSilicon SEC */

    // TPM Vendors
    CRYPTO_VENDOR_INFINEON          = 0x0020,   /**< Infineon TPM */
    CRYPTO_VENDOR_STMICRO           = 0x0021,   /**< STMicroelectronics TPM */
    CRYPTO_VENDOR_NUVOTON           = 0x0022,   /**< Nuvoton TPM */
    CRYPTO_VENDOR_ATMEL             = 0x0023,   /**< Atmel TPM */
    CRYPTO_VENDOR_MICROSOFT         = 0x0024,   /**< Microsoft fTPM */

    // Smart Card Vendors
    CRYPTO_VENDOR_GEMALTO           = 0x0030,   /**< Gemalto/Thales */
    CRYPTO_VENDOR_OBERTHUR          = 0x0031,   /**< Oberthur Technologies */

    // HSM Vendors
    CRYPTO_VENDOR_THALES            = 0x0040,   /**< Thales HSM */
    CRYPTO_VENDOR_UTIMACO           = 0x0041,   /**< Utimaco HSM */
} CRYPTO_VENDOR;

//
// ============================================================================
// Cryptographic Algorithms
// ============================================================================
//

/**
 * @brief Cryptographic Algorithm Types
 */
typedef enum _CRYPTO_ALGORITHM {
    CRYPTO_ALG_NONE                 = 0x0000,

    // Symmetric Encryption
    CRYPTO_ALG_AES_128              = 0x0100,   /**< AES 128-bit */
    CRYPTO_ALG_AES_192              = 0x0101,   /**< AES 192-bit */
    CRYPTO_ALG_AES_256              = 0x0102,   /**< AES 256-bit */
    CRYPTO_ALG_DES                  = 0x0103,   /**< DES (legacy, insecure) */
    CRYPTO_ALG_3DES                 = 0x0104,   /**< Triple DES */
    CRYPTO_ALG_CHACHA20             = 0x0105,   /**< ChaCha20 stream cipher */

    // Hash Functions
    CRYPTO_ALG_SHA1                 = 0x0200,   /**< SHA-1 (legacy, avoid for security) */
    CRYPTO_ALG_SHA224               = 0x0201,   /**< SHA-224 */
    CRYPTO_ALG_SHA256               = 0x0202,   /**< SHA-256 */
    CRYPTO_ALG_SHA384               = 0x0203,   /**< SHA-384 */
    CRYPTO_ALG_SHA512               = 0x0204,   /**< SHA-512 */
    CRYPTO_ALG_SHA3_224             = 0x0205,   /**< SHA3-224 */
    CRYPTO_ALG_SHA3_256             = 0x0206,   /**< SHA3-256 */
    CRYPTO_ALG_SHA3_384             = 0x0207,   /**< SHA3-384 */
    CRYPTO_ALG_SHA3_512             = 0x0208,   /**< SHA3-512 */
    CRYPTO_ALG_BLAKE2B              = 0x0209,   /**< BLAKE2b */
    CRYPTO_ALG_BLAKE2S              = 0x020A,   /**< BLAKE2s */

    // Asymmetric Encryption
    CRYPTO_ALG_RSA_1024             = 0x0300,   /**< RSA 1024-bit (legacy) */
    CRYPTO_ALG_RSA_2048             = 0x0301,   /**< RSA 2048-bit */
    CRYPTO_ALG_RSA_3072             = 0x0302,   /**< RSA 3072-bit */
    CRYPTO_ALG_RSA_4096             = 0x0303,   /**< RSA 4096-bit */

    // Elliptic Curve Cryptography
    CRYPTO_ALG_ECC_P256             = 0x0400,   /**< NIST P-256 (secp256r1) */
    CRYPTO_ALG_ECC_P384             = 0x0401,   /**< NIST P-384 (secp384r1) */
    CRYPTO_ALG_ECC_P521             = 0x0402,   /**< NIST P-521 (secp521r1) */
    CRYPTO_ALG_ED25519              = 0x0403,   /**< Edwards-curve Ed25519 */
    CRYPTO_ALG_ED448                = 0x0404,   /**< Edwards-curve Ed448 */
    CRYPTO_ALG_X25519               = 0x0405,   /**< Curve25519 (ECDH) */

    // Message Authentication Codes
    CRYPTO_ALG_HMAC_SHA1            = 0x0500,   /**< HMAC-SHA1 */
    CRYPTO_ALG_HMAC_SHA256          = 0x0501,   /**< HMAC-SHA256 */
    CRYPTO_ALG_HMAC_SHA384          = 0x0502,   /**< HMAC-SHA384 */
    CRYPTO_ALG_HMAC_SHA512          = 0x0503,   /**< HMAC-SHA512 */
    CRYPTO_ALG_CMAC_AES             = 0x0504,   /**< CMAC with AES */
    CRYPTO_ALG_GMAC                 = 0x0505,   /**< GMAC (Galois MAC) */
    CRYPTO_ALG_POLY1305             = 0x0506,   /**< Poly1305 MAC */
} CRYPTO_ALGORITHM;

/**
 * @brief Block Cipher Modes of Operation
 */
typedef enum _CRYPTO_CIPHER_MODE {
    CRYPTO_MODE_NONE                = 0x00,
    CRYPTO_MODE_ECB                 = 0x01,     /**< Electronic Codebook (avoid for security) */
    CRYPTO_MODE_CBC                 = 0x02,     /**< Cipher Block Chaining */
    CRYPTO_MODE_CTR                 = 0x03,     /**< Counter mode */
    CRYPTO_MODE_GCM                 = 0x04,     /**< Galois/Counter Mode (AEAD) */
    CRYPTO_MODE_XTS                 = 0x05,     /**< XEX-based tweaked-codebook mode (disk encryption) */
    CRYPTO_MODE_CCM                 = 0x06,     /**< Counter with CBC-MAC (AEAD) */
    CRYPTO_MODE_CFB                 = 0x07,     /**< Cipher Feedback */
    CRYPTO_MODE_OFB                 = 0x08,     /**< Output Feedback */
} CRYPTO_CIPHER_MODE;

//
// ============================================================================
// Capability Flags
// ============================================================================
//

/**
 * @brief Crypto Device Capabilities (Bitfield)
 */
#define CRYPTO_CAP_HARDWARE_ACCEL       0x00000001  /**< Hardware acceleration available */
#define CRYPTO_CAP_AES                  0x00000002  /**< AES support */
#define CRYPTO_CAP_DES                  0x00000004  /**< DES/3DES support */
#define CRYPTO_CAP_SHA1                 0x00000008  /**< SHA-1 support */
#define CRYPTO_CAP_SHA2                 0x00000010  /**< SHA-2 family support */
#define CRYPTO_CAP_SHA3                 0x00000020  /**< SHA-3 support */
#define CRYPTO_CAP_RSA                  0x00000040  /**< RSA support */
#define CRYPTO_CAP_ECC                  0x00000080  /**< Elliptic curve support */
#define CRYPTO_CAP_HMAC                 0x00000100  /**< HMAC support */
#define CRYPTO_CAP_CMAC                 0x00000200  /**< CMAC support */
#define CRYPTO_CAP_GCM                  0x00000400  /**< GCM mode support */
#define CRYPTO_CAP_XTS                  0x00000800  /**< XTS mode support */
#define CRYPTO_CAP_CCM                  0x00001000  /**< CCM mode support */
#define CRYPTO_CAP_KEY_STORAGE          0x00002000  /**< Secure key storage */
#define CRYPTO_CAP_KEY_GENERATION       0x00004000  /**< Key generation */
#define CRYPTO_CAP_RNG                  0x00008000  /**< Random number generation */
#define CRYPTO_CAP_TRNG                 0x00010000  /**< True RNG (hardware entropy) */
#define CRYPTO_CAP_DMA                  0x00020000  /**< DMA support */
#define CRYPTO_CAP_ASYNC                0x00040000  /**< Asynchronous operations */
#define CRYPTO_CAP_SECURE_BOOT          0x00080000  /**< Secure boot support */
#define CRYPTO_CAP_ATTESTATION          0x00100000  /**< Attestation support */
#define CRYPTO_CAP_SEALING              0x00200000  /**< Data sealing */
#define CRYPTO_CAP_CHACHA20             0x00400000  /**< ChaCha20 support */
#define CRYPTO_CAP_POLY1305             0x00800000  /**< Poly1305 support */

/**
 * @brief TPM Capabilities (Bitfield)
 */
#define TPM_CAP_TPM_1_2                 0x00000001  /**< TPM 1.2 support */
#define TPM_CAP_TPM_2_0                 0x00000002  /**< TPM 2.0 support */
#define TPM_CAP_PCR_SHA1                0x00000004  /**< SHA-1 PCR banks */
#define TPM_CAP_PCR_SHA256              0x00000008  /**< SHA-256 PCR banks */
#define TPM_CAP_PCR_SHA384              0x00000010  /**< SHA-384 PCR banks */
#define TPM_CAP_PCR_SHA512              0x00000020  /**< SHA-512 PCR banks */
#define TPM_CAP_RSA_2048                0x00000040  /**< RSA 2048-bit keys */
#define TPM_CAP_RSA_3072                0x00000080  /**< RSA 3072-bit keys */
#define TPM_CAP_RSA_4096                0x00000100  /**< RSA 4096-bit keys */
#define TPM_CAP_ECC_P256                0x00000200  /**< ECC P-256 keys */
#define TPM_CAP_ECC_P384                0x00000400  /**< ECC P-384 keys */
#define TPM_CAP_HMAC                    0x00000800  /**< HMAC support */
#define TPM_CAP_AES                     0x00001000  /**< AES support */
#define TPM_CAP_SEALING                 0x00002000  /**< Data sealing */
#define TPM_CAP_QUOTE                   0x00004000  /**< Quote (attestation) */
#define TPM_CAP_NVRAM                   0x00008000  /**< Non-volatile storage */
#define TPM_CAP_CERTIFY_KEY             0x00010000  /**< Key certification */
#define TPM_CAP_HIERARCHY               0x00020000  /**< Key hierarchy */
#define TPM_CAP_POLICY                  0x00040000  /**< Policy-based auth */
#define TPM_CAP_DA_PROTECTION           0x00080000  /**< Dictionary attack protection */

/**
 * @brief RNG Capabilities (Bitfield)
 */
#define RNG_CAP_TRNG                    0x00000001  /**< True RNG (hardware entropy) */
#define RNG_CAP_DRBG                    0x00000002  /**< Deterministic RNG */
#define RNG_CAP_AES_CTR_DRBG            0x00000004  /**< AES-CTR-DRBG */
#define RNG_CAP_HASH_DRBG               0x00000008  /**< Hash-DRBG */
#define RNG_CAP_HMAC_DRBG               0x00000010  /**< HMAC-DRBG */
#define RNG_CAP_RDRAND                  0x00000020  /**< Intel RDRAND instruction */
#define RNG_CAP_RDSEED                  0x00000040  /**< Intel RDSEED instruction */
#define RNG_CAP_CONDITIONED             0x00000080  /**< Entropy conditioning */
#define RNG_CAP_FIPS_140_2              0x00000100  /**< FIPS 140-2 compliant */
#define RNG_CAP_FIPS_140_3              0x00000200  /**< FIPS 140-3 compliant */

//
// ============================================================================
// Data Structures
// ============================================================================
//

/**
 * @brief Crypto Operation Structure
 */
typedef struct _CRYPTO_OPERATION {
    CRYPTO_ALGORITHM    Algorithm;              /**< Cryptographic algorithm */
    CRYPTO_CIPHER_MODE  Mode;                   /**< Cipher mode (for block ciphers) */

    // Key Material
    CONST VOID         *pKey;                   /**< Key data */
    UINTN               cbKey;                  /**< Key size in bytes */

    // Initialization Vector / Nonce
    CONST VOID         *pIV;                    /**< IV/Nonce (if required) */
    UINTN               cbIV;                   /**< IV size in bytes */

    // Additional Authenticated Data (for AEAD modes)
    CONST VOID         *pAAD;                   /**< Additional authenticated data */
    UINTN               cbAAD;                  /**< AAD size in bytes */

    // Input/Output Buffers
    CONST VOID         *pInput;                 /**< Input data */
    UINTN               cbInput;                /**< Input size in bytes */
    VOID               *pOutput;                /**< Output buffer */
    UINTN               cbOutput;               /**< Output buffer size */

    // Authentication Tag (for AEAD modes)
    VOID               *pTag;                   /**< Authentication tag */
    UINTN               cbTag;                  /**< Tag size in bytes */

    // Operation Flags
    UINT32              Flags;                  /**< Operation flags */
} CRYPTO_OPERATION;

/**
 * @brief TPM PCR (Platform Configuration Register)
 */
#define TPM_PCR_COUNT       24                  /**< Standard PCR count (0-23) */
#define TPM_PCR_MAX_SIZE    64                  /**< Maximum PCR size (SHA-512) */

typedef struct _TPM_PCR {
    UINT8               Index;                  /**< PCR index (0-23) */
    CRYPTO_ALGORITHM    HashAlgorithm;          /**< Hash algorithm used */
    UINT8               Value[TPM_PCR_MAX_SIZE];/**< PCR value */
    UINTN               cbValue;                /**< Actual value size */
} TPM_PCR;

/**
 * @brief TPM Version Information
 */
typedef struct _TPM_VERSION_INFO {
    UINT8               MajorVersion;           /**< Major version (1 or 2) */
    UINT8               MinorVersion;           /**< Minor version */
    UINT16              RevisionMajor;          /**< Revision major */
    UINT16              RevisionMinor;          /**< Revision minor */
    CHAR8               VendorID[4];            /**< Vendor ID (4 characters) */
    CHAR8               FirmwareVersion[32];    /**< Firmware version string */
    UINT16              VendorSpecific;         /**< Vendor-specific info */
} TPM_VERSION_INFO;

/**
 * @brief TPM Device Information
 */
typedef struct _TPM_DEVICE_INFO {
    CRYPTO_DEVICE_TYPE  DeviceType;             /**< TPM type (1.2 or 2.0) */
    CRYPTO_VENDOR       Vendor;                 /**< TPM vendor */
    TPM_VERSION_INFO    Version;                /**< Version information */
    UINT32              Capabilities;           /**< Capability flags (TPM_CAP_*) */
    UINT32              NumPCRBanks;            /**< Number of PCR banks */
    UINT32              NVRAMSize;              /**< Non-volatile RAM size */
    UINT32              MaxKeySize;             /**< Maximum key size (bits) */
    BOOLEAN             bOwned;                 /**< TPM is owned */
    BOOLEAN             bEnabled;               /**< TPM is enabled */
    BOOLEAN             bActivated;             /**< TPM is activated */
} TPM_DEVICE_INFO;

/**
 * @brief Crypto Device Information
 */
typedef struct _CRYPTO_DEVICE_INFO {
    CRYPTO_DEVICE_TYPE  DeviceType;             /**< Device type */
    CRYPTO_VENDOR       Vendor;                 /**< Vendor ID */
    CHAR8               DeviceName[64];         /**< Device name */
    CHAR8               VendorName[40];         /**< Vendor name */
    CHAR8               FirmwareVersion[16];    /**< Firmware version */

    // Capabilities
    UINT32              Capabilities;           /**< Capability flags */
    UINT32              MaxKeySize;             /**< Maximum key size (bits) */
    UINT32              MaxOperationSize;       /**< Maximum operation size (bytes) */
    UINT32              NumQueues;              /**< Number of hardware queues */
    UINT32              MaxQueueDepth;          /**< Maximum queue depth */

    // Performance
    UINT64              OperationsPerSecond;    /**< Estimated ops/sec */
    UINT32              ThroughputMBps;         /**< Throughput in MB/s */

    // PCI Info (if applicable)
    UINT16              VendorID;               /**< PCI Vendor ID */
    UINT16              DeviceID;               /**< PCI Device ID */
    UINT8               BusNumber;              /**< Bus number */
    UINT8               DeviceNumber;           /**< Device number */
    UINT8               FunctionNumber;         /**< Function number */
} CRYPTO_DEVICE_INFO;

/**
 * @brief RNG Device Information
 */
typedef struct _RNG_DEVICE_INFO {
    CRYPTO_DEVICE_TYPE  DeviceType;             /**< Device type */
    CRYPTO_VENDOR       Vendor;                 /**< Vendor ID */
    CHAR8               DeviceName[64];         /**< Device name */
    UINT32              Capabilities;           /**< Capability flags (RNG_CAP_*) */
    UINT32              EntropyBitsPerSecond;   /**< Entropy generation rate */
    UINT32              MaxBytesPerRequest;     /**< Maximum bytes per request */
    BOOLEAN             bFIPSCompliant;         /**< FIPS 140-2/3 compliant */
} RNG_DEVICE_INFO;

/**
 * @brief Key Handle (opaque reference to key material)
 */
typedef UINT64 CRYPTO_KEY_HANDLE;

#define CRYPTO_INVALID_KEY_HANDLE   0

/**
 * @brief Crypto Statistics
 */
typedef struct _CRYPTO_STATS {
    UINT64              EncryptOperations;      /**< Encryption operations */
    UINT64              DecryptOperations;      /**< Decryption operations */
    UINT64              HashOperations;         /**< Hash operations */
    UINT64              SignOperations;         /**< Sign operations */
    UINT64              VerifyOperations;       /**< Verify operations */
    UINT64              BytesProcessed;         /**< Total bytes processed */
    UINT64              Errors;                 /**< Error count */
} CRYPTO_STATS;

//
// ============================================================================
// Interface Declarations
// ============================================================================
//

// Forward declarations
DECLARE_INTERFACE_(IIOCryptoDevice, IIOService);
DECLARE_INTERFACE_(IIOTPMDevice, IIOService);
DECLARE_INTERFACE_(IIORNGDevice, IIOService);
DECLARE_INTERFACE_(IIOSmartCardReader, IIOService);

/**
 * @brief IIOCryptoDevice - Crypto Accelerator Interface
 *
 * This interface represents a hardware crypto accelerator device that provides
 * hardware-accelerated cryptographic operations.
 */
#undef INTERFACE
#define INTERFACE IIOCryptoDevice

DECLARE_INTERFACE_(IIOCryptoDevice, IIOService)
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

    // IIOCryptoDevice methods

    /**
     * @brief Get crypto device capabilities
     *
     * @param pDeviceInfo   Receives device information and capabilities
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        CRYPTO_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Encrypt data
     *
     * @param pOperation    Operation parameters (algorithm, key, IV, etc.)
     *
     * @retval IO_SUCCESS       Encryption successful
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_UNSUPPORTED   Algorithm not supported
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, Encrypt)(THIS_
        CRYPTO_OPERATION *pOperation
        ) PURE;

    /**
     * @brief Decrypt data
     *
     * @param pOperation    Operation parameters (algorithm, key, IV, etc.)
     *
     * @retval IO_SUCCESS       Decryption successful
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_UNSUPPORTED   Algorithm not supported
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, Decrypt)(THIS_
        CRYPTO_OPERATION *pOperation
        ) PURE;

    /**
     * @brief Compute cryptographic hash
     *
     * @param Algorithm     Hash algorithm
     * @param pInput        Input data
     * @param cbInput       Input size
     * @param pOutput       Output buffer (hash digest)
     * @param cbOutput      Output buffer size
     * @param pcbActual     Receives actual hash size
     *
     * @retval IO_SUCCESS       Hash computed successfully
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_UNSUPPORTED   Algorithm not supported
     */
    STDMETHOD_(IO_RETURN, Hash)(THIS_
        CRYPTO_ALGORITHM Algorithm,
        CONST VOID *pInput,
        UINTN cbInput,
        VOID *pOutput,
        UINTN cbOutput,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Sign data (asymmetric signature)
     *
     * @param Algorithm     Signature algorithm
     * @param hKey          Key handle
     * @param pInput        Data to sign
     * @param cbInput       Input size
     * @param pSignature    Signature buffer
     * @param cbSignature   Signature buffer size
     * @param pcbActual     Receives actual signature size
     *
     * @retval IO_SUCCESS       Signature created successfully
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_UNSUPPORTED   Algorithm not supported
     */
    STDMETHOD_(IO_RETURN, Sign)(THIS_
        CRYPTO_ALGORITHM Algorithm,
        CRYPTO_KEY_HANDLE hKey,
        CONST VOID *pInput,
        UINTN cbInput,
        VOID *pSignature,
        UINTN cbSignature,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Verify signature
     *
     * @param Algorithm     Signature algorithm
     * @param hKey          Key handle
     * @param pInput        Original data
     * @param cbInput       Input size
     * @param pSignature    Signature to verify
     * @param cbSignature   Signature size
     *
     * @retval IO_SUCCESS       Signature valid
     * @retval IO_ERROR         Signature invalid
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     */
    STDMETHOD_(IO_RETURN, Verify)(THIS_
        CRYPTO_ALGORITHM Algorithm,
        CRYPTO_KEY_HANDLE hKey,
        CONST VOID *pInput,
        UINTN cbInput,
        CONST VOID *pSignature,
        UINTN cbSignature
        ) PURE;

    /**
     * @brief Generate cryptographic key
     *
     * @param Algorithm     Key algorithm
     * @param uKeySize      Key size in bits
     * @param phKey         Receives key handle
     *
     * @retval IO_SUCCESS       Key generated successfully
     * @retval IO_UNSUPPORTED   Algorithm/size not supported
     * @retval IO_NO_RESOURCES  Cannot store key
     */
    STDMETHOD_(IO_RETURN, GenerateKey)(THIS_
        CRYPTO_ALGORITHM Algorithm,
        UINT32 uKeySize,
        CRYPTO_KEY_HANDLE *phKey
        ) PURE;

    /**
     * @brief Generate random bytes
     *
     * @param pBuffer       Output buffer
     * @param cbBuffer      Number of random bytes to generate
     *
     * @retval IO_SUCCESS       Random bytes generated
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   RNG not available
     */
    STDMETHOD_(IO_RETURN, GenerateRandom)(THIS_
        VOID *pBuffer,
        UINTN cbBuffer
        ) PURE;

    /**
     * @brief Get crypto statistics
     *
     * @param pStats        Receives statistics
     * @param bReset        Reset statistics after reading
     *
     * @retval IO_SUCCESS       Statistics retrieved
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetStatistics)(THIS_
        CRYPTO_STATS *pStats,
        BOOLEAN bReset
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOTPMDevice - TPM Device Interface
 *
 * This interface represents a Trusted Platform Module (TPM 1.2 or 2.0) device.
 */
#undef INTERFACE
#define INTERFACE IIOTPMDevice

DECLARE_INTERFACE_(IIOTPMDevice, IIOService)
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

    // IIOTPMDevice methods

    /**
     * @brief Get TPM capabilities and information
     *
     * @param pDeviceInfo   Receives TPM device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        TPM_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Get TPM version
     *
     * @param pVersion      Receives version information
     *
     * @retval IO_SUCCESS       Version retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetVersion)(THIS_
        TPM_VERSION_INFO *pVersion
        ) PURE;

    /**
     * @brief Initialize TPM (TPM2_Startup)
     *
     * @param bClear        TRUE for TPM2_SU_CLEAR, FALSE for TPM2_SU_STATE
     *
     * @retval IO_SUCCESS       TPM started successfully
     * @retval IO_ERROR         Startup failed
     */
    STDMETHOD_(IO_RETURN, Startup)(THIS_
        BOOLEAN bClear
        ) PURE;

    /**
     * @brief Shutdown TPM (TPM2_Shutdown)
     *
     * @param bClear        TRUE for TPM2_SU_CLEAR, FALSE for TPM2_SU_STATE
     *
     * @retval IO_SUCCESS       TPM shutdown successfully
     * @retval IO_ERROR         Shutdown failed
     */
    STDMETHOD_(IO_RETURN, Shutdown)(THIS_
        BOOLEAN bClear
        ) PURE;

    /**
     * @brief Get random bytes from TPM
     *
     * @param pBuffer       Output buffer
     * @param cbBuffer      Number of random bytes requested
     * @param pcbActual     Receives actual bytes returned
     *
     * @retval IO_SUCCESS       Random bytes generated
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, GetRandom)(THIS_
        VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Extend PCR value (critical for secure boot)
     *
     * @param uPCRIndex     PCR index (0-23)
     * @param Algorithm     Hash algorithm
     * @param pData         Data to extend
     * @param cbData        Data size
     *
     * @retval IO_SUCCESS       PCR extended successfully
     * @retval IO_BAD_ARGUMENT  Invalid PCR index or algorithm
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, ExtendPCR)(THIS_
        UINT8 uPCRIndex,
        CRYPTO_ALGORITHM Algorithm,
        CONST VOID *pData,
        UINTN cbData
        ) PURE;

    /**
     * @brief Read PCR value
     *
     * @param uPCRIndex     PCR index (0-23)
     * @param Algorithm     Hash algorithm
     * @param pPCR          Receives PCR value
     *
     * @retval IO_SUCCESS       PCR read successfully
     * @retval IO_BAD_ARGUMENT  Invalid PCR index or algorithm
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, ReadPCR)(THIS_
        UINT8 uPCRIndex,
        CRYPTO_ALGORITHM Algorithm,
        TPM_PCR *pPCR
        ) PURE;

    /**
     * @brief Create key in TPM
     *
     * @param Algorithm     Key algorithm
     * @param uKeySize      Key size in bits
     * @param phKey         Receives key handle
     *
     * @retval IO_SUCCESS       Key created successfully
     * @retval IO_UNSUPPORTED   Algorithm/size not supported
     * @retval IO_NO_RESOURCES  No key slots available
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, CreateKey)(THIS_
        CRYPTO_ALGORITHM Algorithm,
        UINT32 uKeySize,
        CRYPTO_KEY_HANDLE *phKey
        ) PURE;

    /**
     * @brief Load key into TPM
     *
     * @param pKeyBlob      Key blob data
     * @param cbKeyBlob     Key blob size
     * @param phKey         Receives key handle
     *
     * @retval IO_SUCCESS       Key loaded successfully
     * @retval IO_BAD_ARGUMENT  Invalid key blob
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, LoadKey)(THIS_
        CONST VOID *pKeyBlob,
        UINTN cbKeyBlob,
        CRYPTO_KEY_HANDLE *phKey
        ) PURE;

    /**
     * @brief Sign data using TPM key
     *
     * @param hKey          Key handle
     * @param Algorithm     Signature algorithm
     * @param pInput        Data to sign
     * @param cbInput       Input size
     * @param pSignature    Signature buffer
     * @param cbSignature   Signature buffer size
     * @param pcbActual     Receives actual signature size
     *
     * @retval IO_SUCCESS       Signature created
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, Sign)(THIS_
        CRYPTO_KEY_HANDLE hKey,
        CRYPTO_ALGORITHM Algorithm,
        CONST VOID *pInput,
        UINTN cbInput,
        VOID *pSignature,
        UINTN cbSignature,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Verify signature using TPM key
     *
     * @param hKey          Key handle
     * @param Algorithm     Signature algorithm
     * @param pInput        Original data
     * @param cbInput       Input size
     * @param pSignature    Signature to verify
     * @param cbSignature   Signature size
     *
     * @retval IO_SUCCESS       Signature valid
     * @retval IO_ERROR         Signature invalid or operation failed
     */
    STDMETHOD_(IO_RETURN, Verify)(THIS_
        CRYPTO_KEY_HANDLE hKey,
        CRYPTO_ALGORITHM Algorithm,
        CONST VOID *pInput,
        UINTN cbInput,
        CONST VOID *pSignature,
        UINTN cbSignature
        ) PURE;

    /**
     * @brief Seal data to TPM (encrypt with PCR binding)
     *
     * @param pData         Data to seal
     * @param cbData        Data size
     * @param pPCRMask      PCR indices to bind (bitmask)
     * @param pSealedBlob   Sealed data blob buffer
     * @param cbSealedBlob  Sealed blob buffer size
     * @param pcbActual     Receives actual sealed blob size
     *
     * @retval IO_SUCCESS       Data sealed successfully
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_NO_RESOURCES  Insufficient TPM resources
     */
    STDMETHOD_(IO_RETURN, Seal)(THIS_
        CONST VOID *pData,
        UINTN cbData,
        CONST UINT32 *pPCRMask,
        VOID *pSealedBlob,
        UINTN cbSealedBlob,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Unseal data from TPM
     *
     * @param pSealedBlob   Sealed data blob
     * @param cbSealedBlob  Sealed blob size
     * @param pData         Output buffer for unsealed data
     * @param cbData        Output buffer size
     * @param pcbActual     Receives actual data size
     *
     * @retval IO_SUCCESS       Data unsealed successfully
     * @retval IO_ERROR         Unseal failed (PCR mismatch or invalid blob)
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     */
    STDMETHOD_(IO_RETURN, Unseal)(THIS_
        CONST VOID *pSealedBlob,
        UINTN cbSealedBlob,
        VOID *pData,
        UINTN cbData,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Get attestation quote
     *
     * @param pPCRMask      PCR indices to quote (bitmask)
     * @param pNonce        Nonce for freshness
     * @param cbNonce       Nonce size
     * @param pQuote        Quote data buffer
     * @param cbQuote       Quote buffer size
     * @param pcbActual     Receives actual quote size
     *
     * @retval IO_SUCCESS       Quote generated successfully
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, Quote)(THIS_
        CONST UINT32 *pPCRMask,
        CONST VOID *pNonce,
        UINTN cbNonce,
        VOID *pQuote,
        UINTN cbQuote,
        UINTN *pcbActual
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIORNGDevice - Random Number Generator Interface
 *
 * This interface represents a hardware random number generator.
 */
#undef INTERFACE
#define INTERFACE IIORNGDevice

DECLARE_INTERFACE_(IIORNGDevice, IIOService)
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

    // IIORNGDevice methods

    /**
     * @brief Get RNG capabilities
     *
     * @param pDeviceInfo   Receives RNG device information
     *
     * @retval IO_SUCCESS       Information retrieved successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetCapabilities)(THIS_
        RNG_DEVICE_INFO *pDeviceInfo
        ) PURE;

    /**
     * @brief Get random bytes
     *
     * @param pBuffer       Output buffer
     * @param cbBuffer      Number of random bytes to generate
     * @param pcbActual     Receives actual bytes generated
     *
     * @retval IO_SUCCESS       Random bytes generated
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_NOT_READY     RNG not ready (reseeding)
     * @retval IO_ERROR         Operation failed
     */
    STDMETHOD_(IO_RETURN, GetRandom)(THIS_
        VOID *pBuffer,
        UINTN cbBuffer,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Reseed RNG with additional entropy
     *
     * @param pEntropy      Entropy data
     * @param cbEntropy     Entropy size
     *
     * @retval IO_SUCCESS       RNG reseeded successfully
     * @retval IO_BAD_ARGUMENT  Invalid argument
     * @retval IO_UNSUPPORTED   Manual reseeding not supported
     */
    STDMETHOD_(IO_RETURN, Reseed)(THIS_
        CONST VOID *pEntropy,
        UINTN cbEntropy
        ) PURE;
};

#undef INTERFACE

/**
 * @brief IIOSmartCardReader - Smart Card Reader Interface
 *
 * This interface represents a smart card reader device (ISO 7816).
 */
#undef INTERFACE
#define INTERFACE IIOSmartCardReader

DECLARE_INTERFACE_(IIOSmartCardReader, IIOService)
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

    // IIOSmartCardReader methods

    /**
     * @brief Check if card is present
     *
     * @param pbPresent     Receives TRUE if card present
     *
     * @retval IO_SUCCESS       Status retrieved
     * @retval IO_BAD_ARGUMENT  Invalid argument
     */
    STDMETHOD_(IO_RETURN, IsCardPresent)(THIS_
        BOOLEAN *pbPresent
        ) PURE;

    /**
     * @brief Power on card and get ATR
     *
     * @param pATR          Answer To Reset buffer
     * @param cbATR         ATR buffer size
     * @param pcbActual     Receives actual ATR size
     *
     * @retval IO_SUCCESS       Card powered on
     * @retval IO_NO_MEDIA      No card present
     * @retval IO_ERROR         Power-on failed
     */
    STDMETHOD_(IO_RETURN, PowerOn)(THIS_
        VOID *pATR,
        UINTN cbATR,
        UINTN *pcbActual
        ) PURE;

    /**
     * @brief Power off card
     *
     * @retval IO_SUCCESS       Card powered off
     * @retval IO_ERROR         Power-off failed
     */
    STDMETHOD_(IO_RETURN, PowerOff)(THIS) PURE;

    /**
     * @brief Transmit APDU to card
     *
     * @param pCommand      Command APDU
     * @param cbCommand     Command size
     * @param pResponse     Response APDU buffer
     * @param cbResponse    Response buffer size
     * @param pcbActual     Receives actual response size
     *
     * @retval IO_SUCCESS       APDU transmitted successfully
     * @retval IO_BAD_ARGUMENT  Invalid parameters
     * @retval IO_NOT_READY     Card not powered on
     * @retval IO_ERROR         Transmission failed
     */
    STDMETHOD_(IO_RETURN, Transmit)(THIS_
        CONST VOID *pCommand,
        UINTN cbCommand,
        VOID *pResponse,
        UINTN cbResponse,
        UINTN *pcbActual
        ) PURE;
};

#undef INTERFACE

//
// ============================================================================
// Convenience Macros
// ============================================================================
//

#if !defined(__cplusplus) || defined(CINTERFACE)

// IIOCryptoDevice macros
#define IIOCryptoDevice_GetCapabilities(p,a)        (p)->lpVtbl->GetCapabilities(p,a)
#define IIOCryptoDevice_Encrypt(p,a)                (p)->lpVtbl->Encrypt(p,a)
#define IIOCryptoDevice_Decrypt(p,a)                (p)->lpVtbl->Decrypt(p,a)
#define IIOCryptoDevice_Hash(p,a,b,c,d,e,f)         (p)->lpVtbl->Hash(p,a,b,c,d,e,f)
#define IIOCryptoDevice_Sign(p,a,b,c,d,e,f,g)       (p)->lpVtbl->Sign(p,a,b,c,d,e,f,g)
#define IIOCryptoDevice_Verify(p,a,b,c,d,e,f)       (p)->lpVtbl->Verify(p,a,b,c,d,e,f)
#define IIOCryptoDevice_GenerateKey(p,a,b,c)        (p)->lpVtbl->GenerateKey(p,a,b,c)
#define IIOCryptoDevice_GenerateRandom(p,a,b)       (p)->lpVtbl->GenerateRandom(p,a,b)
#define IIOCryptoDevice_GetStatistics(p,a,b)        (p)->lpVtbl->GetStatistics(p,a,b)

// IIOTPMDevice macros
#define IIOTPMDevice_GetCapabilities(p,a)           (p)->lpVtbl->GetCapabilities(p,a)
#define IIOTPMDevice_GetVersion(p,a)                (p)->lpVtbl->GetVersion(p,a)
#define IIOTPMDevice_Startup(p,a)                   (p)->lpVtbl->Startup(p,a)
#define IIOTPMDevice_Shutdown(p,a)                  (p)->lpVtbl->Shutdown(p,a)
#define IIOTPMDevice_GetRandom(p,a,b,c)             (p)->lpVtbl->GetRandom(p,a,b,c)
#define IIOTPMDevice_ExtendPCR(p,a,b,c,d)           (p)->lpVtbl->ExtendPCR(p,a,b,c,d)
#define IIOTPMDevice_ReadPCR(p,a,b,c)               (p)->lpVtbl->ReadPCR(p,a,b,c)
#define IIOTPMDevice_CreateKey(p,a,b,c)             (p)->lpVtbl->CreateKey(p,a,b,c)
#define IIOTPMDevice_LoadKey(p,a,b,c)               (p)->lpVtbl->LoadKey(p,a,b,c)
#define IIOTPMDevice_Sign(p,a,b,c,d,e,f,g)          (p)->lpVtbl->Sign(p,a,b,c,d,e,f,g)
#define IIOTPMDevice_Verify(p,a,b,c,d,e,f)          (p)->lpVtbl->Verify(p,a,b,c,d,e,f)
#define IIOTPMDevice_Seal(p,a,b,c,d,e,f)            (p)->lpVtbl->Seal(p,a,b,c,d,e,f)
#define IIOTPMDevice_Unseal(p,a,b,c,d,e)            (p)->lpVtbl->Unseal(p,a,b,c,d,e)
#define IIOTPMDevice_Quote(p,a,b,c,d,e,f)           (p)->lpVtbl->Quote(p,a,b,c,d,e,f)

// IIORNGDevice macros
#define IIORNGDevice_GetCapabilities(p,a)           (p)->lpVtbl->GetCapabilities(p,a)
#define IIORNGDevice_GetRandom(p,a,b,c)             (p)->lpVtbl->GetRandom(p,a,b,c)
#define IIORNGDevice_Reseed(p,a,b)                  (p)->lpVtbl->Reseed(p,a,b)

// IIOSmartCardReader macros
#define IIOSmartCardReader_IsCardPresent(p,a)      (p)->lpVtbl->IsCardPresent(p,a)
#define IIOSmartCardReader_PowerOn(p,a,b,c)        (p)->lpVtbl->PowerOn(p,a,b,c)
#define IIOSmartCardReader_PowerOff(p)             (p)->lpVtbl->PowerOff(p)
#define IIOSmartCardReader_Transmit(p,a,b,c,d,e)   (p)->lpVtbl->Transmit(p,a,b,c,d,e)

#endif

//
// ============================================================================
// Public API Functions
// ============================================================================
//

/**
 * @brief Initialize Crypto family subsystem
 *
 * Initializes the crypto/security abstraction layer and registers it with IOKit.
 * This must be called during system initialization.
 *
 * @retval IO_SUCCESS   Initialization successful
 * @retval IO_ERROR     Initialization failed
 */
IO_RETURN
CryptoInitialize(
    VOID
    );

/**
 * @brief Shutdown Crypto family subsystem
 *
 * Shuts down the crypto/security abstraction layer and releases resources.
 *
 * @retval IO_SUCCESS   Shutdown successful
 */
IO_RETURN
CryptoShutdown(
    VOID
    );

/**
 * @brief Detect CPU crypto extensions
 *
 * Detects CPU-based crypto extensions (AES-NI, SHA-NI, etc.) via CPUID.
 *
 * @param pCapabilities Receives detected capability flags
 *
 * @retval IO_SUCCESS       Detection completed
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
CryptoDetectCPUExtensions(
    UINT32 *pCapabilities
    );

/**
 * @brief Detect TPM device
 *
 * Detects TPM device (via ACPI or memory-mapped interface) and returns
 * TPM type and version.
 *
 * @param pDeviceType   Receives TPM type (1.2 or 2.0)
 * @param pVendor       Receives TPM vendor
 *
 * @retval IO_SUCCESS       TPM detected
 * @retval IO_NO_DEVICE     No TPM found
 * @retval IO_BAD_ARGUMENT  Invalid argument
 */
IO_RETURN
CryptoDetectTPM(
    CRYPTO_DEVICE_TYPE *pDeviceType,
    CRYPTO_VENDOR *pVendor
    );

/**
 * @brief Create a crypto device instance
 *
 * Creates a crypto device interface wrapping a hardware crypto device.
 *
 * @param DeviceType    Device type
 * @param Vendor        Vendor ID
 * @param pHardwareBase Hardware base address or handle
 * @param ppDevice      Receives crypto device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
CryptoDeviceCreate(
    CRYPTO_DEVICE_TYPE DeviceType,
    CRYPTO_VENDOR Vendor,
    VOID *pHardwareBase,
    IIOCryptoDevice **ppDevice
    );

/**
 * @brief Create a TPM device instance
 *
 * Creates a TPM device interface wrapping a TPM hardware device.
 *
 * @param DeviceType    TPM type (1.2 or 2.0)
 * @param Vendor        TPM vendor
 * @param pHardwareBase Hardware base address
 * @param ppDevice      Receives TPM device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
TPMDeviceCreate(
    CRYPTO_DEVICE_TYPE DeviceType,
    CRYPTO_VENDOR Vendor,
    VOID *pHardwareBase,
    IIOTPMDevice **ppDevice
    );

/**
 * @brief Create an RNG device instance
 *
 * Creates an RNG device interface wrapping a hardware RNG.
 *
 * @param DeviceType    RNG type
 * @param Vendor        Vendor ID
 * @param pHardwareBase Hardware base address or handle
 * @param ppDevice      Receives RNG device interface
 *
 * @retval IO_SUCCESS           Device created successfully
 * @retval IO_NO_MEMORY         Insufficient memory
 * @retval IO_BAD_ARGUMENT      Invalid argument
 */
IO_RETURN
RNGDeviceCreate(
    CRYPTO_DEVICE_TYPE DeviceType,
    CRYPTO_VENDOR Vendor,
    VOID *pHardwareBase,
    IIORNGDevice **ppDevice
    );

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_CRYPTO_H */
