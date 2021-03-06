
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
#include "colormodels.inc"
#include "filexml.h"
#include "picon_png.h"
#include "aging.h"
#include "agingwindow.h"
#include "effecttv.h"
#include "language.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


REGISTER_PLUGIN


int AgingConfig::dx[] = { 1, 1, 0, -1, -1, -1,  0, 1};
int AgingConfig::dy[] = { 0, -1, -1, -1, 0, 1, 1, 1};

AgingConfig::AgingConfig()
{
	dust_interval = 0;
	pits_interval = 0;
	aging_mode = 0;
	area_scale = 10;
	scratch_lines = 7;
	colorage = 1;
	scratch = 1;
	pits = 1;
	dust = 1;
}

AgingMain::AgingMain(PluginServer *server)
 : PluginVClient(server)
{
	aging_server = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

AgingMain::~AgingMain()
{
	if(aging_server) delete aging_server;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

int AgingMain::load_configuration()
{
    return 0;
}

VFrame *AgingMain::process_tmpframe(VFrame *input_ptr)
{
	load_configuration();
	this->input_ptr = input_ptr;
	this->output_ptr = input_ptr;

	if(!aging_server) aging_server = new AgingServer(this, 
		PluginClient::smp + 1, 
		PluginClient::smp + 1);
	aging_server->process_packages();
	return input_ptr;
}


AgingServer::AgingServer(AgingMain *plugin, int total_clients, int total_packages)
 : LoadServer(1, 1)
{
	this->plugin = plugin;
}

LoadClient* AgingServer::new_client() 
{
	return new AgingClient(this);
}

LoadPackage* AgingServer::new_package() 
{ 
	return new AgingPackage; 
}

void AgingServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		AgingPackage *package = (AgingPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() * i / get_total_packages();
		package->row2 = plugin->input_ptr->get_h() * (i + 1) / get_total_packages();
	}
}


AgingClient::AgingClient(AgingServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}

#define COLORAGE(type, components) \
{ \
	int a, b; \
	int i, j, k; \
 \
	for(i = 0; i < h; i++) \
	{ \
		type *inrow = (type*)input_frame->get_row_ptr(row1 + i); \
		type *outrow = (type*)output_frame->get_row_ptr(row1 + i); \
 \
		for(j = 0; j < w; j++) \
		{ \
			for(k = 0; k < 3; k++) \
			{ \
				if(sizeof(type) == 4) \
				{ \
					a = inrow[j * components + k] * 0xffff; \
					CLAMP(a, 0, 0xffff); \
				} \
				else \
					a = inrow[j * components + k]; \
 \
				if(sizeof(type) == 4) \
				{ \
					b = (a & 0xffff) >> 2; \
					outrow[j * components + k] = \
						(type)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000)) / 0xffff; \
				} \
				else \
				if(sizeof(type) == 2) \
				{ \
					b = (a & 0xffff) >> 2; \
					outrow[j * components + k] = \
						(type)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000)); \
				} \
				else \
				{ \
					b = (a & 0xff) >> 2; \
					outrow[j * components + k] =  \
						(type)(a - b + 0x18 + ((EffectTV::fastrand() >> 8) & 0x10)); \
				} \
			} \
		} \
	} \
}

void AgingClient::coloraging(VFrame *output_frame,
	VFrame *input_frame, int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();

	switch(output_frame->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		COLORAGE(uint8_t, 3);
		break;

	case BC_RGB_FLOAT:
		COLORAGE(float, 3);
		break;

	case BC_RGBA_FLOAT:
		COLORAGE(float, 4);
		break;

	case BC_RGBA8888:
	case BC_YUVA8888:
		COLORAGE(uint8_t, 4);
		break;

	case BC_RGB161616:
	case BC_YUV161616:
		COLORAGE(uint16_t, 3);
		break;

	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		COLORAGE(uint16_t, 4);
		break;
	}
}

