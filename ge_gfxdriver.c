/*
 * This file is based on the GLES2 driver from DirectFB2
 */

#include <core/graphics_driver.h>
#include <core/system.h>
#include <systems/drmkms/drmkms_system.h>
#include <misc/conf.h>

#include "ge_gfxdriver.h"

D_DEBUG_DOMAIN( GE_Driver, "GE/Driver", "MStar/Sigmastar GE Driver" );

DFB_GRAPHICS_DRIVER( ge )

/**********************************************************************************************************************/

extern const GraphicsDeviceFuncs geGraphicsDeviceFuncs;

/**********************************************************************************************************************/

static int
driver_probe()
{
     DRMKMSData       *drmkms = dfb_system_data();

     printf("%s()\n", __FUNCTION__ );

     if (!drmkms->shared->use_prime_fd) {
          D_INFO( "GE/driver: \"drmkms-use-prime-fd\" not enabled, not possible to use GE\n");
          return 0;
     }

     return 1;
}

static void
driver_get_info( GraphicsDriverInfo *info )
{
     info->version.major = 0;
     info->version.minor = 1;

     snprintf( info->name,   DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH,   "GE" );
     snprintf( info->vendor, DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH, "linux-chenxing" );

     info->driver_data_size = sizeof(GEDriverData);
     info->device_data_size = sizeof(GEDeviceData);
}

static DFBResult
driver_init_driver( GraphicsDeviceFuncs *funcs,
                    void                *driver_data,
                    void                *device_data,
                    CoreDFB             *core )
{
     D_DEBUG_AT( GE_Driver, "%s()\n", __FUNCTION__ );

     dfb_config->font_format = DSPF_ARGB;

     *funcs = geGraphicsDeviceFuncs;

     return DFB_OK;
}

static DFBResult
driver_init_device( GraphicsDeviceInfo *device_info,
                    void               *driver_data,
                    void               *device_data )
{
     GEDeviceData *dev = device_data;

     D_DEBUG_AT( GE_Driver, "%s()\n", __FUNCTION__ );

     if (libge_open(&dev->ge))
          goto fail;

     D_INFO( "GE/driver: Opened GE, caps 0x%x\n", dev->ge.info.caps);

     /* Fill device information. */
     snprintf( device_info->name,   DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH, "ge");
     snprintf( device_info->vendor, DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH, "MStar/SigmaStar" );

     device_info->caps.flags      = 0;
     device_info->caps.accel      = DFXL_FILLRECTANGLE |
                                    DFXL_DRAWRECTANGLE |
                                    DFXL_DRAWLINE |
                                    DFXL_BLIT |
                                    DFXL_STRETCHBLIT;
     device_info->caps.blitting   = /* rotation */
                                    DSBLIT_ROTATE90 |
                                    DSBLIT_ROTATE180 |
                                    DSBLIT_ROTATE270 |
                                    /* flipping */
                                    DSBLIT_FLIP_HORIZONTAL |
                                    DSBLIT_FLIP_VERTICAL;
     device_info->caps.drawing    = 0;

     //device_info->caps.flags    = CCF_CLIPPING | CCF_RENDEROPTS;
     //device_info->caps.accel    = DFXL_FILLRECTANGLE | DFXL_DRAWRECTANGLE | DFXL_DRAWLINE | DFXL_FILLTRIANGLE |
     //                             DFXL_BLIT          | DFXL_STRETCHBLIT;
     //device_info->caps.blitting = DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA          |
     //                             DSBLIT_SRC_COLORKEY       | DSBLIT_SRC_PREMULTIPLY  | DSBLIT_SRC_PREMULTCOLOR |
     //                             DSBLIT_ROTATE180;
     //device_info->caps.drawing  = DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY;

     device_info->limits.dst_max.w = 4096;
     device_info->limits.dst_max.h = 4096;

     return DFB_OK;

fail:
     return DFB_INIT;
}

static void
driver_close_device( void *driver_data,
                     void *device_data )
{
     GEDeviceData *dev = device_data;

     D_DEBUG_AT( GE_Driver, "%s()\n", __FUNCTION__ );

     libge_close(&dev->ge);
}

static void
driver_close_driver( void *driver_data )
{
     D_DEBUG_AT( GE_Driver, "%s()\n", __FUNCTION__ );
}
