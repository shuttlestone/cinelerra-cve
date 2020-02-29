
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

#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H

#include "aframe.h"
#include "pluginclient.h"

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient() {};

	int is_audio();

	virtual int process_loop(AFrame *aframe) { return 1; };
	virtual int process_loop(AFrame **aframes) { return 1; };
	int plugin_process_loop(AFrame **buffers);

	static int get_project_samplerate();
};

#endif
