
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

#include "bchash.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "bcslider.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "oil.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>


// Algorithm by Torsten Martinsen
// Ported to Cinelerra by Heroine Virtual Ltd.


OilConfig::OilConfig()
{
	radius = 5;
	use_intensity = 0;
}

void OilConfig::copy_from(OilConfig &src)
{
	this->radius = src.radius;
	this->use_intensity = src.use_intensity;
}

int OilConfig::equivalent(OilConfig &src)
{
	return (EQUIV(this->radius, src.radius) &&
		this->use_intensity == src.use_intensity);
}

void OilConfig::interpolate(OilConfig &prev, 
		OilConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->use_intensity = prev.use_intensity;
}


OilRadius::OilRadius(OilEffect *plugin, int x, int y)
 : BC_FSlider(x, 
		y, 
		0,
		200,
		200, 
		(float)0, 
		(float)30,
		plugin->config.radius)
{
	this->plugin = plugin;
}
int OilRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}


OilIntensity::OilIntensity(OilEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_intensity, _("Use intensity"))
{
	this->plugin = plugin;
}

int OilIntensity::handle_event()
{
	plugin->config.use_intensity = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

OilWindow::OilWindow(OilEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	300, 
	160)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Radius:")));
	add_subwindow(radius = new OilRadius(plugin, x + 70, y));
	y += 40;
	add_subwindow(intensity = new OilIntensity(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void OilWindow::update()
{
	radius->update(plugin->config.radius);
	intensity->update(plugin->config.use_intensity);
}

REGISTER_PLUGIN

OilEffect::OilEffect(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

OilEffect::~OilEffect()
{
	if(engine) delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void OilEffect::load_defaults()
{
	defaults = load_defaults_file("oilpainting.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.use_intensity = defaults->get("USE_INTENSITY", config.use_intensity);
}

void OilEffect::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("USE_INTENSITY", config.use_intensity);
	defaults->save();
}

void OilEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("OIL_PAINTING");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("USE_INTENSITY", config.use_intensity);
	output.append_tag();
	output.tag.set_title("/OIL_PAINTING");
	output.append_tag();
	keyframe->set_data(output.string);
}

void OilEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OIL_PAINTING"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.use_intensity = input.tag.get_property("USE_INTENSITY", config.use_intensity);
		}
	}
}

VFrame *OilEffect::process_tmpframe(VFrame *input)
{
	need_reconfigure |= load_configuration();

	this->input = input;
	output = input;

	if(!EQUIV(config.radius, 0))
	{
		output = clone_vframe(input);

		if(!engine)
		{
			engine = new OilServer(this, (PluginClient::smp + 1));
		}
		engine->process_packages();
	}
	return output;
}


OilPackage::OilPackage()
 : LoadPackage()
{
}


OilUnit::OilUnit(OilEffect *plugin, OilServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

#define SUBSCRIPT(type, value) \
	if(sizeof(type) == 4) \
	{ \
		subscript = (int)(value * 255); \
		CLAMP(subscript, 0, 255); \
	} \
	else if(sizeof(type) == 2) \
		subscript = (int)value >> 8; \
	else \
		subscript = (int)value;

#define INTENSITY(p) ((unsigned int)(((p)[0]) * 77+ \
				((p)[1] * 150) + \
				((p)[2] * 29)) >> 8)


