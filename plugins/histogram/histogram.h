
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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI

#define PLUGIN_TITLE N_("Histogram")
#define PLUGIN_CLASS HistogramMain
#define PLUGIN_CONFIG_CLASS HistogramConfig
#define PLUGIN_THREAD_CLASS HistogramThread
#define PLUGIN_GUI_CLASS HistogramWindow

#include "pluginmacros.h"

#include "histogram.inc"
#include "histogramconfig.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"

class HistogramMain : public PluginVClient
{
public:
	HistogramMain(PluginServer *server);
	~HistogramMain();

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void *data);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

// Interpolate quantized transfer table to linear output
	double calculate_linear(double input, int mode, int do_value);
	double calculate_smooth(double input, int subscript);
// Calculate automatic settings
	void calculate_automatic(VFrame *data);
// Calculate histogram.
// Value is only calculated for preview.
	void calculate_histogram(VFrame *data, int do_value);
// Calculate the linear, smoothed, lookup curves
	void tabulate_curve(int subscript, int use_value);

	VFrame *input, *output;
	HistogramEngine *engine;
	int *lookup[HISTOGRAM_MODES];
	float *smoothed[HISTOGRAM_MODES];
	float *linear[HISTOGRAM_MODES];
// No value applied to this
	int *preview_lookup[HISTOGRAM_MODES];
	int *accum[HISTOGRAM_MODES];
// Input point being dragged or edited
	int current_point;
// Current channel being viewed
	int mode;
	int dragging_point;
	int point_x_offset;
	int point_y_offset;
};

class HistogramPackage : public LoadPackage
{
public:
	HistogramPackage();
	int start, end;
};

class HistogramUnit : public LoadClient
{
public:
	HistogramUnit(HistogramEngine *server, HistogramMain *plugin);
	~HistogramUnit();
	void process_package(LoadPackage *package);
	HistogramEngine *server;
	HistogramMain *plugin;
	int *accum[5];
};

class HistogramEngine : public LoadServer
{
public:
	HistogramEngine(HistogramMain *plugin, 
		int total_clients, 
		int total_packages);
	void process_packages(int operation, VFrame *data, int do_value);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HistogramMain *plugin;
	int total_size;

	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	VFrame *data;
	int do_value;
};

#endif
