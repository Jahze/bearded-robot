#include <algorithm>
#include <cassert>
#include <iterator>
#include "SymbolTable.h"

Symbol * SymbolTable::AddSymbol(const std::string & name, ScopeType scopeType, SymbolType symbolType,
	BuiltinType * type, Function * scope)
{
	if (FindSymbol(name, scope) != nullptr)
		return nullptr;

	auto && vec = m_symbols[scope];

	auto iter = vec.insert(vec.end(), std::make_unique<Symbol>(name, scopeType, symbolType, type, scope));

	return iter->get();
}

Symbol * SymbolTable::AddIntrinsicSymbol(const std::string & name, ScopeType scopeType, SymbolType symbolType,
	BuiltinType *type)
{
	Symbol *symbol = AddSymbol(name, scopeType, symbolType, type, nullptr);

	if (! symbol)
		return nullptr;

	symbol->SetIntrinsic(true);
	return symbol;
}

Symbol * SymbolTable::FindSymbol(const std::string & name, Function * scope) const
{
	auto iter = m_symbols.find(scope);

	if (iter == m_symbols.end())
	{
		// Search global scope
		if (scope != nullptr)
			return FindSymbol(name, nullptr);

		return nullptr;
	}

	auto jter = std::find_if(iter->second.begin(), iter->second.end(),
		[&](const std::unique_ptr<Symbol> & s)
		{ return s->GetName() == name && s->GetScope() == scope; });

	if (jter == iter->second.end())
	{
		// Search global scope
		if (scope != nullptr)
			return FindSymbol(name, nullptr);

		return nullptr;
	}

	return jter->get();
}

std::vector<Symbol*> SymbolTable::GetGlobalSymbols() const
{
	std::vector<Symbol*> out;

	auto symbols = m_symbols.find(nullptr);

	if (symbols == m_symbols.end())
		return out;

	std::transform(symbols->second.begin(), symbols->second.end(), std::back_inserter(out),
		[](const std::unique_ptr<Symbol> & s) { return s.get(); });

	return out;
}
