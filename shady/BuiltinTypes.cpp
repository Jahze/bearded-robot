#include <algorithm>
#include <memory>
#include "BuiltinTypes.h"

BuiltinType * BuiltinType::Get(BuiltinTypeType type)
{
	static BuiltinType types[] =
	{
		{ "void", 0, BuiltinTypeType::Void },
		{ "function", 0, BuiltinTypeType::Function },
		{ "float", 4, BuiltinTypeType::Float },
		{ "int", 4, BuiltinTypeType::Int },
		{ "bool", 4, BuiltinTypeType::Bool },
		//{ "vec3", BuiltinTypeType::Float, 12, BuiltinTypeType::Vec3 },
		{ "vec4", BuiltinTypeType::Float, 16, BuiltinTypeType::Vec4 },
		//{ "mat3x3", BuiltinTypeType::Vec3, 36, BuiltinTypeType::Mat3x3 },
		{ "mat4x4", BuiltinTypeType::Vec4, 64, BuiltinTypeType::Mat4x4 },
	};

	auto iter = std::find_if(std::begin(types), std::end(types),
		[&](const BuiltinType & b)
		{ return b.GetType() == type; });

	if (iter == std::end(types))
		return nullptr;

	return iter;
}

BuiltinTypeType BuiltinType::FromName(const std::string & name)
{
	if (name == "void")
		return BuiltinTypeType::Void;

	if (name == "float")
		return BuiltinTypeType::Float;

	if (name == "int")
		return BuiltinTypeType::Int;

	if (name == "bool")
		return BuiltinTypeType::Bool;

	if (name == "vec3")
		return BuiltinTypeType::Vec3;

	if (name == "vec4")
		return BuiltinTypeType::Vec4;

	if (name == "mat3x3")
		return BuiltinTypeType::Mat3x3;

	if (name == "mat4x4")
		return BuiltinTypeType::Mat4x4;

	throw std::runtime_error("unknown type '" + name + "'");
}
