
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

#define GL_GLEXT_PROTOTYPES

#include "clip.h"
#include "colormodels.inc"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "rgb601.h"
#include "rgb601window.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


RGB601Config::RGB601Config()
{
	direction = 0;
}

RGB601Main::RGB601Main(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

RGB601Main::~RGB601Main()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void RGB601Main::load_defaults()
{
	defaults = load_defaults_file("rgb601.rc");

	config.direction = defaults->get("DIRECTION", config.direction);
}

void RGB601Main::save_defaults()
{
	defaults->update("DIRECTION", config.direction);
	defaults->save();
}

int RGB601Main::load_configuration()
{
	KeyFrame *prev_keyframe;

	prev_keyframe = prev_keyframe_pts(source_pts);
// Must also switch between interpolation between keyframes and using first keyframe
	read_data(prev_keyframe);
	return 1;
}

void RGB601Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("RGB601");
	output.tag.set_property("DIRECTION", config.direction);
	output.append_tag();
	output.tag.set_title("/RGB601");
	output.append_tag();
	keyframe->set_data(output.string);
}

void RGB601Main::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	float new_threshold;

	while(!input.read_tag())
	{
		if(input.tag.title_is("RGB601"))
		{
			config.direction = input.tag.get_property("DIRECTION", config.direction);
		}
	}

	if(thread)
	{
		thread->window->update();
	}
}


#define CREATE_TABLE(max) \
{ \
	for(int i = 0; i < max; i++) \
	{ \
		int forward_output = (int)((double)0.8588 * i + max * 0.0627 + 0.5); \
		int reverse_output = (int)((double)1.1644 * i - max * 0.0627 + 0.5); \
		forward_table[i] = CLIP(forward_output, 0, max - 1); \
		reverse_table[i] = CLIP(reverse_output, 0, max - 1); \
	} \
}

void RGB601Main::create_table(VFrame *input_ptr)
{
	switch(input_ptr->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
	case BC_RGBA8888:
	case BC_YUVA8888:
		CREATE_TABLE(0x100);
		break;

	case BC_RGB161616:
	case BC_YUV161616:
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		CREATE_TABLE(0x10000);
		break;
	}
}

#define PROCESS(table, type, components, yuv) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)input_ptr->get_row_ptr(i); \
		type *out_row = (type*)output_ptr->get_row_ptr(i); \
 \
		if(yuv) \
		{ \
/* Just do Y */ \
			for(int j = 0; j < w; j++) \
			{ \
				out_row[j * components] = table[(int)in_row[j * components]]; \
				out_row[j * components + 1] = in_row[j * components + 1]; \
				out_row[j * components + 2] = in_row[j * components + 2]; \
			} \
		} \
		else \
		if(sizeof(type) == 4) \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				if(table == forward_table) \
				{ \
					out_row[j * components] = (type)(in_row[j * components] * 0.8588 + 0.0627); \
					out_row[j * components + 1] = (type)(in_row[j * components + 1] * 0.8588 + 0.0627); \
					out_row[j * components + 2] = (type)(in_row[j * components + 2] * 0.8588 + 0.0627); \
				} \
				else \
				{ \
					out_row[j * components] = (type)(in_row[j * components] * 1.1644 - 0.0627); \
					out_row[j * components + 1] = (type)(in_row[j * components + 1] * 1.1644 - 0.0627); \
					out_row[j * components + 2] = (type)(in_row[j * components + 2] * 1.1644 - 0.0627); \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				out_row[j * components] = table[(int)in_row[j * components]]; \
				out_row[j * components + 1] = table[(int)in_row[j * components + 1]]; \
				out_row[j * components + 2] = table[(int)in_row[j * components + 2]]; \
			} \
		} \
	} \
}

#define PROCESS_A(table, type) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)input_ptr->get_row_ptr(i); \
		type *out_row = (type*)output_ptr->get_row_ptr(i); \
 \
