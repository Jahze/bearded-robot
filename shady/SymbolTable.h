#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class BuiltinType;
class Function;

enum class ScopeType
{
	Global,
	Local,
};

enum class SymbolType
{
	Function,
	Variable,
};

struct SymbolLocation
{
	enum Type
	{
		None,
		GlobalMemory,
		LocalMemory,
		Register,
		XmmRegister,
	};

	Type m_type = None;
	uint32_t m_data = 0u;

	bool InMemory() const
	{
		return m_type == GlobalMemory || m_type == LocalMemory;
	}
};

class Symbol
{
public:
	Symbol(const std::string & name, ScopeType scopeType, SymbolType symbolType, BuiltinType * type, Function * scope)
		: m_name(name)
		, m_scopeType(scopeType)
		, m_symbolType(symbolType)
		, m_type(type)
		, m_scope(scope)
	{ }

	void SetDeclarationLocation(uint32_t line, uint32_t column)
	{
		m_line = line;
		m_column = column;
	}

	void SetIntrinsic(bool intrinsic)
	{
		m_intrinsic = intrinsic;
	}

	const std::string & GetName() const
	{
		return m_name;
	}

	uint32_t GetDeclarationLine() const
	{
		return m_line;
	}

	uint32_t GetDeclarationColumn() const
	{
		return m_column;
	}

	BuiltinType * GetType() const
	{
		return m_type;
	}

	SymbolType GetSymbolType() const
	{
		return m_symbolType;
	}

	bool IsIntrinsic() const
	{
		return m_intrinsic;
	}

	Function * GetScope() const
	{
		return m_scope;
	}

	SymbolLocation & GetLocation()
	{
		return m_location;
	}

private:
	std::string m_name;
	ScopeType m_scopeType;
	SymbolType m_symbolType;
	uint32_t m_line = 0u;
	uint32_t m_column = 0u;
	BuiltinType *m_type;
	bool m_intrinsic = false;
	Function * m_scope = nullptr;
	SymbolLocation m_location;
};

class SymbolTable
{
public:
	Symbol * AddSymbol(
		const std::string & name,
		ScopeType scopeType,
		SymbolType symbolType,
		BuiltinType * type,
		Function * scope);

	Symbol * AddIntrinsicSymbol(
		const std::string & name,
		ScopeType scopeType,
		SymbolType symbolType,
		BuiltinType * type);

	Symbol * FindSymbol(const std::string & name, Function * scope) const;

	std::vector<Symbol*> GetGlobalSymbols() const;

private:
	std::unordered_map<Function*, std::vector<std::unique_ptr<Symbol>>> m_symbols;
};
