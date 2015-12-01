#pragma once

#include <cassert>
#include <cstdint>
#include <string>

enum class BuiltinTypeType
{
	Function,
	Boolean,
	Scalar,
	Vector,
};

class BuiltinType
{
public:
	static BuiltinType * Get(const std::string & name);

	BuiltinType(const std::string & name, uint32_t size, BuiltinTypeType type)
		: m_name(name)
		, m_size(size)
		, m_type(type)
	{ }

	BuiltinType(const std::string & name, const std::string & elementType, uint32_t size, BuiltinTypeType type)
		: m_name(name)
		, m_elementType(elementType)
		, m_size(size)
		, m_type(type)
	{ }

	const std::string & GetName() const
	{
		return m_name;
	}

	BuiltinTypeType GetType() const
	{
		return m_type;
	}

	BuiltinType *GetElementType() const
	{
		assert(m_type == BuiltinTypeType::Vector);
		return Get(m_elementType);
	}

	uint32_t GetSize() const
	{
		return m_size;
	}

private:
	std::string m_name;
	std::string m_elementType;
	uint32_t m_size;
	BuiltinTypeType m_type;
};
