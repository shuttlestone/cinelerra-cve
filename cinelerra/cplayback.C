
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

#include "bcsignals.h"
#include "cplayback.h"
#include "ctracking.h"
#include "cwindow.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playtransport.h"
#include "trackcanvas.h"

// Playback engine for composite window

CPlayback::CPlayback(CWindow *cwindow, Canvas *output)
 : PlaybackEngine(output)
{
	this->cwindow = cwindow;
}

void CPlayback::init_cursor()
{
	mwindow_global->gui->canvas->deactivate();
	cwindow->playback_cursor->start_playback(get_tracking_position());
}

void CPlayback::stop_cursor()
{
	cwindow->playback_cursor->stop_playback();

	if(is_playing_back)
		mwindow_global->gui->canvas->activate();
}

int CPlayback::brender_available(ptstime position)
{
	return mwindow_global->brender_available(position);
}
