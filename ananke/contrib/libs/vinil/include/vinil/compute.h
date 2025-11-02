/*
** ==========================================================================
**
** VINIL Compute Extensions
**
** Support for OpenCL, CUDA, HIP compute workloads
**
** --------------------------------------------------------------------------
**
** Vincent 3D Rendering Library, Programmable Pipeline Edition
**
** Copyright (C) 2003-2007 Hans-Martin Will.
** Copyright (C) 2025 NUX Project
**
** @CDDL_HEADER_START@
**
** The contents of this file are subject to the terms of the
** Common Development and Distribution License, Version 1.0 only
** (the "License").  You may not use this file except in compliance
** with the License.
**
** You can obtain a copy of the license at
** http://www.vincent3d.com/software/ogles2/license/license.html
** See the License for the specific language governing permissions
** and limitations under the License.
**
** When distributing Covered Code, include this CDDL_HEADER in each
** file and include the License file named LICENSE.TXT in the root folder
** of your distribution.
** If applicable, add the following below this CDDL_HEADER, with the
** fields enclosed by brackets "[]" replaced with your own identifying
** information: Portions Copyright [yyyy] [name of copyright owner]
**
** @CDDL_HEADER_END@
**
** ==========================================================================
*/

#ifndef VINIL_COMPUTE_H
#define VINIL_COMPUTE_H 1

#include <vinil/vinil.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** ==========================================================================
** COMPUTE KERNEL LAUNCH
** ==========================================================================
*/

/**
 * Launch a compute kernel with specified work dimensions
 *
 * @param ctx Execution context
 * @param executable Compiled kernel
 * @param work_dim Number of dimensions (1, 2, or 3)
 * @param global_work_size Global work size for each dimension
 * @param local_work_size Local work-group size for each dimension (can be NULL for auto)
 * @param global_work_offset Global work offset for each dimension (can be NULL)
 * @param num_args Number of kernel arguments
 * @param args Array of kernel argument pointers
 * @param arg_sizes Array of argument sizes in bytes
 * @return Error code
 */
vinil_error vinil_launch_kernel(
    vinil_context* ctx,
    vinil_executable* executable,
    vinil_uint32 work_dim,
    const vinil_size* global_work_size,
    const vinil_size* local_work_size,
    const vinil_size* global_work_offset,
    vinil_uint32 num_args,
    void** args,
    vinil_size* arg_sizes
);

/*
** ==========================================================================
** MEMORY MANAGEMENT (Compute)
** ==========================================================================
*/

typedef enum vinil_mem_flags {
    VINIL_MEM_READ_WRITE    = 0x01,
    VINIL_MEM_READ_ONLY     = 0x02,
    VINIL_MEM_WRITE_ONLY    = 0x04,
    VINIL_MEM_USE_HOST_PTR  = 0x08,
    VINIL_MEM_ALLOC_HOST_PTR = 0x10,
    VINIL_MEM_COPY_HOST_PTR = 0x20,
} vinil_mem_flags;

typedef struct vinil_buffer vinil_buffer;

/**
 * Create a memory buffer for compute operations
 *
 * @param ctx Execution context
 * @param flags Memory flags
 * @param size Buffer size in bytes
 * @param host_ptr Host pointer (optional, depends on flags)
 * @return Buffer handle, or NULL on failure
 */
vinil_buffer* vinil_buffer_create(
    vinil_context* ctx,
    vinil_mem_flags flags,
    vinil_size size,
    void* host_ptr
);

/**
 * Destroy a memory buffer
 *
 * @param buffer Buffer to destroy
 */
void vinil_buffer_destroy(vinil_buffer* buffer);

/**
 * Read data from a buffer
 *
 * @param ctx Execution context
 * @param buffer Source buffer
 * @param blocking Block until complete
 * @param offset Offset in buffer
 * @param size Number of bytes to read
 * @param ptr Destination host pointer
 * @return Error code
 */
vinil_error vinil_buffer_read(
    vinil_context* ctx,
    vinil_buffer* buffer,
    vinil_bool blocking,
    vinil_size offset,
    vinil_size size,
    void* ptr
);

/**
 * Write data to a buffer
 *
 * @param ctx Execution context
 * @param buffer Destination buffer
 * @param blocking Block until complete
 * @param offset Offset in buffer
 * @param size Number of bytes to write
 * @param ptr Source host pointer
 * @return Error code
 */
vinil_error vinil_buffer_write(
    vinil_context* ctx,
    vinil_buffer* buffer,
    vinil_bool blocking,
    vinil_size offset,
    vinil_size size,
    const void* ptr
);

/*
** ==========================================================================
** SYNCHRONIZATION
** ==========================================================================
*/

/**
 * Wait for all operations in the context to complete
 *
 * @param ctx Execution context
 * @return Error code
 */
vinil_error vinil_finish(vinil_context* ctx);

/**
 * Flush all queued operations
 *
 * @param ctx Execution context
 * @return Error code
 */
vinil_error vinil_flush(vinil_context* ctx);

/*
** ==========================================================================
** DEVICE INFORMATION
** ==========================================================================
*/

typedef enum vinil_device_info {
    VINIL_DEVICE_NAME,
    VINIL_DEVICE_VENDOR,
    VINIL_DEVICE_VERSION,
    VINIL_DEVICE_MAX_COMPUTE_UNITS,
    VINIL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
    VINIL_DEVICE_MAX_WORK_GROUP_SIZE,
    VINIL_DEVICE_MAX_WORK_ITEM_SIZES,
    VINIL_DEVICE_LOCAL_MEM_SIZE,
    VINIL_DEVICE_GLOBAL_MEM_SIZE,
} vinil_device_info;

/**
 * Get device information
 *
 * @param ctx Execution context
 * @param param_name Information parameter
 * @param param_value_size Size of output buffer
 * @param param_value Output buffer
 * @param param_value_size_ret Actual size returned
 * @return Error code
 */
vinil_error vinil_get_device_info(
    vinil_context* ctx,
    vinil_device_info param_name,
    vinil_size param_value_size,
    void* param_value,
    vinil_size* param_value_size_ret
);

#ifdef __cplusplus
}
#endif

#endif /* VINIL_COMPUTE_H */
