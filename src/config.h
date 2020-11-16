/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_CONFIG_H__
#define __TLMODDER_CONFIG_H__

#include "adm.h"

#include <string>
#include <set>

namespace tlmodder{

struct ModConfig
{
	std::string name;
	int priority;
	bool enabled;
};

struct ModConfigByName
{
	bool operator()(ModConfig const& c1, ModConfig const& c2) const
	{ return c1.name.compare(c2.name) < 0; }
};

struct ModConfigByPriorityAndName
{
	bool operator()(ModConfig const& c1, ModConfig const& c2) const
	{
		if (c1.priority != c2.priority)
			return c1.priority < c2.priority;
		
		return c1.name.compare(c2.name) < 0;
	}
};

using ModConfigSet      = std::set<ModConfig, ModConfigByName>;
using ModConfigIterator = ModConfigSet::const_iterator;

class Config
{
public:
	Config()
	{ setDefaults(); }
	
	~Config() {}
	
	void setDefaults();
	
	void loadFrom(std::string const& fn);
	
	bool lookForNew() const
	{ return m_lookForNew; }
	
	bool mergeClassMods() const
	{ return m_mergeClassMods; }
	
	ModConfigSet const& modConfigs() const
	{ return m_modConfigs; }
	
	std::string const& modDir() const
	{ return m_modDir; }
	
	std::string const& originalGameData() const
	{ return m_originalGameData; }
	
	std::string const& outputDir() const
	{ return m_outputDir; }
protected:
	ModConfigSet m_modConfigs;
	bool m_lookForNew;
	bool m_mergeClassMods;
	std::string m_modDir;
	std::string m_originalGameData;
	std::string m_outputDir;
};

}

#endif
