/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "dat_file_adm_loader.h"
#include "adm_file_writer.h"
#include "modcompiler.h"
#include "charactercreate.h"

#include <fstream>

namespace tlmodder {

using std::stack;

void ModCompiler::copyFile(string const& src, string const& dst)
{
	try
	{
		std::ifstream infile;
		std::ofstream outfile;
		
		infile.exceptions(std::ios::badbit);
		outfile.exceptions(std::ios::badbit);
		
		infile.open(src, std::ios::binary);
		outfile.open(dst, std::ios::binary | std::ios::trunc);
		
		outfile << infile.rdbuf(); // FIXME: kinda slow
		
		infile.close();
		outfile.close();
	}
	catch (...)
	{
		std::cerr << "Failed to copy " << src << " into output directory." << std::endl;
		throw;
	}
}

void ModCompiler::processDat(ModFileEntry const& entry, ExtInfo const& extInfo)
{
	std::shared_ptr<adm::Adm> admPtr;
	
	std::cerr << "Compiling " << m_currentModDir.build(entry.first) << std::endl;
	
	try {
		admPtr = adm::Adm::createFromFile(entry.second.front());
	}
	catch (adm::DatFileLoader::Exception& e)
	{
		std::cerr << e.format() << std::endl;
		throw;
	}
	
	// Create LAYOUT.CMP file for LAYOUT files
	if (extInfo.isLayout)
	{
		// TODO: create LAYOUT.CMP file
		//       mods seem to work without it and reads source one, so just copy it
		
		if (!extInfo.isAdm)
			copyFile(entry.second.front(), m_currentDir.build(entry.first));
	}
	
	if ((extInfo.isDat || extInfo.isAnimation) && m_massfile.isDirWhitelisted(m_currentModDirUpper))
	{
		std::cerr << "Adding " << m_currentModDir.build(entry.first) << " to massfile" << std::endl;
		m_massfile.addFile(*admPtr, admPtr->root(), m_currentModDirUpper.build(utf8_to_upper(entry.first)));
	}
	else if (extInfo.isDat && m_currentModDirUpper.isChildOf("MEDIA/UNITS"))
	{
		std::cerr << "Adding " << m_currentModDir.build(entry.first) << " to masterresourceunits" << std::endl;
		addToMasterResourceUnits(entry, admPtr);
	}
	
	adm::admFileWrite(m_currentDir.build(entry.first) + ".adm", *admPtr);
}

void ModCompiler::loadClasses()
{
	ModDirectoryIterator dirIt;
	ModFileIterator playerDatEntry;
	adm::AttributeIterator attr;
	uint32_t UNIT_id, NAME_id;
	
	if (!m_files.lookupDir("MEDIA/UNITS/PLAYERS", dirIt))
		return;
	
	for (ModDirectoryEntry& player : dirIt->second.dirs)
	{
		playerDatEntry = player.second.files.find(player.first + ".dat");
		
		if (playerDatEntry == player.second.files.end())
			continue;
		
		try {
			adm::Adm adm;
			
			adm.loadFromFile(playerDatEntry->second.front());
			
			// Player DAT file must contain UNIT and NAME strigs and root's name must be UNIT
			if (adm.stringMap().find("UNIT", UNIT_id) &&
			    adm.stringMap().find("NAME", NAME_id) &&
			    adm.root().name == UNIT_id)
			{
				// Find the NAME attribute and if it is string, add its value to classes list
				attr = adm.root().attributes.find(NAME_id);
				if (attr == adm.root().attributes.end() ||
				    attr->second.type != adm::AttributeValue::TYPE_STRING)
					continue;
				
				string const& name = adm.getString(attr->second.valu32);
				
				attr = adm.root().attributes.find(adm.addString("DISPLAYNAME"));
				if (attr == adm.root().attributes.end() ||
				    (attr->second.type != adm::AttributeValue::TYPE_STRING && attr->second.type != adm::AttributeValue::TYPE_TRANSLATE))
					m_classes[name] = name;
				else
					m_classes[name] = adm.getString(attr->second.valu32);
			}
		}
		catch (...)
		{ /* ignore errors */ }
	}
}

void ModCompiler::addMod(ModDirectory&& mod)
{
	m_files.merge(std::move(mod));
}

void ModCompiler::addMod(std::string const& modPath)
{
	m_files.loadFromDir(modPath);
}

void ModCompiler::processFile(ModFileEntry const& entry)
{
	ExtInfo extInfo;
	string ext;
	
	ext = utf8_to_upper(FileName::extension(entry.first));
	
	extInfo.isDat = (ext == "DAT");
	extInfo.isAnimation = (ext == "ANIMATION");
	extInfo.isLayout = (ext == "LAYOUT");
	extInfo.isAdm = (utf8_to_upper(FileName::extension(entry.second.front())) == "ADM");
	
	if (extInfo.isLayout && !extInfo.isAdm && m_currentModDirUpper.isChildOf("MEDIA/UI"))
		extInfo.isLayout = false;
	
	extInfo.isDatFile = extInfo.isDat || extInfo.isAnimation || extInfo.isLayout;
	
	if (extInfo.isDatFile)
	{
		processDat(entry, extInfo);
	}
	else
	{
		copyFile(entry.second.front(), m_currentDir.build(entry.first));
	}
}

void ModCompiler::createCharacterCreateLayout()
{
	size_t id, toppos;
	string nameUpper;
	
	FileName charCreateLayoutFn(m_outputDir);
	
	// TODO: clean this up
	{
		ModDirectoryIterator mediaIt, mediaUiIt;
		mediaIt = m_files.dirs.find("media");
		// FIXME: check mediaIt
		charCreateLayoutFn.cd(mediaIt->first);
	
		mediaUiIt = mediaIt->second.dirs.find("UI");
		if (mediaUiIt != mediaIt->second.dirs.end())
		{
			ModFileIterator it;
			charCreateLayoutFn.cd(mediaUiIt->first);
			
			it = mediaUiIt->second.files.find("charactercreate.layout");
			if (it != mediaUiIt->second.files.end())
				charCreateLayoutFn.cd(it->first);
			else
				charCreateLayoutFn.cd("charactercreate.layout");
		}
		else
		{
			charCreateLayoutFn.cd("UI");
			
			if (mkdir(charCreateLayoutFn.str().c_str(), S_IRWXU | S_IRGRP | S_IXGRP) != 0)
			{
				if (errno != EEXIST)
				{
					std::cerr << "Failed to create directory " << charCreateLayoutFn << std::endl;
					throw std::runtime_error("Cannot create output directory");
				}
			}
			
			charCreateLayoutFn.cd("charactercreate.layout");
		}
	}
	
	std::ofstream outfile;
	outfile.exceptions(std::ios::badbit);
	outfile.open(charCreateLayoutFn.str(), std::ios::binary | std::ios::trunc);
	
	outfile << charactercreate_layout_0;
	
	id = 3;
	toppos = 233;
	for (auto& classEntry : m_classes)
	{
		nameUpper = utf8_to_upper(classEntry.first);
		
		if (nameUpper == "DESTROYER" || nameUpper == "VANQUISHER" || nameUpper == "ALCHEMIST")
			continue;
		
		outfile << "<Window Type=\"GuiLook/StandardButton\" Name=\"" << classEntry.first << "\">";
		outfile << "<Property Name=\"UnifiedPosition\" Value=\"{{0,5},{0," << toppos << "}\" />";
		outfile << "<Property Name=\"UnifiedSize\" Value=\"{{0,132},{0,28}}\" />";
		outfile << "<Property Name=\"ID\" Value=\"" << id << "\" />";
		outfile << "<Property Name=\"Text\" Value=\"" << classEntry.second << "\" />";
		outfile << "<Property Name=\"Tooltip\" Value=\"Select " << classEntry.second << "\" />";
		outfile << "<Property Name=\"onClick\" Value=\"guiSelect1\"/>";
		outfile << "</Window>" << std::endl;
		
		++id;
		toppos += 30;
	}
	
	outfile << charactercreate_layout_1;
	
	toppos = 45;
	for (auto& petEntry : m_pets)
	{
		outfile << "<Window Type=\"GuiLook/StandardButton\" Name=\"" << petEntry.first << "\">";
		outfile << "<Property Name=\"UnifiedPosition\" Value=\"{{0,0},{0," << toppos << "}}\" />";
		outfile << "<Property Name=\"UnifiedSize\" Value=\"{{0,140},{0,28}}\" />";
		outfile << "<Property Name=\"Text\" Value=\"" << petEntry.second << "\"/>";
		outfile << "<Property Name=\"onClick\" Value=\"guiPet1\"/>";
		outfile << "</Window>" << std::endl;
		
		toppos += 30;
	}
	
	outfile << charactercreate_layout_2;
	
	outfile.close();
}

void ModCompiler::compile()
{
	using ModDirState      = std::pair<ModDirectory const*, ModDirectoryConstIterator>;
	using ModDirStateStack = std::stack<ModDirState>;
	
	ModDirStateStack stateStack;
	
	m_currentDir = m_outputDir;
	m_currentModDir = {};
	m_currentModDirUpper = {};
	
	loadClasses();
	
	stateStack.push(std::make_pair(&m_files, m_files.dirs.begin()));
	
	if (mkdir(m_currentDir.str().c_str(), S_IRWXU | S_IRGRP | S_IXGRP) != 0)
	{
		if (errno != EEXIST)
		{
			std::cerr << "Failed to create directory " << m_currentDir << std::endl;
			throw std::runtime_error("Cannot create output directory");
		}
	}
	
	while (!stateStack.empty())
	{
		ModDirState& state = stateStack.top();
		ModDirectory const& modDir = *state.first;
		
		if (state.second == modDir.dirs.begin())
		{
			for (auto& fileEntry : modDir.files)
			{
				processFile(fileEntry);
			}
		}
		
		if (state.second != modDir.dirs.end())
		{
			ModDirectoryEntry const& childDirEntry = *state.second;
			ModDirectory const& childDir = childDirEntry.second;
			
			m_currentDir.cd(childDirEntry.first);
			m_currentModDir.cd(childDirEntry.first);
			m_currentModDirUpper.cd(utf8_to_upper(childDirEntry.first));
			
			stateStack.push(std::make_pair(&childDir, childDir.dirs.begin()));
			++state.second;
			
			if (mkdir(m_currentDir.str().c_str(), S_IRWXU | S_IRGRP | S_IXGRP) != 0)
			{
				if (errno != EEXIST)
				{
					std::cerr << "Failed to create directory " << m_currentDir << std::endl;
					throw std::runtime_error("Cannot create output directory");
				}
			}
		}
		else
		{
			stateStack.pop();
			
			if (!stateStack.empty())
			{
				m_currentDir.up();
				m_currentModDir.up();
				m_currentModDirUpper.up();
			}
		}
	}
	
	std::cerr << "Generating media/MASSFILE.DAT.ADM" << std::endl;
	adm::admFileWrite(m_currentDir.build("media/MASSFILE.DAT.ADM"), m_massfile);
	
	std::cerr << "Generating media/MASTERRESOURCEUNITS.DAT.ADM" << std::endl;
	adm::admFileWrite(m_currentDir.build("media/MASTERRESOURCEUNITS.DAT.ADM"), m_masterresourceunits);
	
	if (mergeClasses())
	{
		std::cerr << "Generating media/UI/charactercreate.layout" << std::endl;
		createCharacterCreateLayout();
	}
	
	std::cerr << std::endl;
	std::cerr << "Done! Now pack 'media' directory located in " << m_currentDir << " into ZIP archive";
	std::cerr << " called 'pak.zip' and replace the one in game directory." << std::endl;
	std::cerr << "Note that this program might contain bugs, don't forget to backup your save files!";
	std::cerr << std::endl;
}

void ModCompiler::tryAddPet(ModFileEntry const& entry, std::shared_ptr<adm::Adm> admPtr)
{
	adm::StringMap& stringMap = admPtr->stringMap();
	adm::AttributeIterator attr;
	uint32_t UNIT_id, NAME_id, UNITTYPE_id, PET_id;
	uint32_t DISPLAYNAME_id;
	
	// Player DAT file must contain UNIT, NAME, UNITTYPE and PET strigs
	// and root's name must be UNIT
	if (!stringMap.find("UNIT", UNIT_id) ||
	    !stringMap.find("NAME", NAME_id) ||
	    !stringMap.find("UNITTYPE", UNITTYPE_id) ||
	    !stringMap.find("PET", PET_id) ||
	    admPtr->root().name != UNIT_id)
		return;
	
	// Check if the monster is PET
	attr = admPtr->root().attributes.find(UNITTYPE_id);
	if (attr == admPtr->root().attributes.end() ||
	    attr->second.type != adm::AttributeValue::TYPE_STRING ||
	    attr->second.valu32 != PET_id)
		return;
	
	// Find the NAME attribute and if it is string, add its value to pets list
	attr = admPtr->root().attributes.find(NAME_id);
	if (attr == admPtr->root().attributes.end() ||
	    attr->second.type != adm::AttributeValue::TYPE_STRING)
		return;
	
	string const& name = stringMap.get(attr->second.valu32);
	
	// If there is no DISPLAYNAME, use NAME as display name
	if (stringMap.find("DISPLAYNAME", DISPLAYNAME_id))
	{
		attr = admPtr->root().attributes.find(DISPLAYNAME_id);
		if (attr != admPtr->root().attributes.end() && (
		    attr->second.type == adm::AttributeValue::TYPE_STRING ||
		    attr->second.type == adm::AttributeValue::TYPE_TRANSLATE))
		{
			m_pets[name] = stringMap.get(attr->second.valu32);
			return;
		}
	}
	m_pets[name] = name;
}

void ModCompiler::addToMasterResourceUnits(
		ModFileEntry const& entry,
		std::shared_ptr<adm::Adm>& admPtr)
	{
		stack<std::shared_ptr<adm::Adm>> admStack;
		adm::AttributeIterator attr;
		string baseFn;
		ModFileIterator fileIt;
		uint32_t DONTCREATE_id, BASEFILE_id;
		
		if (admPtr->stringMap().find("DONTCREATE", DONTCREATE_id))
			attr = admPtr->root().attributes.find(DONTCREATE_id);
		else
			attr = admPtr->root().attributes.end();
		
		// Skip items marked as DONTCREATE
		if (attr != admPtr->root().attributes.end() && attr->second.type == adm::AttributeValue::TYPE_BOOL)
		{
			if (attr->second.valu32 == 1)
				return;
		}
		
		// Load all basefiles onto admStack
		for (;;)
		{
			if (!admPtr->stringMap().find("BASEFILE", BASEFILE_id))
				break;
			
			attr = admPtr->root().attributes.find(BASEFILE_id);
			
			if (attr == admPtr->root().attributes.end() || attr->second.type != adm::AttributeValue::TYPE_STRING)
				break;
			
			baseFn = admPtr->getString(attr->second.valu32);
			FileName::winSlashesToPosix(baseFn);
			baseFn = utf8_to_upper(baseFn);
			
			if (!m_files.lookupFile(baseFn, fileIt))
			{
				std::cerr << "ERROR: cannot find file " << baseFn 
				          << " needed by " << m_currentModDir.build(entry.first) << std::endl;
				throw std::runtime_error("Cannot find BASEFILE");
			}
			
			admStack.push(admPtr);
			admPtr = adm::Adm::createFromFile(fileIt->second.front());
		}
		
		// Merge them
		while (!admStack.empty())
		{
			std::shared_ptr<adm::Adm>& top = admStack.top();
			
			admPtr->mergeNodes(*top, top->root(), admPtr->root(), adm::AttributeReplaceMode::ReplaceAtRoot);
			admStack.pop();
		}
		
		if (m_currentModDirUpper.isChildOf("MEDIA/UNITS/ITEMS"))
			tryMergeClassWardrobes(entry, admPtr);
		else if (m_currentModDirUpper.isChildOf("MEDIA/UNITS/MONSTERS"))
			tryAddPet(entry, admPtr);
		
		m_masterresourceunits.addUnit(utf8_to_upper(entry.first), m_currentModDirUpper, *admPtr);
	}

void ModCompiler::tryMergeClassWardrobes(ModFileEntry const& entry, std::shared_ptr<adm::Adm> admPtr)
{
	struct AdmStringInfo
	{
		bool present;
		uint32_t id;
	};
	
	adm::AttributeConstIterator attrIt;
	AdmStringInfo wardrobeString, classString;
	string wardrobeClassName;
	
	using WardrobeMap  = map<string, adm::NodeIterator>;
	using WardrobeIter = WardrobeMap::iterator;
	
	WardrobeMap wardrobes;
	WardrobeIter wardrobe;
	bool inserted;
	
	wardrobeString.present = admPtr->stringMap().find("WARDROBE", wardrobeString.id);
	classString.present    = admPtr->stringMap().find("CLASS", classString.id);
	
	adm::NodeIterator nodeIt, nodeEndIt;
	
	nodeEndIt = admPtr->root().subnodes.end();
	
	if (wardrobeString.present && classString.present)
	{
		nodeIt = admPtr->root().subnodes.begin();
		
		while (nodeIt != nodeEndIt)
		{
			if (nodeIt->name == wardrobeString.id)
			{
				attrIt = nodeIt->attributes.find(classString.id);
				if (attrIt != nodeIt->attributes.end() && attrIt->second.type == adm::AttributeValue::TYPE_STRING)
				{
					wardrobeClassName = utf8_to_upper(admPtr->getString(attrIt->second.valu32));
					
					std::tie(wardrobe, inserted) = wardrobes.insert(std::make_pair(wardrobeClassName, nodeIt));
					
					// Only keep first wardrobe if two with the same name exist
					if (!inserted)
					{
						nodeIt = admPtr->root().subnodes.erase(nodeIt);
						continue;
					}
				}
			}
			++nodeIt;
		}
	}
	
	// Merge wardrobes from previous mods
	{
		std::list<string>::const_iterator fileIt, fileItEnd;
		uint32_t WARDROBE_id, CLASS_id;
		
		fileIt = entry.second.begin();
		fileItEnd = entry.second.end();
		
		// Skipping first entry since it is admPtr
		while (++fileIt != fileItEnd)
		{
			adm::Adm prevAdm;
			
			try {
				prevAdm.loadFromFile(*fileIt);
			}
			catch (...)
			{ continue; }
			
			// If the ADM file doesn't have these strings, we know it doesn't have
			// WARDROBE node with CLASS property in it
			if (!prevAdm.stringMap().find("WARDROBE", WARDROBE_id))
				continue;
			
			if (!prevAdm.stringMap().find("CLASS", CLASS_id))
				continue;
			
			// Find WARDROBE subnodes
			for (auto& node : prevAdm.root().subnodes)
			{
				if (node.name != WARDROBE_id)
					continue;
				
				// See if it has CLASS property of string type
				attrIt = node.attributes.find(CLASS_id);
				if (attrIt == node.attributes.end() || attrIt->second.type != adm::AttributeValue::TYPE_STRING)
					continue;
				
				// Get the class name
				wardrobeClassName = utf8_to_upper(prevAdm.getString(attrIt->second.valu32));
				
				std::tie(wardrobe, inserted) = wardrobes.insert(std::make_pair(wardrobeClassName, adm::NodeIterator()));
				
				if (inserted)
				{
					wardrobe->second = admPtr->root().insertSubnode();
					
					wardrobe->second->name = admPtr->addString("WARDROBE");
					admPtr->mergeNodes(prevAdm, node, *wardrobe->second, adm::AttributeReplaceMode::DontReplace);
				}
			}
		}
	}
}

/*
	{
		struct AdmStringInfo
		{
			bool present;
			uint32_t id;
		};
		
		std::shared_ptr<adm::Adm> prevAdm;
		std::list<string>::const_iterator it, it_end;
		uint32_t WARDROBE_id, CLASS_id;
		adm::AttributeConstIterator attrIt;
		std::string wardrobeClass;
		bool canHaveWardrobe;
		
		AdmStringInfo wardrobeString, classString, wardrobeClassString;
		
		it = entry.second.begin();
		it_end = entry.second.end();
		
		wardrobeString.present = admPtr->stringMap().find("WARDROBE", wardrobeString.id);
		classString.present    = admPtr->stringMap().find("CLASS", classString.id);
		
		// First entry is file for admPtr, so skip it
		while (++it != it_end)
		{
			try {
				prevAdm = adm::Adm::createFromFile(*it);
			}
			catch (...)
			{
				//std::cout << "Cannot load previous file for " << entry.second.front() << " ("
				//          << *it << "), I won't try to merge wardrobes for it." << std::endl;
				continue;
			}
			
			// If the ADM file doesn't have these strings, we know it doesn't have
			// WARDROBE node with CLASS property in it
			if (!prevAdm->stringMap().find("WARDROBE", WARDROBE_id))
				continue;
			
			if (!prevAdm->stringMap().find("CLASS", CLASS_id))
				continue;
			
			// Find WARDROBE subnodes
			for (auto& node : prevAdm->root().subnodes)
			{
				if (node.name != WARDROBE_id)
					continue;
				
				// See if it has CLASS property of string type
				attrIt = node.attributes.find(CLASS_id);
				if (attrIt == node.attributes.end() || attrIt->second.type != adm::AttributeValue::TYPE_STRING)
					continue;
				
				// Get the class name
				wardrobeClass = prevAdm->getString(attrIt->second.valu32);
				
				canHaveWardrobe = true;
				
				if (!wardrobeString.present) {
					canHaveWardrobe = false;
					wardrobeString.id = admPtr->addString("WARDROBE");
				}
				
				if (!classString.present) {
					canHaveWardrobe = false;
					classString.id = admPtr->addString("CLASS");
				}
				
				wardrobeClassString.present = admPtr->stringMap().find(wardrobeClass, wardrobeClassString.id);
				if (!wardrobeClassString.present)
					canHaveWardrobe = false;
				
				adm::NodeIterator nodeIt = admPtr->root().subnodes.end();
				
				if (canHaveWardrobe)
				{
					adm::NodeIterator nodeEnd = nodeIt;
					
					for (nodeIt = admPtr->root().subnodes.begin(); nodeIt != nodeEnd; ++nodeIt)
					{
						if (nodeIt->name != wardrobeString.id)
							continue;
						
						attrIt = nodeIt->attributes.find(classString.id);
						if (attrIt != nodeIt->attributes.end() &&
						    attrIt->second.type == adm::AttributeValue::TYPE_STRING &&
						    attrIt->second.valu32 == wardrobeClassString.id)
							break;
					}
				}
				
				if (nodeIt == admPtr->root().subnodes.end())
				{
					//std::cout << "I didn't find WARDROBE for class " << wardrobeClass << " in "
					//          << entry.second.front() << ". Merging it from "
					//          << *it << "." << std::endl;
					
					nodeIt = admPtr->root().insertSubnode();
					nodeIt->name = wardrobeString.id;
					admPtr->mergeNodes(*prevAdm, node, *nodeIt, adm::AttributeReplaceMode::DontReplace);
				}
			}
			
		}
	}
*/

}
