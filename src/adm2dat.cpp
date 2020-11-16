/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "adm.h"
#include "adm_file_loader.h"
#include "dat_file_adm_loader.h"

#include <iostream>
#include <fstream>

using std::cout;
using std::endl;

using tlmodder::adm::Adm;
using tlmodder::adm::AdmFileLoader;

int main(int argc, const char**argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << "input_file [output_file]" << endl;
		std::cerr << "If output file is not specified, stdout will be used" << endl;
		return 0;
	}
	
	Adm adm;
	{
		AdmFileLoader loader(argv[1]);
		loader.load(adm);
	}
	
	if (argc > 2)
	{
		std::ofstream strm;
		strm.exceptions(std::ios::badbit | std::ios::failbit);
		strm.open(argv[2], std::ios::trunc);
		adm.dump(strm);
		strm.close();
	}
	else
	{
		adm.dump(cout);
	}
	
	return 0;
}

