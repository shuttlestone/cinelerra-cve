
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

#include "awindowgui.h"
#include "awindow.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "keys.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "samplescroll.h"
#include "statusbar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "trackscroll.h"
#include "tracks.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "zoombar.h"

#include <stdarg.h>

// the main window uses its own private colormap for video
MWindowGUI::MWindowGUI(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME,
		mainsession->mwindow_x,
		mainsession->mwindow_y,
		mainsession->mwindow_w,
		mainsession->mwindow_h,
		100,
		100,
		1,
		1,
		1)
{
	this->mwindow = mwindow;
	mbuttons = 0;
	statusbar = 0;
	zoombar = 0;
	samplescroll = 0;
	trackscroll = 0;
	cursor = 0;
	canvas = 0;
	cursor = 0;
	patchbay = 0;
	timebar = 0;
	mainclock = 0;
}

MWindowGUI::~MWindowGUI()
{
	delete mbuttons;
	delete statusbar;
	delete zoombar;
	delete samplescroll;
	delete trackscroll;
	delete cursor;
	delete patchbay;
	delete timebar;
	delete mainclock;
}

void MWindowGUI::get_scrollbars()
{
	int need_xscroll;
	int need_yscroll;
	view_w = mwindow->theme->mcanvas_w;
	view_h = mwindow->theme->mcanvas_h;

// Scrollbars are constitutive
	need_xscroll = need_yscroll = 1;

	if(canvas && (view_w != canvas->get_w() || view_h != canvas->get_h()))
	{
		canvas->reposition_window(mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			view_w,
			view_h);
	}

	if(need_xscroll)
	{
		if(!samplescroll)
			add_subwindow(samplescroll = new SampleScroll(mwindow, 
				mwindow->theme->mhscroll_x, 
				mwindow->theme->mhscroll_y, 
				mwindow->theme->mhscroll_w));
		else
			samplescroll->resize_event();

		samplescroll->set_position();
	}
	else
	{
		if(samplescroll) delete samplescroll;
		samplescroll = 0;
		master_edl->local_session->view_start_pts = 0;
	}

	if(need_yscroll)
	{
		if(!trackscroll)
			add_subwindow(trackscroll = new TrackScroll(mwindow, 
				mwindow->theme->mvscroll_x,
				mwindow->theme->mvscroll_y,
				mwindow->theme->mvscroll_h));
		else
			trackscroll->resize_event();

		trackscroll->set_position(view_h);
	}
	else
	{
		if(trackscroll) delete trackscroll;
		trackscroll = 0;
		master_edl->local_session->track_start = 0;
	}
}

void MWindowGUI::show()
{
	set_icon(mwindow->get_window_icon());

	add_subwindow(mainmenu = new MainMenu(mwindow, this));

	mwindow->theme->get_mwindow_sizes(this, get_w(), get_h());
	mwindow->theme->draw_mwindow_bg(this);

	add_subwindow(mbuttons = new MButtons(mwindow, this));
	mbuttons->show();

	add_subwindow(timebar = new MTimeBar(mwindow, 
		this,
		mwindow->theme->mtimebar_x,
		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h));
	timebar->update();

	add_subwindow(patchbay = new PatchBay(mwindow, this));
	patchbay->show();

	get_scrollbars();

	cursor = new MainCursor(this);

	mwindow->gui->add_subwindow(canvas = new TrackCanvas(mwindow, this));
	canvas->show();

	add_subwindow(zoombar = new ZoomBar(mwindow, this));
	zoombar->show();

	add_subwindow(statusbar = new StatusBar(mwindow, this));
	statusbar->show();

	add_subwindow(mainclock = new MainClock(
		mwindow->theme->mclock_x,
		mwindow->theme->mclock_y,
		mwindow->theme->mclock_w));
	mainclock->set_frame_offset(
		edlsession->get_frame_offset());
	mainclock->update(0);

	canvas->activate();
}

void MWindowGUI::redraw_time_dependancies() 
{
	zoombar->redraw_time_dependancies();
	timebar->update();
	mainclock->update(master_edl->local_session->get_selectionstart(1));
}

void MWindowGUI::focus_out_event()
{
	cursor->focus_out_event();
}

