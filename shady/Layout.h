#pragma once

#include <array>
#include <string>
#include "SymbolTable.h"

// strip all the symbols out of SyntaxTree

// includes
//   globals
//   locals
//   func params

// lay them out

// globals
//   input vec3 and vec4 in registers spilling to memory
//   input scalars in registers (not bool probably)
//   input mat3x3 and mat4x4 in memory

//   output vec3 and vec4 in registers spilling to memory
//   output mat3x3 and mat4x4 in registers
//   output scalars in registers (not bool probably)

// locals
//   func params
//   locals
//   no rbp/rsp split for above just one register at top of stack and offsets

class Function;

enum Register
{
	Eax,
	Ecx,
	Edx,
	Ebx,
	Esp,
	Ebp,
	Esi,
	Edi,
};

enum XmmRegister
{
	Xmm0,
	Xmm1,
	Xmm2,
	Xmm3,
	Xmm4,
	Xmm5,
	Xmm6,
	Xmm7,
};

struct Memory
{
public:
	void Reset()
	{
		m_offset = 0u;
	}

	typedef uint32_t Marker;

	void Reset(Marker mark)
	{
		m_offset = mark;
	}

	Marker Mark()
	{
		return m_offset;
	}

	uint32_t Allocate(uint32_t size, uint32_t align)
	{
		uint32_t r = m_offset % align;

		if (r != 0)
		{
			m_offset += (align - r);
		}

		uint32_t out = m_offset;

		m_offset += size;

		return out;
	}

private:
	uint32_t m_offset = 0u;
};

struct LayoutException
{
	std::string m_message;

	LayoutException(const std::string & message)
		: m_message(message)
	{ }
};

class Layout
{
public:
	uint32_t GlobalMemoryUsed();

	void PlaceGlobal(Symbol * symbol);
	void PlaceGlobalInMemory(Symbol * symbol);
	SymbolLocation PlaceGlobalFloatInMemory();
	SymbolLocation PlaceGlobalVectorInMemory();

	void PlaceParametersAndLocals(Function * function);
	void RelinquishParametersAndLocals(Function * function);

	friend class StackLayout;
	friend class TemporaryRegister;

	class StackLayout
	{
	public:
		StackLayout(Layout * layout);
		~StackLayout();

		SymbolLocation PlaceTemporary(BuiltinType * type);
		SymbolLocation PlaceTemporaryInMemory(BuiltinType * type);
		SymbolLocation PlaceRegisterPart(XmmRegister reg, uint32_t shift);

	private:
		Layout * m_layout;
		Memory::Marker m_marker;
		std::vector<SymbolLocation> m_locations;
	};

	class TemporaryRegister
	{
	public:
		TemporaryRegister(Layout * layout, SymbolLocation location, SymbolLocation spiltRegister)
			: m_layout(layout)
			, m_location(location)
			,m_spiltRegister(spiltRegister)
		{ }

		~TemporaryRegister()
		{
			if (m_spiltRegister.m_type == SymbolLocation::None)
				m_layout->RelinquishLocation(m_location);
		}

		uint32_t Register() const
		{
			return m_location.m_data;
		}

		SymbolLocation Location() const
		{
			return m_location;
		}

		SymbolLocation SpiltLocation() const
		{
			return m_spiltRegister;
		}

	private:
		Layout * m_layout;
		SymbolLocation m_location;
		SymbolLocation m_spiltRegister;
	};

	StackLayout TemporaryLayout();

	std::unique_ptr<TemporaryRegister> GetFreeRegister();
	std::unique_ptr<TemporaryRegister> GetFreeXmmRegister();

private:
	bool StandardPlacement(BuiltinType * type, SymbolLocation & location, ScopeType scope);
	void PlaceInRegisterOrMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope);
	void PlaceInXmmRegisterOrMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope);
	void PlaceInMemory(BuiltinType * type, SymbolLocation & location, ScopeType scope);

	bool NextRegisterSlot(Register & reg);
	bool NextXmmRegisterSlot(XmmRegister & reg);

	void RelinquishLocation(const SymbolLocation & location);

private:
	std::array<std::pair<Register, bool>, 8> m_registers =
	{{
		{ Eax, false },
		{ Ecx, false },
		{ Edx, false },
		{ Ebx, false },
		{ Esp, true  }, // Leave rsp and rbp alone
		{ Ebp, true  },
		{ Esi, true  }, // esi will be stack pointer
		{ Edi, false },
	}};

	std::array<std::pair<XmmRegister, bool>, 8> m_xmmRegisters =
	{{
		{ Xmm0, false },
		{ Xmm1, false },
		{ Xmm2, false },
		{ Xmm3, false },
		{ Xmm4, false },
		{ Xmm5, false },
		{ Xmm6, false },
		{ Xmm7, false },
	}};

	Memory m_globalMemory;
	Memory m_localMemory;
};


// StackLayout l = Layout.StartStack(function)
// generate offsets for all params & locals
// dtor resets -> or class just holds does it internally