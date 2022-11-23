/*
 * This file is based on the GLES2 driver from DirectFB2
 */

#include <core/state.h>
#include <core/surface_allocation.h>

#include <drm/drm_fourcc.h>

#include "ge_gfxdriver.h"

D_DEBUG_DOMAIN( GE, "GE", "MStar/Sigmastar GE 2D Acceleration" );

/**********************************************************************************************************************/

static u32 geDBFmtToDRMDmt(DFBSurfacePixelFormat fmt)
{
     switch(fmt) {
          case DSPF_A8:
               // I guess? DRM doesn't have an A8.
               return DRM_FORMAT_R8;
          case DSPF_RGB16:
               return DRM_FORMAT_RGB565;
          case DSPF_RGB32:
               return DRM_FORMAT_XRGB8888;
          case DSPF_ARGB:
               return DRM_FORMAT_ARGB8888;
          default:
               return DRM_FORMAT_INVALID;
     }
}

static void
geCheckState( void                *driver_data,
              void                *device_data,
              CardState           *state,
              DFBAccelerationMask  accel )
{
     GraphicsDeviceInfo device_info;

     dfb_gfxcard_get_device_info( &device_info );

     D_DEBUG_AT( GE_2D, "%s( %p, 0x%08x )\n", __FUNCTION__, state, accel );

     /* Check if function is accelerated. */
     if (accel & ~device_info.caps.accel) {
          D_DEBUG_AT(GE_2D, "  -> unsupported function\n");
          return;
     }

     /* Check if drawing or blitting flags are supported. */
     if (DFB_DRAWING_FUNCTION(accel)) {
          if (state->drawingflags & ~device_info.caps.drawing) {
               //printf("unsupported drawing flags 0x%08x\n", state->drawingflags);
               return;
          }
     }
     else {
          if (state->blittingflags & ~device_info.caps.blitting) {
               //printf("unsupported blitting flags 0x%08x\n", state->blittingflags);
               return;
          }
     }

     /* Enable acceleration of the function. */
     state->accel |= accel;
}

static void
geSetState( void                *driver_data,
            void                *device_data,
            GraphicsDeviceFuncs *funcs,
            CardState           *state,
            DFBAccelerationMask  accel )
{
     GEDeviceData           *dev    = device_data;
     int                     src_fd = (int) state->src.offset;
     int                     dst_fd = (int) state->dst.offset;
     bool                    have_src;
     bool                    have_dst;

     dev->src_fd = -1;
     dev->dst_fd = -1;

     have_src = src_fd > 0;
     have_dst = dst_fd > 0;
#if 0
     static int call = 0;
     call++;
     printf("%s:%d(%d) %lu %lu %d %d - %d %d\n", __FUNCTION__, __LINE__, call,
               state->src.offset, state->dst.offset,
               src_fd, dst_fd, have_src, have_dst);
#endif

     if (!have_src && !have_dst) {
          printf("no fds for buffers? %d and %d\n", src_fd, dst_fd);
          return;
     }

     if (!state->src.buffer && !state->dst.buffer) {
          printf("no src or dst buffer?\n");
          return;
     }

     if (have_src) {
          if (!state->src.buffer) {
               printf("no buffer for src!??\n");
               return;
          }
          dev->src_fd = src_fd;
          dev->src_w = state->src.buffer->config.size.w;
          dev->src_h = state->src.buffer->config.size.h;
          dev->src_fmt = geDBFmtToDRMDmt(state->src.buffer->config.format);
          if (dev->src_fmt == DRM_FORMAT_INVALID)
               printf("unhandled source format %d\n", state->src.buffer->config.format);
          dev->src_pitch = state->src.pitch;
     }

     if (have_dst) {
          if (!state->dst.buffer) {
               printf("no buffer for dst!??\n");
               return;
          }
          dev->dst_fd = dst_fd;
          dev->dst_w = state->dst.buffer->config.size.w;
          dev->dst_h = state->dst.buffer->config.size.h;
          dev->dst_fmt = geDBFmtToDRMDmt(state->dst.buffer->config.format);
          if (dev->dst_fmt == DRM_FORMAT_INVALID)
               printf("unhandled dst format %d\n", state->dst.buffer->config.format);
          dev->dst_pitch = state->dst.pitch;
     }

     dev->stclr_a = state->color.a;
     dev->stclr_r = state->color.r;
     dev->stclr_g = state->color.g;
     dev->stclr_b = state->color.b;

     if (state->blittingflags & DSBLIT_ROTATE90)
          dev->rotation = MSTAR_GE_ROTATION_90;
     else if (state->blittingflags & DSBLIT_ROTATE180)
          dev->rotation = MSTAR_GE_ROTATION_180;
     else if (state->blittingflags & DSBLIT_ROTATE270)
          dev->rotation = MSTAR_GE_ROTATION_270;
     else
          dev->rotation = MSTAR_GE_ROTATION_0;

     dev->src_v_flip = false;
     dev->dst_h_flip = state->blittingflags & DSBLIT_FLIP_HORIZONTAL;
     dev->dst_v_flip = state->blittingflags & DSBLIT_FLIP_VERTICAL;
}

