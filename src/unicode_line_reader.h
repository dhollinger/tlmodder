/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __UNICODE_LINE_READER_H__
#define __UNICODE_LINE_READER_H__

#include "unicode.h"

namespace tlmodder
{

using std::string;
using std::size_t;


// Abstract class for unicode line readers
class unicode_line_reader
{
public:
	unicode_line_reader() {}
	virtual ~unicode_line_reader() {}
	
	// This should read a line and set 'line' to UTF-8 encoded line
	// without line ending characters
	// Return true if line was read, false if end was reached
	virtual bool read_line(string& line) = 0;
};


// UTF-16 line reader, handles both little-endian and big-endian encodings
// BOM is not handled and must be skipped before passing to line reader
template<
	typename utf16_tag = utf16_native_t
	>
class utf16_line_reader : public unicode_line_reader
{
public:
	utf16_line_reader(char16_t const* ptr, size_t size);
	virtual ~utf16_line_reader() {}
	
	bool read_line(string& line) override;
protected:
	using endian_conv_type = utf16_endian_conv<utf16_tag>;
	using iter_type        = utf16_iterator<utf16_tag>;
	
	static const char16_t CR = endian_conv_type::conv(u'\r');
	static const char16_t LF = endian_conv_type::conv(u'\n');
	char16_t const* m_cur;
	char16_t const* m_end;
};


// UTF-8 line reader
class utf8_line_reader : public unicode_line_reader
{
public:
	utf8_line_reader(char const* ptr, size_t size);
	virtual ~utf8_line_reader() {}
	
	bool read_line(string& line) override;
protected:
	static const char CR = '\r';
	static const char LF = '\n';
	char const* m_cur;
	char const* m_end;
};


// ~~ UTF-16 line reader implementation ~~

template<
	typename endian_conv
	>
utf16_line_reader<endian_conv>::utf16_line_reader(char16_t const* ptr, size_t size):
	m_cur(ptr),
	m_end(ptr+size)
{}

template<
	typename endian_conv
	>
bool utf16_line_reader<endian_conv>::read_line(string& line)
{
	char16_t const *line_end;
	char16_t chr;
	
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
	
	// Convert to UTF-8
	string utf8str;
	char32_t utf32chr;
	char utf8buf[4];
	size_t utf8len;
	iter_type iter(m_cur, line_end-m_cur);
	
	while (iter.next(utf32chr))
	{
		utf8len = utf32chr_to_utf8(utf32chr, utf8buf);
		utf8str.append(utf8buf, utf8len);
	}
	line = std::move(utf8str);
	
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

#endif
