
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

#ifndef FILEBASE_H
#define FILEBASE_H

#include "asset.inc"
#include "assets.inc"
#include "colormodels.h"
#include "datatype.h"
#include "edit.inc"
#include "guicast.h"
#include "file.inc"
#include "filelist.inc"
#include "overlayframe.inc"
#include "strategies.inc"
#include "vframe.inc"

#include <sys/types.h>

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
	virtual ~FileBase();


	friend class File;
	friend class FileList;
	friend class FrameWriter;




	int get_mode(char *mode, int rd, int wr);
	int reset_parameters();



	virtual void get_parameters(BC_WindowBase *parent_window, 
			Asset *asset, 
			BC_WindowBase **format_window,
			int audio_options,
			int video_options,
			int lock_compressor) {};



	virtual int get_index(const char *index_path) { return 1; };
	virtual int check_header() { return 0; };  // Test file to see if it is of this type.
	virtual int reset_parameters_derived() {};
	virtual int read_header() {};     // WAV files for getting header
	virtual int open_file(int rd, int wr) {};
	virtual int close_file();
	virtual int close_file_derived() {};
	int set_dither();
	virtual int seek_end() { return 0; };
	virtual int seek_start() { return 0; };
	virtual framenum get_video_position() { return 0; };
	virtual samplenum get_audio_position() { return 0; };
	virtual int set_video_position(framenum x) { return 0; };
	virtual int set_audio_position(samplenum x) { return 0; };

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual int64_t get_memory_usage() { return 0; };

	virtual int write_samples(double **buffer, 
		int len) { return 0; };
	virtual int write_frames(VFrame ***frames, int len) { return 0; };
	virtual int read_compressed_frame(VFrame *buffer) { return 0; };
	virtual int write_compressed_frame(VFrame *buffers) { return 0; };
	virtual int compressed_frame_size() { return 0; };
// Doubles are used to allow resampling
	virtual int read_samples(double *buffer, int len) { return 0; };


	virtual int prefer_samples_float() {return 0;};
	virtual int read_samples_float(float *buffer, int len) { return 0; };

	virtual int read_frame(VFrame *frame) { return 1; };

// Return either the argument or another colormodel which read_frame should
// use.
	virtual int colormodel_supported(int colormodel) { return BC_RGB888; };
// This file can copy compressed frames directly from the asset
	virtual int can_copy_from(Edit *edit, int64_t position) { return 0; }; 
	virtual int get_render_strategy(ArrayList<int>* render_strategies) { return VRENDER_VPIXEL; };

protected:
// Return 1 if the render_strategy is present on the list.
	static int search_render_strategies(ArrayList<int>* render_strategies, int render_strategy);

// convert samples into file format
	int64_t samples_to_raw(char *out_buffer, 
					float **in_buffer, // was **buffer
					int input_len, 
					int bits, 
					int channels,
					int byte_order,
					int signed_);

// overwrites the buffer from PCM data depending on feather.
	int raw_to_samples(float *out_buffer, const char *in_buffer, 
		int samples, int bits, int channels, int channel, int feather, 
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// Overwrite the buffer from float data using feather.
	int overlay_float_buffer(float *out_buffer, float *in_buffer, 
		int samples, 
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// allocate a buffer for translating int to float
	int get_audio_buffer(char **buffer, int len, int bits, int channels); // audio

// Allocate a buffer for feathering floats
	int get_float_buffer(float **buffer, int len);

// allocate a buffer for translating video to VFrame
	int get_video_buffer(unsigned char **buffer, int depth); // video
	static int match4(const char *in, const char *out);   // match 4 bytes for a quicktime type

	char *audio_buffer_in, *audio_buffer_out;    // for raw audio reads and writes
	float *float_buffer;          // for floating point feathering
	unsigned char *video_buffer_in, *video_buffer_out;
	unsigned char **row_pointers_in, **row_pointers_out;

	int prev_bytes; // determines if new raw buffer is needed and used for getting memory usage
	int prev_len;
	int prev_track;
	int prev_layer;
	Asset *asset;
	int wr, rd;
	int dither;
	int internal_byte_order;
	File *file;

private:



// ================================= Audio compression
// ULAW
	float ulawtofloat(char ulaw);
	char floattoulaw(float value);
	int generate_ulaw_tables();
	int delete_ulaw_tables();
	float *ulawtofloat_table, *ulawtofloat_ptr;
	unsigned char *floattoulaw_table, *floattoulaw_ptr;
};

#endif
