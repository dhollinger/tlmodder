/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "mapped_file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace tlmodder
{

MappedFile::MappedFile(string const& path)
{
	_mapfd(::open(path.c_str(), O_RDONLY | O_CLOEXEC));
}

MappedFile::MappedFile(DirIterator const& dir, string const& path)
{
	_mapfd(dir.open(path, O_RDONLY | O_CLOEXEC));
}

void MappedFile::_mapfd(int fd)
{
	if (fd == -1)
		throw MappingFailed("cannot open file");
	
	struct stat st;
	if (::fstat(fd, &st) != 0)
	{
		::close(fd);
		throw MappingFailed("fstat() failed");
	}
	
	// FIXME: check if file size negative or value too large to cast to size_t
	m_size = (size_t)st.st_size;
	
	m_ptr = ::mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
	::close(fd);
	
	if (m_ptr == MAP_FAILED)
		throw MappingFailed("mmap() failed");
}

MappedFile::~MappedFile()
{
	::munmap(m_ptr, m_size);
}

} // namespace tlmodder
