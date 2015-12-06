#pragma once

#include <array>
#include <string>
#include <vector>
#include "BuiltinTypes.h"

class FunctionTable;
class SymbolTable;

struct ContextVariable
{
	enum Type
	{
		Input,
		Output
	};

	std::string m_name;
	BuiltinTypeType m_builtinType;
	Type m_type;
};

struct ContextFunction
{
	std::string m_name;
	BuiltinTypeType m_returnType;
	std::vector<BuiltinTypeType> m_arguments;
};

class ProgramContext
{
public:
	static const ProgramContext & VertexShaderContext();
	static const ProgramContext & FragmentShaderContext();

	// TODO : combine FunctionTable/SymbolTable
	void ApplyToSymbolTable(SymbolTable & symbolTable, FunctionTable & functionTable) const;

	const ContextVariable * GetVariable(const std::string & name) const;

private:
	template<std::size_t N>
	ProgramContext(std::vector<ContextVariable> && variables, const std::array<ContextFunction, N> & functions)
		: m_variables(std::move(variables))
		, m_functions(functions.begin(), functions.end())
	{ }

private:
	std::vector<ContextVariable> m_variables;
	std::vector<ContextFunction> m_functions;
};
