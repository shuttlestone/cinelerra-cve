
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

#include "autos.h"
#include "automation.inc"
#include "clip.h"
#include "edl.inc"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "localsession.h"

FloatAuto::FloatAuto(EDL *edl, FloatAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	control_in_pts = 0;
	control_out_pts = 0;
	pos_valid = -1;    // "dirty"
	tangent_mode = TGNT_FREE;
//  note: in most cases the tangent_mode-value is set
//        by the method interpolate_from() rsp. copy_from()
}

FloatAuto::~FloatAuto()
{
	// as we are going away, the neighbouring float auto nodes
	// need to re-adjust their ctrl point positions and tangents
	if(is_floatauto_node(this))
	{
		if(next)
			((FloatAuto*)next)->tangent_dirty();
		if (previous)
			((FloatAuto*)previous)->tangent_dirty();
	}
}

int FloatAuto::operator==(Auto &that)
{
	return identical((FloatAuto*)&that);
}

int FloatAuto::operator==(FloatAuto &that)
{
	return identical((FloatAuto*)&that);
}

inline bool FloatAuto::is_floatauto_node(Auto *candidate)
{
	return (candidate && candidate->autos &&
		AUTOMATION_TYPE_FLOAT == candidate->autos->get_type());
}

int FloatAuto::identical(FloatAuto *src)
{
	if(src == this)
		return 1;

	return EQUIV(value, src->value) &&
		EQUIV(control_in_value, src->control_in_value) &&
		EQUIV(control_out_value, src->control_out_value);
// ctrl positions ignored, as they may depend on neighbours
// tangent_mode is ignored, no recalculations
}

void FloatAuto::copy_from(Auto *that)
{
	copy_from((FloatAuto*)that);
}

void FloatAuto::copy_from(FloatAuto *that)
{
	Auto::copy_from(that);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->control_in_pts = that->control_in_pts;
	this->control_out_pts = that->control_out_pts;
	this->tangent_mode = that->tangent_mode;
// note: literate copy, no recalculations
}

inline void FloatAuto::handle_automatic_tangent_after_copy()
// in most cases, we don't want to use the manual tangent modes
// of the left neighbour used as a template for interpolation.
// Rather, we (re)set to automatically smoothed tangents. Note
// auto generated nodes (while tweaking values) indeed are
// inserted by using this "interpolation" approach, thus making
// this defaulting to auto-smooth tangents very important.
{
	if(tangent_mode == TGNT_FREE || tangent_mode == TGNT_TFREE)
	{
		this->tangent_mode = TGNT_SMOOTH;
	}
}

void FloatAuto::interpolate_from(Auto *a1, Auto *a2, ptstime pos, Auto *templ)
// bézier interpolates this->value and tangents for the given position
// between the positions of a1 and a2. If a1 or a2 are omitted, they default
// to this->previous and this->next. If this FloatAuto has automatic tangents,
// this may trigger re-adjusting of this and its neighbours in this->autos.
// Note while a1 and a2 need not be members of this->autos, automatic
// readjustments are always done to the neighbours in this->autos.
// If the template is given, it will be used to fill out this
// objects fields prior to interpolating.
{
	if(!a1) a1 = previous;
	if(!a2) a2 = next;
	Auto::interpolate_from(a1, a2, pos, templ);
	handle_automatic_tangent_after_copy();

	// set this->value using bézier interpolation if possible
	if(is_floatauto_node(a1) && is_floatauto_node(a2) &&
		a1->pos_time <= pos && pos <= a2->pos_time)
	{
		FloatAuto *left = (FloatAuto*)a1;
		FloatAuto *right = (FloatAuto*)a2;

		if(!PTSEQU(pos, pos_time))
		{
			// this may trigger smoothing
			adjust_to_new_coordinates(pos,
				FloatAutos::calculate_bezier(left, right, pos));
		}

		float new_slope = FloatAutos::calculate_bezier_derivation(left, right, pos);
		this->set_control_in_value(new_slope * control_in_pts);
		this->set_control_out_value(new_slope * control_out_pts);
	}
	else
		adjust_ctrl_positions(); // implies adjust_tangents()
}

