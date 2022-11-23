/*
 * This file is based on the GLES2 driver from DirectFB2
 */

#ifndef __GE_GFXDRIVER_H__
#define __GE_GFXDRIVER_H__

#include <libge.h>

/**********************************************************************************************************************/

#define GE_OPSBATCHSZ   32

typedef struct {
    int x;
} GEDriverData;

typedef struct {
	struct ge_cntx ge;
	/* PRIME fds for the current buffers */
	int src_fd, dst_fd;
	/* src surface config */
	unsigned int src_w, src_h, src_fmt, src_pitch;
	/* dst surface config */
	unsigned int dst_w, dst_h, dst_fmt, dst_pitch;
	/* start color */
	unsigned int stclr_a, stclr_r, stclr_g, stclr_b;

	bool src_v_flip,  dst_h_flip, dst_v_flip;
	unsigned int rotation;

	struct mstar_ge_opdata opdata[GE_OPSBATCHSZ];
} GEDeviceData;

#endif
