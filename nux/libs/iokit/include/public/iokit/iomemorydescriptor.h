/**
 * @file iomemorydescriptor.h
 * @brief IOMemoryDescriptor Interface - I/O memory management
 *
 * IIOMemoryDescriptor provides an abstract representation of memory for I/O operations.
 * It handles physical and virtual memory mappings, DMA operations, and memory preparation
 * for device access.
 *
 * @copyright Copyright (c) 2025 NUX Project
 */

#ifndef IOMEMORYDESCRIPTOR_H
#define IOMEMORYDESCRIPTOR_H

#include <iokit/iokit.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IIOMemoryDescriptor interface GUID
 * {4E7F8D9C-5A6B-4C8E-9F3D-7E5C8B6A9D4F}
 */
DEFINE_GUID(IID_IIOMemoryDescriptor,
    0x4E7F8D9C, 0x5A6B, 0x4C8E, 0x9F, 0x3D, 0x7E, 0x5C, 0x8B, 0x6A, 0x9D, 0x4F);

/**
 * @brief Memory descriptor direction flags
 */
typedef enum _IO_DIRECTION {
    IO_DIRECTION_NONE       = 0x00000000,   /**< No direction specified */
    IO_DIRECTION_IN         = 0x00000001,   /**< DMA from device to memory */
    IO_DIRECTION_OUT        = 0x00000002,   /**< DMA from memory to device */
    IO_DIRECTION_IN_OUT     = 0x00000003,   /**< Bidirectional DMA */
} IO_DIRECTION;

/**
 * @brief Memory descriptor options
 */
typedef enum _IO_MEMORY_OPTIONS {
    IO_MEMORY_PHYSICAL              = 0x00000001,   /**< Physical memory range */
    IO_MEMORY_VIRTUAL               = 0x00000002,   /**< Virtual memory range */
    IO_MEMORY_KERNEL                = 0x00000004,   /**< Kernel memory */
    IO_MEMORY_USER                  = 0x00000008,   /**< User memory */
    IO_MEMORY_PERSISTENT            = 0x00000010,   /**< Persistent mapping */
    IO_MEMORY_PAGEABLE              = 0x00000020,   /**< Pageable memory */
    IO_MEMORY_NON_CACHEABLE         = 0x00000040,   /**< Non-cacheable */
    IO_MEMORY_WRITE_COMBINE         = 0x00000080,   /**< Write-combining */
    IO_MEMORY_WRITE_THROUGH         = 0x00000100,   /**< Write-through caching */
} IO_MEMORY_OPTIONS;

/**
 * @brief Physical segment descriptor for scatter-gather DMA
 */
typedef struct _IO_DMA_SEGMENT {
    UINT64  PhysicalAddress;        /**< Physical address of segment */
    UINT64  Length;                 /**< Length of segment in bytes */
} IO_DMA_SEGMENT;

/**
 * @brief IIOMemoryDescriptor - I/O memory management interface
 *
 * This interface provides methods for managing memory regions used in I/O operations,
 * including DMA preparation, physical/virtual address translation, and memory mapping.
 */
#undef INTERFACE
#define INTERFACE IIOMemoryDescriptor

