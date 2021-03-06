
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

#ifndef RESAMPLE_H
#define RESAMPLE_H

#define BPC 160
#define BLACKSIZE 25

#include "aframe.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "file.inc"

class Resample
{
public:
	Resample(File *file, int channels);
	~Resample();

// Reset after seeking
	void reset(int channel = -1);
	double blackman(int i, double offset, double fcn, int l);
// Query output temp
	int get_output_size(int channel);
	void read_output(double *output, int channel, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(double *input,
		int in_len,
		int in_rate,
		int out_rate,
		int channel);
// Resample from the file handler and store in *output.
// Returns the total samples read from the file handler.
	int resample(double *output, 
		int out_len,
		int in_rate,
		int out_rate,
		int channel,
		samplenum out_position);      // Starting sample in output samplerate

// History buffer for resampling.
	double *old[MAX_CHANNELS];
	double itime[MAX_CHANNELS];

// Unaligned resampled output
	double **output_temp;

// Total samples in unaligned output
// Tied to each channel independantly
	int output_size[MAX_CHANNELS];

// Allocation of unaligned output
	int output_allocation;
// input chunk
	AFrame *inframe;
// Sample end of input chunks in the input domain.
	samplenum input_chunk_end[MAX_CHANNELS];
	int input_size;
	int channels;
	int resample_init[MAX_CHANNELS];
// Last sample ratio configured to
	double last_ratio;
	double blackfilt[2 * BPC + 1][BLACKSIZE];
	File *file;
// Determine whether to reset after a seek
// Sample end of last buffer read for each channel
	samplenum last_out_end[MAX_CHANNELS];
};

class Resample_float
{
public:
	Resample_float(File *file, int channels);
	~Resample_float();

// Reset after seeking
	void reset(int channel = -1);
	float blackman(int i, float offset, float fcn, int l);
// Query output temp
	int get_output_size(int channel);
	void read_output(double *output, int channel, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(float *input,
		int in_len,
		int in_rate,
		int out_rate,
		int channel);
// Resample from the file handler and store in *output.
// Returns the total samples read from the file handler.
	int resample(double *output, 
		int out_len,
		int in_rate,
		int out_rate,
		int channel,
		samplenum out_position);      // Starting sample in output samplerate

// History buffer for resampling.
	float *old[MAX_CHANNELS];
	float itime[MAX_CHANNELS];

// Unaligned resampled output
	double **output_temp;

// Total samples in unaligned output
// Tied to each channel independantly
	int output_size[MAX_CHANNELS];

// Allocation of unaligned output
	int output_allocation;
// input chunk
	AFrame *inframe;
// Sample end of input chunks in the input domain.
	samplenum input_chunk_end[MAX_CHANNELS];
	int input_size;
	int channels;
	int resample_init[MAX_CHANNELS];
// Last sample ratio configured to
	float last_ratio;
	float blackfilt[2 * BPC + 1][BLACKSIZE];
	File *file;
// Determine whether to reset after a seek
// Sample end of last buffer read for each channel
	samplenum last_out_end[MAX_CHANNELS];
};

#endif
