#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define DRM_DEVICE "/dev/dri/card1"
#define COLOR_RED 0xFFFF0000  // ARGB for Red
#define COLOR_BLUE 0xFF0000FF // ARGB for Blue

// function to create a dumb buffer.
void create_dumb_buffer(int drm_fd, struct drm_mode_create_dumb *create_dumb, void **buffer_map, uint32_t *fb_id)
{
    if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, create_dumb))
    {
        perror("DRM_IOCTL_MODE_CREATE_DUMB failed");
        exit(EXIT_FAILURE);
    }

    struct drm_mode_map_dumb map_dumb = {0};
    map_dumb.handle = create_dumb->handle;
    if (ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb))
    {
        perror("DRM_IOCTL_MODE_MAP_DUMB failed");
        exit(EXIT_FAILURE);
    }

    *buffer_map = mmap(0, create_dumb->size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map_dumb.offset);
    if (*buffer_map == MAP_FAILED)
    {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    if (drmModeAddFB(drm_fd, create_dumb->width, create_dumb->height, 24, create_dumb->bpp, create_dumb->pitch, create_dumb->handle, fb_id))
    {
        fprintf(stderr, "Cannot create framebuffer (%d): %m\n", errno);
        exit(EXIT_FAILURE);
    }
}

// function to fill a buffer with a color
void fill_buffer_with_color(uint32_t *pixels, size_t size, uint32_t color)
{
    for (size_t i = 0; i < size / sizeof(uint32_t); i++)
    {
        pixels[i] = color;
    }
}

// A function to get a available plane
drmModePlane *get_plane(int drm_fd, drmModeRes *resources, drmModePlaneRes *plane_res, uint32_t *used_plane_ids, int used_count, int possible_crtc)
{

    for (uint32_t i = 0; i < plane_res->count_planes; i++)
    {
        drmModePlane *plane = drmModeGetPlane(drm_fd, plane_res->planes[i]);

        if (plane && used_plane_ids[0] != plane->plane_id && resources->crtcs[plane->possible_crtcs >> 1] == possible_crtc)
            return plane;

        // if (plane && (plane->fb_id == 0))
        // { // Check if the plane can be used
        //     // Check if the plane is already used
        //     int already_used = 0;
        //     for (int j = 0; j < used_count; j++)
        //     {
        //         if (used_plane_ids[j] == plane->plane_id)
        //         {
        //             already_used = 1;
        //             break;
        //         }
        //     }
        //     if (!already_used)
        //     {
        //         return plane; // Found an unused plane
        //     }
        // }

        drmModeFreePlane(plane);
    }
    return NULL;
    // No available plane found
}

// NOT NEEDED FOR PROGRAM EXECUTION
// utility functions to printout the various details in Resources

// A function that will printout the frame properties
void print_plane_properties(int drm_fd, drmModePlane *plane)
{

    // Print basic plane information
    printf("------------------------\n");
    printf("Plane ID: %u\n", plane->plane_id);
    printf("Framebuffer ID: %u\n", plane->fb_id);
    printf("CRTC ID: %u\n", plane->crtc_id);
    printf("CRTC Position: (%d, %d)\n", plane->crtc_x, plane->crtc_y);

    printf("Possible CRTC ID: %d\n", plane->possible_crtcs);

    // // Print possible CRTC IDs
    // printf("Possible CRTC IDs: ");
    // for (int i = 0; i < 32; i++)
    // {
    //     if (plane->possible_crtcs & (1 << i))
    //     {
    //         printf("%u ", i);
    //     }
    // }
    // printf("\n");

    // Print formats
    printf("Number of formats: %u\n", plane->count_formats);
    if (plane->count_formats > 0)
    {
        printf("Formats: ");
        for (uint32_t i = 0; i < plane->count_formats; i++)
        {
            printf("0x%08X ", plane->formats[i]);
        }
        printf("\n");
    }

    // drmModeObjectProperties *props = drmModeObjectGetProperties(drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
    // if (!props)
    // {
    //     fprintf(stderr, "Could not get properties for plane %u\n", plane->plane_id);
    //     return;
    // }

    // printf("Properties for plane %u:\n", plane->plane_id);
    // for (uint32_t i = 0; i < props->count_props; i++)
    // {
    //     drmModePropertyRes *prop = drmModeGetProperty(drm_fd, props->props[i]);
    //     if (!prop)
    //     {
    //         fprintf(stderr, "Could not get property %u\n", props->props[i]);
    //         continue;
    //     }

    //     printf("  Property ID %u: %s\n", prop->prop_id, prop->name);
    //     drmModeFreeProperty(prop);
    // }

    // drmModeFreeObjectProperties(props);
}

