/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_MODDIRECTORY_H__
#define __TLMODDER_MODDIRECTORY_H__

#include <string>
#include <map>
#include <list>

namespace tlmodder {

using std::string;

// Functor used for case-insensitive file search in map
struct StringLessNoCase
{
	bool operator()(string const& str1, string const& str2);
};

struct ModDirectory;

using ModFileMap = std::map<
                     string,            // in-mod filename
                     std::list<string>, // actual on-disk filenames, front() is file from mod with higher priority
                     StringLessNoCase
                     >;
using ModFileEntry         = ModFileMap::value_type;
using ModFileIterator      = ModFileMap::iterator;
using ModFileConstIterator = ModFileMap::const_iterator;

using ModDirectoryMap = std::map<
                          string,           // in-mod directory name
                          ModDirectory,     // content of the directory
                          StringLessNoCase
                          >;
using ModDirectoryEntry         = ModDirectoryMap::value_type;
using ModDirectoryIterator      = ModDirectoryMap::iterator;
using ModDirectoryConstIterator = ModDirectoryMap::const_iterator;


struct ModDirectory
{
	ModFileMap      files;
	ModDirectoryMap dirs;
	
	bool lookupFile(string const& name, ModFileIterator& it);
	bool lookupDir(string const& name, ModDirectoryIterator& it);
	
	void loadFromDir(string const& modPath);
	
	void merge(ModDirectory&& src);
};

}

#endif
