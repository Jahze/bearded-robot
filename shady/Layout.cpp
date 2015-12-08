#include "BuiltinTypes.h"
#include "FunctionTable.h"
#include "Layout.h"
#include "SymbolTable.h"

Layout::StackLayout::StackLayout(Layout * layout)
	: m_layout(layout)
	, m_marker(layout->m_localMemory.Mark())
{ }

Layout::StackLayout::~StackLayout()
{
	for (auto && location : m_locations)
		m_layout->RelinquishLocation(location);

	m_layout->m_localMemory.Reset(m_marker);
}

SymbolLocation Layout::StackLayout::PlaceTemporary(BuiltinType * type)
{
	SymbolLocation out;

	bool result = m_layout->StandardPlacement(type, out, ScopeType::Local);

	assert(result);

	m_locations.push_back(out);

	return out;
}

SymbolLocation Layout::StackLayout::PlaceTemporaryInMemory(BuiltinType * type)
{
	SymbolLocation out;

	m_layout->PlaceInMemory(type, out, ScopeType::Local);

	m_locations.push_back(out);

	return out;
}

SymbolLocation Layout::StackLayout::PlaceRegisterPart(XmmRegister reg, uint32_t shift)
{
	SymbolLocation out;

	// check the memory has been allocated
	assert(shift < m_layout->m_localMemory.Mark());

	out.m_type = SymbolLocation::RegisterPart;
	out.m_data = reg;
	out.m_shift = shift;

	// reg is a temporary so it's our job to give it back
	if (! m_layout->m_xmmRegisters[static_cast<int>(reg)].second)
	{
		m_layout->m_xmmRegisters[static_cast<int>(reg)].second = true;

		SymbolLocation temp;
		temp.m_type = SymbolLocation::XmmRegister;
		temp.m_data = reg;
		m_locations.push_back(temp);
	}

	return out;
}

uint32_t Layout::GlobalMemoryUsed()
{
	return m_globalMemory.Mark();
}

void Layout::PlaceGlobal(Symbol * symbol)
{
	BuiltinType * type = symbol->GetType();

	//if (! StandardPlacement(type, symbol->GetLocation(), ScopeType::Global))
	//	throw LayoutException("Cannot layout variable '" + symbol->GetName() + "' of type '" + type->GetName() + "'");

	// XXX : always putting them in memory is faster -- because of register contention or something else?
	PlaceGlobalInMemory(symbol);
}

void Layout::PlaceGlobalInMemory(Symbol * symbol)
{
	PlaceInMemory(symbol->GetType(), symbol->GetLocation(), ScopeType::Global);
}

void Layout::PlaceLocalInMemory(Symbol * symbol)
{
	PlaceInMemory(symbol->GetType(), symbol->GetLocation(), ScopeType::Local);
}

SymbolLocation Layout::PlaceGlobalFloatInMemory()
{
	SymbolLocation out;

	PlaceInMemory(BuiltinType::Get(BuiltinTypeType::Float), out, ScopeType::Global);

	return out;
}

SymbolLocation Layout::PlaceGlobalVectorInMemory()
{
	SymbolLocation out;

	PlaceInMemory(BuiltinType::Get(BuiltinTypeType::Vec4), out, ScopeType::Global);

	return out;
}

void Layout::PlaceParametersAndLocals(Function * function)
{
	for (auto && parameter : function->GetParameters())
	{
		BuiltinType * type = parameter->GetType();

		if (! StandardPlacement(type, parameter->GetLocation(), ScopeType::Local))
			throw LayoutException("Cannot layout variable '" + parameter->GetName() +
				"' of type '" + type->GetName() + "'");
	}

	for (auto && local : function->GetLocals())
	{
		//BuiltinType * type = local->GetType();
		//
		//if (! StandardPlacement(type, local->GetLocation(), ScopeType::Local))
		//	throw LayoutException("Cannot layout variable '" + local->GetName() +
		//		"' of type '" + type->GetName() + "'");

		// TODO : placing locals in registers isn't a good idea when there are a lot of locals
		// need a way to flag when locals are in registers and keep them there if possible

		PlaceLocalInMemory(local);
	}
}

