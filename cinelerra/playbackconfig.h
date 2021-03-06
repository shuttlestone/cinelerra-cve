
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

#ifndef PLAYBACKCONFIG_H
#define PLAYBACKCONFIG_H

#include "audiodevice.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "bchash.inc"
#include "playbackconfig.inc"

// This structure is passed to the driver for configuration during playback
class AudioOutConfig
{
public:
	AudioOutConfig(int duplex);

	int operator!=(AudioOutConfig &that);
	int operator==(AudioOutConfig &that);
	AudioOutConfig& operator=(AudioOutConfig &that);
	void copy_from(AudioOutConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	void set_fragment_size(const char *val);
	void set_fragment_size(int val);
	int get_fragment_size(int sample_rate);
	const char *fragment_size_text(void);

// Offset for synchronization in seconds
	ptstime audio_offset;

// Change default titles for duplex
	int duplex;
	int driver;
	int oss_enable[MAXDEVICES];
	char oss_out_device[MAXDEVICES][BCTEXTLEN];
	int oss_out_bits;

	char esound_out_server[BCTEXTLEN];
	int esound_out_port;

// ALSA options
	char alsa_out_device[BCTEXTLEN];
	int alsa_out_bits;
private:
	int fragment_size;
	char frag_text[32];
};

// This structure is passed to the driver
class VideoOutConfig
{
public:
	VideoOutConfig();

	int operator!=(VideoOutConfig &that);
	int operator==(VideoOutConfig &that);
	VideoOutConfig& operator=(VideoOutConfig &that);
	void copy_from(VideoOutConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	char* get_path();

	int driver;
	char lml_out_device[BCTEXTLEN];

// X11 options
	char x11_host[BCTEXTLEN];
	int x11_use_fields;

// Values for x11_use_fields
	enum
	{
		USE_NO_FIELDS,
		USE_EVEN_FIRST,
		USE_ODD_FIRST
	};

// Picture quality
	int brightness;
	int hue;
	int color;
	int contrast;
	int whiteness;
};

class PlaybackConfig
{
public:
	PlaybackConfig();
	~PlaybackConfig();

	PlaybackConfig& operator=(PlaybackConfig &that);
	void copy_from(PlaybackConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);

	char hostname[BCTEXTLEN];
	int port;

	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
};

#endif
