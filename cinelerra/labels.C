
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

#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "labels.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "timebar.h"
#include <string.h>



Labels::Labels(EDL *edl, const char *xml_tag)
 : List<Label>()
{
	this->edl = edl;
	this->xml_tag = xml_tag;
}

Labels::~Labels()
{
	delete_all();
}

void Labels::reset_instance()
{
	delete_all();
}

void Labels::dump(int indent)
{
	printf("%*sLabels %p dump(%d):\n", indent, "", this, total());
	indent += 2;
	for(Label *current = first; current; current = NEXT)
	{
		printf("%*slabel: %.3f '%s'\n", indent, "",
			current->position, current->textstr);
	}
}

void Labels::insert_labels(Labels *labels, ptstime start, ptstime length, int paste_silence)
{
	Label *new_label;
	Label *old_label;

// Insert silence in old labels
	if(paste_silence)
	{
		for(old_label = first; old_label; old_label = old_label->next)
		{
			if(old_label->position > start ||
				edl->equivalent(old_label->position, start))
				old_label->position += length;
		}
	}


// Insert one new label at a time
	for(new_label = labels->first; new_label; new_label = new_label->next)
	{
		int exists = 0;

// Check every old label for existence
		for(old_label = first; old_label; old_label = old_label->next)
		{
			if(edl->equivalent(old_label->position, new_label->position + start))
			{
				exists = 1;
				break;
			}
			else
			if(old_label->position > new_label->position + start)
				break;
		}

		if(!exists)
		{
			if(old_label)
				insert_before(old_label, new Label(edl, this, new_label->position + start, new_label->textstr));
			else
				append(new Label(edl, this, new_label->position + start, new_label->textstr));
		}
	}
}

void Labels::toggle_label(ptstime start, ptstime end)
{
	Label *current;

// handle selection start
// find label the selectionstart is after
	for(current = first; 
		current && current->position < start && !edl->equivalent(current->position, start); 
		current = NEXT);

	if(current)
	{
		if(edl->equivalent(current->position, start))
		{        // remove it
			remove(current);
		}
		else
		{        // insert before it
			current = insert_before(current, new Label(edl, this, start, ""));
		}
	}
	else
	{           // insert after last
		current = append(new Label(edl, this, start, ""));
	}

// handle selection end
	if(!EQUIV(start, end))
	{
		for(current = first; 
			current && current->position < end && !edl->equivalent(current->position, end); 
			current = NEXT);

		if(current)
		{
			if(edl->equivalent(current->position, end))
			{
				remove(current);
			}
			else
			{
				current = insert_before(current, new Label(edl, this, end, ""));
			}
		}
		else
		{
			current = append(new Label(edl, this, end, ""));
		}
	}
}

void Labels::delete_all()
{
	while(last)
		remove(last);
}

void Labels::save_xml(FileXML *xml)
{
	char string[BCTEXTLEN];

	if(!total())
		return;

	xml->tag.set_title(xml_tag);
	xml->append_tag();
	xml->append_newline();

	Label *current;
	sprintf(string, "/%s", xml_tag);
	string[strlen(string) - 1] = 0; // remove trailing "S" on "LABELS" giving "LABEL"
	for(current = first; current; current = NEXT)
	{
		xml->tag.set_title(string + 1); // skip the "/" for opening tag
		xml->tag.set_property("TIME", current->position);
		xml->tag.set_property("TEXTSTR", current->textstr);
		xml->append_tag();
		xml->tag.set_title(string); // closing tag
		xml->append_tag();
		xml->append_newline();
	}

	sprintf(string, "/%s", xml_tag);
	xml->tag.set_title(string);
	xml->append_tag();
	xml->append_newline();
}

void Labels::copy(Labels *that, ptstime start, ptstime end)
{
	while(last)
		delete(last);

	for(Label *current = that->first; current; current = NEXT)
	{
		if(current->position < start + EPSILON || current->position > end - EPSILON)
			continue;
		append(new Label(edl, this, current->position - start, current->textstr));
	}
}

void Labels::copy_from(Labels *labels)
{
	while(last) delete last;

	for(Label *current = labels->first; current; current = NEXT)
	{
		append(new Label(edl, this, current->position, current->textstr));
	}
}


