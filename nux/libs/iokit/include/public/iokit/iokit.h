/**
 * @file iokit.h
 * @brief IOKit-inspired Driver Framework for NUX
 *
 * This header provides the main driver framework interface inspired by Apple's IOKit,
 * adapted for the NUX microkernel with COM-based architecture. The framework supports
 * both kernel-space and user-space drivers.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOKIT_H
#define IOKIT_H

#include <ananke/basetsd.h>
#include <ananke/guiddef.h>
#include <ananke/iunknown.h>
#include <ananke/retcode.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IOKit return status codes
 */
typedef enum _IO_RETURN {
    IO_SUCCESS              = 0x00000000,   /**< Operation succeeded */
    IO_ERROR                = 0xE0000001,   /**< General error */
    IO_NO_MEMORY           = 0xE0000002,   /**< Insufficient memory */
    IO_NO_RESOURCES        = 0xE0000003,   /**< Insufficient resources */
    IO_IPC_ERROR           = 0xE0000004,   /**< IPC communication error */
    IO_NO_DEVICE           = 0xE0000005,   /**< Device not found */
    IO_NOT_PRIVILEGED      = 0xE0000006,   /**< Insufficient privileges */
    IO_BAD_ARGUMENT        = 0xE0000007,   /**< Invalid argument */
    IO_LOCKED_READ         = 0xE0000008,   /**< Device locked for reading */
    IO_LOCKED_WRITE        = 0xE0000009,   /**< Device locked for writing */
    IO_EXCLUSIVE_ACCESS    = 0xE000000A,   /**< Exclusive access required */
    IO_BAD_MESSAGE_ID      = 0xE000000B,   /**< Invalid message ID */
    IO_UNSUPPORTED         = 0xE000000C,   /**< Operation not supported */
    IO_VM_ERROR            = 0xE000000D,   /**< Virtual memory error */
    IO_INTERNAL_ERROR      = 0xE000000E,   /**< Internal driver error */
    IO_IO_ERROR            = 0xE000000F,   /**< I/O operation error */
    IO_CANNOT_LOCK         = 0xE0000010,   /**< Cannot lock resource */
    IO_NOT_OPEN            = 0xE0000011,   /**< Device not open */
    IO_NOT_READABLE        = 0xE0000012,   /**< Device not readable */
    IO_NOT_WRITABLE        = 0xE0000013,   /**< Device not writable */
    IO_NOT_ALIGNED         = 0xE0000014,   /**< Data not aligned */
    IO_BAD_MEDIA           = 0xE0000015,   /**< Media error */
    IO_STILL_OPEN          = 0xE0000016,   /**< Device still open */
    IO_RLD_ERROR           = 0xE0000017,   /**< Runtime linker error */
    IO_DMA_ERROR           = 0xE0000018,   /**< DMA error */
    IO_BUSY                = 0xE0000019,   /**< Device busy */
    IO_TIMEOUT             = 0xE000001A,   /**< Operation timed out */
    IO_OFFLINE             = 0xE000001B,   /**< Device offline */
    IO_NOT_READY           = 0xE000001C,   /**< Device not ready */
    IO_NOT_ATTACHED        = 0xE000001D,   /**< Device not attached */
    IO_NO_CHANNELS         = 0xE000001E,   /**< No DMA channels */
    IO_NO_SPACE            = 0xE000001F,   /**< No space available */
    IO_PORT_EXISTS         = 0xE0000020,   /**< Port already exists */
    IO_CANNOT_WIRE         = 0xE0000021,   /**< Cannot wire memory */
    IO_NO_INTERRUPT        = 0xE0000022,   /**< No interrupt attached */
    IO_NO_FRAMES           = 0xE0000023,   /**< No DMA frames */
    IO_MESSAGE_TOO_LARGE   = 0xE0000024,   /**< Message too large */
    IO_NOT_PERMITTED       = 0xE0000025,   /**< Operation not permitted */
    IO_NO_POWER            = 0xE0000026,   /**< No power to device */
    IO_NO_MEDIA            = 0xE0000027,   /**< Media not present */
    IO_UNFORMATTED_MEDIA   = 0xE0000028,   /**< Media not formatted */
    IO_UNSUPPORTED_MODE    = 0xE0000029,   /**< Mode not supported */
    IO_UNDERRUN            = 0xE000002A,   /**< Data underrun */
    IO_OVERRUN             = 0xE000002B,   /**< Data overrun */
    IO_DEVICE_ERROR        = 0xE000002C,   /**< Device hardware error */
    IO_NO_COMPLETION       = 0xE000002D,   /**< No completion routine */
    IO_ABORTED             = 0xE000002E,   /**< Operation aborted */
    IO_NO_BANDWIDTH        = 0xE000002F,   /**< Insufficient bandwidth */
    IO_NOT_RESPONDING      = 0xE0000030,   /**< Device not responding */
    IO_IS_TERMINATED       = 0xE0000031,   /**< Service is terminated */
    IO_NO_MATCH            = 0xE0000032,   /**< No matching driver found */
} IO_RETURN;