void FloatAuto::change_tangent_mode(tgnt_mode new_mode)
{
	if(new_mode == TGNT_TFREE && !(control_in_pts && control_out_pts))
		new_mode = TGNT_FREE; // only if tangents on both sides...

	tangent_mode = new_mode;
	adjust_tangents();
}

void FloatAuto::toggle_tangent_mode()
{
	switch (tangent_mode)
	{
	case TGNT_SMOOTH:
		change_tangent_mode(TGNT_TFREE);
		break;
	case TGNT_LINEAR:
		change_tangent_mode(TGNT_FREE);
		break;
	case TGNT_TFREE:
		change_tangent_mode(TGNT_LINEAR);
		break;
	case TGNT_FREE:
		change_tangent_mode(TGNT_SMOOTH);
		break;
	}
}

void FloatAuto::set_value(float newvalue)
{
	this->value = newvalue;
	this->adjust_tangents();
	if(previous)
		((FloatAuto*)previous)->adjust_tangents();
	if(next)
		((FloatAuto*)next)->adjust_tangents();
}

void FloatAuto::add_value(float increment)
{
	value += increment;
	adjust_tangents();

	if(previous)
		((FloatAuto*)previous)->adjust_tangents();
	if(next)
		((FloatAuto*)next)->adjust_tangents();
}

void FloatAuto::set_control_in_value(float newvalue)
{
	switch(tangent_mode)
	{
	case TGNT_TFREE:
		control_out_value = control_out_pts * newvalue / control_in_pts;
	default:
		control_in_value = newvalue;
	}
}

void FloatAuto::set_control_out_value(float newvalue)
{
	switch(tangent_mode)
	{
	case TGNT_TFREE:
		control_in_value = control_in_pts * newvalue / control_out_pts;
	default:
		control_out_value = newvalue;
	}
}

inline int sgn(float value)
{
	return (value == 0) ? 0 : (value < 0) ? -1 : 1;
}

inline float weighted_mean(float v1, float v2, float w1, float w2)
{
	if(0.000001 > fabs(w1 + w2))
		return 0;
	else
		return (w1 * v1 + w2 * v2) / (w1 + w2);
}

void FloatAuto::adjust_tangents()
// recalculates tangents if current mode
// implies automatic adjustment of tangents
{
	if(!autos) return;

	if(tangent_mode == TGNT_SMOOTH)
	{
		// normally, one would use the slope of chord between the neighbours.
		// but this could cause the curve to overshot extremal automation nodes.
		// (e.g when setting a fade node at zero, the curve could go negative)
		// we can interpret the slope of chord as a weighted mean value, where
		// the length of the interval is used as weight; we just use other
		// weights: intervall length /and/ reciprocal of slope. So, if the
		// connection to one of the neighbours has very low slope this will
		// dominate the calculated tangent slope at this automation node.
		// if the slope goes beyond the zero line, e.g if left connection
		// has positive and right connection has negative slope, then
		// we force the calculated tangent to be horizontal.

		float s, dxl, dxr, sl, sr;

		calculate_slope((FloatAuto*) previous, this, sl, dxl);
		calculate_slope(this, (FloatAuto*) next, sr, dxr);

		if(0 < sgn(sl) * sgn(sr))
		{
			float wl = fabs(dxl) * (fabs(1.0/sl) + 1);
			float wr = fabs(dxr) * (fabs(1.0/sr) + 1);
			s = weighted_mean(sl, sr, wl, wr);
		}
		else
			s = 0; // fixed hoizontal tangent

		control_in_value = s * control_in_pts;
		control_out_value = s * control_out_pts;
	}
	else
	if(tangent_mode == TGNT_LINEAR)
	{
		float g, dx;

		if(previous)
		{
			calculate_slope(this, (FloatAuto*)previous, g, dx);
			control_in_value = g * dx / 3;
		}
		if(next)
		{
			calculate_slope(this, (FloatAuto*)next, g, dx);
			control_out_value = g * dx / 3;
		}
	}
	else
	if(tangent_mode == TGNT_TFREE && control_in_pts && control_out_pts)
	{
		float gl = control_in_value / control_in_pts;
		float gr = control_out_value / control_out_pts;
		float wl = fabs(control_in_value);
		float wr = fabs(control_out_value);
		float g = weighted_mean(gl, gr, wl, wr);

		control_in_value = g * control_in_pts;
		control_out_value = g * control_out_pts;
	}
}

