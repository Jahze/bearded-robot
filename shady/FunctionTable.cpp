#include <algorithm>
#include <cassert>
#include "BuiltinTypes.h"
#include "FunctionTable.h"
#include "SymbolTable.h"

void Function::SetReturnType(const std::string & name)
{
	m_type = BuiltinType::Get(name);
	assert(m_type);
}

const std::string & Function::GetName() const
{
	return m_symbol->GetName();
}

Symbol * Function::GetLocal(const std::string & name) const
{
	auto iter = std::find_if(m_locals.begin(), m_locals.end(),
		[&](Symbol * s) { return s->GetName() == name; });

	if (iter == m_locals.end())
		return nullptr;

	return *iter;
}

Function * FunctionTable::AddFunction(Symbol * symbol)
{
	if (FindFunction(symbol->GetName()) != nullptr)
		return nullptr;

	auto iter = m_functions.insert(m_functions.end(), std::make_unique<Function>(symbol));

	return iter->get();
}

Function * FunctionTable::FindFunction(const std::string & name)
{
	auto iter = std::find_if(m_functions.begin(), m_functions.end(),
		[&](const std::unique_ptr<Function> & f) { return f->GetName() == name; });

	if (iter == m_functions.end())
		return nullptr;

	return iter->get();
}
