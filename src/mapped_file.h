/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "dir_iterator.h"

namespace tlmodder
{

using std::string;
using std::size_t;
using std::uint8_t;

class MappedFile
{
public:
	MappedFile(string const& path);
	MappedFile(DirIterator const& dir, string const& path);
	~MappedFile();
	
	const uint8_t* ptr() const { return (const uint8_t*)m_ptr; }
	size_t size() const { return m_size; }
	
	class MappingFailed : public std::runtime_error
	{
	public:
		MappingFailed(char const* reason):
			std::runtime_error(reason)
		{}
	};
protected:
	void _mapfd(int fd);
protected:
	void* m_ptr;
	size_t m_size;
};

} // namespace tlmodder

#endif
