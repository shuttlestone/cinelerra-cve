
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

#ifndef COLORBALANCEWINDOW_H
#define COLORBALANCEWINDOW_H


class ColorBalanceSlider;
class ColorBalancePreserve;
class ColorBalanceLock;
class ColorBalanceWhite;
class ColorBalanceReset;

#include "bcbutton.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "colorbalance.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class ColorBalanceWindow : public PluginWindow
{
public:
	ColorBalanceWindow(ColorBalanceMain *plugin, int x, int y);

	void update();

	ColorBalanceSlider *cyan;
	ColorBalanceSlider *magenta;
	ColorBalanceSlider *yellow;
	ColorBalanceLock *lock_params;
	ColorBalancePreserve *preserve;
	ColorBalanceWhite *use_eyedrop;
	ColorBalanceReset *reset;
	PLUGIN_GUI_CLASS_MEMBERS
};

class ColorBalanceSlider : public BC_ISlider
{
public:
	ColorBalanceSlider(ColorBalanceMain *client, float *output, int x, int y);

	int handle_event();
	char* get_caption();

	ColorBalanceMain *client;
	float *output;
	float old_value;
	char string[64];
};

class ColorBalancePreserve : public BC_CheckBox
{
public:
	ColorBalancePreserve(ColorBalanceMain *client, int x, int y);

	int handle_event();

	ColorBalanceMain *client;
};

class ColorBalanceLock : public BC_CheckBox
{
public:
	ColorBalanceLock(ColorBalanceMain *client, int x, int y);

	int handle_event();

	ColorBalanceMain *client;
};

class ColorBalanceWhite : public BC_GenericButton
{
public:
	ColorBalanceWhite(ColorBalanceMain *plugin, ColorBalanceWindow *gui,
		int x, int y);

	int handle_event();

	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

class ColorBalanceReset : public BC_GenericButton
{
public:
	ColorBalanceReset(ColorBalanceMain *plugin, ColorBalanceWindow *gui,
		int x, int y);

	int handle_event();

	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

#endif
