
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
#include "language.h"
#include "scale.h"

PLUGIN_THREAD_METHODS

ScaleWin::ScaleWin(ScaleMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	240,
	150)
{ 
	x = y = 10;

	add_tool(new BC_Title(x, y, _("X Scale:")));
	y += 20;
	width = new ScaleWidth(this, plugin, x, y);
	y += 30;
	add_tool(new BC_Title(x, y, _("Y Scale:")));
	y += 20;
	height = new ScaleHeight(this, plugin, x, y);
	y += 35;
	add_tool(constrain = new ScaleConstrain(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ScaleWin::~ScaleWin()
{
}

void ScaleWin::update()
{
	width->update((float)plugin->config.w);
	height->update((float)plugin->config.h);
	constrain->update(plugin->config.constrain);
}

ScaleWidth::ScaleWidth(ScaleWin *win, 
	ScaleMain *client, 
	int x, 
	int y)
 : BC_TumbleTextBox(win,
	(float)client->config.w,
	(float)0,
	(float)100,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
}

ScaleWidth::~ScaleWidth()
{
}

int ScaleWidth::handle_event()
{
	client->config.w = atof(get_text());
	CLAMP(client->config.w, 0, 100);

	if(client->config.constrain)
	{
		client->config.h = client->config.w;
		win->height->update((float)client->config.h);
	}

	client->send_configure_change();
	return 1;
}


ScaleHeight::ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win,
	(float)client->config.h,
	(float)0,
	(float)100,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
}

ScaleHeight::~ScaleHeight()
{
}

int ScaleHeight::handle_event()
{
	client->config.h = atof(get_text());
	CLAMP(client->config.h, 0, 100);

	if(client->config.constrain)
	{
		client->config.w = client->config.h;
		win->width->update((float)client->config.w);
	}

	client->send_configure_change();
	return 1;
}


ScaleConstrain::ScaleConstrain(ScaleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.constrain, _("Constrain ratio"))
{
	this->client = client;
}

ScaleConstrain::~ScaleConstrain()
{
}

int ScaleConstrain::handle_event()
{
	client->config.constrain = get_value();
	client->send_configure_change();
}
