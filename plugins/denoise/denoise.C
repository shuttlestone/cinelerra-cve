
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


#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "denoise.h"
#include "picon_png.h"
#include "units.h"

#define WINDOW_BORDER (window_size / 2)
#define SGN(x) ((x) < 0 ? -1: 1)

REGISTER_PLUGIN

DenoiseEffect::DenoiseEffect(PluginServer *server)
 : PluginAClient(server)
{
	first_window = 1;
	thread = 0;
	ex_coeff_d = 0;
	ex_coeff_r = 0;
	ex_coeff_rn = 0;
	wave_coeff_d = 0;
	wave_coeff_r = 0;
	decomp_filter = 0;
	recon_filter = 0;
	input_buffer = 0;
	output_buffer = 0;
	input_size = 0;
	output_size = 0;
	input_allocation = 0;
	output_allocation = 0;
	dsp_iteration = 0;
	in_scale = 0;
	out_scale = 0;
	dsp_in = 0;
	dsp_out = 0;
	initialized = 0;

	alpha = 1.359803732;
	beta = -0.782106385;
	window_size = 4096;
	output_level = 1.0;
	levels = 1;
	iterations = 1;

	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseEffect::~DenoiseEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(ex_coeff_d) delete ex_coeff_d;
	if(ex_coeff_r) delete ex_coeff_r;
	if(ex_coeff_rn) delete ex_coeff_rn;
	if(wave_coeff_d) delete wave_coeff_d;
	if(wave_coeff_r) delete wave_coeff_r;
	if(decomp_filter) delete decomp_filter;
	if(recon_filter) delete recon_filter;
	if(input_buffer) delete [] input_buffer;
	if(output_buffer) delete [] output_buffer;
	if(dsp_in) delete [] dsp_in;
	if(dsp_out) delete [] dsp_out;
	if(dsp_iteration) delete [] dsp_iteration;
}

PLUGIN_CLASS_METHODS

void DenoiseEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DENOISE"))
			{
				config.level = input.tag.get_property("LEVEL", config.level);
			}
		}
	}
}

void DenoiseEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DENOISE");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/DENOISE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DenoiseEffect::load_defaults()
{
	defaults = load_defaults_file("denoise.rc");

	config.level = defaults->get("LEVEL", config.level);
}

void DenoiseEffect::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->save();
}

double DenoiseEffect::dot_product(double *data, double *filter, char filtlen)
{
	static int i;
	static double sum;

	sum = 0.0;
	for(i = 0; i < filtlen; i++) sum += *data-- * *filter++;
	return sum;
}

void DenoiseEffect::convolve_dec_2(double *input_sequence, 
	int length,
	double *filter, 
	int filtlen, 
	double *output_sequence)
{
// convolve the input sequence with the filter and decimate by two
	int i, shortlen, offset;
	int lengthp4 = length + 4;
	int lengthm4 = length - 4;
	int lengthp5 = length + 5;
	int lengthp8 = length + 8;

	for(i = 0; (i <= lengthp8) && ((i - filtlen) <= lengthp8); i += 2)
	{
		if(i < filtlen)
			*output_sequence++ = dot_product(input_sequence + i,
				filter, i + 1);
		else 
		if(i > lengthp5)
		{
			offset = i - lengthm4;
			shortlen = filtlen - offset;
			*output_sequence++ = dot_product(input_sequence + lengthp4,
				filter + offset, shortlen);
		}
		else
			*output_sequence++ = dot_product(input_sequence + i, filter, filtlen);
	}
}

int DenoiseEffect::decompose_branches(double *in_data, 
	int length, 
	WaveletFilters *decomp_filter, 
	double *out_low, 
	double *out_high)
{
// Take input data and filters and form two branches of half the
// original length. Length of branches is returned.
	convolve_dec_2(in_data, length, decomp_filter->h, decomp_filter->length, out_low);
	convolve_dec_2(in_data, length, decomp_filter->g, decomp_filter->length, out_high);
	return(length / 2);
}

void DenoiseEffect::wavelet_decomposition(double *in_data, 
	int in_length, 
	double **out_data)
{
	for(int i = 0; i < levels; i++)
	{
		in_length = decompose_branches(in_data, 
			in_length, 
			decomp_filter, 
			out_data[2 * i], 
			out_data[(2 * i) + 1]);

		in_data = out_data[2 * i];
	}
}

