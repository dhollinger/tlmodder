/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __UNICODE_H__
#define __UNICODE_H__

#include <cstddef>
#include <cstdint>
#include <string>

namespace tlmodder
{

using std::string;
using std::u16string;
using std::size_t;
using std::uint32_t;

struct utf8_t     {};
struct utf16_le_t {};
struct utf16_be_t {};
struct utf32_le_t {};
struct utf32_be_t {};

#if !defined(TLMODDER_BIG_ENDIAN)
using utf16_native_t = utf16_le_t;
using utf32_native_t = utf32_le_t;
#else
using utf16_native_t = utf16_be_t;
using utf32_native_t = utf32_be_t;
#endif

template<
	typename utf16_tag,
	bool is_native = std::is_same<utf16_native_t, utf16_tag>::value
	>
struct utf16_endian_conv
{
	static inline constexpr char16_t conv(char16_t chr)
	{ return chr; }
};

template<
	typename utf16_tag
>
struct utf16_endian_conv<utf16_tag, false>
{
	static inline constexpr char16_t conv(char16_t chr)
	{ return ((chr & UINT16_C(0xff)) << 8) | ((chr >> 8) & UINT16_C(0xff)); }
};

class unicode_iterator
{
public:
	unicode_iterator(): m_replacement(U'\ufffd') {}
	virtual ~unicode_iterator() {}
	
	void set_replacement(char32_t replacement)
	{ m_replacement = replacement; }
	
	char32_t replacement() const
	{ return m_replacement; }
	
	virtual void reset() = 0;
	virtual bool next(char32_t& chr_out) = 0;
protected:
	char32_t m_replacement;
};

template<
	typename utf16_tag = utf16_native_t
	>
class utf16_iterator : public unicode_iterator
{
public:
	using char_type = char16_t;
	
	utf16_iterator():
		m_begin(nullptr), m_end(nullptr), m_cur(nullptr)
	{}
	
	utf16_iterator(char16_t const* str, size_t len):
		m_begin(str), m_end(str+len), m_cur(str)
	{}
	
	utf16_iterator(u16string const& str)
	{
		m_begin = str.c_str();
		m_end   = m_begin + str.size();
		m_cur   = m_begin;
	}
	
	void reset() override
	{ m_cur = m_begin; }
	
	bool next(char32_t& chr_out) override
	{
		uint32_t c;
		
		if (m_cur == m_end)
			return false;
		
		c = (uint32_t)endian_conv(*m_cur);
		++m_cur;
		
		// No surrogate
		if (c < 0xd800u || c > 0xdfff) 
		{
			chr_out = (char32_t)c;
		}
		// Trail surrogate, missing trail surrogate
		else if (c > 0xdbffu || m_cur == m_end)
		{
			chr_out = m_replacement;
		}
		else
		{
			uint32_t trail = (uint32_t)endian_conv(*m_cur);
			
			if (trail < 0xdc00u || trail > 0xdfffu) // invalid trail surrogate?
			{
				chr_out = m_replacement;
			}
			else
			{
				++m_cur;
				
				c = (c - 0xd800u) << 10;
				c |= trail - 0xdc00u;
				c += 0x10000u;
				chr_out = (char32_t)trail;
			}
		}
		
		return true;
	}
protected:
	inline char16_t endian_conv(char16_t chr)
	{ return utf16_endian_conv<utf16_tag>::conv(chr); }
protected:
	char16_t const* m_begin;
	char16_t const* m_end;
	char16_t const* m_cur;
};




class utf8_iterator : public unicode_iterator
{
public:
	using char_type = char;
	
	utf8_iterator():
		m_begin(nullptr), m_end(nullptr), m_cur(nullptr)
	{}
	
	utf8_iterator(char const* str, size_t len):
		m_begin(str), m_end(str+len), m_cur(str)
	{}
	
	utf8_iterator(string const& str)
	{
		m_begin = str.c_str();
		m_end   = m_begin + str.size();
		m_cur   = m_begin;
	}
	
	void reset() override
	{ m_cur = m_begin; }
	
	bool next(char32_t& chr_out) override;
	
protected:
	char const* m_begin;
	char const* m_end;
	char const* m_cur;
};

// 4 characters must fit in the buffer
size_t utf32chr_to_utf8(char32_t c, char *buf);

// 2 characters must fit in the buffer
size_t utf32chr_to_utf16(char32_t c, char16_t *buf);

string utf16_to_utf8(char16_t const* utf16str, size_t size);

inline string utf16_to_utf8(u16string const& utf16str)
{ return std::move(utf16_to_utf8(utf16str.c_str(), utf16str.size())); }

// FIXME: for now handle just ASCII
static inline string utf8_to_upper(string const& str)
{
	string result;
	for (char c : str)
		result += (char)::toupper(c);
	return std::move(result);
}

}

#endif
