
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

#include "bcsignals.h"
#include "clip.h"
#include "colormodels.inc"
#include "effecttv.h"
#include "vframe.h"

#include <stdint.h> 
#include <stdio.h>



unsigned int EffectTV::fastrand_val = 0;

EffectTV::EffectTV(int w, int h)
{
	this->w = w;
	this->h = h;
	background = (unsigned char*)new uint16_t[w * h];
	diff = new unsigned char[w * h];
	diff2 = new unsigned char[w * h];
}

EffectTV::~EffectTV()
{
	delete [] background;
	delete [] diff;
	delete [] diff2;
}

void EffectTV::image_set_threshold_y(int threshold)
{
	y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}

#define IMAGE_BGSUBTRACT_P(type, components, is_yuv) \
{ \
	int i, j; \
	int R, G, B; \
	type *p; \
 \
	for(i = 0; i < h; i++) \
	{ \
		p = (type*)&input[i * bytes_per_line]; \
 \
		for(j = 0; j < w; j++) \
		{ \
			if(is_yuv && sizeof(type) == 2) \
			{ \
				R = G = B = (int)p[0] >> 8; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(is_yuv && sizeof(type) == 1) \
			{ \
				R = G = B = (int)p[0]; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				R = (int)(p[0] * 0x1ff); \
				G = (int)(p[1] * 0x3ff); \
				B = (int)(p[2] * 0xff); \
				CLAMP(R, 0, 0x1ff); \
				CLAMP(G, 0, 0x3ff); \
				CLAMP(B, 0, 0xff); \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				R = (int)p[0] >> (8 - 1); \
				G = (int)p[1] >> (8 - 2); \
				B = (int)p[2] >> 8; \
			} \
			else \
			{ \
				R = (int)p[0] << 1; \
				G = (int)p[1] << 2; \
				B = (int)p[2]; \
			} \
 \
			v = (R + G + B) - (int)(*q); \
			*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24); \
 \
			p += components; \
			q++; \
			r++; \
		} \
	} \
}

unsigned char* EffectTV::image_bgsubtract_y(unsigned char *input,
	int color_model, int bytes_per_line)
{
	int16_t *q;
	unsigned char *r;
	int v;

	q = (int16_t *)background;
	r = diff;

	switch(color_model)
	{
	case BC_RGB888:
		IMAGE_BGSUBTRACT_P(uint8_t, 3, 0);
		break;
	case BC_YUV888:
		IMAGE_BGSUBTRACT_P(uint8_t, 3, 1);
		break;
	case BC_RGB_FLOAT:
		IMAGE_BGSUBTRACT_P(float, 3, 0);
		break;
	case BC_RGBA_FLOAT:
		IMAGE_BGSUBTRACT_P(float, 4, 0);
		break;
	case BC_RGBA8888:
		IMAGE_BGSUBTRACT_P(uint8_t, 4, 0);
		break;
	case BC_YUVA8888:
		IMAGE_BGSUBTRACT_P(uint8_t, 4, 1);
		break;
	case BC_RGB161616:
		IMAGE_BGSUBTRACT_P(uint16_t, 3, 0);
		break;
	case BC_YUV161616:
		IMAGE_BGSUBTRACT_P(uint16_t, 3, 1);
		break;
	case BC_RGBA16161616:
		IMAGE_BGSUBTRACT_P(uint16_t, 4, 0);
		break;
	case BC_YUVA16161616:
		IMAGE_BGSUBTRACT_P(uint16_t, 4, 1);
		break;

	case BC_AYUV16161616:
		{
			int i, j;
			int R, G, B;
			uint16_t *p;

			for(i = 0; i < h; i++)
			{
				p = (uint16_t*)&input[i * bytes_per_line];

				for(j = 0; j < w; j++)
				{
					R = G = B = (int)p[1] >> 8;
					R <<= 1;
					G <<= 2;

					v = (R + G + B) - (int)(*q);
					*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24);

					p += 4;
					q++;
					r++;
				}
			}
		}
		break;
	}
	return diff;


/* The origin of subtraction function is;
 * diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
 *
 * This functions is transformed to;
 * (threshold > (src - dest) > -threshold) ? 0 : 0xff;
 *
 * (v + threshold)>>24 is 0xff when v is less than -threshold.
 * (v - threshold)>>24 is 0xff when v is less than threshold.
 * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
 * abs(src - dest) > threshold.
 */
}