#define OIL_MACRO(type, hist_size, components) \
{ \
	type *src, *dest; \
	type val[components]; \
	int count[components], count2; \
	int *hist[components]; \
	int *hist2; \
 \
	for(int i = 0; i < components; i++) \
		hist[i] = new int[hist_size + 1]; \
	hist2 = new int[hist_size + 1]; \
 \
	for(int y1 = pkg->row1; y1 < pkg->row2; y1++) \
	{ \
		dest = (type*)plugin->output->get_row_ptr(y1); \
 \
		if(!plugin->config.use_intensity) \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				memset(count, 0, sizeof(count)); \
				memset(val, 0, sizeof(val)); \
				memset(hist[0], 0, sizeof(int) * (hist_size + 1)); \
				memset(hist[1], 0, sizeof(int) * (hist_size + 1)); \
				memset(hist[2], 0, sizeof(int) * (hist_size + 1)); \
				if (components == 4) memset(hist[3], 0, sizeof(int) * (hist_size + 1)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
				int y3 = CLIP((y1 - n), 0, h - 1); \
				int x4 = CLIP((x1 + n + 1), 0, w - 1); \
				int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = (type*)plugin->input->get_row_ptr(y2); \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						int subscript; \
						type value; \
 \
						value = src[x2 * components + 0]; \
						SUBSCRIPT(type, value) \
 \
						if((c = ++hist[0][subscript]) > count[0]) \
						{ \
							val[0] = value; \
							count[0] = c; \
						} \
 \
						value = src[x2 * components + 1]; \
						SUBSCRIPT(type, value) \
 \
						if((c = ++hist[1][subscript]) > count[1]) \
						{ \
							val[1] = value; \
							count[1] = c; \
						} \
 \
						value = src[x2 * components + 2]; \
						SUBSCRIPT(type, value) \
 \
						if((c = ++hist[2][subscript]) > count[2]) \
						{ \
							val[2] = value; \
							count[2] = c; \
						} \
 \
						if(components == 4) \
						{ \
							value = src[x2 * components + 3]; \
							SUBSCRIPT(type, value) \
 \
							if((c = ++hist[3][subscript]) > count[3]) \
							{ \
								val[3] = value; \
								count[3] = c; \
							} \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = val[0]; \
				dest[x1 * components + 1] = val[1]; \
				dest[x1 * components + 2] = val[2]; \
				if(components == 4) dest[x1 * components + 3] = val[3]; \
			} \
		} \
		else \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				count2 = 0; \
				memset(val, 0, sizeof(val)); \
				memset(hist2, 0, sizeof(int) * (hist_size + 1)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
				int y3 = CLIP((y1 - n), 0, h - 1); \
				int x4 = CLIP((x1 + n + 1), 0, w - 1); \
				int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = (type*)plugin->input->get_row_ptr(y2); \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						int ix = INTENSITY(src + x2 * components); \
						if(sizeof(type) >= 2) \
							ix >>= 8; \
						if((c = ++hist2[ix]) > count2) \
						{ \
							val[0] = src[x2 * components + 0]; \
							val[1] = src[x2 * components + 1]; \
							val[2] = src[x2 * components + 2]; \
							if(components == 4) val[3] = src[x2 * components + 3]; \
							count2 = c; \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = val[0]; \
				dest[x1 * components + 1] = val[1]; \
				dest[x1 * components + 2] = val[2]; \
				if(components == 4) dest[x1 * components + 3] = val[3]; \
			} \
		} \
	} \
 \
	for(int i = 0; i < components; i++) \
		delete [] hist[i]; \
	delete [] hist2; \
}


void OilUnit::process_package(LoadPackage *package)
{
	OilPackage *pkg = (OilPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int n = (int)(plugin->config.radius / 2);

	switch(plugin->input->get_color_model())
	{
	case BC_RGB_FLOAT:
		OIL_MACRO(float, 0xff, 3)
		break;
	case BC_RGB888:
	case BC_YUV888:
		OIL_MACRO(unsigned char, 0xff, 3)
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		OIL_MACRO(uint16_t, 0xff, 3)
		break;
	case BC_RGBA_FLOAT:
		OIL_MACRO(float, 0xff, 4)
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		OIL_MACRO(unsigned char, 0xff, 4)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		OIL_MACRO(uint16_t, 0xff, 4)
		break;
	}
}


OilServer::OilServer(OilEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void OilServer::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		OilPackage *pkg = (OilPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* OilServer::new_client()
{
	return new OilUnit(plugin, this);
}

LoadPackage* OilServer::new_package()
{
	return new OilPackage;
}
