
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

#ifndef BCPOT_H
#define BCPOT_H

#include "bcpixmap.h"
#include "vframe.inc"
#include "bcsubwindow.h"

#define POT_UP 0
#define POT_HIGH 1
#define POT_DN 2
#define POT_STATES 3

class BC_FPot;
class BC_IPot;
class BC_QPot;
class BC_PercentagePot;

class BC_Pot : public BC_SubWindow
{
public:
	BC_Pot(int x, int y, VFrame **data);
	virtual ~BC_Pot();

	friend class BC_FPot;
	friend class BC_IPot;
	friend class BC_QPot;
	friend class BC_PercentagePot;


	static int calculate_h();
	void initialize();
	virtual float get_percentage() { return 0; };
	virtual int percentage_to_value(float percentage) { return 0; };
	virtual int handle_event() { return 0; };
	virtual const char* get_caption() { return ""; };
	virtual void increase_value() {};
	virtual void decrease_value() {};
	void set_use_caption(int value);

	void reposition_window(int x, int y);
	void repeat_event(int repeat_id);
	int cursor_enter_event();
	void cursor_leave_event();
	int button_press_event();
	virtual int button_release_event();
	int cursor_motion_event();
	int keypress_event();

private:
	void set_data(VFrame **data);
	void draw();
	float percentage_to_angle(float percentage);
	float angle_to_percentage(float angle);
	void angle_to_coords(int &x1, int &y1, int &x2, int &y2, float angle);
	float coords_to_angle(int x2, int y2);
	void show_value_tooltip();

	VFrame **data;
	BC_Pixmap *images[POT_STATES];
	char caption[BCTEXTLEN], temp_tooltip_text[BCTEXTLEN];
	int status;
	int64_t keypress_tooltip_timer;
	float angle_offset;
	float start_cursor_angle;
	float start_needle_angle;
	float prev_angle, angle_correction;
	int use_caption;
};

class BC_FPot : public BC_Pot
{
public:
	BC_FPot(int x, 
		int y, 
		float value, 
		float minvalue, 
		float maxvalue, 
		VFrame **data = 0);
	~BC_FPot();

	const char* get_caption();
	void increase_value();
	void decrease_value();
	float get_percentage();
	float get_value();
	int percentage_to_value(float percentage);
	void update(float value);
	void update(float value, float minvalue, float maxvalue);
	void set_precision(float value);

private:
	float value, minvalue, maxvalue;
	float precision;
};

class BC_IPot : public BC_Pot
{
public:
	BC_IPot(int x, 
		int y, 
		int64_t value, 
		int64_t minvalue, 
		int64_t maxvalue, 
		VFrame **data = 0);
	~BC_IPot();

	const char* get_caption();
	void increase_value();
	void decrease_value();
	float get_percentage();
	int percentage_to_value(float percentage);
	int64_t get_value();
	void update(int64_t value);
	void update(int64_t value, int64_t minvalue, int64_t maxvalue);

private:
	int64_t value, minvalue, maxvalue;
};

class BC_QPot : public BC_Pot
{
public:
	BC_QPot(int x, 
		int y, 
		int64_t value,      // Units of frequencies
		VFrame **data = 0);
	~BC_QPot();

	const char* get_caption();
	void increase_value();
	void decrease_value();
	float get_percentage();
	int percentage_to_value(float percentage);
// Units of frequencies
	int64_t get_value();
// Units of frequencies
	void update(int64_t value);

private:
// Units of frequency index
	int64_t value, minvalue, maxvalue;
};

class BC_PercentagePot : public BC_Pot
{
public:
	BC_PercentagePot(int x, 
		int y, 
		float value, 
		float minvalue, 
		float maxvalue, 
		VFrame **data = 0);
	~BC_PercentagePot();

	const char* get_caption();
	void increase_value();
	void decrease_value();
	float get_percentage();
	float get_value();
	int percentage_to_value(float percentage);
	void update(float value);

private:
	float value, minvalue, maxvalue;
};

#endif
