
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "downsample.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

REGISTER_PLUGIN

DownSampleConfig::DownSampleConfig()
{
	horizontal = 2;
	vertical = 2;
	horizontal_x = 0;
	vertical_y = 0;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int DownSampleConfig::equivalent(DownSampleConfig &that)
{
	return 
		horizontal == that.horizontal &&
		vertical == that.vertical &&
		horizontal_x == that.horizontal_x &&
		vertical_y == that.vertical_y &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void DownSampleConfig::copy_from(DownSampleConfig &that)
{
	horizontal = that.horizontal;
	vertical = that.vertical;
	horizontal_x = that.horizontal_x;
	vertical_y = that.vertical_y;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void DownSampleConfig::interpolate(DownSampleConfig &prev, 
	DownSampleConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->horizontal = (int)(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->vertical = (int)(prev.vertical * prev_scale + next.vertical * next_scale);
	this->horizontal_x = (int)(prev.horizontal_x * prev_scale + next.horizontal_x * next_scale);
	this->vertical_y = (int)(prev.vertical_y * prev_scale + next.vertical_y * next_scale);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}


PLUGIN_THREAD_METHODS

DownSampleWindow::DownSampleWindow(DownSampleMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	230, 
	380)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Horizontal")));
	y += 30;
	add_subwindow(h = new DownSampleSize(plugin, 
		x, 
		y, 
		&plugin->config.horizontal,
		1,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Horizontal offset")));
	y += 30;
	add_subwindow(h_x = new DownSampleSize(plugin, 
		x, 
		y, 
		&plugin->config.horizontal_x,
		0,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Vertical")));
	y += 30;
	add_subwindow(v = new DownSampleSize(plugin, 
		x, 
		y, 
		&plugin->config.vertical,
		1,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Vertical offset")));
	y += 30;
	add_subwindow(v_y = new DownSampleSize(plugin, 
		x, 
		y, 
		&plugin->config.vertical_y,
		0,
		100));
	y += 30;
	add_subwindow(r = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.r, 
		_("Red")));
	y += 30;
	add_subwindow(g = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.g, 
		_("Green")));
	y += 30;
	add_subwindow(b = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.b, 
		_("Blue")));
	y += 30;
	add_subwindow(a = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.a, 
		_("Alpha")));
	y += 30;

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DownSampleWindow::update()
{
	h->update(plugin->config.horizontal);
	v->update(plugin->config.vertical);
	h_x->update(plugin->config.horizontal_x);
	v_y->update(plugin->config.vertical_y);
	r->update(plugin->config.r);
	g->update(plugin->config.g);
	b->update(plugin->config.b);
	a->update(plugin->config.a);
}


DownSampleToggle::DownSampleToggle(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

DownSampleSize::DownSampleSize(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


DownSampleMain::DownSampleMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DownSampleMain::~DownSampleMain()
{
	if(engine) delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *DownSampleMain::process_tmpframe(VFrame *input_ptr)
{
	this->output = input_ptr;
	load_configuration();

// Process in destination
	if(!engine)
		engine = new DownSampleServer(this,
			get_project_smp() + 1,
			get_project_smp() + 1);
	engine->process_packages();

	if(config.a && ColorModels::has_alpha(output->get_color_model()))
		output->set_transparent();
	return output;
}

void DownSampleMain::load_defaults()
{
	defaults = load_defaults_file("downsample.rc");

	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.vertical = defaults->get("VERTICAL", config.vertical);
	config.horizontal_x = defaults->get("HORIZONTAL_X", config.horizontal_x);
	config.vertical_y = defaults->get("VERTICAL_Y", config.vertical_y);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
}

void DownSampleMain::save_defaults()
{
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("VERTICAL", config.vertical);
	defaults->update("HORIZONTAL_X", config.horizontal_x);
	defaults->update("VERTICAL_Y", config.vertical_y);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
}

void DownSampleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DOWNSAMPLE");

	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL_X", config.horizontal_x);
	output.tag.set_property("VERTICAL_Y", config.vertical_y);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/DOWNSAMPLE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DownSampleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DOWNSAMPLE"))
		{
			config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
			config.vertical = input.tag.get_property("VERTICAL", config.vertical);
			config.horizontal_x = input.tag.get_property("HORIZONTAL_X", config.horizontal_x);
			config.vertical_y = input.tag.get_property("VERTICAL_Y", config.vertical_y);
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}


DownSamplePackage::DownSamplePackage()
 : LoadPackage()
{
}


DownSampleUnit::DownSampleUnit(DownSampleServer *server, 
	DownSampleMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

#define SQR(x) ((x) * (x))


#define DOWNSAMPLE(type, temp_type, components, max) \
{ \
	temp_type r; \
	temp_type g; \
	temp_type b; \
	temp_type a; \
	int do_r = plugin->config.r; \
	int do_g = plugin->config.g; \
	int do_b = plugin->config.b; \
	int do_a = plugin->config.a; \
 \
	for(int i = pkg->y1; i < pkg->y2; i += plugin->config.vertical) \
	{ \
		int y1 = MAX(i, 0); \
		int y2 = MIN(i + plugin->config.vertical, h); \
 \
 \
		for(int j = plugin->config.horizontal_x - plugin->config.horizontal; \
			j < w; \
			j += plugin->config.horizontal) \
		{ \
			int x1 = MAX(j, 0); \
			int x2 = MIN(j + plugin->config.horizontal, w); \
 \
			temp_type scale = (x2 - x1) * (y2 - y1); \
			if(x2 > x1 && y2 > y1) \
			{ \
 \
/* Read in values */ \
				r = 0; \
				g = 0; \
				b = 0; \
				if(components == 4) a = 0; \
 \
				for(int k = y1; k < y2; k++) \
				{ \
					type *row = (type*)plugin->output->get_row_ptr(k) + \
						x1 * components; \
					for(int l = x1; l < x2; l++) \
					{ \
						if(do_r) r += *row++; else row++; \
						if(do_g) g += *row++; else row++;  \
						if(do_b) b += *row++; else row++;  \
						if(components == 4) if(do_a) a += *row++; else row++;  \
					} \
				} \
 \
/* Write average */ \
				r /= scale; \
				g /= scale; \
				b /= scale; \
				if(components == 4) a /= scale; \
				for(int k = y1; k < y2; k++) \
				{ \
					type *row = (type*)plugin->output->get_row_ptr(k) + \
						x1 * components; \
					for(int l = x1; l < x2; l++) \
					{ \
						if(do_r) *row++ = r; else row++; \
						if(do_g) *row++ = g; else row++; \
						if(do_b) *row++ = b; else row++; \
						if(components == 4) if(do_a) *row++ = a; else row++; \
					} \
				} \
			} \
		} \
	} \
}

void DownSampleUnit::process_package(LoadPackage *package)
{
	DownSamplePackage *pkg = (DownSamplePackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();

	switch(plugin->output->get_color_model())
	{
	case BC_RGB888:
		DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
		break;
	case BC_RGB_FLOAT:
		DOWNSAMPLE(float, float, 3, 1.0)
		break;
	case BC_RGBA8888:
		DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
		break;
	case BC_RGBA_FLOAT:
		DOWNSAMPLE(float, float, 4, 1.0)
		break;
	case BC_RGB161616:
		DOWNSAMPLE(uint16_t, int64_t, 3, 0xffff)
		break;
	case BC_RGBA16161616:
		DOWNSAMPLE(uint16_t, int64_t, 4, 0xffff)
		break;
	case BC_YUV888:
		DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
		break;
	case BC_YUVA8888:
		DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
		break;
	case BC_YUV161616:
		DOWNSAMPLE(uint16_t, int64_t, 3, 0xffff)
		break;
	case BC_YUVA16161616:
		DOWNSAMPLE(uint16_t, int64_t, 4, 0xffff)
		break;
	case BC_AYUV16161616:
		{
			int64_t r;
			int64_t g;
			int64_t b;
			int64_t a;
			int do_r = plugin->config.r;
			int do_g = plugin->config.g;
			int do_b = plugin->config.b;
			int do_a = plugin->config.a;

			for(int i = pkg->y1; i < pkg->y2; i += plugin->config.vertical)
			{
				int y1 = MAX(i, 0);
				int y2 = MIN(i + plugin->config.vertical, h);

				for(int j = plugin->config.horizontal_x - plugin->config.horizontal;
					j < w; j += plugin->config.horizontal)
				{
					int x1 = MAX(j, 0);
					int x2 = MIN(j + plugin->config.horizontal, w);

					int64_t scale = (x2 - x1) * (y2 - y1);

					if(x2 > x1 && y2 > y1)
					{
						// Read in values
						r = g = b = a = 0;
						for(int k = y1; k < y2; k++)
						{
							uint16_t *row = ((uint16_t*)plugin->output->get_row_ptr(k)) + x1 * 4;

							for(int l = x1; l < x2; l++)
							{
								if(do_a) a += *row++; else row++;
								if(do_r) r += *row++; else row++;
								if(do_g) g += *row++; else row++;
								if(do_b) b += *row++; else row++;
							}
						}

						// Write average
						r /= scale;
						g /= scale;
						b /= scale;
						a /= scale;

						for(int k = y1; k < y2; k++)
						{
							uint16_t *row = ((uint16_t*)plugin->output->get_row_ptr(k)) + x1 * 4;
							for(int l = x1; l < x2; l++)
							{
								if(do_a) *row++ = a; else row++;
								if(do_r) *row++ = r; else row++;
								if(do_g) *row++ = g; else row++;
								if(do_b) *row++ = b; else row++;
							}
						}
					}
				}
			}
		}
		break;
	}
}


DownSampleServer::DownSampleServer(DownSampleMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void DownSampleServer::init_packages()
{
	int y1 = plugin->config.vertical_y - plugin->config.vertical;
	int total_strips = (int)((float)plugin->output->get_h() / plugin->config.vertical + 1);
	int strips_per_package = (int)((float)total_strips / get_total_packages() + 1);

	for(int i = 0; i < get_total_packages(); i++)
	{
		DownSamplePackage *package = (DownSamplePackage*)get_package(i);
		package->y1 = y1;
		package->y2 = y1 + strips_per_package * plugin->config.vertical;
		package->y1 = MIN(plugin->output->get_h(), package->y1);
		package->y2 = MIN(plugin->output->get_h(), package->y2);
		y1 = package->y2;
	}
}

LoadClient* DownSampleServer::new_client()
{
	return new DownSampleUnit(this, plugin);
}

LoadPackage* DownSampleServer::new_package()
{
	return new DownSamplePackage;
}