Labels& Labels::operator=(Labels &that)
{
	copy_from(&that);
	return *this;
}

void Labels::load(FileXML *xml)
{
	int result = 0;
	char string1[BCTEXTLEN], string2[BCTEXTLEN];

	sprintf(string1, "/%s", xml_tag);
	strcpy(string2, xml_tag);
	string2[strlen(string2) - 1] = 0;

	do{
		result = xml->read_tag();
		if(!result)
		{
			if(xml->tag.title_is(string1))
			{
				result = 1;
			}
			else
			if(xml->tag.title_is(string2))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
				if(position > -1)
				{
					Label *current = label_of(position);
					current = insert_before(current, new Label(edl, this, position, ""));
					xml->tag.get_property("TEXTSTR", current->textstr);
				}
			}
			else
			if(xml->tag.title_is("INPOINT"))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
			}
			else
			if(xml->tag.title_is("OUTPOINT"))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
			}
		}
	}while(!result);
}

void Labels::clear(ptstime start, ptstime end, int follow)
{
	Label *current;
	Label *next;

	current = label_of(start);

// remove selected labels
	while(current && current->position < end)
	{
		next = NEXT;
		delete current;
		current = next;
	}

// Shift later labels
	if(follow)
	{
		while(current)
		{
			current->position -= end - start;   // shift labels forward
			current = NEXT;
		}
		optimize();
	}
}

Label* Labels::prev_label(ptstime position)
{
	Label *current;

// Test for label under cursor position
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT);

// Test for label after cursor position
	if(!current)
		for(current = first;
			current && current->position < position;
			current = NEXT);

// Test for label before cursor position
	if(!current) 
		current = last;
	else
// Get previous label
		current = PREVIOUS;

	return current;
}

Label* Labels::next_label(ptstime position)
{
	Label *current;

// Test for label under cursor position
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT);

// Test for label before cursor position
	if(!current)
		for(current = last;
			current && current->position > position;
			current = PREVIOUS);

// Test for label after cursor position
	if(!current)
		current = first;
	else
// Get next label
		current = NEXT;

	return current;
}

void Labels::insert(ptstime start, ptstime length)
{
// shift every label including the first one back
	Label *current;

	for(current = label_of(start); current; current = NEXT)
	{
		current->position += length;
	}
}

void Labels::paste_silence(ptstime start, ptstime end)
{
	insert(start, end - start);
	optimize();
}

void Labels::modify_handles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	if(edit_labels &&
		handle_mode == MOVE_ALL_EDITS)
	{
		if(currentend == HANDLE_LEFT)          // left handle
		{
			if(newposition < oldposition)
			{
				insert(oldposition, oldposition - newposition);    // shift all labels right
			}
			else
			{
				clear(oldposition, newposition);   // clear selection
			}
		}
		else
		if(currentend == HANDLE_RIGHT)  // right handle
		{
			if(newposition < oldposition)
			{
				clear(newposition, oldposition);
			}
			else
			{
				insert(oldposition, newposition - oldposition);
			}
		}
	}
}

void Labels::optimize()
{
	int result = 1;
	Label *current;

	while(result)
	{
		result = 0;

		for(current = first; current && NEXT && !result;)
		{
			Label *next = NEXT;
			if(EQUIV(current->position, next->position))
			{
				delete current;
				result  = 1;
			}
			current = next;
		}
	}
}

Label* Labels::label_of(ptstime position)
{
	Label *current;

	for(current = first; current; current = NEXT)
	{
		if(current->position >= position) return current;
	}
	return 0;
}

size_t Labels::get_size()
{
	size_t size = sizeof(*this);

	for(Label *current = first; current; current = NEXT)
		size += current->get_size();
	return size;
}

Label::Label(EDL *edl, Labels *labels, double position, const char *textstr = 0)
 : ListItem<Label>()
{
	this->edl = edl;
	this->labels = labels;
	this->position = position;
	if (textstr)
		strcpy(this->textstr, textstr);
	else
		this->textstr[0] = 0;
}

size_t Label::get_size()
{
	return sizeof(*this);
}
