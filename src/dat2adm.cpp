/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "adm.h"
#include "dat_file_adm_loader.h"
#include "adm_file_writer.h"

#include <iostream>

using std::string;
using tlmodder::adm::Adm;
using tlmodder::adm::DatFileLoader;

int main(int argc, const char**argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << "input_file [output_file]" << std::endl;
		return 0;
	}
	
	string input_file = argv[1];
	string output_file;
	
	if (argc > 2)
		output_file = argv[2];
	else
		output_file = input_file + ".adm";
	
	Adm adm;
	
	try
	{
		DatFileLoader loader(input_file);
		loader.load(adm);
	}
	catch (DatFileLoader::Exception& e)
	{
		std::cerr << e.format() << std::endl;
	}
	
	admFileWrite(output_file, adm);
	
	return 0;
}
