
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "edl.inc"
#include "filexml.h"
#include "cropauto.h"
#include "cropautos.h"
#include "track.h"

CropAuto::CropAuto(EDL *edl, CropAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	left = 0;
	top = 0;
	if(autos)
	{
		right = autos->track->track_w;
		bottom = autos->track->track_h;
	}
	else
		right = bottom = 0;
	apply_before_plugins = 0;
}

int CropAuto::identical(CropAuto *that)
{
	if(this == that)
		return 1;
	return that->left == left && that->right == right &&
		that->top == top && that->bottom == bottom &&
		that->apply_before_plugins == apply_before_plugins;
}

void CropAuto::load(FileXML *file)
{
	left = file->tag.get_property("LEFT", left);
	right = file->tag.get_property("RIGHT", right);
	top = file->tag.get_property("TOP", top);
	bottom = file->tag.get_property("BOTTOM", bottom);
	apply_before_plugins = file->tag.get_property("APPLY_BEFORE_PLUGINS",
		apply_before_plugins);
}

void CropAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("LEFT", left);
	file->tag.set_property("RIGHT", right);
	file->tag.set_property("TOP", top);
	file->tag.set_property("BOTTOM", bottom);
	file->tag.set_property("APPLY_BEFORE_PLUGINS", apply_before_plugins);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void CropAuto::copy(Auto *src, ptstime start, ptstime end)
{
	CropAuto *that = (CropAuto*)src;

	pos_time = that->pos_time - start;
	left = that->left;
	right = that->right;
	top = that->top;
	bottom = that->bottom;
	apply_before_plugins = that->apply_before_plugins;
}

void CropAuto::copy_from(Auto *that)
{
	copy_from((CropAuto*)that);
}

void CropAuto::copy_from(CropAuto *that)
{
	Auto::copy_from(that);
	left = that->left;
	right = that->right;
	top = that->top;
	bottom = that->bottom;
	apply_before_plugins = that->apply_before_plugins;
}

size_t CropAuto::get_size()
{
	return sizeof(*this);
}

void CropAuto::dump(int indent)
{
	printf("%*sCropAuto %p: pos_time: %.3f (%d,%d),(%d,%d) before %d\n",
		indent, "", this, pos_time, left, top, right, bottom,
		apply_before_plugins);
}