DECLARE_INTERFACE_(IIOMemoryDescriptor, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void **ppvObject
        ) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IIOMemoryDescriptor methods

    /**
     * @brief Create memory descriptor from address range
     *
     * Creates a memory descriptor for the specified address range.
     *
     * @param pAddress      Starting address (virtual or physical)
     * @param cbLength      Length of memory region in bytes
     * @param Direction     Data transfer direction
     * @param uOptions      Memory options (physical, virtual, kernel, user, etc.)
     *
     * @retval IO_SUCCESS           Descriptor created successfully
     * @retval IO_BAD_ARGUMENT      Invalid address or length
     * @retval IO_NO_MEMORY         Insufficient memory
     */
    STDMETHOD_(IO_RETURN, InitWithAddressRange)(THIS_
        VOID *pAddress,
        UINT64 cbLength,
        IO_DIRECTION Direction,
        UINT32 uOptions
        ) PURE;

    /**
     * @brief Create memory descriptor from physical ranges
     *
     * Creates a memory descriptor from an array of physical address ranges.
     *
     * @param pRanges       Array of physical address ranges
     * @param uRangeCount   Number of ranges
     * @param Direction     Data transfer direction
     * @param uOptions      Memory options
     *
     * @retval IO_SUCCESS           Descriptor created successfully
     * @retval IO_BAD_ARGUMENT      Invalid ranges
     * @retval IO_NO_MEMORY         Insufficient memory
     */
    STDMETHOD_(IO_RETURN, InitWithPhysicalRanges)(THIS_
        CONST IO_DMA_SEGMENT *pRanges,
        UINT32 uRangeCount,
        IO_DIRECTION Direction,
        UINT32 uOptions
        ) PURE;

    /**
     * @brief Prepare memory for I/O
     *
     * Prepares the memory for I/O operations by ensuring it is wired down,
     * cache-coherent, and generating the physical scatter-gather list.
     *
     * @param Direction     Expected data transfer direction
     *
     * @retval IO_SUCCESS           Memory prepared successfully
     * @retval IO_CANNOT_WIRE       Cannot wire memory pages
     * @retval IO_VM_ERROR          Virtual memory error
     */
    STDMETHOD_(IO_RETURN, Prepare)(THIS_
        IO_DIRECTION Direction
        ) PURE;

    /**
     * @brief Complete I/O operation
     *
     * Marks the I/O operation as complete and releases any temporary mappings
     * or locks established during Prepare().
     *
     * @param Direction     Data transfer direction that was used
     *
     * @retval IO_SUCCESS           Completion successful
     */
    STDMETHOD_(IO_RETURN, Complete)(THIS_
        IO_DIRECTION Direction
        ) PURE;

    /**
     * @brief Get physical segments
     *
     * Retrieves the physical memory segments (scatter-gather list) for DMA.
     *
     * @param ullOffset     Offset into the memory descriptor
     * @param pSegments     Array to receive physical segments
     * @param puSegmentCount On input: max segments; On output: actual count
     *
     * @retval IO_SUCCESS           Segments retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid offset or buffer
     * @retval IO_NOT_READY         Memory not prepared
     */
    STDMETHOD_(IO_RETURN, GetPhysicalSegments)(THIS_
        UINT64 ullOffset,
        IO_DMA_SEGMENT *pSegments,
        UINT32 *puSegmentCount
        ) PURE;

    /**
     * @brief Read bytes from memory descriptor
     *
     * Reads data from the memory region described by this descriptor.
     *
     * @param ullOffset     Offset into the memory descriptor
     * @param pBuffer       Buffer to receive data
     * @param cbLength      Number of bytes to read
     *
     * @retval IO_SUCCESS           Data read successfully
     * @retval IO_BAD_ARGUMENT      Invalid offset or length
     * @retval IO_NOT_READABLE      Memory not readable
     */
    STDMETHOD_(IO_RETURN, ReadBytes)(THIS_
        UINT64 ullOffset,
        VOID *pBuffer,
        UINT64 cbLength
        ) PURE;

    /**
     * @brief Write bytes to memory descriptor
     *
     * Writes data to the memory region described by this descriptor.
     *
     * @param ullOffset     Offset into the memory descriptor
     * @param pBuffer       Buffer containing data to write
     * @param cbLength      Number of bytes to write
     *
     * @retval IO_SUCCESS           Data written successfully
     * @retval IO_BAD_ARGUMENT      Invalid offset or length
     * @retval IO_NOT_WRITABLE      Memory not writable
     */
    STDMETHOD_(IO_RETURN, WriteBytes)(THIS_
        UINT64 ullOffset,
        CONST VOID *pBuffer,
        UINT64 cbLength
        ) PURE;

    /**
     * @brief Map memory into address space
     *
     * Creates a virtual memory mapping for this memory descriptor.
     *
     * @param uOptions      Mapping options (kernel, user, caching attributes)
     * @param ppAddress     Receives mapped virtual address
     * @param pcbLength     Receives length of mapped region
     *
     * @retval IO_SUCCESS           Memory mapped successfully
     * @retval IO_VM_ERROR          Virtual memory error
     * @retval IO_NO_RESOURCES      Insufficient address space
     */
    STDMETHOD_(IO_RETURN, Map)(THIS_
        UINT32 uOptions,
        VOID **ppAddress,
        UINT64 *pcbLength
        ) PURE;

    /**
     * @brief Unmap memory
     *
     * Removes a virtual memory mapping created by Map().
     *
     * @param pAddress      Virtual address to unmap
     *
     * @retval IO_SUCCESS           Memory unmapped successfully
     * @retval IO_BAD_ARGUMENT      Invalid address
     */
    STDMETHOD_(IO_RETURN, Unmap)(THIS_
        VOID *pAddress
        ) PURE;

    /**
     * @brief Get memory descriptor length
     *
     * Returns the total length of the memory region described.
     *
     * @param pullLength    Receives the length in bytes
     *
     * @retval IO_SUCCESS           Length retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetLength)(THIS_
        UINT64 *pullLength
        ) PURE;

    /**
     * @brief Get memory descriptor direction
     *
     * Returns the data transfer direction for this descriptor.
     *
     * @param pDirection    Receives the direction
     *
     * @retval IO_SUCCESS           Direction retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetDirection)(THIS_
        IO_DIRECTION *pDirection
        ) PURE;

    /**
     * @brief Get memory descriptor options
     *
     * Returns the options flags for this descriptor.
     *
     * @param puOptions     Receives the options flags
     *
     * @retval IO_SUCCESS           Options retrieved successfully
     * @retval IO_BAD_ARGUMENT      Invalid argument
     */
    STDMETHOD_(IO_RETURN, GetOptions)(THIS_
        UINT32 *puOptions
        ) PURE;
};

