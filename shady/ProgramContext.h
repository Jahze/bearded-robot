#pragma once

#include <string>
#include <vector>
#include "BuiltinTypes.h"

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

class ProgramContext
{
public:
	static const ProgramContext & VertexShaderContext();

	void ApplyToSymbolTable(SymbolTable & symbolTable) const;

	const ContextVariable * GetVariable(const std::string & name) const;

private:
	ProgramContext(std::vector<ContextVariable> && variables)
		: m_variables(variables)
	{ }

private:
	std::vector<ContextVariable> m_variables;
};
