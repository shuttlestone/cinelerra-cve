
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

#ifndef MENUVEFFECTS_H
#define MENUVEFFECTS_H

#include "menueffects.h"

class MenuVEffects : public MenuEffects
{
public:
	MenuVEffects();
	~MenuVEffects();
};


class MenuVEffectThread : public MenuEffectThread
{
public:
	MenuVEffectThread();

	int get_recordable_tracks(Asset *asset);
	void fix_menu(const char *title);
};


class MenuVEffectItem : public MenuEffectItem
{
public:
	MenuVEffectItem(MenuVEffects *menueffect, const char *string);
};

#endif
