
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

#define GL_GLEXT_PROTOTYPES

#include "bcsignals.h"
#include "chromakey.h"
#include "colorspaces.h"
#include "clip.h"
#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


ChromaKeyConfig::ChromaKeyConfig()
{
	red = 0.0;
	green = 0.0;
	blue = 0.0;
	threshold = 60.0;
	use_value = 0;
	slope = 100;
}

void ChromaKeyConfig::copy_from(ChromaKeyConfig &src)
{
	red = src.red;
	green = src.green;
	blue = src.blue;
	threshold = src.threshold;
	use_value = src.use_value;
	slope = src.slope;
}

int ChromaKeyConfig::equivalent(ChromaKeyConfig &src)
{
	return (EQUIV(red, src.red) &&
		EQUIV(green, src.green) &&
		EQUIV(blue, src.blue) &&
		EQUIV(threshold, src.threshold) &&
		EQUIV(slope, src.slope) &&
		use_value == src.use_value);
}

void ChromaKeyConfig::interpolate(ChromaKeyConfig &prev, 
	ChromaKeyConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->red = prev.red * prev_scale + next.red * next_scale;
	this->green = prev.green * prev_scale + next.green * next_scale;
	this->blue = prev.blue * prev_scale + next.blue * next_scale;
	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
	this->use_value = prev.use_value;
}

int ChromaKeyConfig::get_color()
{
	int red = (int)(CLIP(this->red, 0, 1) * 0xff);
	int green = (int)(CLIP(this->green, 0, 1) * 0xff);
	int blue = (int)(CLIP(this->blue, 0, 1) * 0xff);
	return (red << 16) | (green << 8) | blue;
}