#define SCRATCHES(type, components, chroma) \
{ \
	int i, j, y, y1, y2; \
	type *p; \
	int a, b; \
	int w_256 = w * 256; \
	type *row = (type*)output_frame->get_row_ptr(row1); \
 \
	for(i = 0; i < plugin->config.scratch_lines; i++) \
	{ \
		if(plugin->config.scratches[i].life)  \
		{ \
			plugin->config.scratches[i].x = plugin->config.scratches[i].x + plugin->config.scratches[i].dx; \
			if(plugin->config.scratches[i].x < 0 || plugin->config.scratches[i].x > w_256)  \
			{ \
				plugin->config.scratches[i].life = 0; \
				break; \
			} \
\
			p =  row + \
				(plugin->config.scratches[i].x >> 8) * \
				components; \
\
			if(plugin->config.scratches[i].init)  \
			{ \
				y1 = plugin->config.scratches[i].init; \
				plugin->config.scratches[i].init = 0; \
			}  \
			else  \
			{ \
				y1 = 0; \
			} \
\
			plugin->config.scratches[i].life--; \
			if(plugin->config.scratches[i].life)  \
			{ \
				y2 = h; \
			}  \
			else  \
			{ \
				y2 = EffectTV::fastrand() % h; \
			} \
 \
			for(y = y1; y < y2; y++)  \
			{ \
				for(j = 0; j < (chroma ? 1 : 3); j++) \
				{ \
					if(sizeof(type) == 4) \
					{ \
						int temp = (int)(p[j] * 0xffff); \
						CLAMP(temp, 0, 0xffff); \
						a = temp & 0xfeff; \
						a += 0x2000; \
						b = a & 0x10000; \
						p[j] = (type)(a | (b - (b >> 8))) / 0xffff; \
					} \
					else \
					if(sizeof(type) == 2) \
					{ \
						int temp = (int)p[j]; \
						a = temp & 0xfeff; \
						a += 0x2000; \
						b = a & 0x10000; \
						p[j] = (type)(a | (b - (b >> 8))); \
					} \
					else \
					{ \
						int temp = (int)p[j]; \
						a = temp & 0xfe; \
						a += 0x20; \
						b = a & 0x100; \
						p[j] = (type)(a | (b - (b >> 8))); \
					} \
				} \
 \
				if(chroma) \
				{ \
					p[1] = chroma; \
					p[2] = chroma; \
				} \
				p += w * components; \
			} \
		}  \
		else  \
		{ \
			if((EffectTV::fastrand() & 0xf0000000) == 0)  \
			{ \
				plugin->config.scratches[i].life = 2 + (EffectTV::fastrand() >> 27); \
				plugin->config.scratches[i].x = EffectTV::fastrand() % (w_256); \
				plugin->config.scratches[i].dx = ((int)EffectTV::fastrand()) >> 23; \
				plugin->config.scratches[i].init = (EffectTV::fastrand() % (h - 1)) + 1; \
			} \
		} \
	} \
}

void AgingClient::scratching(VFrame *output_frame,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();

	switch(output_frame->get_color_model())
	{
	case BC_RGB888:
		SCRATCHES(uint8_t, 3, 0);
		break;

	case BC_RGB_FLOAT:
		SCRATCHES(float, 3, 0);
		break;

	case BC_YUV888:
		SCRATCHES(uint8_t, 3, 0x80);
		break;

	case BC_RGBA_FLOAT:
		SCRATCHES(float, 4, 0);
		break;

	case BC_RGBA8888:
		SCRATCHES(uint8_t, 4, 0);
		break;

	case BC_YUVA8888:
		SCRATCHES(uint8_t, 4, 0x80);
		break;

	case BC_RGB161616:
		SCRATCHES(uint16_t, 3, 0);
		break;

	case BC_YUV161616:
		SCRATCHES(uint16_t, 3, 0x8000);
		break;

	case BC_RGBA16161616:
		SCRATCHES(uint16_t, 4, 0);
		break;

	case BC_YUVA16161616:
		SCRATCHES(uint16_t, 4, 0x8000);
		break;

	case BC_AYUV16161616:
		{
			int i, j, y, y1, y2;
			uint16_t *p;
			int a, b;
			int w_256 = w * 256;
			uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1);

			for(i = 0; i < plugin->config.scratch_lines; i++)
			{
				if(plugin->config.scratches[i].life)
				{
					plugin->config.scratches[i].x =
						plugin->config.scratches[i].x +
						plugin->config.scratches[i].dx;
					if(plugin->config.scratches[i].x < 0 ||
						plugin->config.scratches[i].x > w_256)
					{
						plugin->config.scratches[i].life = 0;
						break;
					}

					p =  row +
						(plugin->config.scratches[i].x >> 8) * 4;

					if(plugin->config.scratches[i].init)
					{
						y1 = plugin->config.scratches[i].init;
						plugin->config.scratches[i].init = 0;
					}
					else
						y1 = 0;

					plugin->config.scratches[i].life--;
					if(plugin->config.scratches[i].life)
						y2 = h;
					else
						y2 = EffectTV::fastrand() % h;

					for(y = y1; y < y2; y++)
					{
						a = p[1] & 0xfeff;
						a += 0x2000;
						b = a & 0x10000;
						p[1] = a | (b - (b >> 8));
						p[2] = 0x8000;
						p[3] = 0x8000;
						p += w * 4;
					}
				}
				else
				{
					if((EffectTV::fastrand() & 0xf0000000) == 0)
					{
						plugin->config.scratches[i].life = 2 + (EffectTV::fastrand() >> 27);
						plugin->config.scratches[i].x = EffectTV::fastrand() % (w_256);
						plugin->config.scratches[i].dx = ((int)EffectTV::fastrand()) >> 23;
						plugin->config.scratches[i].init = (EffectTV::fastrand() % (h - 1)) + 1;
					}
				}
			}
		}
		break;
	}
}


