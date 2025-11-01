/*++
    Module Name:

        drm_api.c

    Abstract:

        Linux-compatible DRM C API wrapper.
        Provides libdrm-compatible functions on top of COM implementation.

--*/

#include <ananke/drm.h>
#include <string.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */
/*  Internal Handle Management                                     */
/* --------------------------------------------------------------- */

/* Maximum number of open DRM devices */
#define MAX_DRM_DEVICES 8

typedef struct {
    drm_fd fd;
    IDrmDevice *device;
    BOOLEAN inUse;
} DrmDeviceHandle;

static DrmDeviceHandle g_drmDevices[MAX_DRM_DEVICES];
static drm_fd g_nextFd = 1;

/* Get device from handle */
static IDrmDevice *GetDeviceFromFd(drm_fd fd) {
    for (INT32 i = 0; i < MAX_DRM_DEVICES; i++) {
        if (g_drmDevices[i].inUse && g_drmDevices[i].fd == fd) {
            return g_drmDevices[i].device;
        }
    }
    return NULL;
}

/* Allocate new handle */
static drm_fd AllocateHandle(IDrmDevice *device) {
    for (INT32 i = 0; i < MAX_DRM_DEVICES; i++) {
        if (!g_drmDevices[i].inUse) {
            g_drmDevices[i].fd = g_nextFd++;
            g_drmDevices[i].device = device;
            g_drmDevices[i].inUse = TRUE;
            return g_drmDevices[i].fd;
        }
    }
    return -1;
}

/* Free handle */
static VOID FreeHandle(drm_fd fd) {
    for (INT32 i = 0; i < MAX_DRM_DEVICES; i++) {
        if (g_drmDevices[i].inUse && g_drmDevices[i].fd == fd) {
            g_drmDevices[i].inUse = FALSE;
            g_drmDevices[i].device = NULL;
            return;
        }
    }
}

/* --------------------------------------------------------------- */
/*  Native DRM API Implementation                                  */
/* --------------------------------------------------------------- */

/**
 * Initialize DRM subsystem
 */
drm_fd drmOpen(const CHAR8 *name, const CHAR8 *busid) {
    IDrmDevice *device;
    drm_fd fd;

    /* Create DRM device */
    device = DrmCreateDevice();
    if (!device) {
        return -1;
    }

    /* Initialize device */
    if (FAILED(IDrmDevice_Initialize(device))) {
        IUnknown_Release((IUnknown*)device);
        return -1;
    }

    /* Allocate handle */
    fd = AllocateHandle(device);
    if (fd < 0) {
        IUnknown_Release((IUnknown*)device);
        return -1;
    }

    return fd;
}

/**
 * Close DRM device
 */
VOID drmClose(drm_fd fd) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    if (device) {
        IUnknown_Release((IUnknown*)device);
        FreeHandle(fd);
    }
}

/**
 * Get DRM mode resources
 */
drmModeRes *drmModeGetResources(drm_fd fd) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    drmModeRes *res;
    UINT32 count;
    UINT32 i;

    if (!device) {
        return NULL;
    }

    /* Get connector count */
    if (FAILED(IDrmDevice_GetConnectorCount(device, &count))) {
        return NULL;
    }

    /* Allocate resources structure */
    res = (drmModeRes*)malloc(sizeof(drmModeRes));
    if (!res) {
        return NULL;
    }

    memset(res, 0, sizeof(drmModeRes));

    /* Allocate connector array */
    res->count_connectors = count;
    if (count > 0) {
        res->connectors = (UINT32*)malloc(sizeof(UINT32) * count);
        if (!res->connectors) {
            free(res);
            return NULL;
        }

        /* Fill connector IDs */
        for (i = 0; i < count; i++) {
            res->connectors[i] = i;
        }
    }

    /* Allocate CRTC array (1 per connector for simplicity) */
    res->count_crtcs = count;
    if (count > 0) {
        res->crtcs = (UINT32*)malloc(sizeof(UINT32) * count);
        if (!res->crtcs) {
            free(res->connectors);
            free(res);
            return NULL;
        }

        for (i = 0; i < count; i++) {
            res->crtcs[i] = i;
        }
    }

    /* Set display limits (typical values) */
    res->min_width = 640;
    res->min_height = 480;
    res->max_width = 7680;
    res->max_height = 4320;

    return res;
}