void MWindowGUI::resize_event(int w, int h)
{
	mainsession->mwindow_w = w;
	mainsession->mwindow_h = h;
	mwindow->theme->get_mwindow_sizes(this, w, h);
	mwindow->theme->draw_mwindow_bg(this);
	flash();
	mainmenu->reposition_window(0, 0, w, mainmenu->get_h());
	mbuttons->resize_event();
	statusbar->resize_event();
	timebar->resize_event();
	patchbay->resize_event();
	get_scrollbars();
	canvas->resize_event();
	zoombar->resize_event();
}

void MWindowGUI::update(int options)
{
	master_edl->tracks->update_y_pixels(mwindow->theme);
	if(options & WUPD_SCROLLBARS) this->get_scrollbars();
	if(options & WUPD_TIMEBAR) this->timebar->update();
	if(options & WUPD_ZOOMBAR) this->zoombar->update();
	if(options & WUPD_PATCHBAY) this->patchbay->update();
	if(options & WUPD_CLOCK) this->mainclock->update(
		master_edl->local_session->get_selectionstart(1));
	if(options & WUPD_CANVAS)
	{
		this->canvas->draw(options & WUPD_CANVAS);
		this->canvas->flash();
// Activate causes the menubar to deactivate.  Don't want this for
// picon thread.
		if(!(options & WUPD_CANVPICIGN)) this->canvas->activate();
	}
	if(options & WUPD_BUTTONBAR) mbuttons->update();

// Can't age if the cache called this to draw missing picons
	if((options & (WUPD_CANVREDRAW | WUPD_CANVPICIGN)) == 0)
		mwindow->age_caches();
}

int MWindowGUI::visible(int x1, int x2, int view_x1, int view_x2)
{
	return (x1 >= view_x1 && x1 < view_x2) ||
		(x2 > view_x1 && x2 <= view_x2) ||
		(x1 <= view_x1 && x2 >= view_x2);
}

void MWindowGUI::show_message(const char *fmt, ...)
{
	char bufr[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, 1024, fmt, ap);
	va_end(ap);

	statusbar->status_text->set_color(mwindow->theme->message_normal);
	statusbar->status_text->update(bufr);
}

// Drag motion called from other window
void MWindowGUI::drag_motion()
{
	if(!get_hidden())
		canvas->drag_motion();
}

int MWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	return canvas->drag_stop();
}

void MWindowGUI::default_positions()
{
	reposition_window(mainsession->mwindow_x,
		mainsession->mwindow_y,
		mainsession->mwindow_w,
		mainsession->mwindow_h);
	mwindow->vwindow->gui->reposition_window(mainsession->vwindow_x,
		mainsession->vwindow_y,
		mainsession->vwindow_w,
		mainsession->vwindow_h);
	mwindow->cwindow->gui->reposition_window(mainsession->cwindow_x,
		mainsession->cwindow_y,
		mainsession->cwindow_w,
		mainsession->cwindow_h);
	mwindow->awindow->gui->reposition_window(mainsession->awindow_x,
		mainsession->awindow_y,
		mainsession->awindow_w,
		mainsession->awindow_h);

	resize_event(mainsession->mwindow_w,
		mainsession->mwindow_h);
	mwindow->vwindow->gui->resize_event(mainsession->vwindow_w,
		mainsession->vwindow_h);
	mwindow->cwindow->gui->resize_event(mainsession->cwindow_w,
		mainsession->cwindow_h);
	mwindow->awindow->gui->resize_event(mainsession->awindow_w,
		mainsession->awindow_h);

	flush();
	mwindow->vwindow->gui->flush();
	mwindow->cwindow->gui->flush();
	mwindow->awindow->gui->flush();
}

void MWindowGUI::repeat_event(int duration)
{
	cursor->repeat_event(duration);
}

void MWindowGUI::translation_event()
{
	mainsession->mwindow_x = get_x();
	mainsession->mwindow_y = get_y();
}

void MWindowGUI::save_defaults(BC_Hash *defaults)
{
	defaults->update("MWINDOWWIDTH", get_w());
	defaults->update("MWINDOWHEIGHT", get_h());
	mainmenu->save_defaults(defaults);
	BC_WindowBase::save_defaults(defaults);
}