#undef INTERFACE

/**
 * @brief Convenience macros for calling IIOMemoryDescriptor methods
 */
#if !defined(__cplusplus) || defined(CINTERFACE)

#define IIOMemoryDescriptor_QueryInterface(p,a,b)               (p)->lpVtbl->QueryInterface(p,a,b)
#define IIOMemoryDescriptor_AddRef(p)                            (p)->lpVtbl->AddRef(p)
#define IIOMemoryDescriptor_Release(p)                           (p)->lpVtbl->Release(p)
#define IIOMemoryDescriptor_InitWithAddressRange(p,a,b,c,d)     (p)->lpVtbl->InitWithAddressRange(p,a,b,c,d)
#define IIOMemoryDescriptor_InitWithPhysicalRanges(p,a,b,c,d)   (p)->lpVtbl->InitWithPhysicalRanges(p,a,b,c,d)
#define IIOMemoryDescriptor_Prepare(p,a)                         (p)->lpVtbl->Prepare(p,a)
#define IIOMemoryDescriptor_Complete(p,a)                        (p)->lpVtbl->Complete(p,a)
#define IIOMemoryDescriptor_GetPhysicalSegments(p,a,b,c)        (p)->lpVtbl->GetPhysicalSegments(p,a,b,c)
#define IIOMemoryDescriptor_ReadBytes(p,a,b,c)                  (p)->lpVtbl->ReadBytes(p,a,b,c)
#define IIOMemoryDescriptor_WriteBytes(p,a,b,c)                 (p)->lpVtbl->WriteBytes(p,a,b,c)
#define IIOMemoryDescriptor_Map(p,a,b,c)                        (p)->lpVtbl->Map(p,a,b,c)
#define IIOMemoryDescriptor_Unmap(p,a)                          (p)->lpVtbl->Unmap(p,a)
#define IIOMemoryDescriptor_GetLength(p,a)                      (p)->lpVtbl->GetLength(p,a)
#define IIOMemoryDescriptor_GetDirection(p,a)                   (p)->lpVtbl->GetDirection(p,a)
#define IIOMemoryDescriptor_GetOptions(p,a)                     (p)->lpVtbl->GetOptions(p,a)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOMEMORYDESCRIPTOR_H */
