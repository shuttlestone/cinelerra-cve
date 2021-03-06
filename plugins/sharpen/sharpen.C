
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

#include "clip.h"
#include "colormodels.h"
#include "condition.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "sharpen.h"
#include "sharpenwindow.h"

#include <stdio.h>
#include <string.h>

REGISTER_PLUGIN


SharpenConfig::SharpenConfig()
{
	horizontal = 0;
	interlace = 0;
	sharpness = 50;
	luminance = 0;
}

void SharpenConfig::copy_from(SharpenConfig &that)
{
	horizontal = that.horizontal;
	interlace = that.interlace;
	sharpness = that.sharpness;
	luminance = that.luminance;
}

int SharpenConfig::equivalent(SharpenConfig &that)
{
	return horizontal == that.horizontal &&
		interlace == that.interlace &&
		EQUIV(sharpness, that.sharpness) &&
		luminance == that.luminance;
}

void SharpenConfig::interpolate(SharpenConfig &prev, 
	SharpenConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->sharpness = prev.sharpness * prev_scale + next.sharpness * next_scale;
	this->interlace = prev.interlace;
	this->horizontal = prev.horizontal;
	this->luminance = prev.luminance;
}


SharpenMain::SharpenMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SharpenMain::~SharpenMain()
{
	if(engine)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
		}
		delete engine;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *SharpenMain::process_tmpframe(VFrame *input)
{
	int i, j, k;

	load_configuration();
	if(!engine)
	{
		total_engines = PluginClient::smp > 1 ? 2 : 1;
		engine = new SharpenEngine*[total_engines];

		for(int i = 0; i < total_engines; i++)
		{
			engine[i] = new SharpenEngine(this, input);
			engine[i]->start();
		}
	}

	get_luts(pos_lut, neg_lut, input->get_color_model());

	if(config.sharpness)
	{
// Arm first row
		row_step = (config.interlace) ? 2 : 1;

		for(j = 0; j < row_step; j += total_engines)
		{
			for(k = 0; k < total_engines && k + j < row_step; k++)
				engine[k]->start_process_frame(input, k + j);

			for(k = 0; k < total_engines && k + j < row_step; k++)
			{
				engine[k]->wait_process_frame();
			}
		}
	}
	return input;
}

void SharpenMain::load_defaults()
{
	defaults = load_defaults_file("sharpen.rc");

	config.sharpness = defaults->get("SHARPNESS", config.sharpness);
	config.interlace = defaults->get("INTERLACE", config.interlace);
	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.luminance = defaults->get("LUMINANCE", config.luminance);
}

void SharpenMain::save_defaults()
{
	defaults->update("SHARPNESS", config.sharpness);
	defaults->update("INTERLACE", config.interlace);
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("LUMINANCE", config.luminance);
	defaults->save();
}

void SharpenMain::get_luts(int *pos_lut, int *neg_lut, int color_model)
{
	int i, inv_sharpness, vmax;

	vmax = ColorModels::calculate_max(color_model);

	inv_sharpness = (int)(100 - config.sharpness);
	if(config.horizontal) inv_sharpness /= 2;
	if(inv_sharpness < 1) inv_sharpness = 1;

	for(i = 0; i < vmax + 1; i++)
	{
		pos_lut[i] = 800 * i / inv_sharpness;
		neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
	}
}

void SharpenMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SHARPNESS");
	output.tag.set_property("VALUE", config.sharpness);
	output.append_tag();

	if(config.interlace)
	{
		output.tag.set_title("INTERLACE");
		output.append_tag();
		output.tag.set_title("/INTERLACE");
		output.append_tag();
	}

	if(config.horizontal)
	{
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
		output.tag.set_title("/HORIZONTAL");
		output.append_tag();
	}

	if(config.luminance)
	{
		output.tag.set_title("LUMINANCE");
		output.append_tag();
		output.tag.set_title("/LUMINANCE");
		output.append_tag();
	}
	output.tag.set_title("/SHARPNESS");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SharpenMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SHARPNESS"))
			{
				config.sharpness = input.tag.get_property("VALUE", config.sharpness);
			}
			else
			if(input.tag.title_is("INTERLACE"))
			{
				new_interlace = 1;
			}
			else
			if(input.tag.title_is("HORIZONTAL"))
			{
				new_horizontal = 1;
			}
			else
			if(input.tag.title_is("LUMINANCE"))
			{
				new_luminance = 1;
			}
		}
	}

	config.interlace = new_interlace;
	config.horizontal = new_horizontal;
	config.luminance = new_luminance;

	if(config.sharpness > MAXSHARPNESS) 
		config.sharpness = MAXSHARPNESS;
	else
		if(config.sharpness < 0) config.sharpness = 0;
}

SharpenEngine::SharpenEngine(SharpenMain *plugin, VFrame *input)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->plugin = plugin;
	input_lock = new Condition(0,"SharpenEngine::input_lock");
	output_lock = new Condition(0, "SharpenEngine::output_lock");
	last_frame = 0;
	for(int i = 0; i < 4; i++)
	{
		neg_rows[i] = new unsigned char[input->get_w() *
			4 * MAX(sizeof(float), sizeof(int))];
	}
}

SharpenEngine::~SharpenEngine()
{
	last_frame = 1;
	input_lock->unlock();
	Thread::join();

	for(int i = 0; i < 4; i++)
	{
		delete [] neg_rows[i];
	}
	delete input_lock;
	delete output_lock;
}

void SharpenEngine::start_process_frame(VFrame *output, int field)
{
	this->output = output;
	this->field = field;

// Get coefficient for floating point
	sharpness_coef = 100 - plugin->config.sharpness;
	if(plugin->config.horizontal)
		sharpness_coef /= 2;
	if(sharpness_coef < 1)
		sharpness_coef = 1;
	sharpness_coef = 800.0 / sharpness_coef;

	input_lock->unlock();
}

void SharpenEngine::wait_process_frame()
{
	output_lock->lock("SharpenEngine::wait_process_frame");
}

double SharpenEngine::calculate_pos(double value)
{
	return sharpness_coef * value;
}

double SharpenEngine::calculate_neg(double value)
{
	return (calculate_pos(value) - (value * 8)) / 8;
}

#define FILTER(components, vmax) \
{ \
	int *pos_lut = plugin->pos_lut; \
	const int wordsize = sizeof(*src); \
 \
/* Skip first pixel in row */ \
	memcpy(dst, src, components * wordsize); \
	dst += components; \
	src += components; \
 \
	w -= 2; \
 \
	while(w > 0) \
	{ \
		long pixel; \
		pixel = (long)pos_lut[src[0]] -  \
			(long)neg0[-components] -  \
			(long)neg0[0] -  \
			(long)neg0[components] -  \
			(long)neg1[-components] -  \
			(long)neg1[components] -  \
			(long)neg2[-components] -  \
			(long)neg2[0] -  \
			(long)neg2[components]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[0] = 0; \
		else \
		if(pixel > vmax) dst[0] = vmax; \
		else \
		dst[0] = pixel; \
 \
		pixel = (long)pos_lut[src[1]] -  \
			(long)neg0[-components + 1] -  \
			(long)neg0[1] -  \
			(long)neg0[components + 1] -  \
			(long)neg1[-components + 1] -  \
			(long)neg1[components + 1] -  \
			(long)neg2[-components + 1] -  \
			(long)neg2[1] -  \
			(long)neg2[components + 1]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[1] = 0; \
		else \
		if(pixel > vmax) dst[1] = vmax; \
		else \
		dst[1] = pixel; \
 \
		pixel = (long)pos_lut[src[2]] -  \
			(long)neg0[-components + 2] -  \
			(long)neg0[2] -  \
			(long)neg0[components + 2] -  \
			(long)neg1[-components + 2] -  \
			(long)neg1[components + 2] -  \
			(long)neg2[-components + 2] -  \
			(long)neg2[2] -  \
			(long)neg2[components + 2]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[2] = 0; \
		else \
		if(pixel > vmax) dst[2] = vmax; \
		else \
		dst[2] = pixel; \
 \
		src += components; \
		dst += components; \
 \
		neg0 += components; \
		neg1 += components; \
		neg2 += components; \
		w--; \
	} \
 \
/* Skip last pixel in row */ \
	memcpy(dst, src, components * wordsize); \
}