ChromaKeyWindow::ChromaKeyWindow(ChromaKey *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	320, 
	220)
{
	BC_Title *title;
	int x1 = 100;

	x = 10;
	y = 10;

	add_subwindow(title = new BC_Title(x, y, _("Color:")));
	x += title->get_w() + 10;
	add_subwindow(color = new ChromaKeyColor(plugin, this, x, y));
	x += color->get_w() + 10;
	add_subwindow(sample = new BC_SubWindow(x, y, 100, 50));
	y += sample->get_h() + 10;
	x = 10;

	add_subwindow(new BC_Title(x, y, _("Slope:")));
	add_subwindow(slope = new ChromaKeySlope(plugin, x1, y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	add_subwindow(threshold = new ChromaKeyThreshold(plugin, x1, y));

	y += 30;
	add_subwindow(use_value = new ChromaKeyUseValue(plugin, x1, y));

	y += 30;
	add_subwindow(use_colorpicker = new ChromaKeyUseColorPicker(plugin, this, x1, y));

	color_thread = new ChromaKeyColorThread(plugin, this);

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_sample();
}

ChromaKeyWindow::~ChromaKeyWindow()
{
	delete color_thread;
}

void ChromaKeyWindow::update_sample()
{
	sample->set_color(plugin->config.get_color());
	sample->draw_box(0, 
		0, 
		sample->get_w(), 
		sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 
		0, 
		sample->get_w(), 
		sample->get_h());
	sample->flash();
}

void ChromaKeyWindow::update()
{
	threshold->update(plugin->config.threshold);
	slope->update(plugin->config.slope);
	use_value->update(plugin->config.use_value);
	update_sample();
}


ChromaKeyColor::ChromaKeyColor(ChromaKey *plugin, 
	ChromaKeyWindow *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, 
	y,
	_("Color..."))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyColor::handle_event()
{
	gui->color_thread->start_window(
		plugin->config.get_color(),
		0xff);
	return 1;
}


ChromaKeyThreshold::ChromaKeyThreshold(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
		y,
		0,
		200,
		200,
		(float)0,
		(float)100,
		plugin->config.threshold)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeySlope::ChromaKeySlope(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
		y,
		0,
		200,
		200,
		(float)0,
		(float)100,
		plugin->config.slope)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeySlope::handle_event()
{
	plugin->config.slope = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyUseValue::ChromaKeyUseValue(ChromaKey *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_value, _("Use value"))
{
	this->plugin = plugin;
}

int ChromaKeyUseValue::handle_event()
{
	plugin->config.use_value = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyUseColorPicker::ChromaKeyUseColorPicker(ChromaKey *plugin, 
	ChromaKeyWindow *gui,
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Use color picker"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyUseColorPicker::handle_event()
{
	plugin->get_picker_colors(&plugin->config.red, &plugin->config.green,
		&plugin->config.blue);
	gui->update_sample();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyColorThread::ChromaKeyColorThread(ChromaKey *plugin, ChromaKeyWindow *gui)
 : ColorThread(1, _("Inner color"), plugin->new_picon())
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.red = (float)(output & 0xff0000) / 0xff0000;
	plugin->config.green = (float)(output & 0xff00) / 0xff00;
	plugin->config.blue = (float)(output & 0xff) / 0xff;
	gui->update_sample();
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


ChromaKeyServer::ChromaKeyServer(ChromaKey *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
{
	this->plugin = plugin;
}

void ChromaKeyServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ChromaKeyPackage *pkg = (ChromaKeyPackage*)get_package(i);
		pkg->y1 = plugin->input->get_h() * i / get_total_packages();
		pkg->y2 = plugin->input->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ChromaKeyServer::new_client()
{
	return new ChromaKeyUnit(plugin, this);
}

LoadPackage* ChromaKeyServer::new_package()
{
	return new ChromaKeyPackage;
}


ChromaKeyPackage::ChromaKeyPackage()
 : LoadPackage()
{
}

ChromaKeyUnit::ChromaKeyUnit(ChromaKey *plugin, ChromaKeyServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


void ChromaKeyUnit::process_package(LoadPackage *package)
{
	ChromaKeyPackage *pkg = (ChromaKeyPackage*)package;

	int w = plugin->input->get_w();

	float h, s, v;
	ColorSpaces::rgb_to_hsv(plugin->config.red,
		plugin->config.green,
		plugin->config.blue,
		h,
		s,
		v);
	float min_hue = h - plugin->config.threshold * 360 / 100;
	float max_hue = h + plugin->config.threshold * 360 / 100;


#define RGB_TO_VALUE(r, g, b) \
((r) * R_TO_Y + (g) * G_TO_Y + (b) * B_TO_Y)

#define SQR(x) ((x) * (x))

#define OUTER_VARIABLES(plugin) \
	float value = RGB_TO_VALUE(plugin->config.red, \
		plugin->config.green, \
		plugin->config.blue); \
	float threshold = plugin->config.threshold / 100; \
	float min_v = value - threshold; \
	float max_v = value + threshold; \
	float r_key = plugin->config.red; \
	float g_key = plugin->config.green; \
	float b_key = plugin->config.blue; \
	int y_key, u_key, v_key; \
	ColorSpaces::rgb_to_yuv_8((int)(r_key * 0xff), (int)(g_key * 0xff), (int)(b_key * 0xff), y_key, u_key, v_key); \
	float run = plugin->config.slope / 100; \
	float threshold_run = threshold + run;

	OUTER_VARIABLES(plugin)



#define CHROMAKEY(type, components, max, use_yuv) \
{ \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *row = (type*)plugin->input->get_row_ptr(i); \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float a = 1; \
 \
/* Test for value in range */ \
			if(plugin->config.use_value) \
			{ \
				float current_value; \
				if(use_yuv) \
				{ \
					float r = (float)row[0] / max; \
					current_value = r; \
				} \
				else \
				{ \
					float r = (float)row[0] / max; \
					float g = (float)row[1] / max; \
					float b = (float)row[2] / max; \
					current_value = RGB_TO_VALUE(r, g, b); \
				} \
 \
/* Full transparency if in range */ \
				if(current_value >= min_v && current_value < max_v) \
				{ \
					a = 0; \
				} \
				else \
/* Phased out if below or above range */ \
				if(current_value < min_v) \
				{ \
					if(min_v - current_value < run) \
						a = (min_v - current_value) / run; \
				} \
				else \
				if(current_value - max_v < run) \
					a = (current_value - max_v) / run; \
			} \
			else \
/* Use color cube */ \
			{ \
				float difference; \
				if(use_yuv) \
				{ \
					type y = row[0]; \
					type u = row[1]; \
					type v = row[2]; \
					difference = sqrt(SQR(y - y_key) + \
						SQR(u - u_key) + \
						SQR(v - v_key)) / max; \
				} \
				else \
				{ \
					float r = (float)row[0] / max; \
					float g = (float)row[1] / max; \
					float b = (float)row[2] / max; \
					difference = sqrt(SQR(r - r_key) +  \
						SQR(g - g_key) + \
						SQR(b - b_key)); \
				} \
				if(difference < threshold) \
				{ \
					a = 0; \
				} \
				else \
				if(difference < threshold_run) \
				{ \
					a = (difference - threshold) / run; \
				} \
 \
			} \
 \
/* Multiply alpha and put back in frame */ \
			if(components == 4) \
			{ \
				row[3] = MIN((type)(a * max), row[3]); \
			} \
			else \
			if(use_yuv) \
			{ \
				row[0] = (type)(a * row[0]); \
				row[1] = (type)(a * (row[1] - (max / 2 + 1)) + max / 2 + 1); \
				row[2] = (type)(a * (row[2] - (max / 2 + 1)) + max / 2 + 1); \
			} \
			else \
			{ \
				row[0] = (type)(a * row[0]); \
				row[1] = (type)(a * row[1]); \
				row[2] = (type)(a * row[2]); \
			} \
 \
			row += components; \
		} \
	} \
}

	switch(plugin->input->get_color_model())
	{
	case BC_RGB_FLOAT:
		CHROMAKEY(float, 3, 1.0, 0);
		break;
	case BC_RGBA_FLOAT:
		CHROMAKEY(float, 4, 1.0, 0);
		break;
	case BC_RGB888:
		CHROMAKEY(unsigned char, 3, 0xff, 0);
		break;
	case BC_RGBA8888:
		CHROMAKEY(unsigned char, 4, 0xff, 0);
		break;
	case BC_YUV888:
		CHROMAKEY(unsigned char, 3, 0xff, 1);
		break;
	case BC_YUVA8888:
		CHROMAKEY(unsigned char, 4, 0xff, 1);
		break;
	case BC_YUV161616:
		CHROMAKEY(uint16_t, 3, 0xffff, 1);
		break;
	case BC_YUVA16161616:
		CHROMAKEY(uint16_t, 4, 0xffff, 1);
		break;

	case BC_RGBA16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			uint16_t *row = (uint16_t*)plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				double a = 1;

				// Test for value in range
				if(plugin->config.use_value)
				{
					double current_value;

					double r = (double)row[0] / 0xffff;
					double g = (double)row[1] / 0xffff;
					double b = (double)row[2] / 0xffff;
					current_value = RGB_TO_VALUE(r, g, b);

					// Full transparency if in range
					if(current_value >= min_v && current_value < max_v)
						a = 0;
					else
					// Phased out if below or above range
					if(current_value < min_v)
					{
						if(min_v - current_value < run)
							a = (min_v - current_value) / run;
					}
					else
					if(current_value - max_v < run)
						a = (current_value - max_v) / run;
				}
				else
				// Use color cube
				{
					double difference;

					double r = (double)row[0] / 0xffff;
					double g = (double)row[1] / 0xffff;
					double b = (double)row[2] / 0xffff;
					difference = sqrt(SQR(r - r_key) +
						SQR(g - g_key) +
						SQR(b - b_key));
					if(difference < threshold)
						a = 0;
					else
					if(difference < threshold_run)
						a = (difference - threshold) / run;
				}

				// Multiply alpha and put back in frame
				row[3] = MIN((uint16_t)(a * 0xffff), row[3]);

				row += 4;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			uint16_t *row = (uint16_t*)plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				double a = 1;

				// Test for value in range
				if(plugin->config.use_value)
				{
					double current_value = (double)row[1] / 0xffff;;

					// Full transparency if in range
					if(current_value >= min_v && current_value < max_v)
							a = 0;
					else
					// Phased out if below or above range
					if(current_value < min_v)
					{
						if(min_v - current_value < run)
							a = (min_v - current_value) / run;
					}
					else
					if(current_value - max_v < run)
						a = (current_value - max_v) / run;
				}
				else
				// Use color cube
				{
					double difference;

					uint16_t y = row[1];
					uint16_t u = row[2];
					uint16_t v = row[3];

					difference = sqrt(SQR(y - y_key) +
						SQR(u - u_key) +
						SQR(v - v_key)) / 0xffff;

					if(difference < threshold)
						a = 0;
					else
					if(difference < threshold_run)
						a = (difference - threshold) / run;
				}

				// Multiply alpha and put back in frame
				row[0] = MIN((uint16_t)(a * 0xffff), row[0]);

				row += 4;
			}
		}
		break;
	}
}


REGISTER_PLUGIN

ChromaKey::ChromaKey(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ChromaKey::~ChromaKey()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}


VFrame *ChromaKey::process_tmpframe(VFrame *frame)
{
	load_configuration();
	this->input = frame;
	this->output = frame;

	if(!EQUIV(config.threshold, 0))
	{
		if(!engine) engine = new ChromaKeyServer(this);
		engine->process_packages();
		if(ColorModels::has_alpha(frame->get_color_model()))
			frame->set_transparent();
	}
	return frame;
}

PLUGIN_CLASS_METHODS

void ChromaKey::load_defaults()
{
	defaults = load_defaults_file("chromakey.rc");

	config.red = defaults->get("RED", config.red);
	config.green = defaults->get("GREEN", config.green);
	config.blue = defaults->get("BLUE", config.blue);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.slope = defaults->get("SLOPE", config.slope);
	config.use_value = defaults->get("USE_VALUE", config.use_value);
}

void ChromaKey::save_defaults()
{
	defaults->update("RED", config.red);
	defaults->update("GREEN", config.green);
	defaults->update("BLUE", config.blue);
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("SLOPE", config.slope);
	defaults->update("USE_VALUE", config.use_value);
	defaults->save();
}

void ChromaKey::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("CHROMAKEY");
	output.tag.set_property("RED", config.red);
	output.tag.set_property("GREEN", config.green);
	output.tag.set_property("BLUE", config.blue);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SLOPE", config.slope);
	output.tag.set_property("USE_VALUE", config.use_value);
	output.append_tag();
	output.tag.set_title("/CHROMAKEY");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ChromaKey::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("CHROMAKEY"))
		{
			config.red = input.tag.get_property("RED", config.red);
			config.green = input.tag.get_property("GREEN", config.green);
			config.blue = input.tag.get_property("BLUE", config.blue);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.slope = input.tag.get_property("SLOPE", config.slope);
			config.use_value = input.tag.get_property("USE_VALUE", config.use_value);
		}
	}
}

void ChromaKey::handle_opengl()
{
#ifdef HAVE_GL
	OUTER_VARIABLES(this)

	static const char *uniform_frag =
		"uniform sampler2D tex;\n"
		"uniform float min_v;\n"
		"uniform float max_v;\n"
		"uniform float run;\n"
		"uniform float threshold;\n"
		"uniform float threshold_run;\n"
		"uniform vec3 key;\n";

	static const char *get_yuvvalue_frag =
		"float get_value(vec4 color)\n"
		"{\n"
		"	return abs(color.r);\n"
		"}\n";
		
	static const char *get_rgbvalue_frag = 
		"float get_value(vec4 color)\n"
		"{\n"
		"	return dot(color.rgb, vec3(0.29900, 0.58700, 0.11400));\n"
		"}\n";

	static const char *value_frag =
		"void main()\n"
		"{\n"
		"	vec4 color = texture2D(tex, gl_TexCoord[0].st);\n"
		"	float value = get_value(color);\n"
		"	float alpha = 1.0;\n"
		"\n"
		"	if(value >= min_v && value < max_v)\n"
		"		alpha = 0.0;\n"
		"	else\n"
		"	if(value < min_v)\n"
		"	{\n"
		"		if(min_v - value < run)\n"
		"			alpha = (min_v - value) / run;\n"
		"	}\n"
		"	else\n"
		"	if(value - max_v < run)\n"
		"		alpha = (value - max_v) / run;\n"
		"\n"
		"	gl_FragColor = vec4(color.rgb, alpha);\n"
		"}\n";

	static const char *cube_frag = 
		"void main()\n"
		"{\n"
		"	vec4 color = texture2D(tex, gl_TexCoord[0].st);\n"
		"	float difference = length(color.rgb - key);\n"
		"	float alpha = 1.0;\n"
		"	if(difference < threshold)\n"
		"		alpha = 0.0;\n"
		"	else\n"
		"	if(difference < threshold_run)\n"
		"		alpha = (difference - threshold) / run;\n"
		"	gl_FragColor = vec4(color.rgb, min(color.a, alpha));\n"
		"}\n";
/* To be fixed
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	const char *shader_stack[] = { 0, 0, 0, 0, 0 };
	int current_shader = 0;

	shader_stack[current_shader++] = uniform_frag;
	switch(get_output()->get_color_model())
	{
	case BC_YUV888:
	case BC_YUVA8888:
		if(config.use_value)
		{
			shader_stack[current_shader++] = get_yuvvalue_frag;
			shader_stack[current_shader++] = value_frag;
		}
		else
		{
			shader_stack[current_shader++] = cube_frag;
		}
		break;

	default:
		if(config.use_value)
		{
			shader_stack[current_shader++] = get_rgbvalue_frag;
			shader_stack[current_shader++] = value_frag;
		}
		else
		{
			shader_stack[current_shader++] = cube_frag;
		}
		break;
	}

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		0);
	get_output()->bind_texture(0);

	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "min_v"), min_v);
		glUniform1f(glGetUniformLocation(frag, "max_v"), max_v);
		glUniform1f(glGetUniformLocation(frag, "run"), run);
		glUniform1f(glGetUniformLocation(frag, "threshold"), threshold);
		glUniform1f(glGetUniformLocation(frag, "threshold_run"), threshold_run);
		if(get_output()->get_color_model() != BC_YUV888 &&
			get_output()->get_color_model() != BC_YUVA8888)
			glUniform3f(glGetUniformLocation(frag, "key"), 
				r_key, g_key, b_key);
		else
			glUniform3f(glGetUniformLocation(frag, "key"), 
				(float)y_key / 0xff, (float)u_key / 0xff, (float)v_key / 0xff);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if(ColorModels::components(get_output()->get_color_model()) == 3)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		get_output()->clear_pbuffer();
	}

	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDisable(GL_BLEND);
	*/
#endif
}
