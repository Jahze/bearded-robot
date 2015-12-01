#include <algorithm>
#include <cassert>
#include "BuiltinTypes.h"
#include "ProgramContext.h"
#include "SymbolTable.h"

const ProgramContext & ProgramContext::VertexShaderContext()
{
	static ProgramContext context
	{
		{
			{ "g_position", "vec4", ContextVariable::Input },
			{ "g_normal", "vec3", ContextVariable::Input },
			{ "g_model", "mat4x4", ContextVariable::Input },
			{ "g_view", "mat4x4", ContextVariable::Input },
			{ "g_perspective", "mat4x4", ContextVariable::Input },

			{ "g_projected_position", "vec4", ContextVariable::Output },
			{ "g_world_position", "vec4", ContextVariable::Output },
			{ "g_world_normal", "vec4", ContextVariable::Output },
		}
	};

	return context;
}

void ProgramContext::ApplyToSymbolTable(SymbolTable & symbolTable) const
{
	for (auto && variable : m_variables)
	{
		BuiltinType *type = BuiltinType::Get(variable.m_builtinType);

		assert(type);

		Symbol *symbol = symbolTable.AddIntrinsicSymbol(variable.m_name, ScopeType::Global, SymbolType::Variable, type);

		assert(symbol);
	}
}

const ContextVariable * ProgramContext::GetVariable(const std::string & name) const
{
	auto iter = std::find_if(m_variables.begin(), m_variables.end(),
		[&](const ContextVariable & v){ return v.m_name == name; });

	if (iter == m_variables.end())
		return nullptr;

	return &*iter;
}
