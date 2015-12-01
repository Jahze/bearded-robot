#include <algorithm>
#include <memory>
#include "BuiltinTypes.h"

BuiltinType * BuiltinType::Get(const std::string & name)
{
	static BuiltinType types[] =
	{
		{ "function", 0, BuiltinTypeType::Function },
		{ "float", 4, BuiltinTypeType::Scalar },
		{ "int", 4, BuiltinTypeType::Scalar },
		{ "bool", 4, BuiltinTypeType::Boolean },
		{ "vec3", "float", 12, BuiltinTypeType::Vector },
		{ "vec4", "float", 16, BuiltinTypeType::Vector },
		{ "mat3x3", "vec3", 36, BuiltinTypeType::Vector },
		{ "mat4x4", "vec4", 64, BuiltinTypeType::Vector },
	};

	auto iter = std::find_if(std::begin(types), std::end(types),
		[&](const BuiltinType & type)
		{ return type.GetName() == name; });

	if (iter == std::end(types))
		return nullptr;

	return iter;
}
