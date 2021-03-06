
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

#ifndef FILEFORMAT_H
#define FILEFORMAT_H

class FileFormatByteOrderLOHI;
class FileFormatByteOrderHILO;
class FileFormatSigned;
class FileFormatHeader;
class FileFormatChannels;
class FileFormatBits;

#include "asset.inc"
#include "bctextbox.h"
#include "bcwindow.h"
#include "file.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "selection.inc"

class FileFormat : public BC_Window
{
public:
	FileFormat(Asset *asset, const char *string2, int absx, int absy);
	~FileFormat();

	Asset *asset;
	FileFormatByteOrderLOHI *lohi;
	FileFormatByteOrderHILO *hilo;
	FileFormatSigned *signed_button;
	FileFormatHeader *header_button;
	Selection *rate_button;
	FileFormatChannels *channels_button;
};


class FileFormatChannels : public BC_TumbleTextBox
{
public:
	FileFormatChannels(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};


class FileFormatHeader : public BC_TextBox
{
public:
	FileFormatHeader(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};


class FileFormatByteOrderLOHI : public BC_Radial
{
public:
	FileFormatByteOrderLOHI(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};


class FileFormatByteOrderHILO : public BC_Radial
{
public:
	FileFormatByteOrderHILO(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};


class FileFormatSigned : public BC_CheckBox
{
public:
	FileFormatSigned(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};

#endif
