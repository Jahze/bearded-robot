#include <algorithm>
#include <cassert>
#include "BuiltinTypes.h"
#include "FunctionTable.h"
#include "ProgramContext.h"
#include "SymbolTable.h"

namespace
{
	const std::array<ContextFunction, 6> functions =
	{{
		{ "normalize", BuiltinTypeType::Vec4, { BuiltinTypeType::Vec4 } },
		{ "length", BuiltinTypeType::Float, { BuiltinTypeType::Vec4 } },
		{ "max", BuiltinTypeType::Float, { BuiltinTypeType::Float, BuiltinTypeType::Float } },
		{ "dot3", BuiltinTypeType::Float, { BuiltinTypeType::Vec4, BuiltinTypeType::Vec4 } },
		{ "clamp", BuiltinTypeType::Float, { BuiltinTypeType::Float, BuiltinTypeType::Float, BuiltinTypeType::Float } },
		{ "nop", BuiltinTypeType::Void, { BuiltinTypeType::Int } },
		// TODO : clamp vector could have a special implementation using cmpps
	}};

}

const ProgramContext & ProgramContext::VertexShaderContext()
{
	static ProgramContext context
	{
		{
			{ "g_position", BuiltinTypeType::Vec4, ContextVariable::Input },
			//{ "g_normal", BuiltinTypeType::Vec3, ContextVariable::Input },
			{ "g_normal", BuiltinTypeType::Vec4, ContextVariable::Input },
			{ "g_model", BuiltinTypeType::Mat4x4, ContextVariable::Input },
			{ "g_view", BuiltinTypeType::Mat4x4, ContextVariable::Input },
			{ "g_projection", BuiltinTypeType::Mat4x4, ContextVariable::Input },
			//{ "g_normal_matrix", BuiltinTypeType::Mat3x3, ContextVariable::Input },
			{ "g_normal_matrix", BuiltinTypeType::Mat4x4, ContextVariable::Input },

			{ "g_projected_position", BuiltinTypeType::Vec4, ContextVariable::Output },
			{ "g_world_position", BuiltinTypeType::Vec4, ContextVariable::Output },
			{ "g_world_normal", BuiltinTypeType::Vec4, ContextVariable::Output },
		},
		functions
	};

	return context;
}

const ProgramContext & ProgramContext::FragmentShaderContext()
{
	static ProgramContext context
	{
		{
			{ "g_world_position", BuiltinTypeType::Vec4, ContextVariable::Input },
			{ "g_world_normal", BuiltinTypeType::Vec4, ContextVariable::Input },
			{ "g_light0_position", BuiltinTypeType::Vec4, ContextVariable::Input },

			{ "g_colour", BuiltinTypeType::Vec4, ContextVariable::Output },
		},
		functions
	};

	return context;
}

void ProgramContext::ApplyToSymbolTable(SymbolTable & symbolTable, FunctionTable & functionTable) const
{
	for (auto && variable : m_variables)
	{
		BuiltinType *type = BuiltinType::Get(variable.m_builtinType);

		assert(type);

		Symbol *symbol = symbolTable.AddIntrinsicSymbol(variable.m_name, ScopeType::Global, SymbolType::Variable, type);

		assert(symbol);
	}

	BuiltinType * type = BuiltinType::Get(BuiltinTypeType::Function);

	for (auto && function : m_functions)
	{
		Symbol * symbol = symbolTable.AddIntrinsicSymbol(function.m_name, ScopeType::Global, SymbolType::Function, type);

		assert(symbol);

		Function * f = functionTable.AddFunction(symbol);

		assert(f);

		f->SetReturnType(function.m_returnType);

		uint32_t i = 0;

		for (auto && argument : function.m_arguments)
		{
			symbol = symbolTable.AddSymbol("arg" + std::to_string(i++), ScopeType::Local, SymbolType::Variable,
				BuiltinType::Get(argument), f);

			f->AddParameter(symbol);
		}
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
