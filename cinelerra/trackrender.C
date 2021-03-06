
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "asset.h"
#include "automation.h"
#include "bcresources.h"
#include "edit.h"
#include "edlsession.h"
#include "file.h"
#include "intautos.h"
#include "mainerror.h"
#include "tmpframecache.h"
#include "track.h"
#include "tracks.h"
#include "trackrender.h"
#include "vframe.h"

TrackRender::TrackRender(Track *track)
{
	media_track = track;
	plugins_track = track;
	autos_track = track;
	next_plugin = 0;

	for(int i = 0; i < TRACKRENDER_FILES_MAX; i++)
		trackfiles[i] = 0;
}

TrackRender::~TrackRender()
{
	for(int i = 0; i < TRACKRENDER_FILES_MAX; i++)
		delete trackfiles[i];
}

File *TrackRender::media_file(Edit *edit, int filenum)
{
	int media_type;
	File *file = trackfiles[filenum];

	if(edit && edit->asset)
	{
		if(!file || file->asset != edit->asset)
		{
			if(file)
				file->close_file();
			else
				trackfiles[filenum] = file = new File();

			switch(edit->track->data_type)
			{
			case TRACK_AUDIO:
				media_type = FILE_OPEN_AUDIO;
				break;
			case TRACK_VIDEO:
				media_type = FILE_OPEN_VIDEO;
				break;
			}
			if(file->open_file(edit->asset, FILE_OPEN_READ | media_type))
			{
				errormsg("TrackRender::media_file: Failed to open media %s\n",
					edit->asset->path);
				delete file;
				trackfiles[filenum] = 0;
				return 0;
			}
		}
		return file;
	}
	return 0;
}

int TrackRender::is_playable(ptstime pts, Edit *edit)
{
	if(!edit || !media_track->play)
		return 0;

	if(edit->asset || edit->transition || media_track->is_synthesis(pts))
		return !((IntAutos *)autos_track->automation->autos[AUTOMATION_MUTE])->get_value(pts);

	return 0;
}

int TrackRender::is_muted(ptstime pts)
{
	return ((IntAutos *)autos_track->automation->autos[AUTOMATION_MUTE])->get_value(pts);
}

ptstime TrackRender::align_to_frame(ptstime position)
{
	return round(position * edlsession->frame_rate) / edlsession->frame_rate;
}

Track *TrackRender::get_track_number(int number)
{
	return media_track->tracks->get_item_number(number);
}

void TrackRender::set_effects_track(Track *track)
{
	plugins_track = track;
	autos_track = track;
}

int TrackRender::track_ready()
{
	return !next_plugin;
}

void TrackRender::dump(int indent)
{
	int tfs = 0;

	printf("%*smedia track %p plugins_track %p autos_track %p\n", indent, "",
		media_track, plugins_track, autos_track);

	for(int i = 0; i < TRACKRENDER_FILES_MAX; i++)
		if(trackfiles[i])
			tfs++;

	printf("%*snext_plugin %p; %d trackfile(s)\n", indent, "",
		next_plugin, tfs);
}