/* Just do Y */ \
		for(int j = 0; j < w; j++) \
		{ \
			out_row[j * 4 + 1] = table[(int)in_row[j * 4 + 1]]; \
			out_row[j * 4 + 2] = in_row[j * 4 + 2]; \
			out_row[j * 4 + 3] = in_row[j * 4 + 3]; \
		} \
	} \
}

void RGB601Main::process(VFrame *input_ptr)
{
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	VFrame *output_ptr = input_ptr;

	if(config.direction == 1)
		switch(input_ptr->get_color_model())
		{
		case BC_YUV888:
			PROCESS(forward_table, unsigned char, 3, 1);
			break;
		case BC_YUVA8888:
			PROCESS(forward_table, unsigned char, 4, 1);
			break;
		case BC_YUV161616:
			PROCESS(forward_table, u_int16_t, 3, 1);
			break;
		case BC_YUVA16161616:
			PROCESS(forward_table, u_int16_t, 4, 1);
			break;
		case BC_RGB888:
			PROCESS(forward_table, unsigned char, 3, 0);
			break;
		case BC_RGBA8888:
			PROCESS(forward_table, unsigned char, 4, 0);
			break;
		case BC_RGB_FLOAT:
			PROCESS(forward_table, float, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			PROCESS(forward_table, float, 4, 0);
			break;
		case BC_RGB161616:
			PROCESS(forward_table, u_int16_t, 3, 0);
			break;
		case BC_RGBA16161616:
			PROCESS(forward_table, u_int16_t, 4, 0);
			break;
		case BC_AYUV16161616:
			PROCESS_A(forward_table, u_int16_t);
			break;
		}
	else
	if(config.direction == 2)
		switch(input_ptr->get_color_model())
		{
		case BC_YUV888:
			PROCESS(reverse_table, unsigned char, 3, 1);
			break;
		case BC_YUVA8888:
			PROCESS(reverse_table, unsigned char, 4, 1);
			break;
		case BC_YUV161616:
			PROCESS(reverse_table, u_int16_t, 3, 1);
			break;
		case BC_YUVA16161616:
			PROCESS(reverse_table, u_int16_t, 4, 1);
			break;
		case BC_RGB888:
			PROCESS(reverse_table, unsigned char, 3, 0);
			break;
		case BC_RGBA8888:
			PROCESS(reverse_table, unsigned char, 4, 0);
			break;
		case BC_RGB_FLOAT:
			PROCESS(reverse_table, float, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			PROCESS(reverse_table, float, 4, 0);
			break;
		case BC_RGB161616:
			PROCESS(reverse_table, u_int16_t, 3, 0);
			break;
		case BC_RGBA16161616:
			PROCESS(reverse_table, u_int16_t, 4, 0);
			break;
		case BC_AYUV16161616:
			PROCESS_A(reverse_table, u_int16_t);
			break;
		}
}

VFrame *RGB601Main::process_tmpframe(VFrame *frame)
{
	load_configuration();

	if(config.direction)
	{
		create_table(frame);
		process(frame);
	}
	return frame;
}

void RGB601Main::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *yuv_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 0.8588 + 0.0627;\n"
		"}\n";
	static const char *yuv_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 1.1644 - 0.0627;\n"
		"}\n";
	static const char *rgb_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(0.8588, 0.8588, 0.8588) + vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";
	static const char *rgb_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(1.1644, 1.1644, 1.1644) - vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->bind_texture(0);

	unsigned int frag_shader = 0;
	switch(get_output()->get_color_model())
	{
	case BC_YUV888:
	case BC_YUVA8888:
		frag_shader = VFrame::make_shader(0,
			config.direction == 1 ? yuv_fwd_frag : yuv_rev_frag,
			0);
		break;

	default:
		frag_shader = VFrame::make_shader(0,
			config.direction == 1 ? rgb_fwd_frag : rgb_rev_frag,
			0);
		break;
	}

	if(frag_shader)
	{
		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
	}
	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
