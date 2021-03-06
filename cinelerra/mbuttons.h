
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

#ifndef MBUTTONS_H
#define MBUTTONS_H

#include "bcsignals.h"
#include "editpanel.h"
#include "edl.inc"
#include "mbuttons.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playtransport.h"

class MainEditing;

class MButtons : public BC_SubWindow
{
public:
	MButtons(MWindow *mwindow, MWindowGUI *gui);
	~MButtons();

	void show();
	void resize_event();
	int keypress_event();
	void update();

	MWindowGUI *gui;
	MWindow *mwindow;
	PlayTransport *transport;
	MainEditing *edit_panel;
};

class MainTransport : public PlayTransport
{
public:
	MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	void goto_start();
	void goto_end();
	EDL *get_edl();
};

class MainEditing : public EditPanel
{
public:
	MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y);

	MWindow *mwindow;
	MButtons *mbuttons;
};

#endif