//ret = libge_queue_job(&dev->ge, &opdata,
//          dev->src_fd, dev->src_w, dev->src_h, dev->src_fmt, dev->src_pitch,
//          dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
//          &tag);

static bool
geFillRectangle( void         *driver_data,
                 void         *device_data,
                 DFBRectangle *rect )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     struct mstar_ge_opdata opdata;
     unsigned long          tag;
     int                    ret;

     D_DEBUG_AT( GE_2D, "%s()\n", __FUNCTION__ );

     /* wtf? */
     if (dev->dst_fd == -1)
          return false;

     libge_filljob_fillrect(&opdata,
                            rect->x,
                            rect->y,
                            (rect->x + rect->w) - 1,
                            (rect->y + rect->h) - 1,
                            dev->stclr_a,
                            dev->stclr_r,
                            dev->stclr_g,
                            dev->stclr_b);

     ret = libge_queue_job(&dev->ge, &opdata, 1,
               -1, 0, 0, 0, 0,
               dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
               &tag);
     if (ret)
          printf("%s:%d:%d\n", __FUNCTION__, __LINE__, ret);

     return true;
}

static bool
geDrawRectangle( void         *driver_data,
                 void         *device_data,
                 DFBRectangle *rect )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     unsigned long          tag;
     int                    ret;
     int                    i;

     if (dev->dst_fd == -1)
          return false;

     /* top */
     libge_filljob_line(&dev->opdata[0],
                        rect->x,
                        rect->y,
                        (rect->x + rect->w) - 1,
                        rect->y,
                        dev->stclr_a,
                        dev->stclr_r,
                        dev->stclr_g,
                        dev->stclr_b);

     /* left */
     libge_filljob_line(&dev->opdata[1],
                        rect->x,
                        rect->y,
                        rect->x,
                        (rect->y + rect->h) - 1,
                        dev->stclr_a,
                        dev->stclr_r,
                        dev->stclr_g,
                        dev->stclr_b);

     /* right */
     libge_filljob_line(&dev->opdata[2],
                        (rect->x + rect->w) - 1,
                        rect->y,
                        (rect->x + rect->w) - 1,
                        (rect->y + rect->h) - 1,
                        dev->stclr_a,
                        dev->stclr_r,
                        dev->stclr_g,
                        dev->stclr_b);

     /* bottom */
     libge_filljob_line(&dev->opdata[3],
                        rect->x,
                        (rect->y + rect->h) - 1,
                        (rect->x + rect->w) - 1,
                        (rect->y + rect->h) - 1,
                        dev->stclr_a,
                        dev->stclr_r,
                        dev->stclr_g,
                        dev->stclr_b);

     ret = libge_queue_job(&dev->ge, dev->opdata, 4,
               -1, 0, 0, 0, 0,
               dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
               &tag);
     if (ret)
          printf("%s:%d:%d\n", __FUNCTION__, __LINE__, ret);

     return true;
}

static bool
geDrawLine( void      *driver_data,
            void      *device_data,
            DFBRegion *line )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     struct mstar_ge_opdata opdata;
     unsigned long          tag;
     int                    ret;

     if (dev->dst_fd == -1)
          return false;

     libge_filljob_line(&opdata,
                        line->x1,
                        line->y1,
                        line->x2,
                        line->y2,
                        dev->stclr_a,
                        dev->stclr_r,
                        dev->stclr_g,
                        dev->stclr_b);

     ret = libge_queue_job(&dev->ge, &opdata, 1,
               -1, 0, 0, 0, 0,
               dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
               &tag);
     if (ret)
          printf("%s:%d:%d\n", __FUNCTION__, __LINE__, ret);

     return true;
}