void DenoiseEffect::tree_copy(double **output, 
	double **input, 
	int length, 
	int levels)
{
	register int i, j, k, l, m;

	for(i = 0, k = 1; k < levels; i++, k++)
	{
		length /= 2;
		l = 2 * i;
		m = l + 1;

		for(j = 0; j < length + 5; j++)
		{
			output[l][j] = 0.0;
			output[m][j] = input[m][j];
		}
	}

	length /= 2;
	l = 2 * i;
	m = l + 1;

	for(j = 0; j < length + 5; j++)
	{
		output[l][j] = input[l][j];
		output[m][j] = input[m][j];
	}
}

void DenoiseEffect::threshold(int window_size, double gammas, int levels)
{
	int i, j;
	double threshold, cv, cvb, abs_coeff_r;
	double *coeff_r, *coeff_l;
	int length;

	for(i = 0; i < levels; i++) 
	{
		length = (window_size >> (i + 1)) + 5;
		threshold = sqrt(2 * log(length) / log(2)) * gammas / sqrt(length);

		for(j = 0; j < length; j++) 
		{
			coeff_r = &(ex_coeff_r->values[(2 * i) + 1][j]);
			coeff_l = &(ex_coeff_rn->values[(2 * i) + 1][j]);

			cv = SGN(*coeff_r);
			abs_coeff_r = fabs(*coeff_r);
			cvb = abs_coeff_r - threshold;
			cv *= cvb;

			if(abs_coeff_r > threshold) 
			{
				*coeff_r = cv;
				*coeff_l = 0.0;
			}
			else 
			{
				*coeff_l = *coeff_r;
				*coeff_r = 0.0;
			}
		}
	}
}

double DenoiseEffect::dot_product_even(double *data, double *filter, int filtlen)
{
	static int i;
	static double sum;

	sum = 0.0;
	for(i = 0; i < filtlen; i += 2)
		sum += *data-- * filter[i];
	return sum;
}

double DenoiseEffect::dot_product_odd(double *data, double *filter, int filtlen)
{
	static int i;
	static double sum;

	sum = 0.0;
	for(i = 1; i < filtlen; i += 2)
		sum += *data-- * filter[i];
	return sum;
}

void DenoiseEffect::convolve_int_2(double *input_sequence, 
	int length,
	double *filter, 
	int filtlen, 
	int sum_output, 
	double *output_sequence)
