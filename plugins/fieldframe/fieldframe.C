
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
#include "fieldframe.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>

REGISTER_PLUGIN


FieldFrameConfig::FieldFrameConfig()
{
	field_dominance = TOP_FIELD_FIRST;
}

int FieldFrameConfig::equivalent(FieldFrameConfig &src)
{
	return src.field_dominance == field_dominance;
}


FieldFrameWindow::FieldFrameWindow(FieldFrame *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	230, 
	100)
{
	x = y = 10;
	add_subwindow(top = new FieldFrameTop(plugin, this, x, y));
	y += 30;
	add_subwindow(bottom = new FieldFrameBottom(plugin, this, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void FieldFrameWindow::update()
{
	top->update(plugin->config.field_dominance == TOP_FIELD_FIRST);
	bottom->update(plugin->config.field_dominance == BOTTOM_FIELD_FIRST);
}


FieldFrameTop::FieldFrameTop(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	_("Top field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}


FieldFrameBottom::FieldFrameBottom(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	_("Bottom field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


FieldFrame::FieldFrame(PluginServer *server)
 : PluginVClient(server)
{
	input = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

FieldFrame::~FieldFrame()
{
	if(input) delete input;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

int FieldFrame::load_configuration()
{
	KeyFrame *prev_keyframe;
	FieldFrameConfig old_config = config;

	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);

	return !old_config.equivalent(config);
}

void FieldFrame::load_defaults()
{
	defaults = load_defaults_file("fieldframe.rc");

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
}

void FieldFrame::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->save();
}

void FieldFrame::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("FIELD_FRAME");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.append_tag();
	output.tag.set_title("/FIELD_FRAME");
	output.append_tag();
	keyframe->set_data(output.string);
}

void FieldFrame::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("FIELD_FRAME"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
		}
	}
}

VFrame *FieldFrame::process_tmpframe(VFrame *input)
{
	VFrame *frame;
	load_configuration();

	frame = clone_vframe(input);

	frame->copy_pts(input);

	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 0 : 1);
	input->set_pts(input->next_pts() + EPSILON);
	get_frame(input);

	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 1 : 0);
	frame->set_duration(input->next_pts() - frame->get_pts());
	release_vframe(input);
	return frame;
}

void FieldFrame::apply_field(VFrame *output, VFrame *input, int field)
{
	for(int i = field; i < output->get_h(); i += 2)
	{
		memcpy(output->get_row_ptr(i), input->get_row_ptr(i),
			output->get_bytes_per_line());
	}
}