// A function to print the CRTC info
void print_crtc_info(int drm_fd, drmModeRes *resources)
{
    for (int i = 0; i < resources->count_crtcs; ++i)
    {
        drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, resources->crtcs[i]);
        if (!crtc)
        {
            fprintf(stderr, "Cannot get CRTC %u\n", resources->crtcs[i]);
            continue;
        }

        printf("-----------------------------\n");
        printf("CRTC ID: %u\n", crtc->crtc_id);
        printf("Buffer ID: %u\n", crtc->buffer_id);
        printf("Position: (%u, %u)\n", crtc->x, crtc->y);
        printf("Size: (%u, %u)\n", crtc->width, crtc->height);
        printf("Mode Valid: %d\n", crtc->mode_valid);
        if (crtc->mode_valid)
        {
            printf("Mode:\n");
            printf("  Name: %s\n", crtc->mode.name);
            printf("  Clock: %u\n", crtc->mode.clock);
            printf("  Resolution: %ux%u\n", crtc->mode.hdisplay, crtc->mode.vdisplay);
            // Add more mode details if needed
        }

        drmModeFreeCrtc(crtc);
    }
}

// A function to print Connector info
void print_connector_info(int drm_fd, drmModeRes *resources)
{
    for (int i = 0; i < resources->count_connectors; i++)
    {
        drmModeConnector *connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector)
        {
            printf("-----------------------------\n");

            printf("Connector ID: %u\n", connector->connector_id);
            printf("Connection Status: %s\n", connector->connection == DRM_MODE_CONNECTED ? "Connected" : "Disconnected");

            if (connector->connection == DRM_MODE_CONNECTED)
            {
                // Check if this connector can be used with the current CRTC
                if (connector->encoder_id)
                {
                    printf("  Encoder ID: %u\n", connector->encoder_id);
                    for (int j = 0; j < resources->count_encoders; j++)
                    {
                        drmModeEncoder *encoder = drmModeGetEncoder(drm_fd, resources->encoders[j]);
                        if (encoder && encoder->encoder_id == connector->encoder_id)
                        {
                            printf("  Associated CRTC: %u\n", encoder->crtc_id);
                            // if (encoder->crtc_id == crtc2->crtc_id)
                            // {
                            //     printf("  CRTC %u is associated with this connector\n", crtc2->crtc_id);
                            // }
                            drmModeFreeEncoder(encoder);
                        }
                    }
                }
            }

            drmModeFreeConnector(connector);
        }
    }
}

// Function to print all DRM resources
void print_drm_resources(int drm_fd)
{
    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources)
    {
        perror("Failed to get DRM resources");
        return;
    }

    printf("============================================================\n");

    printf("Number of CRTCs: %d\n", resources->count_crtcs);
    printf("Number of Connectors: %d\n", resources->count_connectors);
    printf("Number of Encoders: %d\n", resources->count_encoders);
    printf("Number of Modes: %d\n", resources->count_fbs); // Framebuffers

    // Print CRTC information
    for (int i = 0; i < resources->count_crtcs; i++)
    {
        drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, resources->crtcs[i]);
        if (crtc)
        {
            printf("\nCRTC %d:\n", crtc->crtc_id);
            printf("  Buffer ID: %d\n", crtc->buffer_id);
            printf("  Position: (%d, %d)\n", crtc->x, crtc->y);
            printf("  Size: (%d, %d)\n", crtc->width, crtc->height);
            printf("  Mode Valid: %d\n", crtc->mode_valid);
            if (crtc->mode_valid)
            {
                printf("  Mode:\n");
                printf("    Name: %s\n", crtc->mode.name);
                printf("    Resolution: %dx%d\n", crtc->mode.hdisplay, crtc->mode.vdisplay);
            }
            drmModeFreeCrtc(crtc);
        }
    }

    // Print Connector information
    for (int i = 0; i < resources->count_connectors; i++)
    {
        drmModeConnector *connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector)
        {
            printf("\nConnector %d:\n", connector->connector_id);
            printf("  Connection Status: %s\n", connector->connection == DRM_MODE_CONNECTED ? "Connected" : "Disconnected");
            printf("  Number of Modes: %d\n", connector->count_modes);
            printf("  Encoder ID: %d\n", connector->encoder_id);
            drmModeFreeConnector(connector);
        }
    }

    // Print Encoder information
    for (int i = 0; i < resources->count_encoders; i++)
    {
        drmModeEncoder *encoder = drmModeGetEncoder(drm_fd, resources->encoders[i]);
        if (encoder)
        {
            printf("\nEncoder %d:\n", encoder->encoder_id);
            printf("  CRTC ID: %d\n", encoder->crtc_id);
            drmModeFreeEncoder(encoder);
        }
    }

    drmModeFreeResources(resources);
}

