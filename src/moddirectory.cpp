/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "moddirectory.h"
#include "filename_utils.h"
#include "dir_iterator.h"
#include "unicode.h"

#include <cctype>
#include <stdexcept>
#include <vector>
#include <stack>
#include <tuple>
#include <iostream>

namespace tlmodder {

using std::vector;
using std::stack;
using std::list;

// FIXME: ASCII only, should be UTF-8
bool StringLessNoCase::operator()(string const& str1, string const& str2)
{
	string::size_type size1, size2, cmpSize, i;
	int diff;
	
	size1 = str1.size();
	size2 = str2.size();
	cmpSize = size1 < size2 ? size1 : size2;
	
	for (i = 0; i < cmpSize; ++i)
	{
		diff = std::toupper(str1[i]) - std::toupper(str2[i]);
		if (diff != 0)
			return diff < 0;
	}
	
	return size1 < size2;
}

bool ModDirectory::lookupDir(string const& name, ModDirectoryIterator& it)
{
	string::size_type startPos, slashPos;
	ModDirectory *currentDir;
	
	startPos = 0;
	slashPos = 0;
	currentDir = this;
	
	if (name.empty())
		return false;
	
	for (startPos = 0; slashPos != name.size(); startPos = slashPos+1)
	{
		slashPos = name.find('/', startPos);
		
		if (slashPos == string::npos)
			slashPos = name.size();
		
		if (slashPos != startPos)
		{
			it = currentDir->dirs.find(string(name, startPos, slashPos - startPos));
			
			if (it == currentDir->dirs.end())
				return false;
			
			currentDir = &it->second;
		}
	}
	
	return true;
}

bool ModDirectory::lookupFile(string const& name, ModFileIterator& it)
{
	string::size_type startPos, slashPos;
	ModDirectoryIterator dirIt;
	ModDirectory *currentDir;
	
	slashPos = 0;
	currentDir = this;
	
	if (name.empty())
		return false;
	
	for (startPos = 0; ; startPos = slashPos+1)
	{
		slashPos = name.find('/', startPos);
		
		// No more slashes, file part remaining
		if (slashPos == string::npos)
		{
			it = currentDir->files.find(string(name, startPos));
			return (it != currentDir->files.end());
		}
		
		// Ignoring 2 consecutive '/' characters and also root slash
		if (slashPos != startPos)
		{
			dirIt = currentDir->dirs.find(string(name, startPos, slashPos - startPos));
			
			if (dirIt == currentDir->dirs.end())
				return false;
			
			currentDir = &dirIt->second;
		}
	}
	
	throw std::runtime_error("not reached");
}

void ModDirectory::loadFromDir(string const& modPath)
{
	struct DirectoryLoadState
	{
		ModDirectory *modDir;
		vector<string> dirs;
		vector<string>::const_iterator dirIter;
		
		DirectoryLoadState(ModDirectory *dir): modDir(dir)
		{
			dirIter = dirs.begin();
		}
	};
	
	stack<DirectoryLoadState> loadStateStack;
	
	loadStateStack.emplace(this);
	
	
	FileName currentDir(modPath), currentModDir;
	string fileName, fileNameUpper;
	struct stat st;
	ModDirectory *modDir;
	
	
	while (!loadStateStack.empty())
	{
		DirectoryLoadState& loadState = loadStateStack.top();
		modDir = loadState.modDir;
		
		if (loadState.dirIter == loadState.dirs.begin())
		{
			DirIterator dir(currentDir.str());
			
			while (dir.next(fileName))
			{
				// Ignore the file if we cannot stat it
				if (!dir.stat(fileName, st, false))
					continue;
				
				if (S_ISREG(st.st_mode))
				{
					// Ignore all root-level files
					if (loadStateStack.size() == 1)
						continue;
					
					// Convert name to upper-case
					fileNameUpper = utf8_to_upper(fileName);
					
					// Ignore massfile.dat and masterresourceunits.dat in media directory
					if (loadStateStack.size() == 2)
					{
						if (fileNameUpper == "MASSFILE.DAT" || fileNameUpper == "MASTERRESOURCEUNITS.DAT" ||
								fileNameUpper == "MASSFILE.DAT.ADM" || fileNameUpper == "MASTERRESOURCEUNITS.DAT.ADM")
							continue;
					}
					
					// If directory with the same name exists, issue warning and skip it
					{
						ModDirectoryIterator it = modDir->dirs.find(fileNameUpper);
						if (it != modDir->dirs.end())
						{
							std::cerr << "WARNING: file \"" << currentDir.build(fileName)
												<< "\" conflicts with directory \""
												<< currentModDir.build(it->first) << ", replacing." << std::endl;
							modDir->dirs.erase(it);
						}
					}
					
					string sourceFn = fileName;
					string ext = FileName::extension(fileNameUpper);
					
					bool isDat;
					bool isAdm = (ext == "ADM");
					
					if (isAdm)
					{
						FileName::stripExtInplace(sourceFn);
						ext = utf8_to_upper(FileName::extension(sourceFn));
					}
					isDat = (ext == "DAT" || ext == "ANIMATION" || ext == "LAYOUT");
					
					ModFileIterator it;
					std::tie(it, std::ignore) = modDir->files.insert(std::make_pair(sourceFn, std::list<string>()));
					
					if ((isAdm || isDat) && !it->second.empty())
					{
						if (FileName::parent(it->second.front()) == currentDir.str())
						{
							if (isDat)
								it->second.front() = currentDir.build(fileName);
						}
						else
						{
							it->second.push_front(currentDir.build(fileName));
						}
					}
					else
					{
						it->second.push_front(currentDir.build(fileName));
					}
				}
				else if (S_ISDIR(st.st_mode))
				{
					loadState.dirs.push_back(fileName);
				}
			}
			loadState.dirIter = loadState.dirs.begin();
		}
		
		if (loadState.dirIter != loadState.dirs.end())
		{
			string const& fn = *loadState.dirIter;
			++loadState.dirIter;
			
			// Convert name to upper-case
			fileNameUpper = utf8_to_upper(fn);
			
			// Ignore all root-level non-media directories
			if (loadStateStack.size() == 1 && fileNameUpper != "MEDIA")
				continue;
			
			// If file with the same name exists, remove it and issue warning
			{
				ModFileIterator it = modDir->files.find(fileNameUpper);
				if (it != modDir->files.end())
				{
					std::cerr << "WARNING: directory \"" << currentDir.build(fn)
					          << "\" conflicts with file \""
					          << currentModDir.build(it->second.front()) << ", replacing." << std::endl;
					modDir->files.erase(it);
				}
			}
			
			ModDirectoryIterator it;
			
			// Make sure media directory is always lower-case, or Torchlight won't find textures
			string modDirName;
			if (loadStateStack.size() == 1)
				modDirName = "media";
			else
				modDirName = fn;
			
			std::tie(it, std::ignore) = modDir->dirs.insert(std::make_pair(modDirName, ModDirectory()));
			
			loadStateStack.emplace(&it->second);
			currentDir.cd(fn);
			currentModDir.cd(modDirName);
		}
		else
		{
			// No more entries in current directory, go one level up
			loadStateStack.pop();
			
			if (!loadStateStack.empty())
			{
				currentDir.up();
				currentModDir.up();
			}
		}
	}
}

void ModDirectory::merge(ModDirectory&& srcDir)
{
	using DstStack = std::stack<ModDirectory*>;
	using SrcState = std::pair<ModDirectory*, ModDirectoryIterator>;
	using SrcStack = std::stack<SrcState>;
	
	DstStack dstStack;
	SrcStack srcStack;
	ModDirectory *dst, *src;
	ModDirectoryIterator dirIt;
	ModFileIterator fileIt;
	
	dstStack.emplace(this);
	srcStack.emplace(&srcDir, srcDir.dirs.begin());
	
	while (!dstStack.empty())
	{
		SrcState& srcState = srcStack.top();
		src = srcState.first;
		dst = dstStack.top();
		
		if (srcState.second == src->dirs.begin())
		{
			// Merge files
			for (auto& srcFile : src->files)
			{
				// remove conflicting directory, if any
				dirIt = dst->dirs.find(srcFile.first);
				if (dirIt != dst->dirs.end())
					dst->dirs.erase(dirIt);
				
				std::tie(fileIt, std::ignore) = dst->files.insert(std::make_pair(srcFile.first, list<string>()));
				fileIt->second.splice(fileIt->second.begin(), std::move(srcFile.second));
			}
		}
		
		if (srcState.second != src->dirs.end())
		{
			ModDirectoryEntry& childEntry = *srcState.second;
			
			// remove conflicting file, if any
			fileIt = dst->files.find(childEntry.first);
			if (fileIt != dst->files.end())
				dst->files.erase(fileIt);
			
			// create directory or use existing
			std::tie(dirIt, std::ignore) = dst->dirs.insert(std::make_pair(childEntry.first, ModDirectory()));
			
			// place child directories onto stacks to merge them
			srcStack.emplace(&childEntry.second, childEntry.second.dirs.begin());
			dstStack.emplace(&dirIt->second);
			++srcState.second;
		}
		else
		{
			srcStack.pop();
			dstStack.pop();
		}
	}
}

}