/**
 * Free resources structure
 */
VOID drmModeFreeResources(drmModeRes *ptr) {
    if (!ptr) return;

    free(ptr->fbs);
    free(ptr->crtcs);
    free(ptr->connectors);
    free(ptr->encoders);
    free(ptr);
}

/**
 * Get connector information
 */
drmModeConnector *drmModeGetConnector(drm_fd fd, UINT32 connectorId) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    drmModeConnector *conn;
    DRM_CONNECTOR_INFO info;
    DRM_MODE modes[32];
    UINT32 numModes;
    UINT32 i;

    if (!device) {
        return NULL;
    }

    /* Get connector info from COM interface */
    if (FAILED(IDrmDevice_GetConnectorInfo(device, connectorId, &info))) {
        return NULL;
    }

    /* Allocate connector structure */
    conn = (drmModeConnector*)malloc(sizeof(drmModeConnector));
    if (!conn) {
        return NULL;
    }

    memset(conn, 0, sizeof(drmModeConnector));

    /* Fill connector info */
    conn->connector_id = info.ConnectorId;
    conn->connector_type = info.Type;
    conn->connection = (info.Status == DrmStatusConnected) ? 1 : 2;
    conn->mmWidth = info.PhysicalWidthMm;
    conn->mmHeight = info.PhysicalHeightMm;

    /* Get supported modes */
    if (SUCCEEDED(IDrmDevice_GetConnectorModes(device, connectorId, modes, 32, &numModes))) {
        conn->count_modes = numModes;
        if (numModes > 0) {
            conn->modes = (drmModeModeInfo*)malloc(sizeof(drmModeModeInfo) * numModes);
            if (conn->modes) {
                /* Convert DRM_MODE to drmModeModeInfo */
                for (i = 0; i < numModes; i++) {
                    memset(&conn->modes[i], 0, sizeof(drmModeModeInfo));
                    conn->modes[i].hdisplay = modes[i].Width;
                    conn->modes[i].vdisplay = modes[i].Height;
                    conn->modes[i].vrefresh = modes[i].RefreshRate;
                    conn->modes[i].flags = modes[i].Flags;
                    strncpy(conn->modes[i].name, modes[i].Name, 32);
                }
            }
        }
    }

    return conn;
}

/**
 * Free connector structure
 */
VOID drmModeFreeConnector(drmModeConnector *ptr) {
    if (!ptr) return;

    free(ptr->modes);
    free(ptr->props);
    free(ptr->prop_values);
    free(ptr->encoders);
    free(ptr);
}

/**
 * Get CRTC information
 */
drmModeCrtc *drmModeGetCrtc(drm_fd fd, UINT32 crtcId) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    drmModeCrtc *crtc;
    DRM_CRTC_INFO info;

    if (!device) {
        return NULL;
    }

    /* Get CRTC info from COM interface */
    if (FAILED(IDrmDevice_GetCrtcInfo(device, crtcId, &info))) {
        return NULL;
    }

    /* Allocate CRTC structure */
    crtc = (drmModeCrtc*)malloc(sizeof(drmModeCrtc));
    if (!crtc) {
        return NULL;
    }

    memset(crtc, 0, sizeof(drmModeCrtc));

    /* Fill CRTC info */
    crtc->crtc_id = info.CrtcId;
    crtc->buffer_id = info.FbId;
    crtc->x = info.X;
    crtc->y = info.Y;
    crtc->mode_valid = info.Active;

    return crtc;
}

