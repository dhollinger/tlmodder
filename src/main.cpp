/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include <iostream>
#include <stdexcept>
#include <limits>

#include "config.h"
#include "modcompiler.h"
#include "dat_file_adm_loader.h"
#include "dir_iterator.h"
#include "filename_utils.h"

#include "moddirectory.h"

using namespace tlmodder;

using std::map;
using std::string;

int main(int argc, char **argv)
{
	Config config;
	
	// Load config file
	try {
		if (argc > 1)
			config.loadFrom(argv[1]);
		else
			config.loadFrom("./tlmodder.cfg");
	}
	catch (MappedFile::MappingFailed&)
	{
		std::cerr << "WARNING: Could not open configuration file, using defaults." << std::endl;
	}
	catch (adm::DatFileLoader::Exception& e)
	{
		std::cerr << "WARNING: Could not load configuration file: " << std::endl
		          << e.format() << ", using defaults." << std::endl;
	}
	
	ModCompiler compiler;
	std::multiset<ModConfig, ModConfigByPriorityAndName> mods;
	FileName modPath;
	bool hadWarning = false;
	
	compiler.outputDir(config.outputDir());
	compiler.mergeClasses(config.mergeClassMods());
	
	// Add original game data, quit on failure
	std::cerr << "Loading original game data" << std::endl;
	try {
		compiler.addMod(config.originalGameData());
	}
	catch (DirIterator::OpenFailed& e)
	{
		std::cerr << "ERROR: Could not load original game data: " << e.what() << std::endl;
		return 1;
	}
	
	// Now add mods from configuration file
	for (auto const& modConfig : config.modConfigs())
	{
		mods.insert(modConfig);
	}
	
	if (config.lookForNew())
	try {
		struct stat st;
		
		ModConfig modConfig;
		DirIterator it(config.modDir());
		
		modConfig.priority = std::numeric_limits<int>::min();
		modConfig.enabled = true;
		
		while (it.next(modConfig.name))
		{
			// Skip the file if we cannot stat it
			if (!it.stat(modConfig.name, st, false))
				continue;
			
			// If it is directory
			if (S_ISDIR(st.st_mode))
			{
				// And if it isn't present in config file, add it to the list
				if (config.modConfigs().find(modConfig) == config.modConfigs().end())
					mods.insert(modConfig);
			}
		}
	}
	catch (DirIterator::OpenFailed& e)
	{
		std::cerr << "WARNING: cannot open mod directory: " << e.what() << std::endl;
		hadWarning = true;
	}
	
	// Load the mods, warn on failure
	for (auto const& modConfig : mods)
	{
		if (!modConfig.enabled)
			continue;
		
		std::cerr << "Loading mod " << modConfig.name << std::endl;
		
		modPath = config.modDir();
		modPath.cd(modConfig.name);
		
		try {
			ModDirectory mod;
			mod.loadFromDir(modPath.str());
			compiler.addMod(std::move(mod));
		}
		catch (DirIterator::OpenFailed& e)
		{
			std::cerr << "WARNING: Could not load mod " << modConfig.name << ": " << e.what() << std::endl;
			std::cerr << "WARNING: Mod " << modConfig.name << " skipped." << std::endl;
			hadWarning = true;
		}
	}
	
	// If any mod failed to load, ask if we should continue
	if (hadWarning)
	{
		std::cerr << "There were some warnings while loading mods. Continue and risk possible game"
		          << " crashes and save data corruption? [y/N]: ";
		std::cerr.flush();
		
		char c;
		std::cin.get(c);
		
		std::cerr << std::endl;
		
		if (std::cin.fail() || c != 'y' || c != 'Y')
			return 1;
	}
	
	compiler.compile();
	
	return 0;
}