// Print the plane - CRTC Compatibility
void print_plane_crtc_compatibility(int drm_fd)
{
    drmModePlaneRes *plane_res = drmModeGetPlaneResources(drm_fd);
    if (!plane_res)
    {
        perror("drmModeGetPlaneResources failed");
        return;
    }

    for (uint32_t i = 0; i < plane_res->count_planes; i++)
    {
        drmModePlane *plane = drmModeGetPlane(drm_fd, plane_res->planes[i]);
        if (!plane)
        {
            fprintf(stderr, "Failed to get plane %u\n", plane_res->planes[i]);
            continue;
        }

        printf("--------------------------------\n");
        printf("Plane ID: %u\n", plane->plane_id);
        printf("Possible CRTCs: 0x%x\n", plane->possible_crtcs);

        for (int j = 0; j < 8; j++)
        {
            if (plane->possible_crtcs & (1 << j))
            {
                printf("  Can be associated with CRTC: %d\n", j);
            }
        }

        drmModeFreePlane(plane);
    }

    drmModeFreePlaneResources(plane_res);
}

// connector crtc compatibility
void print_connector_crtc_relationships(int drm_fd)
{
    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources)
    {
        perror("Failed to get resources");
        return;
    }

    // Loop through connectors
    for (int i = 0; i < resources->count_connectors; i++)
    {
        drmModeConnector *connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (!connector)
        {
            continue;
        }

        printf("--------------------------------\n");
        printf("Connector ID: %u\n", connector->connector_id);
        printf("  Connection Status: %s\n", connector->connection == DRM_MODE_CONNECTED ? "Connected" : "Disconnected");

        // Loop through encoders to find which CRTC they are associated with
        for (int j = 0; j < resources->count_encoders; j++)
        {
            drmModeEncoder *encoder = drmModeGetEncoder(drm_fd, resources->encoders[j]);
            if (encoder && encoder->encoder_id == connector->encoder_id)
            {
                printf("  Encoder ID: %u\n", encoder->encoder_id);
                printf("    Associated CRTC: %u\n", encoder->crtc_id);
                drmModeFreeEncoder(encoder);
                break; // We found the associated encoder
            }
            if (encoder)
            {
                drmModeFreeEncoder(encoder);
            }
        }

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);
}