void Layout::RelinquishParametersAndLocals(Function * function)
{
	m_localMemory.Reset();

	for (auto && parameter : function->GetParameters())
	{
		RelinquishLocation(parameter->GetLocation());
	}

	for (auto && local : function->GetLocals())
	{
		RelinquishLocation(local->GetLocation());
	}
}

Layout::StackLayout Layout::TemporaryLayout()
{
	return StackLayout(this);
}

std::unique_ptr<Layout::TemporaryRegister> Layout::GetFreeRegister()
{
	Register reg;
	bool result = NextRegisterSlot(reg);

	SymbolLocation spiltLocation;

	if (! result)
	{
		reg = Eax;
		spiltLocation.m_type = SymbolLocation::LocalMemory;
		spiltLocation.m_data = m_localMemory.Allocate(4, 4);
	}

	SymbolLocation location;
	location.m_type = SymbolLocation::Register;
	location.m_data = reg;

	return std::make_unique<TemporaryRegister>(this, location, spiltLocation);
}

std::unique_ptr<Layout::TemporaryRegister> Layout::GetFreeXmmRegister()
{
	XmmRegister reg;
	bool result = NextXmmRegisterSlot(reg);

	SymbolLocation spiltLocation;

	if (! result)
	{
		reg = Xmm0;
		spiltLocation.m_type = SymbolLocation::LocalMemory;
		spiltLocation.m_data = m_localMemory.Allocate(4, 4);
	}

	SymbolLocation location;
	location.m_type = SymbolLocation::XmmRegister;
	location.m_data = reg;

	return std::make_unique<TemporaryRegister>(this, location, spiltLocation);
}

bool Layout::StandardPlacement(BuiltinType * type, SymbolLocation & location, ScopeType scope)
{
	if (type->IsScalar())
	{
		if (type->GetType() == BuiltinTypeType::Float)
		{
			PlaceInXmmRegisterOrMemory(type, location, scope);
			return true;
		}
		else
		{
			PlaceInRegisterOrMemory(type, location, scope);
			return true;
		}
	}
	else if (type->IsVector())
	{
		if (type->GetElementType()->IsVector())
		{
			PlaceInMemory(type, location, scope);
			return true;
		}
		else
		{
			PlaceInXmmRegisterOrMemory(type, location, scope);
			return true;
		}
	}
	else if (type->GetType() == BuiltinTypeType::Bool)
	{
		PlaceInRegisterOrMemory(type, location, scope);
		return true;
	}

	return false;
}

void Layout::PlaceInRegisterOrMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope)
{
	Register reg;

	if (NextRegisterSlot(reg))
	{
		location.m_type = SymbolLocation::Register;
		location.m_data = reg;
	}
	else
	{
		PlaceInMemory(type, location, scope);
	}
}

void Layout::PlaceInXmmRegisterOrMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope)
{
	XmmRegister xmm;

	if (NextXmmRegisterSlot(xmm))
	{
		location.m_type = SymbolLocation::XmmRegister;
		location.m_data = xmm;
	}
	else
	{
		PlaceInMemory(type, location, scope);
	}
}

void Layout::PlaceInMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope)
{
	uint32_t size = type->GetSize();

	if (scope == ScopeType::Local)
	{
		location.m_type = SymbolLocation::LocalMemory;
		location.m_data = m_localMemory.Allocate(size, size);
	}
	else
	{
		location.m_type = SymbolLocation::GlobalMemory;
		location.m_data = m_globalMemory.Allocate(size, size);
	}
}

bool Layout::NextRegisterSlot(Register & reg)
{
	for (auto && slot : m_registers)
	{
		if (! slot.second)
		{
			slot.second = true;
			reg = slot.first;
			return true;
		}
	}

	return false;
}

bool Layout::NextXmmRegisterSlot(XmmRegister & reg)
{
	for (auto && slot : m_xmmRegisters)
	{
		if (! slot.second)
		{
			slot.second = true;
			reg = slot.first;
			return true;
		}
	}

	return false;
}

void Layout::RelinquishLocation(const SymbolLocation & location)
{
	if (location.m_type == SymbolLocation::Register)
		m_registers[location.m_data].second = false;

	if (location.m_type == SymbolLocation::XmmRegister)
		m_xmmRegisters[location.m_data].second = false;
}
