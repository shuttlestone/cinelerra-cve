
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
#include "bcresources.h"
#include "cinelerra.h"
#include "clip.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "zoombar.h"


MTimeBar::MTimeBar(MWindow *mwindow, 
	MWindowGUI *gui,
	int x, 
	int y,
	int w,
	int h)
 : TimeBar(mwindow, gui, x, y, w, h)
{
	this->gui = gui;
}

int MTimeBar::position_to_pixel(ptstime position)
{
	return round((position - master_edl->local_session->view_start_pts) /
		master_edl->local_session->zoom_time);
}

void MTimeBar::stop_playback()
{
	gui->mbuttons->transport->handle_transport(STOP, 1, 0);
}

#define TEXT_MARGIN 4
#define TICK_SPACING 5
#define LINE_MARGIN 3
#define TICK_MARGIN 16

void MTimeBar::draw_time()
{
	char string[BCTEXTLEN];
	int sample_rate = edlsession->sample_rate;
	double frame_rate = edlsession->frame_rate;
// Seconds between text markings
	double text_interval = 3600.0;
// Seconds between tick marks
	double tick_interval = 3600.0;
	int pixel = 0;

// Calculate tick mark spacing, number spacing, and starting point based
// on zoom, time format, project settings.

// If the time format is for audio, mark round numbers of samples based on
// samplerate.
// Fow low zoom, mark tens of samples.

// If the time format is for video, mark round number of frames based on
// framerate.
// For low zoom, mark individual frames.

	draw_range();

// Number of seconds per pixel
	double time_per_pixel = master_edl->local_session->zoom_time;
// Seconds in each frame
	double frame_seconds = (double)1.0 / frame_rate;
// Starting time of view in seconds.
	ptstime view_start = master_edl->local_session->view_start_pts;
// Ending time of view in seconds
	ptstime view_end = master_edl->local_session->view_start_pts +
		get_w() * time_per_pixel;
// Get minimum distance between text marks
	edlsession->ptstotext(string, view_start);
	int min_pixels1 = get_text_width(MEDIUMFONT, string) + TEXT_MARGIN;
	edlsession->ptstotext(string, view_start);
	int min_pixels2 = get_text_width(MEDIUMFONT, string) + TEXT_MARGIN;
	int min_pixels = (int)MAX(min_pixels1, min_pixels2);

// Minimum seconds between text marks
	double min_time = (double)min_pixels * 
		master_edl->local_session->zoom_time;

// Get first text mark on or before window start
	int starting_mark = 0;

	int progression = 1;

// Default text spacing
	text_interval = 0.5;
	double prev_text_interval = 1.0;

	while(text_interval >= min_time)
	{
		prev_text_interval = text_interval;
		if(progression == 0)
		{
			text_interval /= 2;
			progression++;
		}
		else
		if(progression == 1)
		{
			text_interval /= 2;
			progression++;
		}
		else
		if(progression == 2)
		{
			text_interval /= 2.5;
			progression = 0;
		}
	}

	text_interval = prev_text_interval;

	if(1 >= min_time)
		;
	else
	if(2 >= min_time)
		text_interval = 2;
	else
	if(5 >= min_time)
		text_interval = 5;
	else
	if(10 >= min_time)
		text_interval = 10;
	else
	if(15 >= min_time)
		text_interval = 15;
	else
	if(20 >= min_time)
		text_interval = 20;
	else
	if(30 >= min_time)
		text_interval = 30;
	else
	if(60 >= min_time)
		text_interval = 60;
	else
	if(120 >= min_time)
		text_interval = 120;
	else
	if(300 >= min_time)
		text_interval = 300;
	else
	if(600 >= min_time)
		text_interval = 600;
	else
	if(1200 >= min_time)
		text_interval = 1200;
	else
	if(1800 >= min_time)
		text_interval = 1800;
	else
	if(3600 >= min_time)
		text_interval = 3600;

// Set text interval
	switch(edlsession->time_format)
	{
	case TIME_FEET_FRAMES:
	{
		double foot_seconds = frame_seconds * edlsession->frames_per_foot;
		if(frame_seconds >= min_time)
			text_interval = frame_seconds;
		else
		if(foot_seconds / 8.0 > min_time)
			text_interval = frame_seconds * edlsession->frames_per_foot / 8.0;
		else
		if(foot_seconds / 4.0 > min_time)
			text_interval = frame_seconds * edlsession->frames_per_foot / 4.0;
		else
		if(foot_seconds / 2.0 > min_time)
			text_interval = frame_seconds * edlsession->frames_per_foot / 2.0;
		else
		if(foot_seconds > min_time)
			text_interval = frame_seconds * edlsession->frames_per_foot;
		else
		if(foot_seconds * 2 >= min_time)
			text_interval = foot_seconds * 2;
		else
		if(foot_seconds * 5 >= min_time)
			text_interval = foot_seconds * 5;
		else
		{

			for(int factor = 10, progression = 0; factor <= 100000; )
			{
				if(foot_seconds * factor >= min_time)
				{
					text_interval = foot_seconds * factor;
					break;
				}

				if(progression == 0)
				{
					factor = (int)(factor * 2.5);
					progression++;
				}
				else
				if(progression == 1)
				{
					factor = (int)(factor * 2);
					progression++;
				}
				else
				if(progression == 2)
				{
					factor = (int)(factor * 2);
					progression = 0;
				}
			}

		}
		break;
	}

	case TIME_FRAMES:
	case TIME_HMSF:
// One frame per text mark
		if(frame_seconds >= min_time)
			text_interval = frame_seconds;
		else
		if(frame_seconds * 2 >= min_time)
			text_interval = frame_seconds * 2;
		else
		if(frame_seconds * 5 >= min_time)
			text_interval = frame_seconds * 5;
		else
		{

			for(int factor = 10, progression = 0; factor <= 100000; )
			{
				if(frame_seconds * factor >= min_time)
				{
					text_interval = frame_seconds * factor;
					break;
				}

				if(progression == 0)
				{
					factor = (int)(factor * 2.5);
					progression++;
				}
				else
				if(progression == 1)
				{
					factor = (int)(factor * 2);
					progression++;
				}
				else
				if(progression == 2)
				{
					factor = (int)(factor * 2);
					progression = 0;
				}
			}

		}
		break;

	default:
		break;
	}

// Sanity
	while(text_interval < min_time)
	{
		text_interval *= 2;
	}

// Set tick interval
	tick_interval = text_interval;

	switch(edlsession->time_format)
	{
	case TIME_HMSF:
	case TIME_FEET_FRAMES:
	case TIME_FRAMES:
		if(frame_seconds / time_per_pixel > TICK_SPACING)
			tick_interval = frame_seconds;
		break;
	}

// Get first text mark on or before window start
	starting_mark = master_edl->local_session->view_start_pts / text_interval;

	double start_position = (double)starting_mark * text_interval;
	int iteration = 0;

	while(start_position + text_interval * iteration < view_end)
	{
		double position1 = start_position + text_interval * iteration;
		int pixel = round((position1 - master_edl->local_session->view_start_pts) / time_per_pixel);
		int pixel1 = pixel;

		edlsession->ptstotext(string, position1);
		set_color(get_resources()->default_text_color);
		set_font(MEDIUMFONT);

		draw_text(pixel + TEXT_MARGIN, get_text_ascent(MEDIUMFONT), string);
		draw_line(pixel, LINE_MARGIN, pixel, get_h() - 2);

		double position2 = start_position + text_interval * (iteration + 1);
		int pixel2 = round((position2 -
			master_edl->local_session->view_start_pts) / time_per_pixel);

		for(double tick_position = position1; 
			tick_position < position2; 
			tick_position += tick_interval)
		{
			pixel = round((tick_position -
				master_edl->local_session->view_start_pts) / time_per_pixel);
			if(labs(pixel - pixel1) > 1 &&
				abs(pixel - pixel2) > 1)
				draw_line(pixel, TICK_MARGIN, pixel, get_h() - 2);
		}
		iteration++;
	}
}

