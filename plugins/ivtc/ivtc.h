
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

#ifndef IVTC_H
#define IVTC_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Inverse Telecine")
#define PLUGIN_CLASS IVTCMain
#define PLUGIN_CONFIG_CLASS IVTCConfig
#define PLUGIN_THREAD_CLASS IVTCThread
#define PLUGIN_GUI_CLASS IVTCWindow

#include "pluginmacros.h"

class IVTCEngine;

#include "bchash.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "ivtcwindow.h"
#include <sys/types.h>


class IVTCConfig
{
public:
	IVTCConfig();
	int frame_offset;
// 0 - even     1 - odd
	int first_field;
	int automatic;
	float auto_threshold;
	int pattern;
	enum
	{
		PULLDOWN32,
		SHIFTFIELD,
		AUTOMATIC
	};
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class IVTCMain : public PluginVClient
{
public:
	IVTCMain(PluginServer *server);
	~IVTCMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	VFrame *process_tmpframe(VFrame *input_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();

	void compare_fields(VFrame *frame1, 
		VFrame *frame2, 
		int64_t &field1,
		int64_t &field2);
	int64_t compare(VFrame *current_avg, 
		VFrame *current_orig, 
		VFrame *previous, 
		int field);

	void deinterlace_avg(VFrame *output, VFrame *input, int dominance);

	VFrame *temp_frame[2];
	VFrame *input;

// Automatic IVTC variables
// Difference between averaged current even lines and original even lines
	int64_t even_vs_current;
// Difference between averaged current even lines and previous even lines
	int64_t even_vs_prev;
// Difference between averaged current odd lines and original odd lines
	int64_t odd_vs_current;
// Difference between averaged current odd lines and previous odd lines
	int64_t odd_vs_prev;

// Closest combination of fields in previous calculation.
// If the lowest current combination is too big and the previous strategy
// was direct copy, copy the previous frame.
	int64_t previous_min;
	int previous_strategy;
	IVTCEngine *engine;
};


class IVTCPackage : public LoadPackage
{
public:
	IVTCPackage();
	int y1, y2;
};

class IVTCUnit : public LoadClient
{
public:
	IVTCUnit(IVTCEngine *server, IVTCMain *plugin);
	void process_package(LoadPackage *package);
	void clear_totals();
	IVTCEngine *server;
	IVTCMain *plugin;
	int64_t even_vs_current;
	int64_t even_vs_prev;
	int64_t odd_vs_current;
	int64_t odd_vs_prev;
};


class IVTCEngine : public LoadServer
{
public:
	IVTCEngine(IVTCMain *plugin, int cpus);
	~IVTCEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	IVTCMain *plugin;
};

#endif
