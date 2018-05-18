
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

#ifndef EFFECTTV_H
#define EFFECTTV_H

#include "plugincolors.inc"
#include "vframe.inc"
#include <stdint.h>

// Environment for EffectTV effects
class EffectTV
{
public:
	EffectTV(int w, int h);
	virtual ~EffectTV();

	void image_set_threshold_y(int threshold);
	unsigned char* image_bgsubtract_y(unsigned char *input,
		int color_model, int bytes_per_line);

	void image_bgset_y(VFrame *frame);
	unsigned char* image_diff_filter(unsigned char *diff);

	void yuv_init();

/*
 * fastrand - fast fake random number generator
 * Warning: The low-order bits of numbers generated by fastrand()
 *          are bad as random numbers. For example, fastrand()%4
 *          generates 1,2,3,0,1,2,3,0...
 *          You should use high-order bits.
 */
	static unsigned int fastrand_val;

	static inline unsigned int fastrand()
	{
		return (fastrand_val = fastrand_val * 1103515245 + 12345);
	};

	int w;
	int h;

	int y_threshold;

	unsigned char *background;
	unsigned char *diff, *diff2;

	int YtoRGB[0x100];
	int VtoR[0x100];
	int VtoG[0x100];
	int UtoG[0x100];
	int UtoB[0x100];
	int RtoY[0x100];
	int RtoU[0x100];
	int RtoV[0x100];
	int GtoY[0x100];
	int GtoU[0x100];
	int GtoV[0x100];
	int BtoY[0x100];
	int BtoV[0x100];
	YUV *yuv;

};

#endif