void SharpenEngine::filter(int components,
	int vmax,
	int w, 
	u_int16_t *src, 
	u_int16_t *dst,
	int *neg0, 
	int *neg1, 
	int *neg2)
{
	FILTER(components, vmax);
}

void SharpenEngine::filter(int components,
	int vmax,
	int w, 
	unsigned char *src, 
	unsigned char *dst,
	int *neg0, 
	int *neg1, 
	int *neg2)
{
	FILTER(components, vmax);
}

void SharpenEngine::filter(int components,
	int vmax,
	int w, 
	float *src, 
	float *dst,
	float *neg0, 
	float *neg1, 
	float *neg2)
{
	const int wordsize = sizeof(float);
// First pixel in row
	memcpy(dst, src, components * wordsize);
	dst += components;
	src += components;

	w -= 2;
	while(w > 0)
	{
		float pixel;
		pixel = calculate_pos(src[0]) -
			neg0[-components] -
			neg0[0] - 
			neg0[components] -
			neg1[-components] -
			neg1[components] -
			neg2[-components] -
			neg2[0] -
			neg2[components];
		pixel /= 8;
		dst[0] = pixel;

		pixel = calculate_pos(src[1]) -
			neg0[-components + 1] -
			neg0[1] - 
			neg0[components + 1] -
			neg1[-components + 1] -
			neg1[components + 1] -
			neg2[-components + 1] -
			neg2[1] -
			neg2[components + 1];
		pixel /= 8;
		dst[1] = pixel;

		pixel = calculate_pos(src[2]) -
			neg0[-components + 2] -
			neg0[2] - 
			neg0[components + 2] -
			neg1[-components + 2] -
			neg1[components + 2] -
			neg2[-components + 2] -
			neg2[2] -
			neg2[components + 2];
		pixel /= 8;
		dst[2] = pixel;

		src += components;
		dst += components;
		neg0 += components;
		neg1 += components;
		neg2 += components;
		w--;
	}

/* Last pixel */
	memcpy(dst, src, components * wordsize);
}


#define SHARPEN(components, type, temp_type, vmax) \
{ \
	int count, row; \
	int wordsize = sizeof(type); \
	int w = output->get_w(); \
	int h = output->get_h(); \
 \
	src_rows[0] = src_rows[1] = src_rows[2] = \
		src_rows[3] = output->get_row_ptr(field); \
 \
	for(int j = 0; j < w; j++) \
	{ \
		temp_type *neg = (temp_type*)neg_rows[0]; \
		type *src = (type*)src_rows[0]; \
		for(int k = 0; k < components; k++) \
		{ \
			if(wordsize == 4) \
			{ \
				neg[j * components + k] = \
					(temp_type)calculate_neg(src[j * components + k]); \
			} \
			else \
			{ \
				neg[j * components + k] = \
					(temp_type)plugin->neg_lut[(int)src[j * components + k]]; \
			} \
		} \
	} \
 \
	row = 1; \
	count = 1; \
 \
	for(int i = field; i < h; i += plugin->row_step) \
	{ \
		if((i + plugin->row_step) < h) \
		{ \
			if(count >= 3) count--; \
/* Arm next row */ \
			src_rows[row] = output->get_row_ptr(i + plugin->row_step); \
/* Calculate neg rows */ \
			type *src = (type*)src_rows[row]; \
			temp_type *neg = (temp_type*)neg_rows[row]; \
			for(int k = 0; k < w; k++) \
			{ \
				for(int j = 0; j < components; j++) \
				{ \
					if(wordsize == 4) \
					{ \
						neg[k * components + j] = \
							(temp_type)calculate_neg(src[k * components + j]); \
					} \
					else \
					{ \
						neg[k * components + j] = \
							plugin->neg_lut[(int)src[k * components + j]]; \
					} \
				} \
			} \
 \
			count++; \
			row = (row + 1) & 3; \
		} \
		else \
		{ \
			count--; \
		} \
 \
		dst_row = output->get_row_ptr(i); \
		if(count == 3) \
		{ \
/* Do the filter */ \
			if(plugin->config.horizontal) \
				filter(components, \
					vmax, \
					w,  \
					(type*)src_rows[(row + 2) & 3],  \
					(type*)dst_row, \
					(temp_type*)neg_rows[(row + 2) & 3] + components, \
					(temp_type*)neg_rows[(row + 2) & 3] + components, \
					(temp_type*)neg_rows[(row + 2) & 3] + components); \
			else \
				filter(components, \
					vmax, \
					w,  \
					(type*)src_rows[(row + 2) & 3],  \
					(type*)dst_row, \
					(temp_type*)neg_rows[(row + 1) & 3] + components, \
					(temp_type*)neg_rows[(row + 2) & 3] + components, \
					(temp_type*)neg_rows[(row + 3) & 3] + components); \
		} \
		else  \
		if(count == 2) \
		{ \
			if(i == 0) \
				memcpy(dst_row, src_rows[0], w * components * wordsize); \
			else \
				memcpy(dst_row, src_rows[2], w * components * wordsize); \
		} \
	} \
}

