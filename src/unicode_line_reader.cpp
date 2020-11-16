/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "unicode_line_reader.h"

namespace tlmodder
{

// ~~ UTF-8 line reader implementation ~~

utf8_line_reader::utf8_line_reader(char const* ptr, size_t size):
	m_cur(ptr),
	m_end(ptr+size)
{}

bool utf8_line_reader::read_line(string& line)
{
	char const *line_end;
	char chr;
	
	if (m_cur == m_end)
		return false;
	
	line_end = m_cur;
	
	// Find line end
	do
	{
		chr = *line_end;
		
		if (chr == CR || chr == LF)
			break;
		
		++line_end;
		
	} while (line_end != m_end);
	
	// Assing the line
	line = string(m_cur, line_end-m_cur);
	
	m_cur = line_end;
	
	// Support mixed CR, CR+LF and LF line endings
	if (m_cur != m_end)
	{
		++m_cur;
		if (chr == CR && m_cur != m_end && *m_cur == LF)
			++m_cur;
	}
	
	return true;
}

}
