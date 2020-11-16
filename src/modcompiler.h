/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_MODCOMPILER_H__
#define __TLMODDER_MODCOMPILER_H__

#include "adm.h"
#include "massfile.h"
#include "masterresourceunits.h"
#include "moddirectory.h"

#include <string>
#include <map>
#include <memory>

namespace tlmodder {

using std::map;
using std::string;

class ModCompiler
{
public:
	ModCompiler()
	{}
	
	void addMod(ModDirectory&& mod);
	void addMod(std::string const& modPath);
	
	struct ExtInfo
	{
		bool isDat;       // DAT or DAT.ADM
		bool isAnimation; // ANIMATION or ANIMATION.ADM
		bool isLayout;    // LAYOUT or LAYOUT.ADM
		bool isAdm;       // ADM
		bool isDatFile;   // DAT, ANIMATION or LAYOUT
	};
	
	void compile();
	
	bool mergeClasses() const
	{ return m_mergeClasses; }
	
	void mergeClasses(bool merge)
	{ m_mergeClasses = merge; }
	
	FileName const& outputDir() const
	{ return m_outputDir; }
	
	void outputDir(FileName fn)
	{ m_outputDir = std::move(fn); }
	
protected:
	void copyFile(string const& src, string const& dst);
	void processDat(ModFileEntry const& entry, ExtInfo const& extInfo);
	void loadClasses();
	void tryAddPet(ModFileEntry const& entry, std::shared_ptr<adm::Adm> admPtr);
	void createCharacterCreateLayout();
	void processFile(ModFileEntry const& entry);
	void tryMergeClassWardrobes(ModFileEntry const& entry, std::shared_ptr<adm::Adm> admPtr);
	void addToMasterResourceUnits(ModFileEntry const& entry, std::shared_ptr<adm::Adm>& admPtr);
	
	MassFile m_massfile;
	MasterResourceUnits m_masterresourceunits;
	ModDirectory m_files;
	
	map<string, string> m_classes;
	map<string, string> m_pets;
	
	FileName m_currentDir;         // Current filesystem directory
	FileName m_currentModDir;      // Current in-mod directory
	FileName m_currentModDirUpper; // Current in-mod directory in upper-case
	
	bool m_mergeClasses;
	FileName m_outputDir;
};

}

#endif
