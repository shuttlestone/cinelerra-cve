
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

#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

#include "aframe.inc"
#include "asset.inc"
#include "condition.inc"
#include "datatype.h"
#include "edit.inc"
#include "filebase.inc"
#include "file.inc"
#include "filethread.inc"
#include "filexml.inc"
#include "formattools.h"
#include "framecache.inc"
#include "mutex.inc"
#include "pluginserver.inc"
#include "resample.inc"
#include "vframe.inc"

// ======================================= include file types here



// generic file opened by user
class File
{
public:
	File();
	~File();

// Get attributes for various file formats.
	void get_options(FormatTools *format, int options);

	void raise_window();
// Close parameter window
	void close_window();

// ===================================== start here
	void set_processors(int cpus);   // Set the number of cpus for certain codecs.

// Detect image list
	int is_imagelist(int format);

// Format may be preset if the asset format is not 0.
	int open_file(Asset *asset, int open_method);

// Get index from the file if one exists.  Returns 0 on success.
	int get_index(const char *index_path);

// start a thread for writing to avoid blocking during record
	void start_audio_thread(int buffer_size, int ring_buffers);
	void stop_audio_thread();
// The ring buffer must either be 1 or 2.
// The buffer_size for video needs to be > 1 on SMP systems to utilize 
// multiple processors.
// For audio it's the number of samples per buffer.
	void start_video_thread(int buffer_size, 
		int color_model, 
		int ring_buffers);
	void stop_video_thread();

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	void close_file(int ignore_thread = 0);

// get length of file normalized to base samplerate
	samplenum get_audio_length();

// write audio frames
// written to disk and file pointer updated after
// return 1 if failed
	int write_aframes(AFrame **buffer);

// Only called by filethread to write an array of an array of channels of frames.
	int write_frames(VFrame ***frames, int len);



// For writing buffers in a background thread use these functions to get the buffer.
// Get a pointer to a buffer to write to.
	AFrame** get_audio_buffer();
	VFrame*** get_video_buffer();

// Schedule a buffer for writing on the thread.
// thread calls write_samples
	int write_audio_buffer(int len);
	int write_video_buffer(int len);

// Read audio into aframe
// aframe->source_duration secs starting from aframe->source_pts
	int get_samples(AFrame *aframe);


// pts API - frame must have source_pts, and layer set
	int get_frame(VFrame *frame);
// adjust source pts and duration
	void adjust_times(VFrame *frame, ptstime pts, ptstime src_pts);

// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Edit *edit, int output_w, int output_h) { return 0; };

// Get nearest colormodel that can be decoded without a temporary frame.
// Used by read_frame.
	int colormodel_supported(int colormodel);

// Used by CICache to calculate the total size of the cache.
// Based on temporary frames and a call to the file subclass.
// The return value is limited 1MB each in case of audio file.
// The minimum setting for cache_size should be bigger than 1MB.
	size_t get_memory_usage();

// Returns SUPPORTS_* bits for format
	static int supports(int format);

	Asset *asset;    // Copy of asset since File outlives EDL
	FileBase *file; // virtual class for file type
// Threads for writing data in the background.
	FileThread *audio_thread, *video_thread; 

// Temporary storage for color conversions
	VFrame *temp_frame;

// Resampling engine
	Resample *resample;
	Resample_float *resample_float;

// Lock writes while recording video and audio.
// A binary lock won't do.  We need a FIFO lock.
	Condition *write_lock;
	int cpus;

private:
	void reset_parameters();
	int get_this_frame(framenum pos, VFrame *frame, int is_thread = 0);

	int getting_options;
	BC_WindowBase *format_window;
	Mutex *format_completion;
	int writing;
	VFrame *last_frame;
};

#endif
