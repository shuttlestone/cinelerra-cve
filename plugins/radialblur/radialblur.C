
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

#include "affine.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "radialblur.h"
#include "vframe.h"


REGISTER_PLUGIN

RadialBlurConfig::RadialBlurConfig()
{
	x = 50;
	y = 50;
	steps = 10;
	angle = 33;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int RadialBlurConfig::equivalent(RadialBlurConfig &that)
{
	return 
		angle == that.angle &&
		x == that.x &&
		y == that.y &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void RadialBlurConfig::copy_from(RadialBlurConfig &that)
{
	x = that.x;
	y = that.y;
	angle = that.angle;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void RadialBlurConfig::interpolate(RadialBlurConfig &prev, 
	RadialBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}

PLUGIN_THREAD_METHODS

RadialBlurWindow::RadialBlurWindow(RadialBlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	230, 
	340)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("X:")));
	y += 20;
	add_subwindow(this->x = new RadialBlurSize(plugin, x, y, &plugin->config.x, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y += 20;
	add_subwindow(this->y = new RadialBlurSize(plugin, x, y, &plugin->config.y, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	y += 20;
	add_subwindow(angle = new RadialBlurSize(plugin, x, y, &plugin->config.angle, 0, 360));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new RadialBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	y += 30;
	add_subwindow(r = new RadialBlurToggle(plugin, x, y, &plugin->config.r, _("Red")));
	y += 30;
	add_subwindow(g = new RadialBlurToggle(plugin, x, y, &plugin->config.g, _("Green")));
	y += 30;
	add_subwindow(b = new RadialBlurToggle(plugin, x, y, &plugin->config.b, _("Blue")));
	y += 30;
	add_subwindow(a = new RadialBlurToggle(plugin, x, y, &plugin->config.a, _("Alpha")));
	y += 30;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RadialBlurWindow::update()
{
	x->update(plugin->config.x);
	y->update(plugin->config.y);
	angle->update(plugin->config.angle);
	steps->update(plugin->config.steps);
	r->update(plugin->config.r);
	g->update(plugin->config.g);
	b->update(plugin->config.b);
	a->update(plugin->config.a);
}


RadialBlurToggle::RadialBlurToggle(RadialBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int RadialBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


RadialBlurSize::RadialBlurSize(RadialBlurMain *plugin, 
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

int RadialBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


RadialBlurMain::RadialBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	temp = 0;
	rotate = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

RadialBlurMain::~RadialBlurMain()
{
	if(engine) delete engine;
	if(temp) delete temp;
	delete rotate;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *RadialBlurMain::process_tmpframe(VFrame *frame)
{
	load_configuration();

	if(!engine)
		engine = new RadialBlurEngine(this,
			get_project_smp() + 1,
			get_project_smp() + 1);

	input = frame;
	output = clone_vframe(frame);
	engine->process_packages();
	release_vframe(input);
	return output;
}

void RadialBlurMain::load_defaults()
{
	defaults = load_defaults_file("radialblur.rc");

	config.x = defaults->get("X", config.x);
	config.y = defaults->get("Y", config.y);
	config.angle = defaults->get("ANGLE", config.angle);
	config.steps = defaults->get("STEPS", config.steps);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
}

void RadialBlurMain::save_defaults()
{
	defaults->update("X", config.x);
	defaults->update("Y", config.y);
	defaults->update("ANGLE", config.angle);
	defaults->update("STEPS", config.steps);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
}

void RadialBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("RADIALBLUR");
	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/RADIALBLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void RadialBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("RADIALBLUR"))
		{
			config.x = input.tag.get_property("X", config.x);
			config.y = input.tag.get_property("Y", config.y);
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.steps = input.tag.get_property("STEPS", config.steps);
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}

void RadialBlurMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);


	int is_yuv = ColorModels::is_yuv(get_output()->get_color_model());
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

// Draw unselected channels
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDrawBuffer(GL_BACK);
	if(!config.r || !config.g || !config.b || !config.a)
	{
		glColor4f(config.r ? 0 : 1, 
			config.g ? 0 : 1, 
			config.b ? 0 : 1, 
			config.a ? 0 : 1);
		get_output()->draw_texture();
	}
	glAccum(GL_LOAD, 1.0);

// Blur selected channels
	float fraction = 1.0 / config.steps;
	for(int i = 0; i < config.steps; i++)
	{
		get_output()->set_opengl_state(VFrame::TEXTURE);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);

		float w = get_output()->get_w();
		float h = get_output()->get_h();

		double current_angle = (double)config.angle *
			i / 
			config.steps - 
			(double)config.angle / 2;

		if(!rotate) rotate = new AffineEngine(PluginClient::smp + 1, 
			PluginClient::smp + 1);
		rotate->set_pivot((int)(config.x * w / 100),
			(int)(config.y * h / 100));
		rotate->set_opengl(1);
		rotate->rotate(get_output(),
			get_output(),
			current_angle);

		glAccum(GL_ACCUM, fraction);
		glEnable(GL_TEXTURE_2D);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);
	}

	glDisable(GL_BLEND);
	glReadBuffer(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


RadialBlurPackage::RadialBlurPackage()
 : LoadPackage()
{
}


RadialBlurUnit::RadialBlurUnit(RadialBlurEngine *server, 
	RadialBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

#define BLEND_LAYER(COMPONENTS, TYPE, TEMP_TYPE, MAX, DO_YUV) \
{ \
	int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	int steps = plugin->config.steps; \
	double step = (double)plugin->config.angle / 360 * 2 * M_PI / steps; \
 \
	for(int i = pkg->y1, out_y = pkg->y1 - center_y; \
		i < pkg->y2; \
		i++, out_y++) \
	{ \
		TYPE *out_row = (TYPE*)plugin->output->get_row_ptr(i); \
		TYPE *in_row = (TYPE*)plugin->input->get_row_ptr(i); \
		int y_square = out_y * out_y; \
 \
		for(int j = 0, out_x = -center_x; j < w; j++, out_x++) \
		{ \
			double offset = 0; \
			TEMP_TYPE accum_r = 0; \
			TEMP_TYPE accum_g = 0; \
			TEMP_TYPE accum_b = 0; \
			TEMP_TYPE accum_a = 0; \
 \
/* Output coordinate to polar */ \
			double magnitude = sqrt(y_square + out_x * out_x); \
			double angle; \
			if(out_y < 0) \
				angle = atan((double)out_x / out_y) + M_PI; \
			else \
			if(out_y > 0) \
				angle = atan((double)out_x / out_y); \
			else \
			if(out_x > 0) \
				angle = M_PI / 2; \
			else \
				angle = M_PI * 1.5; \
 \
/* Overlay all steps on this pixel*/ \
			angle -= (double)plugin->config.angle / 360 * M_PI; \
			for(int k = 0; k < steps; k++, angle += step) \
			{ \
/* Polar to input coordinate */ \
				int in_x = (int)(magnitude * sin(angle)) + center_x; \
				int in_y = (int)(magnitude * cos(angle)) + center_y; \
				TYPE *in_row = (TYPE*)plugin->input->get_row_ptr(in_y); \
 \
/* Accumulate input coordinate */ \
				if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h) \
				{ \
					accum_r += in_row[in_x * COMPONENTS]; \
					if(DO_YUV) \
					{ \
						accum_g += (int)in_row[in_x * COMPONENTS + 1]; \
						accum_b += (int)in_row[in_x * COMPONENTS + 2]; \
					} \
					else \
					{ \
						accum_g += in_row[in_x * COMPONENTS + 1]; \
						accum_b += in_row[in_x * COMPONENTS + 2]; \
					} \
					if(COMPONENTS == 4) \
						accum_a += in_row[in_x * COMPONENTS + 3]; \
				} \
				else \
				{ \
					accum_g += chroma_offset; \
					accum_b += chroma_offset; \
				} \
			} \
 \
/* Accumulation to output */ \
			if(do_r) \
			{ \
				*out_row++ = (accum_r * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_g) \
			{ \
				if(DO_YUV) \
					*out_row++ = ((accum_g * fraction) / 0x10000); \
				else \
					*out_row++ = (accum_g * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_b) \
			{ \
				if(DO_YUV) \
					*out_row++ = (accum_b * fraction) / 0x10000; \
				else \
					*out_row++ = (accum_b * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(COMPONENTS == 4) \
			{ \
				if(do_a) \
				{ \
					*out_row++ = (accum_a * fraction) / 0x10000; \
					in_row++; \
				} \
				else \
				{ \
					*out_row++ = *in_row++; \
				} \
			} \
		} \
	} \
}

void RadialBlurUnit::process_package(LoadPackage *package)
{
	RadialBlurPackage *pkg = (RadialBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;
	int fraction = 0x10000 / plugin->config.steps;
	int center_x = plugin->config.x * w / 100;
	int center_y = plugin->config.y * h / 100;

	switch(plugin->input->get_color_model())
	{
	case BC_RGB888:
		BLEND_LAYER(3, uint8_t, int, 0xff, 0)
		break;
	case BC_RGBA8888:
		BLEND_LAYER(4, uint8_t, int, 0xff, 0)
		break;
	case BC_RGB_FLOAT:
		BLEND_LAYER(3, float, float, 1, 0)
		break;
	case BC_RGBA_FLOAT:
		BLEND_LAYER(4, float, float, 1, 0)
		break;
	case BC_RGB161616:
		BLEND_LAYER(3, uint16_t, int, 0xffff, 0)
		break;
	case BC_RGBA16161616:
		BLEND_LAYER(4, uint16_t, int, 0xffff, 0)
		break;
	case BC_YUV888:
		BLEND_LAYER(3, uint8_t, int, 0xff, 1)
		break;
	case BC_YUVA8888:
		BLEND_LAYER(4, uint8_t, int, 0xff, 1)
		break;
	case BC_YUV161616:
		BLEND_LAYER(3, uint16_t, int, 0xffff, 1)
		break;
	case BC_YUVA16161616:
		BLEND_LAYER(4, uint16_t, int, 0xffff, 1)
		break;
	case BC_AYUV16161616:
		{
			int chroma_offset = 0x8000;
			int steps = plugin->config.steps;
			double step = (double)plugin->config.angle / 360 * 2 * M_PI / steps;

			for(int i = pkg->y1, out_y = pkg->y1 - center_y;
					i < pkg->y2; i++, out_y++)
			{
				uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(i);
				uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(i);
				int y_square = out_y * out_y;

				for(int j = 0, out_x = -center_x; j < w; j++, out_x++)
				{
					double offset = 0;
					int accum_r = 0;
					int accum_g = 0;
					int accum_b = 0;
					int accum_a = 0;

					// Output coordinate to polar
					double magnitude = sqrt(y_square + out_x * out_x);
					double angle;
					if(out_y < 0)
						angle = atan((double)out_x / out_y) + M_PI;
					else
					if(out_y > 0)
						angle = atan((double)out_x / out_y);
					else
					if(out_x > 0)
						angle = M_PI / 2;
					else
						angle = M_PI * 1.5;

					// Overlay all steps on this pixel
					angle -= (double)plugin->config.angle / 360 * M_PI;
					for(int k = 0; k < steps; k++, angle += step)
					{
						// Polar to input coordinate
						int in_x = (int)(magnitude * sin(angle)) + center_x;
						int in_y = (int)(magnitude * cos(angle)) + center_y;
						uint16_t *row = ((uint16_t*)plugin->input->get_row_ptr(in_y));

						// Accumulate input coordinate
						if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h)
						{
							accum_a += row[in_x * 4];
							accum_r += row[in_x * 4 + 1];
							accum_g += row[in_x * 4 + 2];
							accum_b += row[in_x * 4 + 3];
						}
						else
						{
							accum_g += chroma_offset;
							accum_b += chroma_offset;
						}
					}

					// Accumulation to output
					if(do_a)
					{
						*out_row++ = (accum_a * fraction) / 0x10000;
						in_row++;
					}
					else
					{
						*out_row++ = *in_row++;
					}

					if(do_r)
					{
						*out_row++ = (accum_r * fraction) / 0x10000;
						in_row++;
					}
					else
						*out_row++ = *in_row++;

					if(do_g)
					{
						*out_row++ = (accum_g * fraction) / 0x10000; \
						in_row++;
					}
					else
						*out_row++ = *in_row++; \

					if(do_b)
					{
						*out_row++ = (accum_b * fraction) / 0x10000; \
						in_row++;
					}
					else
						*out_row++ = *in_row++;
				}
			}
		}
		break;
	}
}

RadialBlurEngine::RadialBlurEngine(RadialBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void RadialBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		RadialBlurPackage *package = (RadialBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* RadialBlurEngine::new_client()
{
	return new RadialBlurUnit(this, plugin);
}

LoadPackage* RadialBlurEngine::new_package()
{
	return new RadialBlurPackage;
}