/**
 * @brief IOKit option flags
 */
typedef enum _IO_OPTION_BITS {
    IO_OPTION_NONE              = 0x00000000,
    IO_OPTION_KERNEL_SPACE      = 0x00000001,   /**< Driver runs in kernel space */
    IO_OPTION_USER_SPACE        = 0x00000002,   /**< Driver runs in user space */
    IO_OPTION_EXCLUSIVE         = 0x00000004,   /**< Exclusive access required */
    IO_OPTION_SHARED            = 0x00000008,   /**< Shared access allowed */
    IO_OPTION_NON_BLOCKING      = 0x00000010,   /**< Non-blocking I/O */
    IO_OPTION_BLOCKING          = 0x00000020,   /**< Blocking I/O */
} IO_OPTION_BITS;

/**
 * @brief Forward declarations
 */
typedef struct IIOService IIOService;
typedef struct IIORegistry IIORegistry;
typedef struct IIOMatching IIOMatching;
typedef struct IIOUserClient IIOUserClient;
typedef struct IIOMemoryDescriptor IIOMemoryDescriptor;
typedef struct IIOWorkLoop IIOWorkLoop;
typedef struct IIOEventSource IIOEventSource;
typedef struct IO_PROPERTY IO_PROPERTY;

/**
 * @brief Property structure for device matching
 */
typedef struct _IO_PROPERTY {
    CONST CHAR8  *pszKey;       /**< Property key name */
    UINT32        uType;         /**< Property type */
    CONST VOID   *pValue;        /**< Property value */
    UINTN         cbSize;        /**< Size of value in bytes */
} IO_PROPERTY;

/**
 * @brief Property types
 */
typedef enum _IO_PROPERTY_TYPE {
    IO_PROPERTY_TYPE_BOOLEAN    = 0,
    IO_PROPERTY_TYPE_NUMBER     = 1,
    IO_PROPERTY_TYPE_STRING     = 2,
    IO_PROPERTY_TYPE_DATA       = 3,
    IO_PROPERTY_TYPE_ARRAY      = 4,
    IO_PROPERTY_TYPE_DICTIONARY = 5,
} IO_PROPERTY_TYPE;

/**
 * @brief Service state flags
 */
typedef enum _IO_SERVICE_STATE {
    IO_SERVICE_INACTIVE         = 0x00000001,   /**< Service is inactive */
    IO_SERVICE_REGISTERED       = 0x00000002,   /**< Service is registered */
    IO_SERVICE_MATCHED          = 0x00000004,   /**< Service is matched */
    IO_SERVICE_STARTED          = 0x00000008,   /**< Service is started */
    IO_SERVICE_BUSY             = 0x00000010,   /**< Service is busy */
    IO_SERVICE_TERMINATED       = 0x00000020,   /**< Service is terminated */
} IO_SERVICE_STATE;

#ifdef __cplusplus
}
#endif

#endif /* IOKIT_H */
