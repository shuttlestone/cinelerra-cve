
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

#ifndef ASSET_INC
#define ASSET_INC


class Asset;

// Index statuses
#define INDEX_READY     0
#define INDEX_NOTTESTED 1
#define INDEX_BUILDING  2
#define INDEX_TOOSMALL  3

// Options of defaults
#define ASSET_FORMAT       1
#define ASSET_COMPRESSION  2
#define ASSET_PATH         4
#define ASSET_TYPES        8
#define ASSET_BITS         0x10
#define ASSET_HEADER       0x20

// All bits exept ASSET_HEADER
#define ASSET_ALL          0x1f

// bits of dump options
#define ASSETDUMP_RENDERPTRS    1
#define ASSETDUMP_RENDERPARAMS  2
#define ASSETDUMP_DECODERPTRS   4
#define ASSETDUMP_DECODERPARAMS 8
#endif
