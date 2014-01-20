
/*
 * CINELERRA
 * Copyright (C) 2014 Einar Rünkaru <einarry at smail dot ee>
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

#ifndef SELECTION_H
#define SELECTION_H

#include "selection.inc"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.inc"
#include "bctextbox.h"
#include "bcwindow.inc"
#include "mwindow.inc"

struct selection_int
{
	const char *text;
	int value;
};

struct selection_double
{
	const char *text;
	double value;
};

class Selection : public BC_TextBox
{
public:
	Selection(int x, int y, BC_WindowBase *base,
		const struct selection_int items[], int *value);
	Selection(int x, int y, BC_WindowBase *base,
		const struct selection_double items[], double *value);

	int handle_event();

	const struct selection_double *current_double;
private:
	BC_PopupMenu *init_objects(int x, int y, BC_WindowBase *base);
	int *intvalue;
	double *doublevalue;
};

class SelectionButton : public BC_Button
{
public:
	SelectionButton(int x, int y, BC_PopupMenu *popupmenu, VFrame **images);

	int handle_event();
private:
	BC_PopupMenu *popupmenu;
};

class SelectionItem : public BC_MenuItem
{
public:
	SelectionItem(const struct selection_int *item, Selection *output);
	SelectionItem(const struct selection_double *item, Selection *output);

	int handle_event();
private:
	const struct selection_int *intitem;
	const struct selection_double *doubleitem;
	Selection *output;
};


class SampleRateSelection : public Selection
{
public:
	SampleRateSelection(int x, int y, BC_WindowBase *base, int *value);

private:
	static const struct selection_int sample_rates[];
};


class FrameRateSelection : public Selection
{
public:
	FrameRateSelection(int x, int y, BC_WindowBase *base, double *value);

private:
	static const struct selection_double frame_rates[];
};

#endif