
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

#ifndef CHROMAKEY_H
#define CHROMAKEY_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Chroma key (HSV)")
#define PLUGIN_CLASS ChromaKeyHSV
#define PLUGIN_CONFIG_CLASS ChromaKeyConfig
#define PLUGIN_THREAD_CLASS ChromaKeyThread
#define PLUGIN_GUI_CLASS ChromaKeyWindow

#include "pluginmacros.h"

#include "bcbutton.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "colorpicker.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

enum {
	CHROMAKEY_POSTPROCESS_NONE,
	CHROMAKEY_POSTPROCESS_BLUR,
	CHROMAKEY_POSTPROCESS_DILATE
};

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int get_color();

	// Output mode
	bool  show_mask;
	// Key color definition
	double red;
	double green;
	double blue;
	// Key shade definition
	float min_brightness;
	float max_brightness;
	float saturation;
	float min_saturation;
	float tolerance;
	// Mask feathering
	float in_slope;
	float out_slope;
	float alpha_offset;
	// Spill light compensation
	float spill_threshold;
	float spill_amount;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKeyHSV *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyMinBrightness : public BC_FSlider
{
public:
	ChromaKeyMinBrightness(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyMaxBrightness : public BC_FSlider
{
public:
	ChromaKeyMaxBrightness(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeySaturation : public BC_FSlider
{
public:
	ChromaKeySaturation(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyMinSaturation : public BC_FSlider
{
public:
	ChromaKeyMinSaturation(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyTolerance : public BC_FSlider
{
public:
	ChromaKeyTolerance(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyInSlope : public BC_FSlider
{
public:
	ChromaKeyInSlope(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyOutSlope : public BC_FSlider
{
public:
	ChromaKeyOutSlope(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyAlphaOffset : public BC_FSlider
{
public:
	ChromaKeyAlphaOffset(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeySpillThreshold : public BC_FSlider
{
public:
	ChromaKeySpillThreshold(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeySpillAmount : public BC_FSlider
{
public:
	ChromaKeySpillAmount(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKeyHSV *plugin, ChromaKeyWindow *gui, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKeyHSV *plugin, ChromaKeyWindow *gui);
	int handle_new_color(int output, int alpha);
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyShowMask : public BC_CheckBox
{
public:
	ChromaKeyShowMask(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};


class ChromaKeyWindow : public PluginWindow
{
public:
	ChromaKeyWindow(ChromaKeyHSV *plugin, int x, int y);
	~ChromaKeyWindow();

	void update();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeyMinBrightness *min_brightness;
	ChromaKeyMaxBrightness *max_brightness;
	ChromaKeySaturation *saturation;
	ChromaKeyMinSaturation *min_saturation;
	ChromaKeyTolerance *tolerance;
	ChromaKeyInSlope *in_slope;
	ChromaKeyOutSlope *out_slope;
	ChromaKeyAlphaOffset *alpha_offset;
	ChromaKeySpillThreshold *spill_threshold;
	ChromaKeySpillAmount *spill_amount;
	ChromaKeyShowMask *show_mask;
	BC_SubWindow *sample;
	ChromaKeyColorThread *color_thread;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKeyHSV *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKeyHSV *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKeyHSV *plugin, ChromaKeyServer *server);
	void process_package(LoadPackage *package);
	template <typename component_type> void process_chromakey(int components, component_type max, bool use_yuv, ChromaKeyPackage *pkg);
	bool is_same_color(float r, float g, float b, float rk,float bk,float gk, float color_threshold, float light_threshold, int key_main_component);

	ChromaKeyHSV *plugin;

};

class ChromaKeyHSV : public PluginVClient
{
public:
	ChromaKeyHSV(PluginServer *server);
	~ChromaKeyHSV();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void handle_opengl();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	VFrame *input, *output;
	ChromaKeyServer *engine;
};

#endif







