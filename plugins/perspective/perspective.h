
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Perspective")
#define PLUGIN_CLASS PerspectiveMain
#define PLUGIN_CONFIG_CLASS PerspectiveConfig
#define PLUGIN_THREAD_CLASS PerspectiveThread
#define PLUGIN_GUI_CLASS PerspectiveWindow

#include "pluginmacros.h"

#include "keyframe.inc"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class PerspectiveConfig
{
public:
	PerspectiveConfig();

	int equivalent(PerspectiveConfig &that);
	void copy_from(PerspectiveConfig &that);
	void interpolate(PerspectiveConfig &prev, 
		PerspectiveConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	float x1, y1, x2, y2, x3, y3, x4, y4;
	int mode;
	int current_point;
	int forward;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PerspectiveCanvas : public BC_SubWindow
{
public:
	PerspectiveCanvas(PerspectiveMain *plugin, 
		int x, 
		int y, 
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int state;

	enum
	{
		NONE,
		DRAG,
		DRAG_FULL,
		ZOOM
	};

	int start_cursor_x, start_cursor_y;
	float start_x1, start_y1;
	float start_x2, start_y2;
	float start_x3, start_y3;
	float start_x4, start_y4;
	PerspectiveMain *plugin;
};

class PerspectiveCoord : public BC_TumbleTextBox
{
public:
	PerspectiveCoord(PerspectiveWindow *gui,
		PerspectiveMain *plugin, 
		int x, 
		int y,
		float value,
		int is_x);
	int handle_event();
	PerspectiveMain *plugin;
	int is_x;
};

class PerspectiveReset : public BC_GenericButton
{
public:
	PerspectiveReset(PerspectiveMain *plugin, 
		int x, 
		int y);
	int handle_event();
	PerspectiveMain *plugin;
};

class PerspectiveMode : public BC_Radial
{
public:
	PerspectiveMode(PerspectiveMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);
	int handle_event();
	PerspectiveMain *plugin;
	int value;
};

class PerspectiveDirection : public BC_Radial
{
public:
	PerspectiveDirection(PerspectiveMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);
	int handle_event();
	PerspectiveMain *plugin;
	int value;
};

class PerspectiveWindow : public PluginWindow
{
public:
	PerspectiveWindow(PerspectiveMain *plugin, int x, int y);

	void update();
	void update_canvas();
	void update_mode();
	void update_coord();
	void calculate_canvas_coords(int &x1, 
		int &y1, 
		int &x2, 
		int &y2, 
		int &x3, 
		int &y3, 
		int &x4, 
		int &y4);

	PerspectiveCanvas *canvas;
	PerspectiveCoord *x, *y;
	PerspectiveReset *reset;
	PerspectiveMode *mode_perspective, *mode_sheer, *mode_stretch;
	PerspectiveDirection *forward, *reverse;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class PerspectiveMain : public PluginVClient
{
public:
	PerspectiveMain(PluginServer *server);
	~PerspectiveMain();

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	float get_current_x();
	float get_current_y();
	void set_current_x(float value);
	void set_current_y(float value);
	VFrame *input, *output;
	VFrame *strech_temp;
	AffineEngine *engine;
};