#define PITS(type, components, luma, chroma) \
{ \
	int i, j, k; \
	int pnum, size, pnumscale; \
	int x, y; \
 \
	pnumscale = plugin->config.area_scale * 2; \
 \
	if(plugin->config.pits_interval)  \
	{ \
		pnum = pnumscale + (EffectTV::fastrand() % pnumscale); \
		plugin->config.pits_interval--; \
	}  \
	else \
	{ \
		pnum = EffectTV::fastrand() % pnumscale; \
		if((EffectTV::fastrand() & 0xf8000000) == 0)  \
		{ \
			plugin->config.pits_interval = (EffectTV::fastrand() >> 28) + 20; \
		} \
	} \
 \
	for(i = 0; i < pnum; i++)  \
	{ \
		x = EffectTV::fastrand() % (w - 1); \
		y = EffectTV::fastrand() % (h - 1); \
 \
		size = EffectTV::fastrand() >> 28; \
 \
		for(j = 0; j < size; j++)  \
		{ \
			x = x + EffectTV::fastrand() % 3 - 1; \
			y = y + EffectTV::fastrand() % 3 - 1; \
 \
			CLAMP(x, 0, w - 1); \
			CLAMP(y, 0, h - 1); \
 \
			type *row = (type*)output_frame->get_row_ptr(row1 + y); \
 \
			for(k = 0; k < (chroma ? 1 : 3); k++) \
			{ \
				row[x * components + k] = luma; \
			} \
 \
			if(chroma) \
			{ \
				row[x * components + 1] = chroma; \
				row[x * components + 2] = chroma; \
			} \
 \
		} \
	} \
}

void AgingClient::pits(VFrame *output_frame,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();

	switch(output_frame->get_color_model())
	{
	case BC_RGB888:
		PITS(uint8_t, 3, 0xc0, 0);
		break;
	case BC_RGB_FLOAT:
		PITS(float, 3, (float)0xc0 / 0xff, 0);
		break;
	case BC_YUV888:
		PITS(uint8_t, 3, 0xc0, 0x80);
		break;
	case BC_RGBA_FLOAT:
		PITS(float, 4, (float)0xc0 / 0xff, 0);
		break;
	case BC_RGBA8888:
		PITS(uint8_t, 4, 0xc0, 0);
		break;
	case BC_YUVA8888:
		PITS(uint8_t, 4, 0xc0, 0x80);
		break;
	case BC_RGB161616:
		PITS(uint16_t, 3, 0xc000, 0);
		break;
	case BC_YUV161616:
		PITS(uint16_t, 3, 0xc000, 0x8000);
		break;
	case BC_RGBA16161616:
		PITS(uint16_t, 4, 0xc000, 0);
		break;
	case BC_YUVA16161616:
		PITS(uint16_t, 4, 0xc000, 0x8000);
		break;
	case BC_AYUV16161616:
		{
			int i, j, k;
			int pnum, size, pnumscale;
			int x, y;

			pnumscale = plugin->config.area_scale * 2;

			if(plugin->config.pits_interval)
			{
				pnum = pnumscale + (EffectTV::fastrand() % pnumscale);
				plugin->config.pits_interval--;
			}
			else
			{
				pnum = EffectTV::fastrand() % pnumscale;
				if((EffectTV::fastrand() & 0xf8000000) == 0)
					plugin->config.pits_interval = (EffectTV::fastrand() >> 28) + 20; \
			}

			for(i = 0; i < pnum; i++)
			{
				x = EffectTV::fastrand() % (w - 1);
				y = EffectTV::fastrand() % (h - 1);

				size = EffectTV::fastrand() >> 28;

				for(j = 0; j < size; j++)
				{
					x = x + EffectTV::fastrand() % 3 - 1;
					y = y + EffectTV::fastrand() % 3 - 1;

					CLAMP(x, 0, w - 1);
					CLAMP(y, 0, h - 1);
					uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1 + y);

					row[x * 4 + 1] = 0xc000;
					row[x * 4 + 2] = 0x8000;
					row[x * 4 + 2] = 0x8000;
				}
			}
		}
		break;
	}
}


