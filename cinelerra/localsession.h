
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

#ifndef LOCALSESSION_H
#define LOCALSESSION_H

#include "bcwindowbase.inc"
#include "bchash.inc"
#include "edl.inc"
#include "filexml.inc"


// Unique session for every EDL

class LocalSession
{
public:
	LocalSession(EDL *edl);

	void reset_instance();
// Get selected range based on precidence of in/out points and
// highlighted region.
// 1) If a highlighted selection exists it's used.
// 2) If in_point or out_point exists they're used.
// 3) If no in/out points exist, the insertion point is returned.
// highlight_only - forces it to use highlighted region only.
	ptstime get_selectionstart(int highlight_only = 0);
	ptstime get_selectionend(int highlight_only = 0);
	ptstime get_inpoint();
	ptstime get_outpoint();
	int inpoint_valid();
	int outpoint_valid();
// Returns values[4] - selection values
	void get_selections(ptstime *values);
// Set selection start/end at once
	void set_selection(ptstime start, ptstime end);
	void set_selection(ptstime pos);
	void set_selectionstart(ptstime value);
	void set_selectionend(ptstime value);
	void set_inpoint(ptstime value);
	void set_outpoint(ptstime value);
	void unset_inpoint();
	void unset_outpoint();

	void copy_from(LocalSession *that);
	void copy(LocalSession *that, ptstime start, ptstime end);
	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
// Set clip title from path
	void set_clip_title(const char *path);

	void boundaries();
	size_t get_size();
	void dump(int indent = 0);

	EDL *edl;

// Variables specific to each EDL
// Title if clip
	char clip_title[BCTEXTLEN];
	char clip_notes[BCTEXTLEN];
// Folder in parent EDL of clip
	int awindow_folder;

	int loop_playback;
	ptstime loop_start;
	ptstime loop_end;
// Vertical start of track view
	int64_t track_start;
// Horizontal start of view in seconds
	ptstime view_start_pts;
// Zooming of the timeline. Length of media per pixel
	ptstime zoom_time;
// Amplitude zoom
	int zoom_y;
// Track zoom
	int zoom_track;
// Vertical automation scale
	float automation_mins[6];
	float automation_maxs[6];
	int zoombar_showautotype;

// Eye dropper
	double red, green, blue;

// Range for CWindow and VWindow preview in seconds.
	ptstime preview_start;
	ptstime preview_end;

private:
// The reason why selection ranges and inpoints have to be separate:
// The selection position has to change to set new in points.
// For editing functions we have a precidence for what determines
// the selection.

	ptstime selectionstart, selectionend;
	ptstime in_point, out_point;
};

#endif
