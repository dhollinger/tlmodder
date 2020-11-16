/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_DIR_ITERATOR_H__
#define __TLMODDER_DIR_ITERATOR_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

namespace tlmodder {

using std::string;

class DirIterator
{
public:
	class OpenFailed : public std::runtime_error
	{
	public:
		OpenFailed(string const& name):
			std::runtime_error(name)
		{}
	};
	
	DirIterator(): m_dir(nullptr) {}
	
	DirIterator(string const& path) : m_dir(nullptr)
	{ open(path); }
	
	DirIterator(DirIterator const& dir, string const& path) : m_dir(nullptr)
	{ open(dir, path); }
	
	DirIterator(DirIterator&& other) noexcept
	{
		m_dir = other.m_dir;
		other.m_dir = nullptr;
	}
	
	DirIterator(DirIterator const&) = delete;
	DirIterator& operator=(DirIterator const&) = delete;
	
	DirIterator& operator=(DirIterator&& other)
	{
		close();
		m_dir = other.m_dir;
		other.m_dir = nullptr;
		return *this;
	}
	
	~DirIterator() noexcept
	{
		if (m_dir != nullptr)
			::closedir(m_dir);
	}
	
	bool open(std::nothrow_t, string const& path) noexcept
	{
		close();
		m_dir = ::opendir(path.c_str());
		return (m_dir != nullptr);
	}
	
	bool open(std::nothrow_t, DirIterator const& dir, string const& path) noexcept
	{
		close();
		int fd = dir.open(path.c_str(), O_RDONLY | O_DIRECTORY);
		if (fd == -1)
			return false;
		
		m_dir = fdopendir(fd);
		return (m_dir != nullptr);
	}
	
	void open(string const& path)
	{
		if (!open(std::nothrow, path))
			throw OpenFailed(path);
	}
	
	void open(DirIterator const& dir, string const& path)
	{
		if (!open(std::nothrow, dir, path))
			throw OpenFailed(path);
	}
	
	void close() noexcept
	{
		if (m_dir != nullptr) {
			::closedir(m_dir);
			m_dir = nullptr;
		}
	}
	
	bool next(string& fn)
	{
		struct dirent *e;
		
		while ((e = ::readdir(m_dir)) != nullptr)
		{
			if (!isDots(e->d_name))
			{
				fn = e->d_name;
				return true;
			}
		}
		
		return false;
	}
	
	static inline bool isDots(char const* fn) noexcept
	{
		return (fn[0] == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')));
	}
	
	inline int open(char const* fn, int flags) const noexcept
	{ return ::openat(::dirfd(m_dir), fn, flags); }
	
	inline int open(string const& fn, int flags) const noexcept
	{ return open(fn.c_str(), flags); }
	
	inline bool stat(char const* fn, struct stat& st, bool followSymlinks = true) const noexcept
	{ return ::fstatat(::dirfd(m_dir), fn, &st, followSymlinks ? 0 : AT_SYMLINK_NOFOLLOW) == 0; }
	
	inline bool stat(string const& fn, struct stat& st, bool followSymlinks = true) const noexcept
	{ return stat(fn.c_str(), st, followSymlinks); }
	
protected:
	DIR *m_dir;
};



}

#endif