/**
 * Free CRTC structure
 */
VOID drmModeFreeCrtc(drmModeCrtc *ptr) {
    if (!ptr) return;
    free(ptr);
}

/**
 * Set CRTC mode and framebuffer
 */
INT32 drmModeSetCrtc(drm_fd fd, UINT32 crtcId, UINT32 bufferId,
                     UINT32 x, UINT32 y, UINT32 *connectors, INT32 count,
                     drmModeModeInfo *mode) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    DRM_MODE drmMode;

    if (!device || !mode) {
        return -1;
    }

    /* Convert drmModeModeInfo to DRM_MODE */
    memset(&drmMode, 0, sizeof(DRM_MODE));
    drmMode.Width = mode->hdisplay;
    drmMode.Height = mode->vdisplay;
    drmMode.RefreshRate = mode->vrefresh;
    drmMode.BitsPerPixel = 32; /* Default to 32bpp */
    drmMode.Flags = mode->flags;
    strncpy(drmMode.Name, mode->name, 32);

    /* Set mode on the connector */
    if (count > 0 && connectors) {
        if (FAILED(IDrmDevice_SetMode(device, connectors[0], &drmMode))) {
            return -1;
        }
    }

    /* Display framebuffer */
    if (bufferId != 0) {
        if (FAILED(IDrmDevice_DisplayFramebuffer(device, crtcId, bufferId))) {
            return -1;
        }
    }

    return 0;
}

/**
 * Create framebuffer
 */
INT32 drmModeAddFB(drm_fd fd, UINT32 width, UINT32 height, UINT8 depth,
                   UINT8 bpp, UINT32 pitch, UINT32 bo_handle,
                   UINT32 *buf_id) {
    IDrmDevice *device = GetDeviceFromFd(fd);
    DRM_FRAMEBUFFER fb;

    if (!device || !buf_id) {
        return -1;
    }

    /* Create framebuffer */
    if (FAILED(IDrmDevice_CreateFramebuffer(device, width, height, bpp, &fb))) {
        return -1;
    }

    *buf_id = fb.FbId;
    return 0;
}

/**
 * Remove framebuffer
 */
INT32 drmModeRmFB(drm_fd fd, UINT32 bufferId) {
    IDrmDevice *device = GetDeviceFromFd(fd);

    if (!device) {
        return -1;
    }

    if (FAILED(IDrmDevice_DestroyFramebuffer(device, bufferId))) {
        return -1;
    }

    return 0;
}

/**
 * Get framebuffer information
 */
drmModeFB *drmModeGetFB(drm_fd fd, UINT32 bufferId) {
    /* Not implemented - would require storing framebuffer info in device */
    return NULL;
}

/**
 * Free framebuffer structure
 */
VOID drmModeFreeFB(drmModeFB *ptr) {
    if (!ptr) return;
    free(ptr);
}

/**
 * Page flip
 */
INT32 drmModePageFlip(drm_fd fd, UINT32 crtc_id, UINT32 fb_id,
                      UINT32 flags, VOID *user_data) {
    IDrmDevice *device = GetDeviceFromFd(fd);

    if (!device) {
        return -1;
    }

    /* Display framebuffer on CRTC */
    if (FAILED(IDrmDevice_DisplayFramebuffer(device, crtc_id, fb_id))) {
        return -1;
    }

    /* Wait for VBlank unless async */
    if (!(flags & DRM_MODE_PAGE_FLIP_ASYNC)) {
        IDrmDevice_WaitVBlank(device, crtc_id);
    }

    return 0;
}

/**
 * Wait for vertical blank
 */
INT32 drmWaitVBlank(drm_fd fd, UINT32 crtc_id) {
    IDrmDevice *device = GetDeviceFromFd(fd);

    if (!device) {
        return -1;
    }

    if (FAILED(IDrmDevice_WaitVBlank(device, crtc_id))) {
        return -1;
    }

    return 0;
}
