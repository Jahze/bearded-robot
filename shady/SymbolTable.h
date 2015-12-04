#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "br.h"

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

// TODO : rename - doesn't just hold location for symbols also temps

struct SymbolLocation
{
	enum Type
	{
		None,
		GlobalMemory,
		LocalMemory,

		// TODO: check these two don't need to be considered specially in places
		IndirectRegister,
		RegisterPart,
		//

		Register,
		XmmRegister,
	};

	Type m_type = None;
	uint32_t m_data = 0u;
	uint32_t m_shift = 0u;

	bool InMemory() const
	{
		return m_type == GlobalMemory || m_type == LocalMemory || m_type == IndirectRegister;
	}

	bool operator==(const SymbolLocation & rhs) const
	{
		assert(m_type != RegisterPart && rhs.m_type != RegisterPart);
		return m_type == rhs.m_type && m_data == rhs.m_data;
	}

	bool operator!=(const SymbolLocation & rhs) const
	{
		return ! (*this == rhs);
	}

	void MakeIndirect()
	{
		assert(br::none_of(m_type, None, RegisterPart, IndirectRegister));

		// TODO : if the pointer is in memory then spill a register and put it there

		assert(! InMemory());

		m_type = IndirectRegister;
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
