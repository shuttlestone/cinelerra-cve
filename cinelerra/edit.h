
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

#ifndef EDIT_H
#define EDIT_H

#include "asset.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "edl.inc"
#include "edits.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "linklist.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "track.inc"

// Generic edit of something

class Edit : public ListItem<Edit>
{
public:
	Edit(EDL *edl, Edits *edits);
	Edit(EDL *edl, Track *track);
	~Edit();

	ptstime length(void);
	ptstime end_pts(void);
	void copy_from(Edit *edit);
	Edit& operator=(Edit& edit);
// Called by Edits and PluginSet
	void equivalent_output(Edit *edit, ptstime *result);
// Get size of frame to draw on timeline
	double picon_w(void);
	int picon_h(void);

	void save_xml(FileXML *xml, const char *output_path, int streamno);
// Shift in time
	void shift(ptstime difference);
	void shift_source(ptstime difference);
	Plugin *insert_transition(PluginServer *server);
// Determine if silence depending on existance of asset or plugin title
	int silence(void);
// Returns size of data in bytes
	size_t get_size();

// Media edit information
// Channel or layer of source
	int channel;
// ID for resource pixmaps
	int id;

	ptstime set_pts(ptstime pts);
	ptstime get_pts();
	ptstime set_source_pts(ptstime pts);
	ptstime get_source_pts();
// End of current edit in source
	ptstime source_end_pts();
// End of asset
	ptstime get_source_length();

// Transition if one is present at the beginning of this edit
// This stores the length of the transition
	Plugin *transition;
	EDL *edl;

	Edits *edits;

	Track *track;

// Asset is 0 if silence, otherwise points an object in edl->assets
	Asset *asset;

// ============================= initialization

	posnum load_properties(FileXML *xml, ptstime project_pts);

	void dump(int indent = 0);
private:
// Start of edit in source file in seconds
	ptstime source_pts;
// Start of edit in project
	ptstime project_pts;

};

#endif
