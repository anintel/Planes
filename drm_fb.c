// modules for lib drm
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h> // For ioctl
#include <sys/mman.h>  // For mmap

/*
    SUMMARY OF THE PROGRAM
    ----------------------
    1. We open the DRM device (display), normally card0 but here card1, with its permissions.
    2. We get the resources of that DRM fbs, CRTCs, Connectors & Encoders
    3. We select a available DRM Connector
    4. We create a dumb buffer
    5. Map the dumb buffer
    6. Setup a frame buffer for the dumb buffer so the DRM can use it
    7. GET & SET the CRTC Configuration
    8. Draw to the buffer
    9. Perform page flip
    10. Clena up.
*/

int main()
{
    // Open the DRM device
    const int drm_fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0)
    {
        perror("Failed to open DRM device");
        return -1;
    }

    // Query available resources
    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources)
    {
        perror("drmModeGetResources failed");
        close(drm_fd);
        return -1;
    }
    else
    {
        printf("Count of framebuffers: %d\n", resources->count_fbs);
        printf("Count of CRTCs: %d\n", resources->count_crtcs);
        printf("Count of connectors: %d\n", resources->count_connectors);
        printf("Count of encoders: %d\n", resources->count_encoders);
    }

    // Find an active connector
    drmModeConnector *connector = NULL;
    for (int i = 0; i < resources->count_connectors; i++)
    {
        connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
        {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector)
    {
        fprintf(stderr, "No active connector found.\n");
        drmModeFreeResources(resources);
        close(drm_fd);
        return -1;
    }

    // Create a dumb buffer
    struct drm_mode_create_dumb create_dumb = {0};
    create_dumb.width = connector->modes[0].hdisplay;
    create_dumb.height = connector->modes[0].vdisplay;
    create_dumb.bpp = 32;

    ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

    void *buffer_map;
    struct drm_mode_map_dumb map_dumb = {0};
    map_dumb.handle = create_dumb.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);

    buffer_map = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map_dumb.offset);

    int ret;
    uint32_t fb_id;

    // Set up a framebuffer for the dumb buffer
    ret = drmModeAddFB(drm_fd, create_dumb.width, create_dumb.height, 24, create_dumb.bpp, create_dumb.pitch, create_dumb.handle, &fb_id);
    if (ret)
    {
        fprintf(stderr, "Cannot create framebuffer (%d): %m\n", errno);
        return -1;
    }

    // Configure the CRTC
    drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, resources->crtcs[0]);
    if (!crtc)
    {
        fprintf(stderr, "Cannot get CRTC (%d): %m\n", errno);
        return -1;
    }

    ret = drmModeSetCrtc(drm_fd, crtc->crtc_id, fb_id, 0, 0, &connector->connector_id, 1, &connector->modes[0]);
    if (ret)
    {
        fprintf(stderr, "Cannot set CRTC for connector (%d): %m\n", errno);
        return -1;
    }

    uint32_t *pixels = (uint32_t *)buffer_map;
    uint32_t color = 0xFF0000FF; // Blue

    for (size_t i = 0; i < (create_dumb.size / sizeof(uint32_t)); i++)
    {
        pixels[i] = color;
    }

    // Page flip
    ret = drmModePageFlip(drm_fd, crtc->crtc_id, fb_id, DRM_MODE_PAGE_FLIP_EVENT, NULL);
    if (ret)
    {
        fprintf(stderr, "Cannot flip CRTC for connector (%d): %m\n", errno);
        return -1;
    }
    sleep(5);

    // Clean up
    munmap(buffer_map, create_dumb.size);
    drmModeRmFB(drm_fd, fb_id);
    drmModeFreeConnector(connector);
    drmModeFreeCrtc(crtc);
    drmModeFreeResources(resources);
    close(drm_fd);

    return 0;
}

// UP NEXT: Create a smaller buffer and overlay on the bigger one. creating two planes