/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __TLMODDER_MASSFILE_H__
#define __TLMODDER_MASSFILE_H__

#include "adm.h"
#include "filename_utils.h"

namespace tlmodder {

class MassFile : public adm::Adm
{
public:
	MassFile():
		adm::Adm("MAINDATA")
	{}
	
	void addFile(
		adm::Adm const& sourceAdm,
		adm::Node const& sourceNode,
		std::string fileName)
	{
		adm::NodeIterator it;
		
		it = root().insertSubnode();
		it->name = addString(std::move(fileName));
		mergeNodes(sourceAdm, sourceNode, *it, adm::AttributeReplaceMode::DontReplace);
	}
	
	static bool isDirWhitelisted(FileName const& mod_dir);
};

}

#endif
