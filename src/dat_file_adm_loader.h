/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#ifndef __DAT_FILE_ADM_LOADER_H__
#define __DAT_FILE_ADM_LOADER_H__

#include "adm.h"
#include "mapped_file.h"
#include "unicode_line_reader.h"

#include <memory>

namespace tlmodder {
namespace adm {

class DatFileLoader : public Loader
{
public:
	DatFileLoader(std::string const& file);
	DatFileLoader(DirIterator const& dir, std::string const& file);
	
	virtual ~DatFileLoader() {}
	
	void load(Adm& adm) override;
	
	// NOTE: There are few mods out there that have wrong closing section name.
	//       Torchlight's seems to ignore closing section names, but
	//       there are also few mods that OPEN wrong section (such as Demonologist for skill level)
	//       so default is false for this to detect such errors
	
	void ignoreWrongNodeClosed(bool ignore);
	
	bool ignoreWrongNodeClosed() const
	{ return _hasFlag(IgnoreWrongNodeClosed);  }
public:
	class Exception : public std::exception
	{
	protected:
		int m_lineNumber;
		std::string m_msg;
	public:
		Exception(int lineNumber) : m_lineNumber(lineNumber) {}
		Exception(int lineNumber, char const *msg): m_lineNumber(lineNumber), m_msg(msg) {}
		Exception(int lineNumber, string msg): m_lineNumber(lineNumber), m_msg(std::move(msg)) {}
		
		char const* what() const noexcept override
		{ return m_msg.c_str(); }
		
		int lineNumber() const { return m_lineNumber; }
		std::string const& message() const { return m_msg; }
		
		virtual std::string format() const;
	};

	class WrongNodeClosed : public Exception
	{
	protected:
		std::string m_openNode;
		std::string m_closedNode;
	public:
		WrongNodeClosed(int lineNumber, std::string openNode, std::string closedNode):
			Exception(lineNumber, "wrong node closed"),
			m_openNode(std::move(openNode)),
			m_closedNode(std::move(closedNode))
		{}
		
		std::string const& openNode() const { return m_openNode; }
		std::string const& closedNode() const { return m_closedNode; }
		
		std::string format() const override;
	};

	class MultipleRootSections : public Exception
	{
	public:
		MultipleRootSections(int lineNumber):
			Exception(lineNumber, "second root section found")
		{}
	};

	class RootLevelAttribute : public Exception
	{
	public:
		RootLevelAttribute(int lineNumber):
			Exception(lineNumber, "root-level attribute found")
		{}
	};

	class MissingRootSection : public Exception
	{
	public:
		MissingRootSection(int lineNumber):
			Exception(lineNumber, "no root section found")
		{}
	};

	class MalformedAttribute : public Exception
	{
	public:
		MalformedAttribute(int lineNumber, std::string msg):
			Exception(lineNumber, std::move(msg))
		{}
	};

	class InvalidAttributeType : public MalformedAttribute
	{
		std::string m_attrType;
	public:
		InvalidAttributeType(int lineNumber, std::string attributeTypeString):
			MalformedAttribute(lineNumber, "invalid attribute type"),
			m_attrType(std::move(attributeTypeString))
		{}
		
		std::string const& attributeType() const
		{ return m_attrType; }
		
		std::string format() const override;
	};
protected:
	void _detectEncoding();
	bool _hasFlag(uint32_t flag) const { return (m_flags & flag) == flag; }
protected:
	enum : uint32_t {
		IgnoreWrongNodeClosed = 1u << 0,
	};
	using LineReaderPtr = std::shared_ptr<unicode_line_reader>;
	
	MappedFile m_file;
	uint32_t m_flags;
	LineReaderPtr m_lineReader;
};

}
}

#endif
