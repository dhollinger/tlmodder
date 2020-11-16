/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __FILENAME_UTILS_H__
#define __FILENAME_UTILS_H__

#include <string>
#include <iostream>
#include <initializer_list>

namespace tlmodder
{

using std::string;

class FileName
{
public:
	static string extension(string const& fn);
	static string stripExt(string const& fn);
	static void   stripExtInplace(string& fn);
	static string baseName(string const& fn);
	static string build(string path, string const& basename);
	static void   buildInplace(string& path, string const& basename);
	static string parent(string const& path);
	static void   parentInplace(string& path);
	static bool   isParentOf(string const& parent, string const& child);
	static string build(std::initializer_list<string const> parts);
	
	static void winSlashesToPosix(string& fn);
	
	FileName() = default;
	FileName(FileName const&) = default;
	FileName(FileName&&) = default;
	
	FileName& operator=(FileName const&) = default;
	FileName& operator=(FileName&&) = default;
	
	FileName(string path):
		m_fn(std::move(path))
	{}
	
	inline FileName(string path, string const& fn):
		m_fn(std::move(path))
	{ buildInplace(m_fn, fn); }
	
	inline FileName(FileName path, string const& fn):
		m_fn(std::move(path.m_fn))
	{ buildInplace(m_fn, fn); }
	
	inline void cd(string const& fn)
	{ buildInplace(m_fn, fn); }
	
	inline void up()
	{ parentInplace(m_fn); }
	
	inline string const& str() const
	{ return m_fn; }
	
	inline string extension() const
	{ return extension(m_fn); }
	
	inline string baseName() const
	{ return baseName(m_fn); }
	
	inline string parent() const
	{ return parent(m_fn); }
	
	inline string build(string const& fn) const
	{ return build(m_fn, fn); }
	
	inline bool isParentOf(string const& child) const
	{ return isParentOf(m_fn, child); }
	
	inline bool isChildOf(string const& parent) const
	{ return isParentOf(parent, m_fn); }
	
	inline bool isParentOf(FileName const& child) const
	{ return isParentOf(m_fn, child.str()); }
	
	inline bool isChildOf(FileName const& parent) const
	{ return isParentOf(parent.str(), m_fn); }
	
	friend std::ostream& operator<<(std::ostream& strm, FileName const& fn)
	{
		return (strm << fn.m_fn);
	}
protected:
	string m_fn;
};

}

#endif
