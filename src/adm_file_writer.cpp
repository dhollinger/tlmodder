/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "adm_file_writer.h"
#include "unicode.h"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <stack>

namespace tlmodder {
namespace adm {

using namespace ::std;

static void admFileWriteStringmap(std::ostream& strm, Adm const& adm);
static void admFileWriteTree(std::ostream& strm, Adm const& adm);

void admFileWrite(std::string const& filename, Adm const& adm)
{
	ofstream strm;
	strm.exceptions(ostream::failbit | ostream::badbit);
	strm.open(filename, ios::binary | ios::trunc);
	admFileWrite(strm, adm);
	strm.close();
}

void admFileWrite(std::ostream& strm, Adm const& adm)
{
	strm.exceptions(ostream::failbit | ostream::badbit);
	
	uint32_t version = 1;
	strm.write((char const*)&version, sizeof(version));
	
	admFileWriteStringmap(strm, adm);
	admFileWriteTree(strm, adm);
}

void admFileWriteStringmap(std::ostream& strm, Adm const& adm)
{
	uint32_t num;
	vector<char16_t> utf16buf;
	size_t len;
	char32_t chr;
	
	// Write number of strings, no overflow possible here since key is uint32_t
	num = (uint32_t)adm.stringMap().idToString().size();
	strm.write((char const*)&num, sizeof(num));
	
	// Now for each string its id and length and the string
	for (auto& entry : adm.stringMap().idToString())
	{
		utf8_iterator iter(*entry.second);
		len = 0;
		
		while (iter.next(chr))
		{
			if (len + 2 > utf16buf.size())
				utf16buf.resize(utf16buf.size()+64);
				
			len += utf32chr_to_utf16(chr, &utf16buf[len]);
		}
		
		// ID
		num = entry.first;
		strm.write((char const*)&num, sizeof(num));
		
		// Length
		num = (uint32_t)len;
		strm.write((char const*)&num, sizeof(num));
		
		// The string
		strm.write((char const*)utf16buf.data(), (size_t)len * 2);
	}
}

void admFileWriteAttributes(
	std::ostream& strm,
	Adm const& adm,
	Node const* node
	)
{
	uint32_t num;
	
	// Write number of attributes
	num = node->attributes.size();
	strm.write((char const*)&num, sizeof(num));
	
	for (auto& attribute : node->attributes)
	{
		// Name
		num = attribute.first;
		strm.write((char const*)&num, sizeof(num));
		
		// Type
		num = attribute.second.type;
		strm.write((char const*)&num, sizeof(num));
		
		// Value
		switch (attribute.second.type)
		{
			case AttributeValue::TYPE_INT:
			case AttributeValue::TYPE_UINT:
			case AttributeValue::TYPE_STRING:
			case AttributeValue::TYPE_BOOL:
			case AttributeValue::TYPE_TRANSLATE:
				strm.write((char const*)&attribute.second.valu32, sizeof(attribute.second.valu32));
				break;
			case AttributeValue::TYPE_INT64:
				strm.write((char const*)&attribute.second.vali64, sizeof(attribute.second.vali64));
				break;
			case AttributeValue::TYPE_FLOAT:
				strm.write((char const*)&attribute.second.valf, sizeof(attribute.second.valf));
				break;
			case AttributeValue::TYPE_DOUBLE:
				strm.write((char const*)&attribute.second.vald, sizeof(attribute.second.vald));
				break;
			default:
				throw std::runtime_error("Unknown attribute type");
		}
	}
}

void admFileWriteTree(std::ostream& strm, Adm const& adm)
{
	using NodePair  = std::pair<Node const*, NodeConstIterator>;
	using NodeStack = std::stack<NodePair>;
	
	NodeStack nodeStack;
	Node const *node;
	uint32_t num;
	
	nodeStack.push(std::make_pair(&adm.root(), adm.root().subnodes.begin()));
	
	while (!nodeStack.empty())
	{
		NodePair& topPair = nodeStack.top();
		node = topPair.first;
		
		if (topPair.second == node->subnodes.begin())
		{
			// Write node name
			num = (uint32_t)node->name;
			strm.write((char const*)&num, sizeof(num));
			
			// Write attributes
			admFileWriteAttributes(strm, adm, node);
			
			// Write number of nodes
			num = (uint32_t)node->subnodes.size();
			strm.write((char const*)&num, sizeof(num));
		}
		
		if (topPair.second != node->subnodes.end())
		{
			nodeStack.push(std::make_pair(&*topPair.second, topPair.second->subnodes.begin()));
			++topPair.second;
		}
		else
		{
			nodeStack.pop();
		}
	}
}

} // namespace adm
} // namespace tlmodder
