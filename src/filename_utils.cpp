/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "filename_utils.h"
#include <fstream>

namespace tlmodder
{

string FileName::extension(string const& fn)
{
	string::size_type dot_pos = fn.rfind('.');
	
	if (dot_pos == 0 || dot_pos == string::npos)
		return {};
	
	return {fn, dot_pos+1};
}

string FileName::stripExt(string const& fn)
{
	string::size_type dot_pos = fn.rfind('.');
	
	if (dot_pos == 0 || dot_pos == string::npos)
		return fn;
	
	return {fn, 0, dot_pos};
}

void FileName::stripExtInplace(string& fn)
{
	string::size_type dot_pos = fn.rfind('.');
	
	if (dot_pos != 0 && dot_pos != string::npos)
		return fn.resize(dot_pos);
}

string FileName::baseName(string const& fn)
{
	string::size_type slash = fn.rfind('/');
	
	if (slash == string::npos)
		return fn;
	
	return {fn, slash+1};
}

string FileName::build(string path, string const& basename)
{
	string result(std::move(path));
	
	if (result.empty())
		return basename;
	
	if (!basename.empty())
	{
		string::size_type base_start = (string::size_type)(basename[0] == '/');
		
		if (result[result.size()-1] != '/')
			result += '/';
		result.append(basename, base_start, string::npos);
	}
	
	return std::move(result);
}

void FileName::buildInplace(string& path, string const& basename)
{
	if (!path.empty() && path[path.size()-1] != '/')
		path += '/';
	path += basename;
}

string FileName::parent(string const& path)
{
	string::size_type slash = path.rfind('/');
	
	if (slash == string::npos)
		return {};
	else if (slash == 0)
		return {1, '/'}; // Keep '/' if absolute
	else
		return {path, 0, slash};
}

void FileName::parentInplace(string& path)
{
	string::size_type slash = path.rfind('/');
	
	if (slash == string::npos)
		path.clear();
	else if (slash == 0)
		path.resize(1); // Keep '/' if absolute
	else
		path.resize(slash);
}

bool FileName::isParentOf(string const& parent, string const& child)
{
	string::size_type check_size, parent_size, child_size;
	
	check_size = parent_size = parent.size();
	child_size = child.size();
	
	if (parent_size == 0)
		return (child_size == 0 || child[0] != '/');
	
	if (parent[parent_size-1] == '/')
		--check_size;
	
	if (child_size < check_size)
		return false;
	
	for (string::size_type i = 0; i < check_size; ++i)
	{
		if (child[i] != parent[i])
			return false;
	}
	
	if (child_size > check_size && child[check_size] != '/')
		return false;
	
	return true;
}

void FileName::winSlashesToPosix(string& fn)
{
	string::size_type size = fn.size();
	
	for (string::size_type i = 0; i < size; ++i)
	{
		if (fn[i] == '\\') fn[i] = '/';
	}
}

string FileName::build(std::initializer_list<string const> parts)
{
	string result;
	string::size_type part_start;
	
	for (auto& part : parts)
	{
		if (result.empty()) {
			result.append(part);
		}
		else if (!part.empty()) {
			
			part_start = (string::size_type)(part[0] == '/');
			
			if (result[result.size()-1])
				result += '/';
			
			result.append(part, part_start, string::npos);
		}
	}
	
	return std::move(result);
}





}
