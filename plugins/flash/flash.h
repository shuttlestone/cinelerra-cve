
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

#ifndef FLASH_H
#define FLASH_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION

#define PLUGIN_TITLE N_("Flash")
#define PLUGIN_CLASS FlashMain

#include "pluginmacros.h"

class FlashMain;

#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"

class FlashMain : public PluginVClient
{
public:
	FlashMain(PluginServer *server);
	~FlashMain();

	PLUGIN_CLASS_MEMBERS
// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);
};

#endif
