
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef VDEVICEX11_H
#define VDEVICEX11_H

#define MAX_XV_CMODELS 16

#include "bcbitmap.inc"
#include "canvas.inc"
#include "edl.inc"
#include "maskauto.inc"
#include "maskautos.inc"
#include "pluginclient.inc"
#include "thread.h"
#include "videodevice.inc"


class VDeviceX11
{
public:
	VDeviceX11(VideoDevice *device, Canvas *output);
	~VDeviceX11();

	int open_output(int colormodel);
	int output_visible();

// After loading the bitmap with a picture, write it
	int write_buffer(VFrame *result, EDL *edl);

private:

// Closest colormodel the hardware can do for playback.
// Only used by VDeviceX11::new_output_buffer.
	int get_accel_colormodel(int colormodel);

// Bitmap to be written to device
	BC_Bitmap *bitmap;

// Canvas for output
	Canvas *output;
	VideoDevice *device;
// Transfer coordinates from the output frame to the canvas 
// for last frame rendered.
// These stick the last frame to the display.
// Must be floats to support OpenGL
	double output_x1, output_y1, output_x2, output_y2;
	double canvas_x1, canvas_y1, canvas_x2, canvas_y2;
// XV accelerated colormodels
	int accel_cmodel;
	int num_xv_cmodels;
	int xv_cmodels[MAX_XV_CMODELS];
};

#endif
