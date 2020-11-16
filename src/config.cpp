/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "config.h"

namespace tlmodder {

void Config::setDefaults()
{
	m_lookForNew = true;
	m_mergeClassMods = false;
	m_modDir = "./mods";
	m_originalGameData = "./original";
	m_outputDir = "./output";
}

void Config::loadFrom(std::string const& fn)
{
	adm::Adm config;
	adm::AttributeIterator attr;
	uint32_t PRIORITY_id, NAME_id, ENABLED_id, MOD_id;
	uint32_t MOD_DIR_id, ORIGINAL_GAME_DATA_id, OUTPUT_DIR_id;
	uint32_t MERGE_CLASS_MODS_id, LOOK_FOR_NEW_id;
	
	setDefaults();
	config.loadFromDat(fn);
	
	if (config.root().name != config.addString("TLMODDER"))
		std::cerr << "WARNING: configuration file root node should be called TLMODDER" << std::endl;
	
	MOD_DIR_id = config.addString("MOD_DIR");
	ORIGINAL_GAME_DATA_id = config.addString("ORIGINAL_GAME_DATA");
	OUTPUT_DIR_id = config.addString("OUTPUT_DIR");
	
	MERGE_CLASS_MODS_id = config.addString("MERGE_CLASS_MODS");
	LOOK_FOR_NEW_id = config.addString("LOOK_FOR_NEW");
	
	PRIORITY_id = config.addString("PRIORITY");
	NAME_id = config.addString("NAME");
	ENABLED_id = config.addString("ENABLED");
	MOD_id = config.addString("MOD");
	
	
	for (auto const& attribute : config.root().attributes)
	{
		if (attribute.first == MOD_DIR_id)
		{
			if (attribute.second.type == adm::AttributeValue::TYPE_STRING)
				m_modDir = config.getString(attribute.second.valu32);
			else
				std::cerr << "WARNING: attribute MOD_DIR should be of type STRING" << std::endl;
		}
		else if (attribute.first == ORIGINAL_GAME_DATA_id)
		{
			if (attribute.second.type == adm::AttributeValue::TYPE_STRING)
				m_originalGameData = config.getString(attribute.second.valu32);
			else
				std::cerr << "WARNING: attribute ORIGINAL_GAME_DATA should be of type STRING" << std::endl;
		}
		else if (attribute.first == OUTPUT_DIR_id)
		{
			if (attribute.second.type == adm::AttributeValue::TYPE_STRING)
				m_outputDir = config.getString(attribute.second.valu32);
			else
				std::cerr << "WARNING: attribute OUTPUT_DIR should be of type STRING" << std::endl;
		}
		else if (attribute.first == MERGE_CLASS_MODS_id)
		{
			if (attribute.second.type == adm::AttributeValue::TYPE_BOOL)
				m_mergeClassMods = attribute.second.valu32 != 0 ? true : false;
			else
				std::cerr << "WARNING: attribute MERGE_CLASS_MODS should be of type BOOL" << std::endl;
		}
		else if (attribute.first == LOOK_FOR_NEW_id)
		{
			if (attribute.second.type == adm::AttributeValue::TYPE_BOOL)
				m_lookForNew = attribute.second.valu32 != 0 ? true : false;
			else
				std::cerr << "WARNING: attribute LOOK_FOR_NEW should be of type BOOL" << std::endl;
		}
		else
		{
			std::cerr << "WARNING: ignoring unknown attribute " << config.getString(attribute.first) << std::endl;
		}
	}
	
	for (auto const& modNode : config.root().subnodes)
	{
		if (modNode.name != MOD_id)
		{
			std::cerr << "WARNING: skipping unknown node " << config.getString(modNode.name) << std::endl;
			continue;
		}
		
		ModConfig modConfig;
		modConfig.priority = 0;
		modConfig.enabled = true;
		
		for (auto const& attribute : modNode.attributes)
		{
			if (attribute.first == PRIORITY_id)
			{
				if (attribute.second.type == adm::AttributeValue::TYPE_INT)
					modConfig.priority = attribute.second.vali32;
				else
					std::cerr << "WARNING: attribute PRIORITY should be of type INTEGER" << std::endl;
			}
			else if (attribute.first == ENABLED_id)
			{
				if (attribute.second.type == adm::AttributeValue::TYPE_BOOL)
					modConfig.enabled = attribute.second.valu32 != 0 ? true : false;
				else
					std::cerr << "WARNING: attribute ENABLED should be of type BOOL" << std::endl;
			}
			else if (attribute.first == NAME_id)
			{
				if (attribute.second.type == adm::AttributeValue::TYPE_STRING)
					modConfig.name = config.getString(attribute.second.valu32);
				else
					std::cerr << "WARNING: attribute NAME should be of type STRING" << std::endl;
			}
			else
			{
				std::cerr << "WARNING: ignoring unknown MOD attribute " << config.getString(attribute.first) << std::endl;
			}
		}
		
		if (!modConfig.name.empty())
		{
			bool inserted;
			
			std::tie(std::ignore, inserted) = m_modConfigs.insert(modConfig);
			
			if (!inserted)
			{
				std::cerr << "WARNING: mod " << modConfig.name << " listed more then once. Using first settings." << std::endl;
			}
		}
	}
}

}
