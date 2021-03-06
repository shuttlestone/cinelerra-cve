
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef PARAMLIST_H
#define PARAMLIST_H

#include "filexml.inc"
#include "paramlist.inc"
#include "linklist.h"
#include "stdint.h"

class Param : public ListItem<Param>
{
public:
	Param(const char *name, int value);
	Param(const char *name, int64_t value);
	Param(const char *name, const char *value);
	Param(const char *name, double value);
	Param(const char *name);
	~Param();

	void set(const char *value);
	void set(int value);
	void set(int64_t value);
	void set(double value);

	// Copy data
	void copy_from(Param *that);
	// Copy data and help text
	void copy_all(Param *that);
	// Copy data without subparams and help text
	void copy_top_level(Param *that);
	void set_help(const char *txt);
	void set_string(const char *txt);
	Paramlist *add_subparams(const char *name);
	void save_param(FileXML *file);
	void load_param(FileXML *file);
	void copy_values(Param *that);
	int equiv_value(Param *that);
	int equiv(Param *that);
	void store_defaults();
	void reset_defaults();
	void dump(int indent = 0);

	int type;
	char name[PARAM_NAMELEN];
	int help_allocated;
	int string_allocated;
	char *helptext;
	char *stringvalue;
	int intvalue;
	int64_t longvalue;
	double floatvalue;
	Paramlist *subparams;
	static const char *known_properties[];
private:
	void initialize(const char *name);
	// Saved defaults
	int defaulttype;
	int defaultint;
	int defaultstr_allocated;
	char *defaultstring;
	int64_t defaultlong;
	double defaultfloat;
};

class Paramlist : public List<Param>
{
public:
	Paramlist(const char *name = 0);
	~Paramlist();

	void delete_params();
	// Removes subparams and helptexts
	void clean_list();
	// Copy all data
	void copy_from(Paramlist *that);
	void copy_all(Paramlist *that);
	void copy_top_level(Paramlist *that);
	void copy_values(Paramlist *src);
	Param *append_param(const char *name, const char *value);
	Param *append_param(const char *name, int value);
	Param *append_param(const char *name, int64_t value);
	Param *append_param(const char *name, double value);
	Param *append_param(const char *name);

	Param *set(const char *name, const char *value);
	Param *set(const char *name, int value);
	Param *set(const char *name, int64_t value);
	Param *set(const char *name, double value);
	Param *set(Param *param);
	int get(const char *name, int dflt);
	int64_t get(const char *name, int64_t dflt);
	double get(const char *name, double dflt);
	char *get(const char *name, char *dflt);

	void set_selected(int value);
	void set_selected(int64_t value);
	void set_selected(double value);

	Param *find(const char *name);
	Param *find_value(int value);
	Param *find_value(int64_t value);
	Param *find_value(double value);
	void save_list(FileXML *file);
	void load_list(FileXML *file);
	void remove_equiv(Paramlist *that);
	void remove_param(const char *name);
	void join_list(Paramlist *that);
	int equiv(Paramlist *that, int sublvl = 0);
	void store_defaults();
	void reset_defaults();
	void dump(int indent = 0);

	char name[PARAM_NAMELEN];
	// Type of values in use here
	int type;
	int selectedint;
	int64_t selectedlong;
	double selectedfloat;
};

#endif
