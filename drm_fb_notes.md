# DRM Code Explanation

## Opening the DRM Device
- Opens the DRM device node `/dev/dri/card1` for read and write access.
- `O_CLOEXEC` ensures that the file descriptor is closed on execution of a new program.

## Querying DRM Resources
- Uses `drmModeGetResources` to query available DRM resources such as framebuffers, CRTCs (Cathode Ray Tube Controllers), connectors, and encoders.

## Finding an Active Connector
- Iterates through available connectors to find one that is connected and has at least one valid mode.
- Frees the connector if it's not suitable.

## Creating a Dumb Buffer
- A dumb buffer is a simple block of memory used to store pixel data.
- Uses `DRM_IOCTL_MODE_CREATE_DUMB` to create the buffer.
- Maps the buffer to user space memory using `DRM_IOCTL_MODE_MAP_DUMB` and `mmap`.

## Setting Up the Framebuffer
- Adds a framebuffer using `drmModeAddFB`, which provides the necessary interface for DRM to manage and display the pixel data from the dumb buffer.

## Configuring the CRTC
- Gets the CRTC using `drmModeGetCrtc`.
- Configures the CRTC to use the framebuffer with `drmModeSetCrtc`.

## Drawing to the Buffer
- Writes a color (blue) to the dumb buffer to be displayed.
- Updates the entire buffer to the specified color.

## Page Flip
- Initiates a page flip using `drmModePageFlip` to display the framebuffer content.
- `DRM_MODE_PAGE_FLIP_EVENT` schedules the flip to occur on the next vertical blanking interval.
- `sleep(5)` keeps the display active for 5 seconds.

## Cleanup
- Unmaps the buffer, removes the framebuffer, and frees the DRM resources.
- Closes the file descriptor for the DRM device.

# Comments from the Original Code

## Open the DRM Device
-  open the drm device`
-  O_CLOEXEC -> close one execution.`
-  typically card0 points to the primary graphics card`
-  but in systems with external & multiple graphics we can also see card1, card2 and so on.`

## Query Available Resources
-  we use the drm uAPI to query for the available resources`
-  drmModeRes is a structure in libdrm which typically includes a list of`
-  available framebuffers and what not.`

## Find an Active Connector
-  So the drmModeConnector is basically a display device and all the different`
-  details about that device.`
-  drmModeConnector represents a specific type of resource within a DRM device`

## Creating a Dumb Buffer
-  creating a dumb buffer`
-  dumb buffer - a really simple frame buffer (just a block of memory)`
-  it lacks any advanced features thus the name dumb`

## Mapping the Buffer
-  drm_mode_map_dumb is used to prepare a dumb_buffer for memory mapping`
-  here mapping means making the memory of the dumb buffer accessible to the application`
-  as if it was part of the program's own memory - mmap is used for this.`

## Set Up Framebuffer
-  Set up a framebuffer for the dumb buffer`
-  But why do we need a Frame buffer while we already created a Dumb buffer?`
-  The DRM does not know how to display a dumb buffer, it needs a frame buffer to`
-  interpret how to display the buffer's contents.`

## Configure the CRTC
-  CRTC - controls the timing of the display hardware`
-  responsible for sending pixel data from framebuffer to display output`
-  TODO : Understand how crtc is set and what happens when it is not set`

## Page Flip
-  Page flip`
-  The page flip aka double buffering/buffer switching, is the process of switching the display from showing one`
-  buffer to another. Generally there are two buffers the front buffer and the back buffer.`
-  front -> the one that is currently being displayed on the screen`
-  back -> the one that is being rendered by the GPU to be shown next.`
-  The front and back buffers are switched for each refresh cycle.`

## Cleanup
-  Clean up`
-  Unmap the buffer
-  TODO : use a while loop or try sleep for prolonged display output
