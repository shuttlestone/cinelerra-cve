
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

#include "1080to540.h"
#include "clip.h"
#include "bchash.h"
#include "bcsignals.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN


_1080to540Config::_1080to540Config()
{
	first_field = 0;
}

int _1080to540Config::equivalent(_1080to540Config &that)
{
	return first_field == that.first_field;
}

void _1080to540Config::copy_from(_1080to540Config &that)
{
	first_field = that.first_field;
}

void _1080to540Config::interpolate(_1080to540Config &prev, 
	_1080to540Config &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	copy_from(prev);
}


_1080to540Window::_1080to540Window(_1080to540Main *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	200, 
	100)
{ 
	x = 10;
	y = 10;

	add_tool(odd_first = new _1080to540Option(plugin, this, 1, x, y, _("Odd field first")));
	y += 25;
	add_tool(even_first = new _1080to540Option(plugin, this, 0, x, y, _("Even field first")));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void _1080to540Window::update()
{
	int first_field = plugin->config.first_field;

	odd_first->update(first_field == 1);
	even_first->update(first_field == 0);
}


_1080to540Option::_1080to540Option(_1080to540Main *client,
		_1080to540Window *window,
		int output, 
		int x, 
		int y, 
		char *text)
 : BC_Radial(x, 
	y,
	client->config.first_field == output, 
	text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

int _1080to540Option::handle_event()
{
	client->config.first_field = output;
	window->update();
	client->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


_1080to540Main::_1080to540Main(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

_1080to540Main::~_1080to540Main()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS
#define OUT_ROWS 270

void _1080to540Main::reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field)
{
	int w = input->get_w();
	int h = input->get_h();

	if(h > output->get_h()) h = output->get_h();
	if(w > output->get_w()) w = output->get_w();

#define REDUCE_MACRO(type, temp, components) \
for(int i = 0; i < OUT_ROWS; i++) \
{ \
	int in_number1 = dst_field * 2 + src_field + (int)(i * 2) * 2; \
	int in_number2 = in_number1 + 2; \
	int in_number3 = in_number2 + 2; \
	int in_number4 = in_number3 + 2; \
	int out_number = dst_field + i * 2; \
 \
	if(in_number1 >= h) in_number1 = h - 1; \
	if(in_number2 >= h) in_number2 = h - 1; \
	if(in_number3 >= h) in_number3 = h - 1; \
	if(in_number4 >= h) in_number4 = h - 1; \
	if(out_number >= h) out_number = h - 1; \
 \
	type *in_row1 = (type*)input->get_row_ptr(in_number1); \
	type *in_row2 = (type*)input->get_row_ptr(in_number2); \
	type *in_row3 = (type*)input->get_row_ptr(in_number3); \
	type *in_row4 = (type*)input->get_row_ptr(in_number4); \
	type *out_row = (type*)output->get_row_ptr(out_number); \
 \
	for(int j = 0; j < w * components; j++) \
	{ \
		*out_row++ = ((temp)*in_row1++ +  \
			(temp)*in_row2++ +  \
			(temp)*in_row3++ +  \
			(temp)*in_row4++) / 4; \
	} \
}

	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		REDUCE_MACRO(unsigned char, int64_t, 3);
		break;
	case BC_RGB_FLOAT:
		REDUCE_MACRO(float, float, 3);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		REDUCE_MACRO(unsigned char, int64_t, 4);
		break;
	case BC_RGBA_FLOAT:
		REDUCE_MACRO(float, float, 4);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		REDUCE_MACRO(uint16_t, int64_t, 3);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		REDUCE_MACRO(uint16_t, int64_t, 4);
		break;
	}
}

VFrame *_1080to540Main::process_tmpframe(VFrame *input)
{
	VFrame *temp = clone_vframe(input);

	load_configuration();
	temp->clear_frame();
	reduce_field(temp, input, config.first_field == 0 ? 0 : 1, 0);
	reduce_field(temp, input, config.first_field == 0 ? 1 : 0, 1);
	release_vframe(input);
	return temp;
}

void _1080to540Main::load_defaults()
{
	defaults = load_defaults_file("1080to540.rc");
	config.first_field = defaults->get("FIRST_FIELD", config.first_field);
}

void _1080to540Main::save_defaults()
{
	defaults->update("FIRST_FIELD", config.first_field);
	defaults->save();
}

void _1080to540Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("1080TO540");
	output.tag.set_property("FIRST_FIELD", config.first_field);
	output.append_tag();
	output.tag.set_title("/1080TO540");
	output.append_tag();
	keyframe->set_data(output.string);
}

void _1080to540Main::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("1080TO540"))
		{
			config.first_field = input.tag.get_property("FIRST_FIELD", config.first_field);
		}
	}
}

