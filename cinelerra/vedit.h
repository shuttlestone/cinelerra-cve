
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

#ifndef VEDIT_H
#define VEDIT_H

#include "guicast.h"
#include "cache.inc"
#include "edit.h"
#include "vedits.inc"
#include "vframe.inc"

// UNITS ARE SECONDS

class VEdit : public Edit
{
public:
	VEdit(EDL *edl, Edits *edits);
	~VEdit();

	int read_frame(VFrame *video_out, 
			ptstime input_postime,
			CICache *cache,
			int use_nudge,
			int use_cache,
			int use_asynchronous);

	void load_properties_derived(FileXML *xml);
	ptstime get_source_end(ptstime default_value);

private:
	VEdits *vedits;
};




#endif
