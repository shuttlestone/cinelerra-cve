
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

#ifndef MASKAUTO_H
#define MASKAUTO_H


#include "arraylist.h"
#include "auto.h"
#include "maskauto.inc"
#include "maskautos.inc"

class MaskPoint
{
public:
	MaskPoint();

	size_t get_size();
	int operator==(MaskPoint& ptr);
	MaskPoint& operator=(MaskPoint& ptr);

	int x;
	int y;
// Incoming acceleration
	float control_x1, control_y1;
// Outgoing acceleration
	float control_x2, control_y2;
};

class SubMask
{
public:
	SubMask(MaskAuto *keyframe);

	int operator==(SubMask& ptr);
	void copy_from(SubMask& ptr);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	size_t get_size();
	void dump(int indent = 0);

	ArrayList<MaskPoint*> points;
	MaskAuto *keyframe;
};

class MaskAuto : public Auto
{
public:
	MaskAuto(EDL *edl, MaskAutos *autos);
	~MaskAuto();

	int operator==(Auto &that);
	int operator==(MaskAuto &that);
	int identical(MaskAuto *src);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void copy_from(Auto *src);
	void interpolate_from(Auto *a1, Auto *a2, ptstime position, Auto *templ = 0);
	void interpolate_values(ptstime pts, int *new_value, int *new_feather);
	void copy_from(MaskAuto *src);

	size_t get_size();
	void dump(int indent = 0);
// Retrieve submask with clamping
	SubMask* get_submask(int number);
// Translates all submasks
	void translate_submasks(int translate_x, int translate_y);

	ArrayList<SubMask*> masks;

	int feather;
// 0 - 100
	int value;
	int apply_before_plugins;
};

#endif