#define DUSTS(type, components, luma, chroma) \
{ \
	int i, j, k; \
	int dnum; \
	int d, len; \
	int x, y; \
 \
	if(plugin->config.dust_interval == 0)  \
	{ \
		if((EffectTV::fastrand() & 0xf0000000) == 0)  \
		{ \
			plugin->config.dust_interval = EffectTV::fastrand() >> 29; \
		} \
		return; \
	} \
 \
	dnum = plugin->config.area_scale * 4 + (EffectTV::fastrand() >> 27); \
 \
	for(i = 0; i < dnum; i++)  \
	{ \
		x = EffectTV::fastrand() % w; \
		y = EffectTV::fastrand() % h; \
		d = EffectTV::fastrand() >> 29; \
		len = EffectTV::fastrand() % plugin->config.area_scale + 5; \
 \
		for(j = 0; j < len; j++)  \
		{ \
			CLAMP(x, 0, w - 1); \
			CLAMP(y, 0, h - 1); \
 \
			type *row = (type*)output_frame->get_row_ptr(row1 + y); \
 \
			for(k = 0; k < (chroma ? 1 : 3); k++) \
				row[x * components + k] = luma; \
 \
			if(chroma) \
			{ \
				row[x * components + 1] = chroma; \
				row[x * components + 2] = chroma; \
			} \
 \
			y += AgingConfig::dy[d]; \
			x += AgingConfig::dx[d]; \
 \
			if(x < 0 || x >= w) break; \
			if(y < 0 || y >= h) break; \
 \
 \
			d = (d + EffectTV::fastrand() % 3 - 1) & 7; \
		} \
	} \
	plugin->config.dust_interval--; \
}

void AgingClient::dusts(VFrame *output_frame,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();

	switch(output_frame->get_color_model())
	{
	case BC_RGB888:
		DUSTS(uint8_t, 3, 0x10, 0);
		break;

	case BC_RGB_FLOAT:
		DUSTS(float, 3, (float)0x10 / 0xff, 0);
		break;

	case BC_YUV888:
		DUSTS(uint8_t, 3, 0x10, 0x80);
		break;

	case BC_RGBA_FLOAT:
		DUSTS(float, 4, (float)0x10 / 0xff, 0);
		break;

	case BC_RGBA8888:
		DUSTS(uint8_t, 4, 0x10, 0);
		break;

	case BC_YUVA8888:
		DUSTS(uint8_t, 4, 0x10, 0x80);
		break;

	case BC_RGB161616:
		DUSTS(uint16_t, 3, 0x1000, 0);
		break;

	case BC_YUV161616:
		DUSTS(uint16_t, 3, 0x1000, 0x8000);
		break;

	case BC_RGBA16161616:
		DUSTS(uint16_t, 4, 0x1000, 0);
		break;

	case BC_YUVA16161616:
		DUSTS(uint16_t, 4, 0x1000, 0x8000);
		break;

	case BC_AYUV16161616:
		{
			int i, j, k;
			int dnum;
			int d, len;
			int x, y;

			if(plugin->config.dust_interval == 0)
			{
				if((EffectTV::fastrand() & 0xf0000000) == 0)
					plugin->config.dust_interval = EffectTV::fastrand() >> 29;
				return;
			}

			dnum = plugin->config.area_scale * 4 + (EffectTV::fastrand() >> 27);

			for(i = 0; i < dnum; i++)
			{
				x = EffectTV::fastrand() % w;
				y = EffectTV::fastrand() % h;
				d = EffectTV::fastrand() >> 29;
				len = EffectTV::fastrand() % plugin->config.area_scale + 5;

				for(j = 0; j < len; j++)
				{
					CLAMP(x, 0, w - 1);
					CLAMP(y, 0, h - 1);

					uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1 + y);

					row[x * 4 + 1] = 0x1000;
					row[x * 4 + 2] = 0x8000;
					row[x * 4 + 3] = 0x8000;

					y += AgingConfig::dy[d];
					x += AgingConfig::dx[d];

					if(x < 0 || x >= w) break;
					if(y < 0 || y >= h) break;

					d = (d + EffectTV::fastrand() % 3 - 1) & 7;
				}
			}
			plugin->config.dust_interval--;
		}
		break;
	}
}

void AgingClient::process_package(LoadPackage *package)
{
	AgingPackage *local_package = (AgingPackage*)package;

	if(plugin->config.colorage)
		coloraging(plugin->output_ptr,
			plugin->input_ptr,
			local_package->row1,
			local_package->row2);

	if(plugin->config.scratch)
		scratching(plugin->output_ptr,
			local_package->row1,
			local_package->row2);

	if(plugin->config.pits)
		pits(plugin->output_ptr,
			local_package->row1,
			local_package->row2);

	if(plugin->config.dust)
		dusts(plugin->output_ptr,
			local_package->row1,
			local_package->row2);
}


AgingPackage::AgingPackage()
{
}
