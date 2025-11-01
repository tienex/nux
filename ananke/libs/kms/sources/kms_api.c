/*++
    Module Name:

        kms_api.c

    Abstract:

        Linux-compatible KMS C API wrapper.
        Provides KMS plane and atomic operation functions on top of COM implementation.

--*/

#include <ananke/kms.h>
#include <ananke/drm.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */
/*  Internal Handle Management                                     */
/* --------------------------------------------------------------- */

/* Maximum number of KMS devices (matches DRM devices) */
#define MAX_KMS_DEVICES 8

typedef struct {
    drm_fd fd;
    IKmsDevice *device;
    BOOLEAN inUse;
} KmsDeviceHandle;

static KmsDeviceHandle g_kmsDevices[MAX_KMS_DEVICES];

/* Get KMS device from DRM handle */
static IKmsDevice *GetKmsDeviceFromFd(drm_fd fd) {
    IKmsDevice *kmsDevice;
    IDrmDevice *drmDevice;

    /* Try to find existing KMS device */
    for (INT32 i = 0; i < MAX_KMS_DEVICES; i++) {
        if (g_kmsDevices[i].inUse && g_kmsDevices[i].fd == fd) {
            return g_kmsDevices[i].device;
        }
    }

    /* Create new KMS device if not found */
    /* Get DRM device from external DRM API */
    extern IDrmDevice *GetDeviceFromFd(drm_fd fd);
    drmDevice = GetDeviceFromFd(fd);
    if (!drmDevice) {
        return NULL;
    }

    /* Create KMS device */
    kmsDevice = KmsCreateDevice();
    if (!kmsDevice) {
        return NULL;
    }

    /* Initialize with DRM device */
    if (FAILED(IKmsDevice_Initialize(kmsDevice, drmDevice))) {
        IUnknown_Release((IUnknown*)kmsDevice);
        return NULL;
    }

    /* Store device handle */
    for (INT32 i = 0; i < MAX_KMS_DEVICES; i++) {
        if (!g_kmsDevices[i].inUse) {
            g_kmsDevices[i].fd = fd;
            g_kmsDevices[i].device = kmsDevice;
            g_kmsDevices[i].inUse = TRUE;
            return kmsDevice;
        }
    }

    /* No slots available */
    IUnknown_Release((IUnknown*)kmsDevice);
    return NULL;
}

/* --------------------------------------------------------------- */
/*  Native KMS API Implementation                                  */
/* --------------------------------------------------------------- */

/**
 * Get plane resources
 */
INT32 kmsGetPlaneResources(drm_fd fd, UINT32 *planeIds, UINT32 maxPlanes) {
    IKmsDevice *device = GetKmsDeviceFromFd(fd);
    UINT32 count;
    UINT32 i;

    if (!device || !planeIds || maxPlanes == 0) {
        return -1;
    }

    /* Get plane count */
    if (FAILED(IKmsDevice_GetPlaneCount(device, &count))) {
        return -1;
    }

    /* Fill plane IDs */
    for (i = 0; i < count && i < maxPlanes; i++) {
        planeIds[i] = i;
    }

    return (count < maxPlanes) ? count : maxPlanes;
}

/**
 * Get plane information
 */
INT32 kmsGetPlaneInfo(drm_fd fd, UINT32 planeId, kms_plane_info *info) {
    IKmsDevice *device = GetKmsDeviceFromFd(fd);
    KMS_PLANE_DESC desc;

    if (!device || !info) {
        return -1;
    }

    /* Get plane description from COM interface */
    if (FAILED(IKmsDevice_GetPlaneDesc(device, planeId, &desc))) {
        return -1;
    }

    /* Convert to native structure */
    info->plane_id = desc.PlaneId;
    info->possible_crtcs = desc.PossibleCrtcs;
    info->plane_type = desc.Type;
    info->zpos = desc.ZPos;

    return 0;
}

/**
 * Set plane configuration
 */
INT32 kmsSetPlane(drm_fd fd, UINT32 planeId, UINT32 crtcId, UINT32 fbId,
                  UINT32 flags, INT32 crtcX, INT32 crtcY, UINT32 crtcW, UINT32 crtcH,
                  INT32 srcX, INT32 srcY, UINT32 srcW, UINT32 srcH) {
    IKmsDevice *device = GetKmsDeviceFromFd(fd);

    if (!device) {
        return -1;
    }

    /* Call SetPlane on COM interface */
    if (FAILED(IKmsDevice_SetPlane(device, planeId, crtcId, fbId, flags,
                                    crtcX, crtcY, crtcW, crtcH,
                                    srcX, srcY, srcW, srcH))) {
        return -1;
    }

    return 0;
}

/**
 * Disable plane
 */
INT32 kmsDisablePlane(drm_fd fd, UINT32 planeId) {
    IKmsDevice *device = GetKmsDeviceFromFd(fd);

    if (!device) {
        return -1;
    }

    if (FAILED(IKmsDevice_DisablePlane(device, planeId))) {
        return -1;
    }

    return 0;
}

/* --------------------------------------------------------------- */
/*  Atomic API Implementation                                      */
/* --------------------------------------------------------------- */

#define MAX_ATOMIC_PROPERTIES 64

struct kms_atomic_req {
    KMS_ATOMIC_STATE states[MAX_ATOMIC_PROPERTIES];
    UINT32 count;
};

/**
 * Allocate atomic request
 */
kms_atomic_req *kmsAtomicAlloc(VOID) {
    kms_atomic_req *req = (kms_atomic_req*)malloc(sizeof(kms_atomic_req));
    if (req) {
        memset(req, 0, sizeof(kms_atomic_req));
    }
    return req;
}

/**
 * Free atomic request
 */
VOID kmsAtomicFree(kms_atomic_req *req) {
    if (req) {
        free(req);
    }
}

/**
 * Add property to atomic request
 *
 * Note: This is a simplified implementation. Full implementation would
 * need property ID to name mapping and proper state accumulation.
 * For now, we treat each property as a complete plane state.
 */
INT32 kmsAtomicAddProperty(kms_atomic_req *req, UINT32 object_id,
                            UINT32 property_id, UINT64 value) {
    if (!req || req->count >= MAX_ATOMIC_PROPERTIES) {
        return -1;
    }

    /* This is simplified - would need proper property handling */
    /* For now, just increment count to track changes */
    req->count++;

    return 0;
}

/**
 * Commit atomic request
 */
INT32 kmsAtomicCommit(drm_fd fd, kms_atomic_req *req, UINT32 flags, VOID *user_data) {
    IKmsDevice *device = GetKmsDeviceFromFd(fd);
    UINT32 kmsFlags = 0;

    if (!device || !req) {
        return -1;
    }

    /* Convert flags */
    if (flags & DRM_MODE_ATOMIC_NONBLOCK) {
        kmsFlags |= KMS_ATOMIC_NONBLOCK;
    }
    if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET) {
        kmsFlags |= KMS_ATOMIC_ALLOW_MODESET;
    }

    /* Test only? Return success without committing */
    if (flags & DRM_MODE_ATOMIC_TEST_ONLY) {
        return 0;
    }

    /* Commit atomic state if we have any states */
    if (req->count > 0) {
        if (FAILED(IKmsDevice_AtomicCommit(device, req->states, req->count, kmsFlags))) {
            return -1;
        }
    }

    return 0;
}
