#pragma once

#include <cassert>
#include <cstdint>
#include <string>

enum class BuiltinTypeType
{
	Void,
	Function,
	Bool,
	Int,
	Float,
	Vec3,
	Vec4,
	Mat3x3,
	Mat4x4,
};

class BuiltinType
{
public:
	static BuiltinType * Get(BuiltinTypeType type);

	static BuiltinTypeType FromName(const std::string & name);

	BuiltinType(const std::string & name, uint32_t size, BuiltinTypeType type)
		: m_name(name)
		, m_size(size)
		, m_type(type)
	{ }

	BuiltinType(const std::string & name, BuiltinTypeType elementType, uint32_t size, BuiltinTypeType type)
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
		assert(IsVector());
		return Get(m_elementType);
	}

	uint32_t GetSize() const
	{
		return m_size;
	}

	bool IsScalar() const
	{
		return m_type == BuiltinTypeType::Int || m_type == BuiltinTypeType::Float;
	}

	bool IsVector() const
	{
		return m_type == BuiltinTypeType::Vec3 || m_type == BuiltinTypeType::Vec4
			|| m_type == BuiltinTypeType::Mat3x3 || m_type == BuiltinTypeType::Mat4x4;
	}

private:
	std::string m_name;
	BuiltinTypeType m_elementType;
	uint32_t m_size;
	BuiltinTypeType m_type;
};
