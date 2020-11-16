#include "config.h"
#include "moddirectory.h"
#include "mapped_file.h"
#include "dat_file_adm_loader.h"
#include "filename_utils.h"

#include <string>
#include <fstream>
#include <map>

using namespace tlmodder;

using std::string;
using std::ofstream;
using std::map;

Config g_config;
ModDirectory g_gameData;
string g_className;
string g_baseClassName;
string g_classNameUpper;
string g_baseClassNameUpper;

void loadConfig(std::string const& fn)
{
	try {
		g_config.loadFrom(fn);
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
}

void loadGameData()
{
	try {
		g_gameData.loadFromDir(g_config.originalGameData());
	}
	catch (DirIterator::OpenFailed& e)
	{
		throw std::runtime_error("ERROR: Could not load game data");
	}
}


void createDirWithParents(string dirPath)
{
	string::size_type slashPos;
	int mkdirResult;
	const mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP;
	
	slashPos = string::npos;
	
	for (;;)
	{
		mkdirResult = ::mkdir(dirPath.c_str(), mode);
		
		if (mkdirResult == 0 || errno == EEXIST)
			break;
		
		if (mkdirResult == -1 && errno != ENOENT)
			throw std::runtime_error("Cannot create directory");
		
		slashPos = dirPath.rfind('/', slashPos);
		
		if (slashPos == 0 || slashPos == string::npos)
			throw std::runtime_error("Cannot create directory");
		
		dirPath[slashPos] = '\0';
	}
	
	while (slashPos != string::npos)
	{
		dirPath[slashPos] = '/';
		
		mkdirResult = ::mkdir(dirPath.c_str(), mode);
		if (mkdirResult == -1 && errno != EEXIST)
			throw std::runtime_error("Cannot create directory");
		
		slashPos = dirPath.find('\0', slashPos);
	}
}

void createClassDatFile()
{
	FileName modDir(g_config.modDir());
	FileName baseClassFn("media/units/players/");
	ModFileIterator it;
	
	baseClassFn.cd(g_baseClassName);
	baseClassFn.cd(g_baseClassName + ".dat");
	
	if (!g_gameData.lookupFile(baseClassFn.str(), it))
		throw std::runtime_error("Cannot find base class");
	
	modDir.cd(g_className);
	modDir.cd("media/units/players");
	modDir.cd(g_className);
	createDirWithParents(modDir.str());
	
	adm::Adm adm;
	adm.loadFromFile(it->second.front());
	
	adm.root().setAttribute(
		adm.addString("NAME"),
		adm.stringAttribute(g_className)
		);
	
	adm.root().setAttribute(
		adm.addString("DISPLAYNAME"),
		adm.stringAttribute(g_className)
		);
	
	//TODO: generate new UNIT_GUID
	
	ofstream dat;
	dat.exceptions(std::ios::badbit | std::ios::failbit);
	dat.open(modDir.build(g_className + ".dat"), std::ios::trunc);
	adm.dump(dat);
	dat.close();
}

void maybeCreateItem(ModFileEntry const& file, FileName const& modDir)
{
	struct AdmStringInfo
	{
		bool present;
		uint32_t id;
	};
	
	adm::Adm adm;
	adm::AttributeConstIterator attrIt;
	AdmStringInfo wardrobeString, classString;
	string wardrobeClassName;
	
	using WardrobeMap  = map<string, adm::NodeIterator>;
	using WardrobeIter = WardrobeMap::iterator;
	
	WardrobeMap wardrobes;
	WardrobeIter wardrobe;
	bool inserted;
	
	try {
		adm.loadFromFile(file.second.front());
	}
	catch (...)
	{ return; }
	
	wardrobeString.present = adm.stringMap().find("WARDROBE", wardrobeString.id);
	classString.present    = adm.stringMap().find("CLASS", classString.id);
	
	adm::NodeIterator nodeIt, nodeEndIt;
	
	nodeEndIt = adm.root().subnodes.end();
	
	if (wardrobeString.present && classString.present)
	{
		nodeIt = adm.root().subnodes.begin();
		
		while (nodeIt != nodeEndIt)
		{
			if (nodeIt->name == wardrobeString.id)
			{
				attrIt = nodeIt->attributes.find(classString.id);
				if (attrIt != nodeIt->attributes.end() && attrIt->second.type == adm::AttributeValue::TYPE_STRING)
				{
					wardrobeClassName = utf8_to_upper(adm.getString(attrIt->second.valu32));
					
					std::tie(wardrobe, inserted) = wardrobes.insert(std::make_pair(wardrobeClassName, nodeIt));
					
					// Only keep first wardrobe if two with the same name exist
					if (!inserted)
					{
						nodeIt = adm.root().subnodes.erase(nodeIt);
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
		
		fileIt = file.second.begin();
		fileItEnd = file.second.end();
		
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
					wardrobe->second = adm.root().insertSubnode();
					
					wardrobe->second->name = adm.addString("WARDROBE");
					adm.mergeNodes(prevAdm, node, *wardrobe->second, adm::AttributeReplaceMode::DontReplace);
				}
			}
		}
	}
	
	WardrobeIter baseClassWardrobe, classWardrobe;
	adm::NodeIterator classWardrobeNode;
	
	baseClassWardrobe = wardrobes.find(g_baseClassNameUpper);
	
	if (baseClassWardrobe == wardrobes.end())
		return;
	
	classWardrobe = wardrobes.find(g_classNameUpper);
	if (classWardrobe != wardrobes.end())
	{
		classWardrobeNode = classWardrobe->second;
		classWardrobeNode->attributes.clear();
		classWardrobeNode->subnodes.clear();
	}
	else
	{
		classWardrobeNode = adm.root().insertSubnode();
		classWardrobeNode->name = adm.addString("WARDROBE");
	}
	
	adm.mergeNodes(adm, *baseClassWardrobe->second, *classWardrobeNode, adm::AttributeReplaceMode::DontReplace);
	classWardrobeNode->setAttribute(adm.addString("CLASS"), adm.stringAttribute(g_classNameUpper));
	
	ofstream dat;
	dat.exceptions(std::ios::badbit | std::ios::failbit);
	dat.open(modDir.build(file.first), std::ios::trunc);
	adm.dump(dat);
	dat.close();
}

void copyItems()
{
	FileName modDir(g_config.modDir());
	ModDirectoryIterator it;
	string ext;
	
	if (!g_gameData.lookupDir("media/units/items", it))
		throw std::runtime_error("cannot find media/units/items");
	
	modDir.cd(g_className);
	modDir.cd("media/units/items");
	
	using DirState = std::pair<ModDirectory*, ModDirectoryIterator>;
	using DirStack = std::stack<DirState>;
	
	DirStack dirStack;
	ModDirectory *dir;
	
	dirStack.emplace(&it->second, it->second.dirs.begin());
	
	while (!dirStack.empty())
	{
		DirState& dirState = dirStack.top();
		dir = dirState.first;
		
		if (dirState.second == dir->dirs.begin())
		{
			// Create dir
			createDirWithParents(modDir.str());
			
			// Process files
			for (auto& file : dir->files)
			{
				ext = utf8_to_upper(FileName::extension(file.first));
				if (ext == "DAT")
					maybeCreateItem(file, modDir);
			}
		}
		
		if (dirState.second != dir->dirs.end())
		{
			ModDirectoryEntry& childEntry = *dirState.second;
			
			modDir.cd(childEntry.first);
			dirStack.emplace(&childEntry.second, childEntry.second.dirs.begin());
			++dirState.second;
		}
		else
		{
			dirStack.pop();
			if (!dirStack.empty())
				modDir.up();
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: classcreate new_class_name base_class_name" << std::endl;
		return 1;
	}
	
	g_className = argv[1];
	g_baseClassName = argv[2];
	
	g_classNameUpper = utf8_to_upper(g_className);
	g_baseClassNameUpper = utf8_to_upper(g_baseClassName);
	
	loadConfig("./tlmodder.cfg");
	loadGameData();
	
	createClassDatFile();
	copyItems();
	
	return 0;
}
