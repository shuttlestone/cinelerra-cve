
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

#include "automation.inc"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"
#include "localsession.h"

FloatAutos::FloatAutos(EDL *edl,
	Track *track,
	float default_value)
 : Autos(edl, track)
{
	this->default_value = default_value;
	type = AUTOMATION_TYPE_FLOAT;
}

void FloatAutos::straighten(ptstime start, ptstime end)
{
	FloatAuto *current = (FloatAuto*)first;

	while(current)
	{
		FloatAuto *previous_auto = (FloatAuto*)PREVIOUS;
		FloatAuto *next_auto = (FloatAuto*)NEXT;

// Is current auto in range?
		if(current->pos_time >= start && current->pos_time < end)
		{
			float current_value = current->get_value();

// Determine whether to set the control in point.
			if(previous_auto && previous_auto->pos_time >= start)
			{
				float previous_value = previous_auto->get_value();
				current->set_control_in_value((previous_value - current_value) / 3.0);
			}

// Determine whether to set the control out point
			if(next_auto && next_auto->pos_time < end)
			{
				float next_value = next_auto->get_value();
				current->set_control_out_value((next_value - current_value) / 3.0);
			}
		}
		current = (FloatAuto*)NEXT;
	}
}

Auto* FloatAutos::new_auto()
{
	FloatAuto *result = new FloatAuto(edl, this);
	result->set_value(default_value);
	return result;
}

int FloatAutos::automation_is_constant(ptstime start,
	ptstime length,
	double &constant)
{
	int total_autos = total();
	ptstime end;

	end = start + length;

// No keyframes on track
	if(total_autos == 0)
	{
		constant = default_value;
		return 1;
	}
	else
// Only one keyframe on track.
	if(total_autos == 1)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}
	else
// Last keyframe is before region
	if(last->pos_time <= start)
	{
		constant = ((FloatAuto*)last)->get_value();
		return 1;
	}
	else
// First keyframe is after region
	if(first->pos_time > end)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}

// Scan sequentially
	ptstime prev_position = -1;
	for(Auto *current = first; current; current = NEXT)
	{
		int test_current_next = 0;
		int test_previous_current = 0;
		FloatAuto *float_current = (FloatAuto*)current;

// keyframes before and after region but not in region
		if(prev_position >= 0 &&
			prev_position < start && 
			current->pos_time >= end)
		{
// Get value now in case change doesn't occur
			constant = float_current->get_value();
			test_previous_current = 1;
		}
		prev_position = current->pos_time;

// Keyframe occurs in the region
		if(!test_previous_current &&
			current->pos_time < end && 
			current->pos_time >= start)
		{

// Get value now in case change doesn't occur
			constant = float_current->get_value();

// Keyframe has neighbor
			if(current->previous)
				test_previous_current = 1;

			if(current->next)
				test_current_next = 1;
		}

		if(test_current_next)
		{
			FloatAuto *float_next = (FloatAuto*)current->next;

// Change occurs between keyframes
			if(!EQUIV(float_current->get_value(), float_next->get_value()) ||
					!EQUIV(float_current->get_control_out_value(), 0) ||
					!EQUIV(float_next->get_control_in_value(), 0))
				return 0;
		}

		if(test_previous_current)
		{
			FloatAuto *float_previous = (FloatAuto*)current->previous;

// Change occurs between keyframes
			if(!EQUIV(float_current->get_value(), float_previous->get_value()) ||
					!EQUIV(float_current->get_control_in_value(), 0) ||
					!EQUIV(float_previous->get_control_out_value(), 0))
				return 0;
		}
	}

// Got nothing that changes in the region.
	return 1;
}

