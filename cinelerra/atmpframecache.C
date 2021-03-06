
/*
 * CINELERRA
 * Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>
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
#include "atmpframecache.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "mutex.h"

// Cache macimal size is 1% of memory
// It is divisor here
#define MAX_ALLOC 100

Mutex ATmpFrameCache::listlock("ATmpFrameCache::listlock");


ATmpFrameCacheElem::ATmpFrameCacheElem(int buflen)
 : ListItem<ATmpFrameCacheElem>()
{
	length = buflen;
	frame = new AFrame(buflen);
	in_use = 0;
	age = 0;
}

ATmpFrameCacheElem::~ATmpFrameCacheElem()
{
	delete frame;
}

size_t ATmpFrameCacheElem::get_size()
{
	return sizeof(AFrame) + frame->get_data_size();
}

void ATmpFrameCacheElem::dump(int indent)
{
	printf("%*sATmpFrameCacheElem %p dump:\n", indent, "", this);
	printf("%*s length %d in_use %d age %u frame %p\n", indent + 2, "",
		length, in_use, age, frame);
}

ATmpFrameCache::ATmpFrameCache()
 : List<ATmpFrameCacheElem>()
{
	moment = 0;
}

ATmpFrameCache::~ATmpFrameCache()
{
	while(last)
		delete last;
}

AFrame *ATmpFrameCache::get_tmpframe(int buflen)
{
	ATmpFrameCacheElem *elem = 0;

	listlock.lock("ATmpFrameCache::get_tmpframe");

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && buflen == cur->length)
		{
			elem = cur;
			break;
		}
	}
	if(!elem)
	{
		for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
		{
			if(!cur->in_use)
			{
				elem = cur;
				elem->frame->check_buffer(buflen);
				break;
			}
		}
	}
	if(!elem)
	{
		if(!max_alloc)
			max_alloc = BC_Resources::memory_size / MAX_ALLOC;
		if(get_size() > max_alloc)
			delete_old_frames();
		elem = append(new ATmpFrameCacheElem(buflen));
	}
	elem->in_use = 1;

	listlock.unlock();

	return elem->frame;
}

AFrame *ATmpFrameCache::clone_frame(AFrame *frame)
{
	AFrame *aframe;

	if(!frame)
		return 0;

	aframe = get_tmpframe(frame->buffer_length);
	aframe->samplerate = frame->samplerate;
	aframe->channel = frame->channel;
	aframe->set_track(frame->get_track());

	// Make frame empty
	aframe->duration = 0;
	aframe->length = 0;

	return aframe;
}

void ATmpFrameCache::release_frame(AFrame *tmp_frame)
{
	if(!tmp_frame)
		return;

	listlock.lock("TmpFrameCache::release_frame");

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(tmp_frame == cur->frame)
		{
			cur->in_use = 0;
			cur->length = cur->frame->buffer_length;
			cur->age = ++moment;
			break;
		}
	}

	listlock.unlock();
}

void ATmpFrameCache::delete_old_frames()
{
	while(get_size() > max_alloc)
	{
		if(delete_oldest())
			break;
	}
}

int ATmpFrameCache::delete_oldest()
{
	unsigned int min_age = UINT_MAX;
	ATmpFrameCacheElem *min_elem = 0;

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && cur->age < min_age)
		{
			min_age = cur->age;
			min_elem = cur;
		}
	}

	if(!min_elem)
		return 1;

	delete min_elem;
	return 0;
}

void ATmpFrameCache::delete_unused()
{
	ATmpFrameCacheElem *nxt;

	listlock.lock("TmpFrameCache::delete_unused");

	for(ATmpFrameCacheElem *cur = first; cur;)
	{
		if(!cur->in_use)
		{
			nxt = cur->next;
			delete cur;
			cur = nxt;
		}
		else
			cur = cur->next;
	}
	listlock.unlock();
}

size_t ATmpFrameCache::get_size(int *total, int *inuse)
{
	size_t res = 0;
	int used = 0;
	int count = 0;

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		count++;
		if(cur->in_use)
			used++;
		res += cur->get_size();
	}
	if(inuse)
		*inuse = used;
	if(total)
		*total = count;
	return res / 1024;
}

void ATmpFrameCache::dump(int indent)
{
	printf("%*sATmpFrameCache %p dump:\n", indent, "", this);

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
