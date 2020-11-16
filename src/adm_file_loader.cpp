/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "adm_file_loader.h"

#include <stdexcept>
#include <iostream>
#include <map>
#include "unicode.h"

namespace tlmodder {
namespace adm {

using std::size_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::cerr;
using std::endl;

// NOTE: this class is more or less the only remain of original dat->adm adm->dat converter
//       by dengus. His version was windows only and I learned the ADM format from it's source
//       code. Big thanks to him!
//       See http://forums.runicgames.com/viewtopic.php?f=5&t=2903

// FIXME: big endian is no no. But I don't think the game runs on such system so ... maybe someone
//        not lazy want to fix this ^-^.
class c_get
{
public:
	c_get(const uint8_t* buf, size_t len):
		_buf(buf), _len(len)
	{}
	
	const uint8_t* get(size_t len)
	{
		if (len > _len)
			throw std::runtime_error("unexpected eof :(");
			
		const uint8_t* ret = _buf;
		_buf += len;
		_len -= len;
		
		return ret;
	}
	
	template<typename t> t get()
	{
		return *(t*)get(sizeof(t));
	}
	
	uint64_t gu64()  { return get<uint64_t>(); }
	uint32_t gu32()  { return get<uint32_t>(); }
	uint16_t gu16()  { return get<uint16_t>(); }
	uint8_t  gu8()   { return get<uint8_t>(); }
	bool eof() const { return _len == 0; }
	
protected:
	const uint8_t *_buf;
	size_t _len;
};

struct LoadState
{
	c_get m_g;
	Adm& m_adm;
	std::map<uint32_t, uint32_t> m_stringReplacementMap;
	
	LoadState(Adm& adm, const uint8_t* buf, size_t len):
		m_g(buf, len),
		m_adm(adm)
	{}
	
	void loadStringMap();
	void loadAttributes(Node& node, uint32_t cnt);
	void loadSubnodes(Node& node, uint32_t cnt);
	
	uint32_t replaceStringId(uint32_t id);
};


AdmFileLoader::AdmFileLoader(std::string const& file):
	m_file(file)
{}

AdmFileLoader::AdmFileLoader(DirIterator const& dir, std::string const& file):
	m_file(dir, file)
{}

void AdmFileLoader::load(Adm& adm)
{
	LoadState state(adm, m_file.ptr(), m_file.size());
	
	uint32_t version = state.m_g.gu32();
	
	if (version != 1)
	{
		cerr << "Warning: version mismatch, expected 1, got "
		     << version << ". Expect errors." << endl;
	}
	
	state.loadStringMap();
	
	Node dummy;
	state.loadSubnodes(dummy, 1);
	
	adm.root() = std::move(*dummy.subnodes.begin());
}

void LoadState::loadStringMap()
{
	uint32_t str_num = m_g.gu32();
	uint32_t id, len;
	char16_t const* str;
	
	for (uint32_t i=0; i< str_num; ++i)
	{
		id = m_g.gu32();
		len = m_g.gu32();
		
		str = (char16_t const*)m_g.get(len * sizeof(char16_t));
		
		m_stringReplacementMap[id] = m_adm.addString(utf16_to_utf8(str, len));
	}
}

uint32_t LoadState::replaceStringId(uint32_t id)
{
	auto it = m_stringReplacementMap.find(id);
	
	if (it == m_stringReplacementMap.end())
		it = m_stringReplacementMap.insert(it, std::make_pair(id, m_adm.addString("")));
	
	return it->second;
}

void LoadState::loadAttributes(Node& node, uint32_t cnt)
{
	uint32_t name;
	AttributeValue value;
	
	for (uint32_t i = 0; i < cnt; ++i)
	{
		name = replaceStringId(m_g.gu32());
		value.type = m_g.gu32();
		
		switch (value.type)
		{
			case AttributeValue::TYPE_INT:
			case AttributeValue::TYPE_UINT:
			case AttributeValue::TYPE_BOOL:
				value.valu32 = m_g.gu32();
				break;
			case AttributeValue::TYPE_TRANSLATE:
			case AttributeValue::TYPE_STRING:
				value.valu32 = replaceStringId(m_g.gu32());
				break;
			case AttributeValue::TYPE_INT64:
				value.vali64 = (int64_t)m_g.gu64();
				break;
			case AttributeValue::TYPE_FLOAT:
				value.valf = m_g.get<float>();
				break;
			case AttributeValue::TYPE_DOUBLE:
				value.vald = m_g.get<double>();
				break;
			default:
				throw std::runtime_error("Unknown attribute type");
		}
		
		node.insertAttribute(name, value);
	}
}

void LoadState::loadSubnodes(Node& node, uint32_t cnt)
{
	NodeIterator iter;
	uint32_t attr_num, nodes_num;
	
	for (uint32_t i = 0; i < cnt; ++i)
	{
		iter = node.insertSubnode();
		iter->name = replaceStringId(m_g.gu32());
		
		attr_num = m_g.gu32();
		loadAttributes(*iter, attr_num);
		
		nodes_num = m_g.gu32();
		loadSubnodes(*iter, nodes_num);
	}
}

} // namespace adm
} // namespace tlmodder
