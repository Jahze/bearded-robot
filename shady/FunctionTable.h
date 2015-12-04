#pragma once

#include <memory>
#include <string>
#include <vector>
#include "BuiltinTypes.h"

class Symbol;

class Function
{
public:
	Function(Symbol *symbol)
		: m_symbol(symbol)
	{ }

	void AddParameter(Symbol * symbol)
	{
		m_parameters.push_back(symbol);
	}

	void AddLocal(Symbol * symbol)
	{
		m_locals.push_back(symbol);
	}

	void SetReturnType(BuiltinTypeType type);

	void SetExport(bool value)
	{
		m_isExport = value;
	}

	const std::string & GetName() const;

	BuiltinType * GetReturnType() const
	{
		return m_type;
	}

	const std::vector<Symbol*> & GetParameters() const
	{
		return m_parameters;
	}

	const std::vector<Symbol*> & GetLocals() const
	{
		return m_locals;
	}

	Symbol * GetLocal(const std::string & name) const;

	bool IsExport() const
	{
		return m_isExport;
	}

private:
	Symbol * m_symbol;
	std::vector<Symbol*> m_parameters;
	std::vector<Symbol*> m_locals;
	BuiltinType * m_type;
	bool m_isExport = false;
};

class FunctionTable
{
public:
	Function * AddFunction(Symbol * symbol);
	Function * FindFunction(const std::string & name);

private:
	std::vector<std::unique_ptr<Function>> m_functions;
};