int main()
{

    // get the drm file descriptor
    const int drm_fd = open(DRM_DEVICE, O_RDWR | O_CLOEXEC);
    if (drm_fd < 0)
    {
        perror("Failed to open DRM device");
        return EXIT_FAILURE;
    }

    drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

    // get the resources
    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources)
    {
        perror("drmModeGetResources failed");
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // getting connector 1
    drmModeConnector *connector1 = NULL;
    for (int i = 0; i < resources->count_connectors; i++)
    {
        connector1 = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector1 && connector1->connection == DRM_MODE_CONNECTED && connector1->count_modes > 0)
            break;
        drmModeFreeConnector(connector1);
        connector1 = NULL;
    }

    if (!connector1)
    {
        fprintf(stderr, "No active connector found.\n");
        drmModeFreeResources(resources);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // get CRTC 1
    drmModeCrtc *crtc1 = drmModeGetCrtc(drm_fd, resources->crtcs[0]);
    if (!crtc1)
    {
        fprintf(stderr, "Cannot get CRTC\n");
        drmModeFreeConnector(connector1);
        drmModeFreeResources(resources);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Create a dumb buffer for the first plane
    struct drm_mode_create_dumb create_dumb1 = {0};
    create_dumb1.width = connector1->modes[0].hdisplay;
    create_dumb1.height = connector1->modes[0].vdisplay;
    create_dumb1.bpp = 32;

    void *buffer_map1;
    uint32_t fb_id1;
    create_dumb_buffer(drm_fd, &create_dumb1, &buffer_map1, &fb_id1);
    fill_buffer_with_color((uint32_t *)buffer_map1, create_dumb1.size, COLOR_RED);

    // Create a dumb buffer for the second plane
    struct drm_mode_create_dumb create_dumb2 = {0};
    create_dumb2.width = connector1->modes[0].hdisplay / 2;
    create_dumb2.height = connector1->modes[0].vdisplay / 2;
    create_dumb2.bpp = 32;

    void *buffer_map2;
    uint32_t fb_id2;
    create_dumb_buffer(drm_fd, &create_dumb2, &buffer_map2, &fb_id2);
    fill_buffer_with_color((uint32_t *)buffer_map2, create_dumb2.size, COLOR_BLUE);

    // Get plane resources
    drmModePlaneRes *plane_res = drmModeGetPlaneResources(drm_fd);
    uint32_t used_plane_ids[2] = {0};
    int used_count = 0;

    // setting the plane 1
    drmModePlane *plane1 = get_plane(drm_fd, resources, plane_res, used_plane_ids, used_count, crtc1->crtc_id);
    if (plane1)
    {
        used_plane_ids[used_count++] = plane1->plane_id; // Mark as used
        drmModeSetPlane(drm_fd, plane1->plane_id, crtc1->crtc_id, fb_id1, 1, 0, 0,
                        create_dumb1.width, create_dumb1.height, 0, 0,
                        create_dumb1.width << 16, create_dumb1.height << 16);
    }
    else
    {
        fprintf(stderr, "No suitable plane found for the first framebuffer.\n");
    }

    // setting the plane 2
    drmModePlane *plane2 = get_plane(drm_fd, resources, plane_res, used_plane_ids, used_count, crtc1->crtc_id);
    if (plane2)
    {

        used_plane_ids[used_count++] = plane2->plane_id; // Mark as used
        drmModeSetPlane(drm_fd, plane2->plane_id, crtc1->crtc_id, fb_id2, 0, 100, 100,
                        create_dumb2.width, create_dumb2.height, 0, 0,
                        (create_dumb2.width << 16), (create_dumb2.height << 16));
    }
    else
    {
        fprintf(stderr, "No suitable plane found for the second framebuffer.\n");
    }

    // print_plane_crtc_compatibility(drm_fd);
    // print_crtc_info(drm_fd, resources);
    // print_drm_resources(drm_fd);
    // print_connector_crtc_relationships(drm_fd);
    // print_plane_properties(drm_fd, plane1);
    // print_plane_properties(drm_fd, plane2);

    char key;
    int x = 100;
    int y = 100;

    while (1)
    {
        key = getchar();
        printf("%c\n", key);

        if (key == 'q')
            break;

        switch (key)
        {
        case 'w':
            y -= 100;
            break;

        case 's':
            y += 100;
            break;

        case 'a':
            x -= 100;
            break;

        case 'd':
            x += 100;
            break;

        default:
            break;
        }

        drmModeSetPlane(drm_fd, plane2->plane_id, crtc1->crtc_id, fb_id2, 1, x, y,
                        create_dumb2.width, create_dumb2.height, 0, 0,
                        (create_dumb2.width << 16), (create_dumb2.height << 16));
    }

    // Cleanup
    if (plane1)
        drmModeFreePlane(plane1);
    if (plane2)
        drmModeFreePlane(plane2);

    munmap(buffer_map1, create_dumb1.size);
    munmap(buffer_map2, create_dumb2.size);
    drmModeRmFB(drm_fd, fb_id1);
    drmModeRmFB(drm_fd, fb_id2);
    drmModeFreePlaneResources(plane_res);
    drmModeFreeConnector(connector1);
    drmModeFreeCrtc(crtc1);
    drmModeFreeResources(resources);
    close(drm_fd);

    return EXIT_SUCCESS;
}