void SharpenEngine::run()
{
	while(1)
	{
		input_lock->lock("SharpenEngine::run");
		if(last_frame)
		{
			output_lock->unlock();
			return;
		}

		switch(output->get_color_model())
		{
		case BC_RGB_FLOAT:
			SHARPEN(3, float, float, 1);
			break;

		case BC_RGB888:
		case BC_YUV888:
			SHARPEN(3, unsigned char, int, 0xff);
			break;

		case BC_RGBA_FLOAT:
			SHARPEN(4, float, float, 1);
			break;

		case BC_RGBA8888:
		case BC_YUVA8888:
			SHARPEN(4, unsigned char, int, 0xff);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
			SHARPEN(3, u_int16_t, int, 0xffff);
			break;

		case BC_RGBA16161616:
		case BC_YUVA16161616:
			SHARPEN(4, u_int16_t, int, 0xffff);
			break;

		case BC_AYUV16161616:
			{
				int count, row;
				int wordsize = sizeof(uint16_t);
				int w = output->get_w();
				int h = output->get_h();

				src_rows[0] = src_rows[1] =
					src_rows[2] = src_rows[3] =
					output->get_row_ptr(field);

				for(int j = 0; j < w; j++)
				{
					int *neg = (int*)neg_rows[0];
					uint16_t *src = (uint16_t*)src_rows[0];
					for(int k = 0; k < 4; k++)
					{
						neg[j * 4 + k] =
							(int)plugin->neg_lut[(int)src[j * 4 + k]];
					}
				}

				row = 1;
				count = 1;

				for(int i = field; i < h; i += plugin->row_step)
				{
					if((i + plugin->row_step) < h)
					{
						if(count >= 3) count--;
						// Arm next row
						src_rows[row] = output->get_row_ptr(i + plugin->row_step);
						// Calculate neg rows
						uint16_t *src = (uint16_t*)src_rows[row];
						int *neg = (int*)neg_rows[row];

						for(int k = 0; k < w; k++)
						{
							for(int j = 0; j < 4; j++)
							{
								neg[k * 4 + j] =
									plugin->neg_lut[(int)src[k * 4 + j]];
							}
						}

						count++;
						row = (row + 1) & 3;
					}
					else
						count--;

					dst_row = output->get_row_ptr(i);
					if(count == 3)
					{
						// Do the filter
						if(plugin->config.horizontal)
							filter(4, 0xffff, w,
								(uint16_t*)src_rows[(row + 2) & 3],
								(uint16_t*)dst_row,
								(int*)neg_rows[(row + 2) & 3] + 4,
								(int*)neg_rows[(row + 2) & 3] + 4,
								(int*)neg_rows[(row + 2) & 3] + 4);
						else
							filter(4, 0xffff, w,
								(uint16_t*)src_rows[(row + 2) & 3],
								(uint16_t*)dst_row,
								(int*)neg_rows[(row + 1) & 3] + 4,
								(int*)neg_rows[(row + 2) & 3] + 4,
								(int*)neg_rows[(row + 3) & 3] + 4);
					}
					else
					if(count == 2)
					{
						if(i == 0)
							memcpy(dst_row, src_rows[0], w * 4 * 2);
						else
							memcpy(dst_row, src_rows[2], w * 4 * 2);
					}
				}
			}
			break;
		}
		output_lock->unlock();
	}
}
