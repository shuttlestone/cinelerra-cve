
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

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "delayvideo.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN


DelayVideoConfig::DelayVideoConfig()
{
	length = 0;
}

int DelayVideoConfig::equivalent(DelayVideoConfig &that)
{
	return EQUIV(length, that.length);
}

void DelayVideoConfig::copy_from(DelayVideoConfig &that)
{
	length = that.length;
}

void DelayVideoConfig::interpolate(DelayVideoConfig &prev, 
		DelayVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	this->length = prev.length;
}


DelayVideoWindow::DelayVideoWindow(DelayVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	210, 
	120)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Delay seconds:")));
	y += 20;
	add_subwindow(slider = new DelayVideoSlider(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DelayVideoWindow::update()
{
	slider->update(plugin->config.length);
}


DelayVideoSlider::DelayVideoSlider(DelayVideo *plugin, int x, int y)
 : BC_TextBox(x, y, 150, 1, (float)plugin->config.length)
{
	this->plugin = plugin;
}

int DelayVideoSlider::handle_event()
{
	plugin->config.length = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


DelayVideo::DelayVideo(PluginServer *server)
 : PluginVClient(server)
{
	thread = 0;
	defaults = 0;
	need_reconfigure = 1;
	buffer = 0;
	allocation = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DelayVideo::~DelayVideo()
{
	if(buffer)
	{
		for(int i = 0; i < allocation; i++)
			if(buffer[i])
				delete buffer[i];
		delete [] buffer;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void DelayVideo::reconfigure(VFrame *input)
{
	int new_allocation = round(config.length * project_frame_rate);

	if(new_allocation < 1)
	{
		for(int i = 0; i < allocation; i++)
			delete buffer[i];
		delete [] buffer;
		buffer = 0;
		allocation = 0;
		return;
	}

	VFrame **new_buffer = new VFrame*[new_allocation];
	int reuse = MIN(new_allocation, allocation);

	memset(new_buffer, 0, sizeof(new_buffer));

	for(int i = 0; i < reuse; i++)
	{
		new_buffer[i] = buffer[i];
	}
	ptstime cpts = input->get_pts();
	for(int i = reuse; i < new_allocation; i++)
	{
		new_buffer[i] = new VFrame(0, 
			input->get_w(),
			input->get_h(),
			project_color_model);
		new_buffer[i]->clear_frame();
		new_buffer[i]->set_layer(input->get_layer());
		new_buffer[i]->set_pts(cpts);
		new_buffer[i]->set_duration(1 / project_frame_rate);
		cpts = new_buffer[i]->next_pts();
	}

	for(int i = reuse; i < allocation; i++)
	{
		if(buffer[i])
			delete buffer[i];
	}

	if(buffer) delete [] buffer;

	buffer = new_buffer;
	allocation = new_allocation;

	need_reconfigure = 0;
}

VFrame *DelayVideo::process_tmpframe(VFrame *frame)
{
	need_reconfigure += load_configuration();
	CLAMP(config.length, 0, 10);

	if(need_reconfigure)
		reconfigure(frame);

	if(allocation > 1)
	{
		buffer[allocation - 1]->copy_from(frame);
		frame->copy_from(buffer[0], 0);

		VFrame *temp = buffer[0];

		for(int i = 0; i < allocation - 1; i++)
			buffer[i] = buffer[i + 1];

		buffer[allocation - 1] = temp;
	}
	return frame;
}

void DelayVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DELAYVIDEO");
	output.tag.set_property("LENGTH", (double)config.length);
	output.append_tag();
	output.tag.set_title("/DELAYVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);

}

void DelayVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DELAYVIDEO"))
		{
			config.length = input.tag.get_property("LENGTH", (double)config.length);
		}
	}
}

void DelayVideo::load_defaults()
{
	defaults = load_defaults_file("delayvideo.rc");
	config.length = defaults->get("LENGTH", (double)1);
}

void DelayVideo::save_defaults()
{
	defaults->update("LENGTH", config.length);
	defaults->save();
}
