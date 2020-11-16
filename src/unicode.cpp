/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "unicode.h"

namespace tlmodder
{

static const uint8_t utf8_len_table[256] = {
/*   0 -  31 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/*  32 -  63 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/*  64 -  95 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/*  96 - 127 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 128 - 159 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 160 - 191 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 192 - 223 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* 224 - 255 */ 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0
};

static const uint8_t utf8_mask_table[7] = {
	0x00, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

bool utf8_iterator::next(char32_t& chr_out)
{
	uint8_t c;
	uint8_t tableLen;
	uint8_t mask;
	char const* end;
	
	if (m_cur == m_end)
		return false;
	
	c = (uint8_t)*m_cur;
	tableLen = utf8_len_table[c];
	mask = utf8_mask_table[tableLen];
	
	for (end = m_cur+1; end != m_end && ((uint8_t)*end & 0xc0) == 0x80; ++end);
	
	if (end - m_cur != tableLen)
	{
		// FIXME: maybe fall here for tableLen > 4 too, since it was deprecated
		chr_out = 0xfffdu;
	}
	else
	{
		chr_out = c & mask;
		for (uint8_t i = 1; i < tableLen; ++i)
		{
			chr_out = (chr_out << 6) | ((uint8_t)m_cur[i] & 0xc0);
		}
	}
	
	m_cur = end;
	return true;
}

size_t utf32chr_to_utf8(char32_t c, char *buf)
{
	// Replace invalid characters by unicode replacement character
	if ((c > 0xd7ffu && c < 0xe000u) || c > 0x10ffffu)
		c = 0xfffd;
	
	if (c < 0x80u)
	{
		buf[0] = (char)c;
		return 1;
	}
	else if (c < 0x800u)
	{
		buf[1] = (char)(((c >> 0) & 0x3fu) | 0x80u);
		buf[0] = (char)(((c >> 6) & 0x1fu) | 0xc0u);
		return 2;
	}
	else if (c < 0x10000u)
	{
		buf[2] = (char)(((c >>  0) & 0x3fu) | 0x80u);
		buf[1] = (char)(((c >>  6) & 0x3fu) | 0x80u);
		buf[0] = (char)(((c >> 12) & 0xfu) | 0xe0u);
		return 3;
	}
	
	buf[3] = (char)(((c >>  0) & 0x3fu) | 0x80u);
	buf[2] = (char)(((c >>  6) & 0x3fu) | 0x80u);
	buf[1] = (char)(((c >> 12) & 0x3fu) | 0x80u);
	buf[0] = (char)(((c >> 18) & 0x7u) | 0xf0u);
	return 4;
}

size_t utf32chr_to_utf16(char32_t c, char16_t *buf)
{
	// Replace invalid characters by unicode replacement character
	if ((c > 0xd7ffu && c < 0xe000u) || c > 0x10ffffu)
		c = 0xfffd;
	
	if (c < 0x10000u)
	{
		buf[0] = (char16_t)c;
		return 1;
	}
	
	c -= 0x10000;
	
	buf[0] = (char16_t)(((uint32_t)c >> 10) + 0xd800u);
	buf[1] = (char16_t)(((uint32_t)c & 0x3ffu) + 0xdc00u);
	
	return 2;
}

string utf16_to_utf8(char16_t const* utf16str, size_t size)
{
	string result;
	char32_t utf32chr;
	char utf8buf[4];
	size_t nchars;
	
	utf16_iterator<> iter(utf16str, size);
	
	while (iter.next(utf32chr))
	{
		nchars = utf32chr_to_utf8(utf32chr, utf8buf);
		result.append(utf8buf, nchars);
	}
	
	return std::move(result);
}

}
