
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

#ifndef RESIZETRACKTHREAD_H
#define RESIZETRACKTHREAD_h

#include "mutex.inc"
#include "mwindow.inc"
#include "resizetrackthread.inc"
#include "selection.h"
#include "track.inc"


class ResizeTrackWindow;

class ResizeTrackThread : public Thread
{
public:
	ResizeTrackThread();
	~ResizeTrackThread();

	void start_window(Track *track);
private:
	void run();

public:
	ResizeTrackWindow *window;
	Track *track;
	int w, h;
	int w1, h1;
	double w_scale, h_scale;
};

class ResizeTrackScaleW : public BC_TextBox
{
public:
	ResizeTrackScaleW(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);

	int handle_event();
private:
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};

class ResizeTrackScaleH : public BC_TextBox
{
public:
	ResizeTrackScaleH(ResizeTrackWindow *gui, 
		ResizeTrackThread *thread,
		int x,
		int y);

	int handle_event();
private:
	ResizeTrackWindow *gui;
	ResizeTrackThread *thread;
};


class ResizeTrackWindow : public BC_Window
{
public:
	ResizeTrackWindow(ResizeTrackThread *thread,
		int x,
		int y);

	void update(int changed_scale,
		int changed_size);
private:
	ResizeTrackThread *thread;
	FrameSizeSelection *framesize_selection;
	ResizeTrackScaleW *w_scale;
	ResizeTrackScaleH *h_scale;
};


class SetTrackFrameSize : public FrameSizeSelection
{
public:
	SetTrackFrameSize(int x1, int y1, int x2, int y2,
		ResizeTrackWindow *base, int *value1, int *value2);

	int handle_event();
private:
	ResizeTrackWindow *gui;
};

#endif
