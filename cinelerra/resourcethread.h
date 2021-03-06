
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

#ifndef RESOURCETHREAD_H
#define RESOURCETHREAD_H

// This thread tries to draw picons into the timeline, asynchronous
// of the navigation.

// TrackCanvas draws the picons which are in the cache and makes a table of
// picons and locations which need to be decompressed.  Then ResourceThread
// decompresses the picons and draws them one by one, refreshing the
// entire trackcanvas in the process.


#include "aframe.inc"
#include "arraylist.h"
#include "condition.inc"
#include "datatype.h"
#include "resourcepixmap.inc"
#include "thread.h"
#include "vframe.inc"


class ResourceThreadItem
{
public:
	ResourceThreadItem(ResourcePixmap *pixmap, 
		Asset *asset,
		int data_type,
		int operation_count);
	virtual ~ResourceThreadItem() {};

	ResourcePixmap *pixmap;
	Asset *asset;
	int data_type;
	int operation_count;
	int last;
};


class AResourceThreadItem : public ResourceThreadItem
{
public:
	AResourceThreadItem(ResourcePixmap *pixmap,
		Asset *asset,
		int x,
		int channel,
		samplenum start,
		samplenum end,
		int operation_count);

	int x;
	int channel;
	samplenum start;
	samplenum end;
};


class VResourceThreadItem : public ResourceThreadItem
{
public:
	VResourceThreadItem(ResourcePixmap *pixmap,
		int picon_x, 
		int picon_y, 
		int picon_w,
		int picon_h,
		ptstime postime,
		ptstime duration,
		int layer,
		Asset *asset,
		int operation_count);

	int picon_x;
	int picon_y;
	int picon_w;
	int picon_h;
	ptstime postime;
	ptstime duration;
	int layer;
};


class ResourceThread : public Thread
{
public:
	ResourceThread();
	~ResourceThread();

// reset - delete all picons.  Used for index building.
	void stop_draw(int reset);
	void start_draw();

// Be sure to stop_draw before changing the asset table, 
// closing files.
	void add_picon(ResourcePixmap *pixmap, 
		int picon_x, 
		int picon_y, 
		int picon_w,
		int picon_h,
		ptstime position,
		ptstime duration,
		int layer,
		Asset *asset);

	void add_wave(ResourcePixmap *pixmap,
		Asset *asset,
		int x,
		int channel,
// samples relative to asset rate
		samplenum source_start,
		samplenum source_end);

	void run();

	void do_video(VResourceThreadItem *item);
	void do_audio(AResourceThreadItem *item);

	size_t get_cache_size();
	void cache_delete_oldest();
	void reset_caches();
	void remove_asset_from_caches(Asset *asset);
	void show_cache_status(int indent);

	Condition *draw_lock;
	Mutex *item_lock;
	ArrayList<ResourceThreadItem*> items;
	int interrupted;

// Current audio buffer for spanning multiple pixels
	AFrame *aframe;
	int audio_asset_id;

// Waveform state
	int prev_x;
	double prev_h;
	double prev_l;
// Incremented after every start_draw to prevent overlapping operations
	int operation_count;
	CICache *audio_cache;
	CICache *video_cache;
	FrameCache *frame_cache;
	WaveCache *wave_cache;
};

#endif

