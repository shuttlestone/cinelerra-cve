
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

#ifndef SLIDE_H
#define SLIDE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION

#define PLUGIN_TITLE N_("Slide")
#define PLUGIN_CLASS SlideMain
#define PLUGIN_THREAD_CLASS SlideThread
#define PLUGIN_GUI_CLASS SlideWindow

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"
#include "pluginwindow.h"

class SlideLeft : public BC_Radial
{
public:
	SlideLeft(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideRight : public BC_Radial
{
public:
	SlideRight(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideIn : public BC_Radial
{
public:
	SlideIn(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideOut : public BC_Radial
{
public:
	SlideOut(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};


class SlideWindow : public PluginWindow
{
public:
	SlideWindow(SlideMain *plugin, int x, int y);

	SlideLeft *left;
	SlideRight *right;
	SlideIn *in;
	SlideOut *out;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class SlideMain : public PluginVClient
{
public:
	SlideMain(PluginServer *server);
	~SlideMain();

	PLUGIN_CLASS_MEMBERS

	void process_realtime(VFrame *incoming, VFrame *outgoing);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_configuration();

	int motion_direction;
	int direction;
};

#endif