inline void FloatAuto::calculate_slope(FloatAuto *left, 
		FloatAuto *right, float &dvdx, float &dx)
{
	dvdx = 0;
	dx = 0;
	if(!left || !right)
		return;

	dx = right->pos_time - left->pos_time;
	float dv = right->value - left->value;
	dvdx = (fabsf(dx) < EPSILON) ? 0 : dv/dx;
}

void FloatAuto::adjust_ctrl_positions(FloatAuto *prev, FloatAuto *next)
// recalculates location of ctrl points to be
// always 1/3 and 2/3 of the distance to the
// next neighbours. The reason is: for this special
// distance the bézier function yields x(t) = t, i.e.
// we can use the y(t) as if it was a simple function y(x).

// This adjustment is done only on demand and involves
// updating neighbours and adjust_tangents() as well.
{
	if(!prev && !next)
	{ // use current siblings
		prev = (FloatAuto*)this->previous;
		next = (FloatAuto*)this->next;
	}

	if(prev)
	{
		set_ctrl_positions(prev, this);
		prev->adjust_tangents();
	}
	else // disable tangent on left side
		control_in_pts = 0;

	if(next)
	{
		set_ctrl_positions(this, next);
		next->adjust_tangents();
	}
	else // disable right tangent
		control_out_pts = 0;

	this->adjust_tangents();
	pos_valid = pos_time;
// tangents up-to-date
}

inline void redefine_tangent(ptstime &old_pos, ptstime new_pos, float &ctrl_val)
{
	if(old_pos > EPSILON)
		ctrl_val *= new_pos / old_pos;
	old_pos = new_pos;
}

inline void FloatAuto::set_ctrl_positions(FloatAuto *prev, FloatAuto* next)
{
	ptstime distance = next->pos_time - prev->pos_time;
	redefine_tangent(prev->control_out_pts, +distance / 3, prev->control_out_value);
	redefine_tangent(next->control_in_pts, -distance / 3, next->control_in_value);
}

void FloatAuto::adjust_to_new_coordinates(ptstime position, float value)
// define new position and value in one step, do necessary re-adjustments
{
	this->value = value;
	this->pos_time = position;
	adjust_ctrl_positions();
}

void FloatAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value / 2.0);  // compatibility, see below
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value / 2.0);
	file->tag.set_property("TANGENT_MODE", tangent_mode);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void FloatAuto::copy(Auto *src, ptstime start, ptstime end)
{
	FloatAuto *that = (FloatAuto*)src;

	pos_time = that->pos_time - start;
	value = that->value;
	control_in_value = that->control_in_value;
	control_out_value = that->control_out_value;
	tangent_mode = that->tangent_mode;
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	tangent_mode = (tgnt_mode)file->tag.get_property("TANGENT_MODE", TGNT_FREE);

	// Compatibility to old session data format:
	// Versions previous to the bezier auto patch (Jun 2006) applied a factor 2
	// to the y-coordinates of ctrl points while calculating the bezier function.
	// To retain compatibility, we now apply this factor while loading
	control_in_value *= 2.0;
	control_out_value *= 2.0;

// restore ctrl positions and adjust tangents if necessary
	adjust_ctrl_positions();

}

size_t FloatAuto::get_size()
{
	return sizeof(*this);
}

void FloatAuto::dump(int ident)
{
	const char *s;

	switch(tangent_mode)
	{
	case TGNT_SMOOTH:
		s = "smooth";
		break;
	case TGNT_LINEAR:
		s = "linear";
		break;
	case TGNT_TFREE:
		s = "tangent";
		break;
	case TGNT_FREE:
		s = "disjoint";
		break;
	default:
		s = "tangent unknown";
		break;
	}
	printf("%*sFloatAuto %p: pos_time %.3lf value %.3f\n",
		ident, "", this, pos_time, value);
	ident += 2;
	printf("%*scontrols in %.2f/%.3f, out %.2f/%.3f %s\n", ident, "",
		control_in_value, control_in_pts, control_out_value, control_out_pts, s);
}