static bool
geFillTriangle( void        *driver_data,
                void        *device_data,
                DFBTriangle *tri )
{
     GEDriverData *drv   = driver_data;

     printf("%s:%d\n", __FUNCTION__, __LINE__);

     return true;
}

static bool
geBlit( void         *driver_data,
        void         *device_data,
        DFBRectangle *rect,
        int           dx,
        int           dy )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     struct mstar_ge_opdata opdata;
     unsigned long          tag;
     int                    ret;

     /* wtf? */
     if (dev->src_fd == -1 || dev->dst_fd == -1)
          return false;

     libge_filljob_blit(&opdata,
                        rect->x,
                        rect->y,
                        dx,
                        dy,
                        (dx + rect->w) - 1,
                        (dy + rect->h) - 1,
                        dev->src_v_flip,
                        dev->dst_h_flip,
                        dev->dst_v_flip,
                        dev->rotation);

     ret = libge_queue_job(&dev->ge, &opdata, 1,
               dev->src_fd, dev->src_w, dev->src_h, dev->src_fmt, dev->src_pitch,
               dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
               &tag);
     if (ret)
          printf("%s:%d:%d\n", __FUNCTION__, __LINE__, ret);

     return true;
}

static bool
geStretchBlit( void         *driver_data,
               void         *device_data,
               DFBRectangle *srect,
               DFBRectangle *drect )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     struct mstar_ge_opdata opdata;
     unsigned long          tag;
     int                    ret;

     D_DEBUG_AT(GE_2D, "  -> %s:%d\n", __FUNCTION__, __LINE__);

     /* wtf? */
     if (dev->src_fd == -1 || dev->dst_fd == -1)
          return false;

     libge_filljob_strblit(&opdata,
                        srect->x,
                        srect->y,
                        (srect->x + srect->w) - 1,
                        (srect->y + srect->h) - 1,
                        drect->x,
                        drect->y,
                        (drect->x + drect->w) - 1,
                        (drect->y + drect->h) - 1,
                        dev->src_v_flip,
                        dev->dst_h_flip,
                        dev->dst_v_flip,
                        dev->rotation);

     ret = libge_queue_job(&dev->ge, &opdata, 1,
               dev->src_fd, dev->src_w, dev->src_h, dev->src_fmt, dev->src_pitch,
               dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
               &tag);
     if (ret)
          D_DEBUG_AT(GE_2D, "  -> %s:%d:%d\n", __FUNCTION__, __LINE__, ret);

     return true;
}

static bool
geBatchBlit( void               *driver_data,
             void               *device_data,
             const DFBRectangle *rects,
             const DFBPoint     *points,
             unsigned int        num,
             unsigned int       *ret_num )
{
     GEDriverData           *drv    = driver_data;
     GEDeviceData           *dev    = device_data;
     unsigned long          tag;
     int                    ret;
     int                    i;

     //printf("%s:%d\n", __FUNCTION__, __LINE__);

     /* wtf? */
     if (dev->src_fd == -1 || dev->dst_fd == -1)
          return false;

     for (i = 0; i < num; i++) {
          libge_filljob_blit(&dev->opdata[i % GE_OPSBATCHSZ],
                             rects[i].x,
                             rects[i].y,
                             points[i].x,
                             points[i].y,
                             (points[i].x + rects[i].w) - 1,
                             (points[i].y + rects[i].h) - 1,
                             dev->src_v_flip,
                             dev->dst_h_flip,
                             dev->dst_v_flip,
                             dev->rotation);

          if ((i + 1 == GE_OPSBATCHSZ) || (i + 1 == num))
               ret = libge_queue_job(&dev->ge, dev->opdata, (i % GE_OPSBATCHSZ) + 1,
                         dev->src_fd, dev->src_w, dev->src_h, dev->src_fmt, dev->src_pitch,
                         dev->dst_fd, dev->dst_w, dev->dst_h, dev->dst_fmt, dev->dst_pitch,
                         &tag);
          if (ret)
               printf("%s:%d:%d\n", __FUNCTION__, __LINE__, ret);

          //ret_num[i] = ret;
     }

     return true;
}

const GraphicsDeviceFuncs geGraphicsDeviceFuncs = {
     .CheckState    = geCheckState,
     .SetState      = geSetState,
     .FillRectangle = geFillRectangle,
     .DrawRectangle = geDrawRectangle,
     .DrawLine      = geDrawLine,
     .FillTriangle  = geFillTriangle,
     .Blit          = geBlit,
     .StretchBlit   = geStretchBlit,
     .BatchBlit     = geBatchBlit
};
