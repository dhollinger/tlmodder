/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_MASTERRESUNITS_H__
#define __TLMODDER_MASTERRESUNITS_H__

#include "adm.h"

namespace tlmodder {

class MasterResourceUnits : public adm::Adm
{
protected:
	uint32_t DATAFILE_STR;
	uint32_t FILEITEM_STR;
	uint32_t RESOURCEGROUP_STR;
	uint32_t DONTCREATE_STR;
	
	// Values are equal to RESOURCEGROUP number
	enum : uint32_t {
		ITEMS     = 0,
		MONSTERS  = 1,
		PLAYERS   = 2,
		PROPS     = 3
	};
	
	// IDs of each resource type string
	uint32_t m_resourceStrings[4];
public:
	MasterResourceUnits():
		adm::Adm("UNITS")
	{
		DATAFILE_STR      = addString("DATAFILE");
		FILEITEM_STR      = addString("FILEITEM");
		RESOURCEGROUP_STR = addString("RESOURCEGROUP");
		DONTCREATE_STR    = addString("DONTCREATE");
		
		m_resourceStrings[ITEMS]    = addString("ITEMS");
		m_resourceStrings[MONSTERS] = addString("MONSTERS");
		m_resourceStrings[PLAYERS]  = addString("PLAYERS");
		m_resourceStrings[PROPS]    = addString("PROPS");
	}
	
	void addUnit(std::string fileitem, FileName const& modDir, adm::Adm const& adm)
	{
		uint32_t itemtype;
		adm::NodeIterator node;
		
		if (modDir.isChildOf("MEDIA/UNITS/ITEMS"))
			itemtype = ITEMS;
		else if (modDir.isChildOf("MEDIA/UNITS/MONSTERS"))
			itemtype = MONSTERS;
		else if (modDir.isChildOf("MEDIA/UNITS/PLAYERS"))
			itemtype = PLAYERS;
		else if (modDir.isChildOf("MEDIA/UNITS/PROPS"))
			itemtype = PROPS;
		else
		{
			std::cerr << "WARNING: I don't know what section to put " << modDir.build(fileitem)
			          << " into." << std::endl;
			return;
		}
		
		node = m_root.insertSubnode();
		node->name = m_resourceStrings[itemtype];
		
		//adm.dump(std::cout);
		
		mergeNodes(adm, adm.root(), *node, adm::AttributeReplaceMode::DontReplace);
		
		node->setAttribute(DONTCREATE_STR, false);
		node->setAttribute(RESOURCEGROUP_STR, itemtype);
		node->setAttribute(DATAFILE_STR, stringAttribute(modDir.build(fileitem)));
		node->setAttribute(FILEITEM_STR, stringAttribute(fileitem));
	}
};

}

#endif