int MWindowGUI::keypress_event()
{
	int result = 0;
	result = mbuttons->keypress_event();

	if(!result)
	{
		switch(get_keypress())
		{
		case 'e':
			mwindow->toggle_editing_mode();
			result = 1;
			break;
		case LEFT:
			if(!ctrl_down()) 
			{
				if (alt_down())
				{
					mbuttons->transport->handle_transport(STOP, 1, 0);
					mwindow->prev_edit_handle(shift_down());
				}
				else
					mwindow->move_left(); 
				result = 1;
			}
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{
				if (alt_down())
				{
					mbuttons->transport->handle_transport(STOP, 1, 0);
					mwindow->next_edit_handle(shift_down());
				}
				else
					mwindow->move_right(); 
				result = 1;
			}
			break;

		case UP:
			if(ctrl_down() && !alt_down())
			{
				mwindow->expand_y();
				result = 1;
			}
			else
			if(!ctrl_down() && alt_down())
			{
				mwindow->expand_autos(0,1,1);
				result = 1;
			}
			else
			if(ctrl_down() && alt_down())
			{
				mwindow->expand_autos(1,1,1);
				result = 1;
			}
			else
			{
				mwindow->expand_sample();
				result = 1;
			}
			break;

		case DOWN:
			if(ctrl_down() && !alt_down())
			{
				mwindow->zoom_in_y();
				result = 1;
			}
			else
			if(!ctrl_down() && alt_down())
			{
				mwindow->shrink_autos(0,1,1);
				result = 1;
			}
			else
			if(ctrl_down() && alt_down())
			{
				mwindow->shrink_autos(1,1,1);
				result = 1;
			}
			else
			{
				mwindow->zoom_in_sample();
				result = 1;
			}
			break;

		case PGUP:
			if(!ctrl_down())
			{
				mwindow->move_up();
				result = 1;
			}
			else
			{
				mwindow->expand_t();
				result = 1;
			}
			break;

		case PGDN:
			if(!ctrl_down())
			{
				mwindow->move_down();
				result = 1;
			}
			else
			{
				mwindow->zoom_in_t();
				result = 1;
			}
			break;

		case TAB:
		case LEFTTAB:
			int cursor_x, cursor_y;

			canvas->get_relative_cursor_pos(&cursor_x, &cursor_y);

			if(get_keypress() == TAB)
			{
// Switch the record button
				for(Track *track = master_edl->first_track(); track; track = track->next)
				{
					int track_x, track_y, track_w, track_h;
					canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

					if(cursor_y >= track_y && 
						cursor_y < track_y + track_h)
					{
						if (track->record)
							track->record = 0;
						else
							track->record = 1;
						result = 1; 
						break;
					}
				}
			}
			else
			{
				Track *this_track = 0;
				for(Track *track = master_edl->first_track(); track; track = track->next)
				{
					int track_x, track_y, track_w, track_h;
					canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

					if(cursor_y >= track_y &&
						cursor_y < track_y + track_h)
					{
						// This is our track
						this_track = track;
						break;
					}
				}

				int total_selected = master_edl->total_toggled(Tracks::RECORD);

// nothing previously selected
				if(total_selected == 0)
				{
					master_edl->set_all_toggles(Tracks::RECORD,
						1);
				}
				else
				if(total_selected == 1)
				{
// this patch was previously the only one on
					if(this_track && this_track->record)
					{
						master_edl->set_all_toggles(Tracks::RECORD,
							1);
					}
// another patch was previously the only one on
					else
					{
						master_edl->set_all_toggles(Tracks::RECORD,
							0);
						if(this_track)
							this_track->record = 1;

					}
				}
				else
				if(total_selected > 1)
				{
					master_edl->set_all_toggles(Tracks::RECORD,
						0);
					if(this_track)
						this_track->record = 1;
				}

			}

			update (WUPD_CANVINCR | WUPD_PATCHBAY | WUPD_BUTTONBAR);
			mwindow->cwindow->update(WUPD_OVERLAYS | WUPD_TOOLWIN);

			result = 1;
			break;
		}

// since things under cursor have changed...
		if(result) 
			cursor_motion_event(); 
	}

	return result;
}

void MWindowGUI::close_event() 
{
	mainmenu->quit();
}

int MWindowGUI::menu_h()
{
	return mainmenu->get_h();
}
