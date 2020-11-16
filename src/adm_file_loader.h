/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __ADM_FILE_LOADER_H__
#define __ADM_FILE_LOADER_H__

#include "adm.h"
#include "mapped_file.h"

namespace tlmodder {
namespace adm {

class AdmFileLoader : public Loader
{
public:
	AdmFileLoader(std::string const& file);
	AdmFileLoader(DirIterator const& dir, std::string const& file);
	virtual ~AdmFileLoader() {}
	
	void load(Adm& adm) override;
protected:
	MappedFile m_file;
};

} // namespace adm
} // namespace tlmodder

#endif