// insert zeros between each element of the input sequence and
// convolve with the filter to interpolate the data
{
	register int i, j;
	int endpoint = length + filtlen - 2;

	if (sum_output)
	{
// summation with previous convolution
// every other dot product interpolates the data
		for(i = (filtlen / 2) - 1, j = (filtlen / 2); i < endpoint; i++, j++)
		{
			*output_sequence++ += dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ += dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		*output_sequence++ += dot_product_odd(input_sequence + i, filter, filtlen);
	}
	else
	{
// first convolution of pair
// every other dot product interpolates the data
		for(i = (filtlen / 2) - 1, j = (filtlen / 2); i < endpoint; i++, j++)
		{
			*output_sequence++ = dot_product_odd(input_sequence + i,
				filter, filtlen);
			*output_sequence++ = dot_product_even(input_sequence + j,
				filter, filtlen);
		}
		*output_sequence++ = dot_product_odd(input_sequence + i, filter, filtlen);
	}
}

int DenoiseEffect::reconstruct_branches(double *in_low, 
	double *in_high, 
	int in_length,
	WaveletFilters *recon_filter, 
	double *output)
{
// take input data and filters and form two branches of half the
// original length. length of branches is returned
	convolve_int_2(in_low, in_length, recon_filter->h,
			recon_filter->length, 0, output);
	convolve_int_2(in_high, in_length, recon_filter->g,
			recon_filter->length, 1, output);
	return in_length * 2;
}

void DenoiseEffect::wavelet_reconstruction(double **in_data, 
	int in_length, 
	double *out_data)
{
	double *output;
	int i;

	in_length = in_length >> levels;
// destination of all but last branch reconstruction is the next
// higher intermediate approximation
	for(i = levels - 1; i > 0; i--)
	{
		output = in_data[2 * (i - 1)];
		in_length = reconstruct_branches(in_data[2 * i], 
			in_data[(2 * i) + 1],
			in_length, 
			recon_filter, 
			output);
	}

// destination of the last branch reconstruction is the output data
	reconstruct_branches(in_data[0], 
		in_data[1], 
		in_length, 
		recon_filter, 
		out_data);
}

void DenoiseEffect::process_window()
{
	int i, j;
	for(j = 0; j < iterations; j++)
	{
		wavelet_decomposition(dsp_in, window_size, ex_coeff_d->values);

		tree_copy(ex_coeff_r->values, ex_coeff_d->values, window_size, levels);
		tree_copy(ex_coeff_rn->values, ex_coeff_d->values, window_size, levels);

// qualify coeffs
		threshold(window_size, config.level * 10.0, levels);

		wavelet_reconstruction(ex_coeff_r->values, window_size, dsp_iteration);
		wavelet_reconstruction(ex_coeff_rn->values, window_size, dsp_in);

		for(i = 0; i < window_size; i++)
			dsp_out[i] += dsp_iteration[i];
	}
}

AFrame *DenoiseEffect::process_tmpframe(AFrame *input)
{
	int size = input->length;

	load_configuration();

	if(!initialized)
	{
		int size_factor = (int)(pow(2, levels));
		dsp_in = new double[window_size * size_factor];
		dsp_out = new double[window_size * 2];
		dsp_iteration = new double[window_size * 2];

		ex_coeff_d = new Tree(window_size, levels);
		ex_coeff_r = new Tree(window_size, levels);
		ex_coeff_rn = new Tree(window_size, levels);
		wave_coeff_d = new WaveletCoeffs(alpha, beta);
		wave_coeff_r = new WaveletCoeffs(alpha, beta);
		decomp_filter = new WaveletFilters(wave_coeff_d, DECOMP);
		recon_filter = new WaveletFilters(wave_coeff_r, RECON);
		in_scale = 65535 / sqrt(window_size) / iterations;
		out_scale = output_level / 65535 * sqrt(window_size);
		initialized = 1;
	}

// Append input buffer
	if(input_size + size > input_allocation)
	{
		double *new_input = new double[input_size + size];
		if(input_buffer)
		{
			memcpy(new_input, input_buffer, sizeof(double) * input_size);
			delete [] input_buffer;
		}
		input_buffer = new_input;
		input_allocation = input_size + size;
	}
	memcpy(input_buffer + input_size, 
		input->buffer,
		size * sizeof(double));
	input_size += size;

// Have enough to do some windows
	while(input_size >= window_size)
	{
// Load dsp_in
		for(int i = 0; i < window_size; i++)
		{
			dsp_in[i] = input_buffer[i] * in_scale;
		}
		memset(dsp_out, 0, sizeof(double) * window_size);

// First window produces garbage
		if(!first_window)
			process_window();
		first_window = 0;

// Crossfade into the output buffer
		int new_allocation = output_size + window_size;
		if(new_allocation > output_allocation)
		{
			double *new_output = new double[new_allocation];

			if(output_buffer)
			{
				memcpy(new_output, output_buffer, sizeof(double) * output_size);
				delete [] output_buffer;
			}
			output_buffer = new_output;
			output_allocation = new_allocation;
		}

		if(output_size >= WINDOW_BORDER)
		{
			for(int i = 0, j = output_size - WINDOW_BORDER; 
				i < WINDOW_BORDER; 
				i++, j++)
			{
				double src_level = (double)i / WINDOW_BORDER;
				double dst_level = (double)(WINDOW_BORDER - i) / WINDOW_BORDER;
				output_buffer[j] = output_buffer[j] * dst_level + out_scale * dsp_out[i] * src_level;
			}

			for(int i = 0; i < window_size - WINDOW_BORDER; i++)
				output_buffer[output_size + i] = dsp_out[WINDOW_BORDER + i] * out_scale;
			output_size += window_size - WINDOW_BORDER;
		}
		else
		{
// First buffer has no crossfade
			memcpy(output_buffer + output_size, 
				dsp_out, 
				sizeof(double) * window_size);
			output_size += window_size;
		}
// Shift input buffer forward
		for(int i = window_size - WINDOW_BORDER, j = 0; 
			i < input_size; 
			i++, j++)
			input_buffer[j] = input_buffer[i];
		input_size -= window_size - WINDOW_BORDER;
	}
// Have enough to send to output
	if(output_size - WINDOW_BORDER >= size)
	{
		memcpy(input->buffer, output_buffer, sizeof(double) * size);
		for(int i = size, j = 0; i < output_size; i++, j++)
			output_buffer[j] = output_buffer[i];
		output_size -= size;
	}
	else
	{
		memset(input->buffer, 0, sizeof(double) * size);
	}
	return input;
}


Tree::Tree(int input_length, int levels)
{
	this->input_length = input_length;
	this->levels = levels;
	int i, j;

// create decomposition tree
	values = new double*[2 * levels];
	j = input_length;
	for(i = 0; i < levels; i++)
	{
		j /= 2;
		if(j == 0)
		{
			levels = i;
			continue;
		}
		values[2 * i] = new double[j + 5];
		values[2 * i + 1] = new double[j + 5];
	}
}

Tree::~Tree()
{
	int i;

	for(i = 2 * levels - 1; i >= 0; i--)
		delete values[i];

	delete values;
}

WaveletCoeffs::WaveletCoeffs(double alpha, double beta)
{
	int i;
	double tcosa = cos(alpha);
	double tcosb = cos(beta);
	double tsina = sin(alpha);
	double tsinb = sin(beta);

// calculate first two wavelet coefficients  a = a(-2) and b = a(-1)
	values[0] = ((1.0 + tcosa + tsina) * (1.0 - tcosb - tsinb)
		+ 2.0 * tsinb * tcosa) / 4.0;
	values[1] = ((1.0 - tcosa + tsina) * (1.0 + tcosb - tsinb)
		- 2.0 * tsinb * tcosa) / 4.0;

	tcosa = cos(alpha - beta);
	tsina = sin(alpha - beta);

// calculate last four wavelet coefficients  c = a(0), d = a(1), 
// e = a(2), and f = a(3)
	values[2]  = (1.0 + tcosa + tsina) / 2.0;
	values[3]  = (1.0 + tcosa - tsina) / 2.0;
	values[4]  = 1 - values[0] - values[2];
	values[5]  = 1 - values[1] - values[3];

// zero out very small coefficient values caused by truncation error
	for (i = 0; i < 6; i++)
	{
		if (fabs(values[i]) < 1.0e-15) values[i] = 0.0;
	}
}

WaveletFilters::WaveletFilters(WaveletCoeffs *wave_coeffs, wavetype transform)
{
	int i, j, k;

// find the first non-zero wavelet coefficient
	i = 0;
	while(wave_coeffs->values[i] == 0.0)
		i++;

// find the last non-zero wavelet coefficient
	j = 5;
	while(wave_coeffs->values[j] == 0.0)
		j--;

// Form the decomposition filters h~ and g~ or the reconstruction
// filters h and g.  The division by 2 in the construction
// of the decomposition filters is for normalization.
	length = j - i + 1;
	for(k = 0; k < length; k++)
	{
		if(transform == DECOMP)
		{
			h[k] = wave_coeffs->values[j--] / 2.0;
			g[k] = (double)(((i++ & 0x01) * 2) - 1) *
				wave_coeffs->values[i] / 2.0;
		}
		else
		{
			h[k] = wave_coeffs->values[i++];
			g[k] = (double) (((j-- & 0x01) * 2) - 1) *
				wave_coeffs->values[j];
		}
	}

// clear out the additional array locations, if any
	while (k < 6)
	{
		h[k] = 0.0;
		g[k++] = 0.0;
	}
}


DenoiseConfig::DenoiseConfig()
{
	level = 1.0;
}

void DenoiseConfig::copy_from(DenoiseConfig &that)
{
	level = that.level;
}

int DenoiseConfig::equivalent(DenoiseConfig &that)
{
	return EQUIV(level, that.level);
}

void DenoiseConfig::interpolate(DenoiseConfig &prev, 
	DenoiseConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->level = prev.level * prev_scale + next.level * next_scale;
}

PLUGIN_THREAD_METHODS

DenoiseWindow::DenoiseWindow(DenoiseEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	280,
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Level:")));
	x += 70;
	add_subwindow(scale = new DenoiseLevel(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DenoiseWindow::update()
{
	scale->update(plugin->config.level);
}


DenoiseLevel::DenoiseLevel(DenoiseEffect *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.level, 0, 1.0)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int DenoiseLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}
