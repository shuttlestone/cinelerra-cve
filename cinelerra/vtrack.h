
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

#ifndef VTRACK_H
#define VTRACK_H

#include "arraylist.h"
#include "autoconf.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "track.h"
#include "vedit.inc"
#include "vframe.inc"


class VTrack : public Track
{
public:
	VTrack(EDL *edl, Tracks *tracks);
	~VTrack();

	void create_objects();
	int load_defaults(BC_Hash *defaults);
	void set_default_title();
	PluginSet* new_plugins();
	void save_header(FileXML *file);
	int copy_settings(Track *track);
	void synchronize_params(Track *track);
	posnum to_units(ptstime position, int round = 0);
	ptstime from_units(posnum position);

	void calculate_input_transfer(Asset *asset, ptstime position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	void calculate_output_transfer(ptstime position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	int vertical_span(Theme *theme);

// Give whether compressed data can be copied directly from the track to the output file
	int direct_copy_possible(ptstime start, int direction, int use_nudge);

	void translate(float offset_x, float offset_y, int do_camera);

};

#endif
