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
#include "filename_utils.h"
#include "unicode.h"

namespace tlmodder {
namespace adm {

uint32_t StringMap::add(std::string str)
{
	StringToId::iterator it = m_stringToId.lower_bound(str);
	
	if (it != m_stringToId.end() && str == it->first)
		return it->second;
	
	uint32_t id = m_nextId;
	IdToString::iterator it2;
	
	it2 = m_idToString.find(id);
	while (it2 != m_idToString.end() && id == it2->first)
	{
		++it2;
		++id;
	}
	m_nextId = id+1;
	
	it = m_stringToId.insert(it, std::make_pair(std::move(str), id));
	m_idToString.insert(it2, std::make_pair(id, &it->first));
	
	return id;
}

bool StringMap::find(std::string const& str, uint32_t& idOut)
{
	StringToId::const_iterator it;
	it = m_stringToId.find(str);
	
	if (it == m_stringToId.end())
		return false;
	
	idOut = it->second;
	return true;
}

std::string const& StringMap::get(uint32_t id) const
{
	static std::string empty;
	
	IdToString::const_iterator it = m_idToString.find(id);
	if (it == m_idToString.end())
		return empty;
	return *it->second;
}


void Adm::mergeNodes(
	Adm const& sourceAdm,
	Node const& sourceNode,
	Node& targetNode,
	AttributeReplaceMode attrReplaceMode)
{
	using SourcePair  = std::pair<Node const*, NodeConstIterator>;
	using SourceStack = std::stack<SourcePair>;
	using TargetStack = std::stack<Node*>;
	
	SourceStack sourceStack;
	TargetStack targetStack;
	bool replaceAttributes;
	
	targetStack.push(&targetNode);
	sourceStack.push(std::make_pair(&sourceNode, sourceNode.subnodes.begin()));
	
	//std::cerr << "Merging node " << sourceAdm.getString(sourceNode.name) << " into "
	//					<< getString(targetNode.name) << std::endl;
	
	while (!targetStack.empty())
	{
		SourcePair& sourcePair = sourceStack.top();
		Node const* source = sourcePair.first;
		Node* target = targetStack.top();
		
		if (sourcePair.second == source->subnodes.begin())
		{
			if (attrReplaceMode == AttributeReplaceMode::DontReplace ||
			    (attrReplaceMode == AttributeReplaceMode::ReplaceAtRoot && targetStack.size() != 1))
				replaceAttributes = false;
			else
				replaceAttributes = true;
			
			mergeNodeAttributes(sourceAdm, *source, *target, replaceAttributes);
			
			//std::cerr << "Merged attributes (" << target->attributes.size() << ")" << std::endl;
		}
		
		if (sourcePair.second != source->subnodes.end())
		{
			Node const& childNode = *sourcePair.second;
			NodeIterator it;
			
			it = target->insertSubnode();
			it->name = addString(sourceAdm.getString(childNode.name));
			
			//std::cerr << "Creating subnode " << sourceAdm.getString(childNode.name) << " and merging." << std::endl;
			
			targetStack.push(&*it);
			sourceStack.push(std::make_pair(&childNode, childNode.subnodes.begin()));
			++sourcePair.second;
		}
		else
		{
			//std::cerr << "Leaving node " << sourceAdm.getString(source->name) << std::endl;
			targetStack.pop();
			sourceStack.pop();
		}
	}
}

void Adm::mergeNodeAttributes(
	Adm const& sourceAdm,
	Node const& sourceNode,
	Node& targetNode,
	bool replaceExisting)
{
	uint32_t name;
	AttributeValue value;
	
	for (auto& sourceAttribute : sourceNode.attributes)
	{
		value = sourceAttribute.second;
		name = addString(sourceAdm.getString(sourceAttribute.first));
		
		if (value.type == AttributeValue::TYPE_STRING || value.type == AttributeValue::TYPE_TRANSLATE)
			value.valu32 = addString(sourceAdm.getString(value.valu32));
		
		if (replaceExisting)
			targetNode.setAttribute(name, value);
		else
			targetNode.insertAttribute(name, value);
	}
}

void Adm::dumpAttributes(
	std::ostream& strm,
	Node const& node) const
{
	for (auto& attribute : node.attributes)
	{
		std::string const& name = getString(attribute.first);
		
		switch (attribute.second.type)
		{
			case AttributeValue::TYPE_INT:
				strm << "<INTEGER>" << name << ":" << attribute.second.vali32;
				break;
			case AttributeValue::TYPE_FLOAT:
				strm << "<FLOAT>" << name << ":" << attribute.second.valf;
				break;
			case AttributeValue::TYPE_DOUBLE:
				strm << "<DOUBLE>" << name << ":" << attribute.second.vald;
				break;
			case AttributeValue::TYPE_UINT:
				strm << "<UNSIGNED INT>" << name << ":" << attribute.second.valu32;
				break;
			case AttributeValue::TYPE_STRING:
				strm << "<STRING>" << name << ":" << getString(attribute.second.valu32);
				break;
			case AttributeValue::TYPE_BOOL:
				strm << "<BOOL>" << name << ":" << (attribute.second.valu32 != 0 ? "true" : "false");
				break;
			case AttributeValue::TYPE_INT64:
				strm << "<INTEGER64>" << name << ":" << attribute.second.vali64;
				break;
			case AttributeValue::TYPE_TRANSLATE:
				strm << "<TRANSLATE>" << name << ":" << getString(attribute.second.valu32);
				break;
		}
		
		strm << std::endl;
	}
}

void Adm::dumpNode(
	std::ostream& strm,
	Node const& sourceNode) const
{
	using SourcePair = std::pair<Node const*, NodeConstIterator>;
	using SourceStack = std::stack<SourcePair>;
	
	SourceStack sourceStack;
	sourceStack.push(std::make_pair(&sourceNode, sourceNode.subnodes.begin()));
	
	while (!sourceStack.empty())
	{
		SourcePair& source = sourceStack.top();
		Node const& node = *source.first;
		
		if (source.second == node.subnodes.begin())
		{
			strm << "[" << getString(node.name) << "]" << std::endl;
			dumpAttributes(strm, node);
		}
		
		if (source.second != node.subnodes.end())
		{
			Node const& childNode = *source.second;
			sourceStack.push(std::make_pair(&childNode, childNode.subnodes.begin()));
			++source.second;
		}
		else
		{
			strm << "[/" << getString(node.name) << "]" << std::endl;
			sourceStack.pop();
		}
	}
}

void Adm::loadFromDat(std::string const& fn)
{
	DatFileLoader loader(fn);
	loader.load(*this);
}

void Adm::loadFromAdm(std::string const& fn)
{
	AdmFileLoader loader(fn);
	loader.load(*this);
}

void Adm::loadFromFile(std::string const& fn)
{
	string ext = utf8_to_upper(FileName::extension(fn));
	if (ext == "ADM")
		loadFromAdm(fn);
	else if (ext == "DAT" || ext == "LAYOUT" || ext == "ANIMATION" || ext == "HIE")
		loadFromDat(fn);
	else
		throw std::runtime_error("I don't know how to load Adm from this :(");
}

void Adm::loadFromDat(std::istream& strm)
{
	// TODO
	throw std::runtime_error("FIXME: not implemented yet");
}

void Adm::loadFromAdm(std::istream& strm)
{
	// TODO
	throw std::runtime_error("FIXME: not implemented yet");
}

} // namespace adm
} // namespace tlmodder
