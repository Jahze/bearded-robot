#pragma once

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

	// TODO : combine FunctionTable/SymbolTable
	void ApplyToSymbolTable(SymbolTable & symbolTable, FunctionTable & functionTable) const;

	const ContextVariable * GetVariable(const std::string & name) const;

private:
	ProgramContext(std::vector<ContextVariable> && variables, std::vector<ContextFunction> && functions)
		: m_variables(variables)
		, m_functions(functions)
	{ }

private:
	std::vector<ContextVariable> m_variables;
	std::vector<ContextFunction> m_functions;
};