unsigned char* EffectTV::image_diff_filter(unsigned char *diff)
{
	int x, y;
	unsigned char *src, *dest;
	unsigned int count;
	unsigned int sum1, sum2, sum3;
	int width = w;
	int height = h;

	src = diff;
	dest = diff2 + width + 1;
	for(y = 1; y < height - 1; y++) 
	{
		sum1 = src[0] + src[width] + src[width * 2];
		sum2 = src[1] + src[width + 1] + src[width * 2 + 1];
		src += 2;

		for(x = 1; x < width - 1; x++) 
		{
			sum3 = src[0] + src[width] + src[width * 2];
			count = sum1 + sum2 + sum3;
			sum1 = sum2;
			sum2 = sum3;
			*dest++ = (0xff * 3 - count) >> 24;
			src++;
		}

		dest += 2;
	}

	return diff2;
}


#define IMAGE_BGSET_Y(type, components, is_yuv) \
{ \
	int i, j; \
	int R, G, B; \
	type *p; \
	int16_t *q; \
	int width = frame->get_w(); \
	int height = frame->get_h(); \
 \
	q = (int16_t *)background; \
 \
 \
 \
	for(i = 0; i < height; i++) \
	{ \
		p = (type*)frame->get_row_ptr(i); \
 \
		for(j = 0; j < width; j++) \
		{ \
			if(is_yuv && sizeof(type) == 2) \
			{ \
				R = G = B = (int)p[0] >> 8; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(is_yuv && sizeof(type) == 1) \
			{ \
				R = G = B = (int)p[0]; \
				R <<= 1; \
				G <<= 2; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				R = (int)(p[0] * 0x1ff); \
				G = (int)(p[1] * 0x3ff); \
				B = (int)(p[2] * 0xff); \
				CLAMP(R, 0, 0x1ff); \
				CLAMP(G, 0, 0x3ff); \
				CLAMP(B, 0, 0xff); \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				R = (int)p[0] >> (8 - 1); \
				G = (int)p[1] >> (8 - 2); \
				B = (int)p[2] >> 8; \
			} \
			else \
			{ \
				R = (int)p[0] << 1; \
				G = (int)p[1] << 2; \
				B = (int)p[2]; \
			} \
 \
			*q = (int16_t)(R + G + B); \
			p += components; \
 \
 \
			q++; \
		} \
	} \
}


void EffectTV::image_bgset_y(VFrame *frame)
{
	switch(frame->get_color_model())
	{
	case BC_RGB888:
		IMAGE_BGSET_Y(uint8_t, 3, 0);
		break;
	case BC_RGB_FLOAT:
		IMAGE_BGSET_Y(float, 3, 0);
		break;
	case BC_YUV888:
		IMAGE_BGSET_Y(uint8_t, 3, 1);
		break;
	case BC_RGBA8888:
		IMAGE_BGSET_Y(uint8_t, 3, 0);
		break;
	case BC_RGBA_FLOAT:
		IMAGE_BGSET_Y(float, 3, 0);
		break;
	case BC_YUVA8888:
		IMAGE_BGSET_Y(uint8_t, 3, 1);
		break;
	case BC_RGB161616:
		IMAGE_BGSET_Y(uint16_t, 3, 0);
		break;
	case BC_YUV161616:
		IMAGE_BGSET_Y(uint16_t, 3, 1);
		break;
	case BC_RGBA16161616:
		IMAGE_BGSET_Y(uint16_t, 4, 0);
		break;
	case BC_YUVA16161616:
		IMAGE_BGSET_Y(uint16_t, 4, 1);
		break;

	case BC_AYUV16161616:
		{
			int i, j;
			int R, G, B;
			uint16_t *p;
			int16_t *q;
			int width = frame->get_w();
			int height = frame->get_h();

			q = (int16_t *)background;

			for(i = 0; i < height; i++)
			{
				p = (uint16_t*)frame->get_row_ptr(i);

				for(j = 0; j < width; j++)
				{
					R = G = B = (int)p[1] >> 8;
					R <<= 1;
					G <<= 2;

					*q = (int16_t)(R + G + B);
					p += 4;
					q++;
				}
			}
		}
		break;
	}
}
