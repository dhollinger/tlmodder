/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __ADM_H__
#define __ADM_H__

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <stack>
#include <memory>

namespace tlmodder {
namespace adm {

struct AttributeValue
{
	enum : uint32_t {
		TYPE_INVALID = 0,
		TYPE_INT     = 1,
		TYPE_FLOAT,
		TYPE_DOUBLE,
		TYPE_UINT,
		TYPE_STRING,
		TYPE_BOOL,
		TYPE_INT64,
		TYPE_TRANSLATE
	};
	
	uint32_t type;
	
	union {
		int32_t  vali32;
		uint32_t valu32;
		uint64_t valu64;
		int64_t  vali64;
		float    valf;
		double   vald;
	};
	
	AttributeValue():
		type(TYPE_INVALID)
	{}
	
	AttributeValue(uint32_t typeWithUintValue, uint32_t value)
	{
		type = typeWithUintValue;
		valu32 = value;
	}
	
	AttributeValue(bool value): type(TYPE_BOOL)
	{
		valu32 = (value ? 1 : 0);
	}
	
	AttributeValue(uint32_t uintValue): AttributeValue(TYPE_UINT, uintValue)
	{}
};

struct Node;

using AttributeMap           = std::multimap<uint32_t, AttributeValue>;
using Attribute              = AttributeMap::value_type;
using AttributeIterator      = AttributeMap::iterator;
using AttributeConstIterator = AttributeMap::const_iterator;

using NodeList          = std::list<Node>;
using NodeIterator      = NodeList::iterator;
using NodeConstIterator = NodeList::const_iterator;

struct Node
{
	uint32_t name;
	
	AttributeMap attributes;
	NodeList     subnodes;
	
	NodeIterator insertSubnode()
	{
		return subnodes.emplace(subnodes.end());
	}
	
	AttributeIterator insertAttribute(uint32_t name, AttributeValue const& val = AttributeValue())
	{
		return attributes.insert(std::make_pair(name, val));
	}
	
	AttributeIterator setAttribute(uint32_t name, AttributeValue const& val)
	{
		AttributeIterator it = attributes.lower_bound(name);
		
		if (it == attributes.end() || it->first != name)
		{
			it = attributes.insert(it, std::make_pair(name, val));
		}
		else
		{
			it->second = val;
			
			// Make sure there is only one such attribute
			AttributeIterator it2 = it;
			for (++it2; it2 != attributes.end() && it2->first == name;)
				it2 = attributes.erase(it2);
		}
		
		return it;
	}
};

class StringMap
{
public:
	using IdToString = std::map<uint32_t, std::string const*>;
	using StringToId = std::map<std::string, uint32_t>;
public:
	StringMap(): m_nextId(0x1000)
	{}
	
	uint32_t add(std::string str);
	std::string const& get(uint32_t id) const;
	
	IdToString const& idToString() const
	{ return m_idToString; }
	
	StringToId const& stringToId() const
	{ return m_stringToId; }
	
	bool find(std::string const& str, uint32_t& idOut);
protected:
	IdToString m_idToString;
	StringToId m_stringToId;
	uint32_t   m_nextId;
};

enum class AttributeReplaceMode
{
	Replace,
	ReplaceAtRoot,
	DontReplace
};

class Adm;
using AdmPtr = std::shared_ptr<Adm>;

class Adm
{
public:
	StringMap m_stringMap;
	Node      m_root;
	
	Adm()
	{}
	
	Adm(std::string rootName)
	{ m_root.name = addString(std::move(rootName)); }
	
	uint32_t addString(std::string str)
	{ return m_stringMap.add(std::move(str)); }
	
	std::string const& getString(uint32_t id) const
	{ return m_stringMap.get(id); }
	
	Node const& root() const
	{ return m_root; }
	
	Node& root()
	{ return m_root; }
	
	
	void loadFromFile(std::string const& fn);
	
	void loadFromDat(std::string const& fn);
	void loadFromAdm(std::string const& fn);
	
	void loadFromDat(std::istream& strm);
	void loadFromAdm(std::istream& strm);
	
	static AdmPtr createFromFile(std::string const& fn)
	{
		AdmPtr ptr = std::make_shared<Adm>();
		ptr->loadFromFile(fn);
		return ptr;
	}
	
	
	void dump(std::ostream& strm) const
	{ dumpNode(strm, m_root); }
	
	StringMap const& stringMap() const
	{ return m_stringMap; }
	
	StringMap& stringMap()
	{ return m_stringMap; }
	
	void mergeNodes(
		Adm const& sourceAdm,
		Node const& sourceNode,
		Node& targetNode,
		AttributeReplaceMode attrReplaceMode);
	
	void mergeNodeAttributes(
		Adm const& sourceAdm,
		Node const& sourceNode,
		Node& targetNode,
		bool replaceExisting);
	
	AttributeValue stringAttribute(std::string value)
	{
		return AttributeValue(AttributeValue::TYPE_STRING, addString(std::move(value)));
	}
	
	AttributeValue translateAttribute(std::string value)
	{
		return AttributeValue(AttributeValue::TYPE_TRANSLATE, addString(std::move(value)));
	}
protected:
	void dumpNode(std::ostream& strm, Node const& node) const;
	void dumpAttributes(std::ostream& strm, Node const& node) const;
};

class Loader
{
public:
	Loader() {}
	virtual ~Loader() {}
	
	virtual void load(Adm& adm) = 0;
};

} // namespace adm
} // namespace tlmodder

#endif
