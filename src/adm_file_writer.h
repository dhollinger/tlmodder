/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __ADM_FILE_WRITER_H__
#define __ADM_FILE_WRITER_H__

#include "adm.h"
#include <iostream>

namespace tlmodder {
namespace adm {

void admFileWrite(std::ostream& strm, Adm const& adm);
void admFileWrite(std::string const& filename, Adm const& adm);

}
}

#endif