float FloatAutos::get_value(ptstime position,
	FloatAuto* previous,
	FloatAuto* next)
{
// Calculate bezier equation at position

// prev and next will be used to shorten the search, if given
	previous = (FloatAuto*)get_prev_auto(position, (Auto*)previous);
	next = (FloatAuto*)get_next_auto(position, (Auto*)next);

// Constant
	if(!next && !previous)
		return default_value;
	else
	if(!previous)
		return next->get_value();
	else
	if(!next)
		return previous->get_value();
	else
	if(next == previous)
		return previous->get_value();
	else
	{
		if(EQUIV(previous->get_value(), next->get_value()) &&
				EQUIV(previous->get_control_out_value(), 0) &&
				EQUIV(next->get_control_in_value(), 0))
			return previous->get_value();
	}

// at this point: previous and next not NULL, positions differ, value not constant.

	return calculate_bezier(previous, next, position);
}

float FloatAutos::calculate_bezier(FloatAuto *previous, FloatAuto *next, ptstime position)
{
	if(EQUIV(next->pos_time, previous->pos_time))
		return previous->get_value();

	float y0 = previous->get_value();
	float y3 = next->get_value();

// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
	float t =  (position - previous->pos_time) /
		(next->pos_time - previous->pos_time);

	float tpow2 = t * t;
	float tpow3 = t * t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	float invtpow3 = invt * invt * invt;

	float result = (  invtpow3 * y0
		+ 3 * t     * invtpow2 * y1
		+ 3 * tpow2 * invt     * y2 
		+     tpow3            * y3);

	return result;
}

float FloatAutos::calculate_bezier_derivation(FloatAuto *previous, FloatAuto *next, ptstime position)
// calculate the slope of the interpolating bezier function at given position.
{
	float scale = next->pos_time - previous->pos_time;
	if(fabsf(scale) < EPSILON)
		if(fabs(previous->get_control_out_pts()) > EPSILON)
			return previous->get_control_out_value() / previous->get_control_out_pts();
		else
			return 0;

	float y0 = previous->get_value();
	float y3 = next->get_value();

// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
	// normalized scale
	float t = (position - previous->pos_time) / scale;

	float tpow2 = t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;

	float slope = 3 * (
		- invtpow2              * y0
		- invt * ( 2*t - invt ) * y1
		+ t    * ( 2*invt - t ) * y2
		+ tpow2                 * y3);

	return slope / scale;
}

void FloatAutos::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	ptstime start,
	ptstime end)
{
// Use default auto
	if(!first)
	{
		if(*coords_undefined)
		{
			*min = *max = default_value;
			*coords_undefined = 0;
		}

		*min = MIN(default_value, *min);
		*max = MAX(default_value, *max);
	}

// Test all handles
	for(FloatAuto *current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
	{
		if(current->pos_time >= start && current->pos_time < end)
		{
			if(*coords_undefined)
			{
				*min = *max = current->get_value();
				*coords_undefined = 0;
			}

			*min = MIN(current->get_value(), *min);
			*min = MIN(current->get_value() + current->get_control_in_value(), *min);
			*min = MIN(current->get_value() + current->get_control_out_value(), *min);

			*max = MAX(current->get_value(), *max);
			*max = MAX(current->get_value() + current->get_control_in_value(), *max);
			*max = MAX(current->get_value() + current->get_control_out_value(), *max);
		}
	}

// Test joining regions
	ptstime step = MAX(track->one_unit, edl->local_session->zoom_time);

	for(ptstime position = start;
		position < end;
		position += step)
	{
		float value = get_value(position);

		if(*coords_undefined)
		{
			*min = *max = value;
			*coords_undefined = 0;
		}
		else
		{
			*min = MIN(value, *min);
			*max = MAX(value, *max);
		}
	}
}

void FloatAutos::copy_values(Autos *autos)
{
	default_value = ((FloatAutos*)autos)->default_value;
}

size_t FloatAutos::get_size()
{
	size_t size = sizeof(*this);

	for(FloatAuto* current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
		size += current->get_size();
	return size;
}

void FloatAutos::dump(int ident)
{
	printf("%*sFloatAutos %p dump(%d): base %.3f default %.3f\n", ident, "", this,
		total(), base_pts, default_value);
	ident += 2;
	for(FloatAuto* current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
		current->dump(ident);
}