void MTimeBar::draw_range()
{
	int x1 = 0, x2 = 0;
	if(master_edl->playable_tracks_of(TRACK_VIDEO) &&
		mwindow->preferences->use_brender)
	{
		double time_per_pixel = (double)master_edl->local_session->zoom_time;
		x1 = round((edlsession->brender_start -
			master_edl->local_session->view_start_pts) / time_per_pixel);
		x2 = round((mainsession->brender_end -
			master_edl->local_session->view_start_pts) / time_per_pixel);
	}

	if(x2 > x1 && 
		x1 < get_w() && 
		x2 > 0)
	{
		draw_top_background(get_parent(), 0, 0, x1, get_h());

		draw_3segmenth(x1, 0, x2 - x1, mwindow->theme->get_image("timebar_brender"));

		draw_top_background(get_parent(), x2, 0, get_w() - x2, get_h());
	}
	else
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
}

void MTimeBar::select_label(ptstime position)
{
	EDL *edl = master_edl;

	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1, 0);

	position = master_edl->align_to_frame(position);

	if(shift_down())
	{
		if(position > edl->local_session->get_selectionend(1) / 2 + 
			edl->local_session->get_selectionstart(1) / 2)
		{
			edl->local_session->set_selectionend(position);
		}
		else
		{
			edl->local_session->set_selectionstart(position);
		}
	}
	else
	{
		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
	}

// Que the CWindow
	mwindow->cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	mwindow->gui->cursor->update();
	mwindow->gui->canvas->activate();
	mwindow->gui->zoombar->update();
	mwindow->gui->patchbay->update();
	mwindow->update_plugin_guis();
	update_highlights();
	mwindow->gui->canvas->flash();
}

void MTimeBar::resize_event()
{
	reposition_window(mwindow->theme->mtimebar_x,
		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h);
	update();
}

int MTimeBar::test_preview(int buttonpress)
{
	return 0;
}
