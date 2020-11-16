/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "dat_file_adm_loader.h"

#include <stack>
#include <limits>
#include <cctype>
#include <sstream>

namespace tlmodder {
namespace adm {

enum class UnicodeEncoding {
	UTF_8,
	UTF_16LE,
	UTF_16BE,
	UTF_32LE,
	UTF_32BE,
	UNKNOWN
};

DatFileLoader::DatFileLoader(std::string const& file):
	m_file(file),
	m_flags(0)
{
	_detectEncoding();
}

DatFileLoader::DatFileLoader(DirIterator const& dir, std::string const& file):
	m_file(dir, file),
	m_flags(0)
{
	_detectEncoding();
}

void DatFileLoader::_detectEncoding()
{
	uint8_t const* data = m_file.ptr();
	size_t size = m_file.size();
	UnicodeEncoding encoding = UnicodeEncoding::UNKNOWN;
	
	// NOTE: support for UTF-32 can be added, but I am too lazy ^^
	
	// Detect encoding based on BOM, if present
	if (size > 2 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf)
	{
		data += 3;
		size -= 3;
		encoding = UnicodeEncoding::UTF_8;
	}
	else if (size > 1 && data[0] == 0xfe && data[1] == 0xff)
	{
		data += 2;
		size -= 2;
		encoding = UnicodeEncoding::UTF_16BE;
	}
	else if (size > 1 && data[0] == 0xff && data[1] == 0xfe)
	{
		data += 2;
		size -= 2;
		encoding = UnicodeEncoding::UTF_16LE;
	}
	
	if (encoding == UnicodeEncoding::UNKNOWN)
	{
		// No BOM, try to guess based on file content
		// Since valid DAT files should start with ASCII character, we
		// can guess by looking for zero byte at #0 (big endian) or #1 (little endian)
		// if we don't find it, assume UTF-8
		
		// NOTE: should one want to default to something else, here is the right place
		//       to do it
		
		if (size > 1 && data[0] == 0x00)
			encoding = UnicodeEncoding::UTF_16BE;
		else if (size > 1 && data[1] == 0x00)
			encoding = UnicodeEncoding::UTF_16LE;
		else
			encoding = UnicodeEncoding::UTF_8;
	}
	
	switch (encoding)
	{
		case UnicodeEncoding::UTF_8:
			m_lineReader = LineReaderPtr(
				new utf8_line_reader((char const*)data, size)
			);
			break;
		case UnicodeEncoding::UTF_16BE:
			m_lineReader = LineReaderPtr(
				new utf16_line_reader<utf16_be_t>((char16_t const*)data, size / 2)
			);
			break;
		case UnicodeEncoding::UTF_16LE:
			m_lineReader = LineReaderPtr(
				new utf16_line_reader<utf16_le_t>((char16_t const*)data, size / 2)
			);
			break;
		default:
			throw std::runtime_error("Ups, invalid encoding");
	}
}

// FIXME: this needs little cleanup ^^

void DatFileLoader::load(Adm& adm)
{
	std::stack<Node*> nodeStack;
	string line;
	size_t lineNum;
	string::size_type closingBracketPos;
	string::size_type lineStart;
	bool hasRoot = false;
	
	for (lineNum = 1; m_lineReader->read_line(line); ++lineNum)
	{
		lineStart = 0;
		
		// Skip white spaces
		for (lineStart = 0; lineStart < line.size(); ++lineStart)
		{
			if (!std::isspace(line[lineStart]))
				break;
		}
		
		// Skip empty lines
		if (line.size() == lineStart)
			continue;
		
		if (line[lineStart] == '[')
		{
			string sectionName;
			string::size_type bracketPos;
			bool sectionClose = false;
			
			++lineStart;
			
			// If '/' is at first position of section name, it is "close the section" mark
			if (line.size() > lineStart && line[lineStart] == '/')
			{
				sectionClose = true;
				++lineStart;
			}
			
			bracketPos = line.find(']', lineStart);
			
			if (bracketPos != string::npos)
			{
				sectionName = string(line, lineStart, bracketPos - lineStart);
			}
			else
			{
				// FIXME: treat missing ']' character as error?
				
				std::cerr << "WARNING at line " << lineNum << ": "
				          << "missing closing ']' bracket at the end of section name."
				          << std::endl;
				sectionName = string(line, lineStart);
			}
			
			// NOTE: for now ignoring anything following the ']' character and treating
			//       that like comment
			//       fix should come here
			
			if (sectionClose)
			{
				if (nodeStack.empty())
				{
					throw Exception(lineNum, string("section \"" + sectionName + "\" is being "
						"closed, but no section is open"));
				}
				
				string const& openSection = adm.getString(nodeStack.top()->name);
				
				if (sectionName != openSection)
				{
					if (!ignoreWrongNodeClosed())
						throw WrongNodeClosed(lineNum, openSection, sectionName);
					
					std::cerr << "WARNING at line " << lineNum << ": section \""
					          << sectionName << "\" is being closed but section \""
					          << openSection << "\" is open." << std::endl;
				}
				
				nodeStack.pop();
			}
			else
			{
				Node *nodeptr;
				
				// FIXME: for now, sections with empty name are allowed,
				//        maybe treat as error?
				
				if (nodeStack.empty())
				{
					if (hasRoot)
						throw MultipleRootSections(lineNum);
					
					hasRoot = true;
					nodeptr = &adm.root();
				}
				else
				{
					NodeIterator it = nodeStack.top()->insertSubnode();
					nodeptr = &*it;
				}
				
				nodeptr->name = adm.addString(sectionName);
				nodeStack.push(nodeptr);
			}
		}
		else if (line[lineStart] == '<')
		{
			string type_str, attr_name;
			string::size_type pos;
			AttributeValue value;
			
			if (nodeStack.empty())
				throw RootLevelAttribute(lineNum);
			
			++lineStart;
			
			// Find '>' and get value type string
			pos = line.find('>', lineStart);
			
			if (pos == string::npos)
				throw MalformedAttribute(lineNum, "missing '>' character after attribute type");
			
			type_str = string(line, lineStart, pos - lineStart);
			
			if (type_str == "INTEGER")
				value.type = AttributeValue::TYPE_INT;
			else if (type_str == "FLOAT")
				value.type = AttributeValue::TYPE_FLOAT;
			else if (type_str == "DOUBLE")
				value.type = AttributeValue::TYPE_DOUBLE;
			else if (type_str == "UNSIGNED INT")
				value.type = AttributeValue::TYPE_UINT;
			else if (type_str == "STRING")
				value.type = AttributeValue::TYPE_STRING;
			else if (type_str == "BOOL")
				value.type = AttributeValue::TYPE_BOOL;
			else if (type_str == "INTEGER64")
				value.type = AttributeValue::TYPE_INT64;
			else if (type_str == "TRANSLATE")
				value.type = AttributeValue::TYPE_TRANSLATE;
			
			if (value.type == AttributeValue::TYPE_INVALID)
				InvalidAttributeType(lineNum, type_str);
			
			lineStart = pos + 1;
			
			pos = line.find(':', lineStart);
			if (pos == string::npos)
				throw MalformedAttribute(lineNum, "missing ':' character after attribute value");
			
			attr_name = string(line, lineStart, pos - lineStart);
			
			static_assert(
				sizeof(long long) == sizeof(int64_t),
				"Don't know string to int64_t conversion for this platform"
			);
			static_assert(
				sizeof(int) == sizeof(int32_t),
				"Don't know string to int32_t conversion for this platform"
			);
			
			try
			{
				switch (value.type)
				{
					case AttributeValue::TYPE_INT:
						value.vali32 = std::stoi({line, pos+1});
						break;
					case AttributeValue::TYPE_FLOAT:
						value.valf = std::stof({line, pos+1});
						break;
					case AttributeValue::TYPE_DOUBLE:
						value.vald = std::stod({line, pos+1});
						break;
					case AttributeValue::TYPE_UINT:
						{
							// No string -> unsigned conversion in C++,
							// string -> unsigned long used instead and range-checked
							unsigned long valul = std::stoul({line, pos+1});
							if (valul > std::numeric_limits<unsigned>::max())
								throw std::out_of_range("stou");
							
							value.valu32 = (uint32_t)valul;
						}
						break;
					case AttributeValue::TYPE_BOOL:
						{
							string strval = utf8_to_upper({line, pos+1});
							
							if (strval.compare(0, 4, "TRUE") == 0)
								value.valu32 = 1;
							else if (strval.compare(0, 5, "FALSE") == 0)
								value.valu32 = 0;
							else
								value.valu32 = (std::stoul({line, pos+1}) == 0UL ? 0 : 1);
						}
						break;
					case AttributeValue::TYPE_INT64:
						try {
							value.vali64 = std::stoll({line, pos+1});
						}
						catch (std::out_of_range&)
						{
							// This may trigger undefined behavior on some platforms, but it should silently
							// overflow on x86-64 linux system with GCC compiler. Unfortunately there is no
							// TYPE_UINT64 for attributes and I found few mods that think it is unsigned value
							value.vali64 = std::stoull({line, pos+1});
						}
						break;
					case AttributeValue::TYPE_STRING:
					case AttributeValue::TYPE_TRANSLATE:
						value.valu32 = adm.addString({line, pos+1});
						break;
				}
			}
			catch (std::out_of_range&)
			{
				throw MalformedAttribute(lineNum, "attribute value is out of range");
			}
			catch (std::invalid_argument&)
			{
				throw MalformedAttribute(lineNum, "invalid attribute value");
			}
			
			nodeStack.top()->insertAttribute(adm.addString(attr_name), value);
		}
		else
		{
			// FIXME: for now silently ignore all lines starting with bogus data as it could be comment
			//        I saw "//" used for comments in few mods and also commenting by prefixing section
			//        name with things such as 'x' :   x[SECTION]
		}
	}
	
	// Check if all sections were closed
	if (!nodeStack.empty())
	{
		do
		{
			Node* node = nodeStack.top();
			nodeStack.pop();
			
			std::cerr << "ERROR at end of file: section \""
			          << adm.getString(node->name) << "\" not closed."
			          << std::endl;
			
		} while (nodeStack.size() > 1);
		
		throw std::runtime_error("Section not closed.");
	}
	
	// Check if at least one root section was present
	if (!hasRoot)
		throw MissingRootSection(lineNum);
}


void DatFileLoader::ignoreWrongNodeClosed(bool ignore)
{
	if (ignore)
		m_flags |= IgnoreWrongNodeClosed;
	else
		m_flags &= ~IgnoreWrongNodeClosed;
}


// ~~ DatFileLoader::Exception ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::string DatFileLoader::Exception::format() const
{
	std::ostringstream strm;
	
	strm << "ERROR at line " << m_lineNumber << ": " << m_msg << ".";
	return strm.str();
}

std::string DatFileLoader::WrongNodeClosed::format() const
{
	std::ostringstream strm;
	
	strm << "ERROR at line " << m_lineNumber << ": ";
	strm << "node \"" << m_openNode << "\" is open, but node \"" << m_closedNode
	     << "\" is being closed.";
	return strm.str();
}

std::string DatFileLoader::InvalidAttributeType::format() const
{
	std::ostringstream strm;
	
	strm << "ERROR at line " << m_lineNumber << ": ";
	strm << "invalid attribute type \"" << m_attrType << "\".";
	return strm.str();
}


} // namespace adm
} // namespace tlmodder
