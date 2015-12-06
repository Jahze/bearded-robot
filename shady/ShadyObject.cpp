#include <Windows.h>
#include "ShadyObject.h"

ScopedAlloc::ScopedAlloc(uint32_t size)
{
	m_pointer = ::VirtualAlloc(nullptr, static_cast<SIZE_T>(size), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

ScopedAlloc::~ScopedAlloc()
{
	::VirtualFree(m_pointer, 0, MEM_RELEASE);
}

ShadyObject::ShadyObject(uint32_t size)
	: m_object(size)
{
}

void ShadyObject::Execute()
{
	assert(m_entryPoint);

	void *fp = m_entryPoint;
	uint32_t esi_store;

	__asm
	{
		mov [esi_store], esi
		call [fp]
		mov esi, [esi_store]
	}
}

void ShadyObject::ReserveGlobalSize(uint32_t size)
{
	m_cursor = size;
}

void ShadyObject::WriteConstants(
	const std::unordered_map<float, SymbolLocation> & floatConstants,
	const std::map<std::tuple<float, float, float, float>, SymbolLocation> & vectorConstants)
{
	static_assert(sizeof(float) == 4, "sizeof(float) == 4");

	char * pointer = reinterpret_cast<char*>((void*)m_object);

	for (auto && constant : floatConstants)
	{
		assert(constant.second.m_type == SymbolLocation::GlobalMemory);

		std::memcpy(pointer + constant.second.m_data, &constant.first, 4);
	}

	for (auto && constant : vectorConstants)
	{
		assert(constant.second.m_type == SymbolLocation::GlobalMemory);

		std::memcpy(pointer + constant.second.m_data +  0, &std::get<0>(constant.first), 4);
		std::memcpy(pointer + constant.second.m_data +  4, &std::get<1>(constant.first), 4);
		std::memcpy(pointer + constant.second.m_data +  8, &std::get<2>(constant.first), 4);
		std::memcpy(pointer + constant.second.m_data + 12, &std::get<3>(constant.first), 4);
	}
}

void ShadyObject::NoteGlobals(const SymbolTable & symbolTable)
{
	assert(m_globalTrampoline == nullptr);

	m_globalTrampoline = ObjectCursor();

	std::vector<Symbol*> globals = symbolTable.GetGlobalSymbols();

	for (auto && global : globals)
	{
		SymbolLocation location = global->GetLocation();

		if (location.m_type == SymbolLocation::GlobalMemory)
		{
			m_globals[global->GetName()] = { Memory, location.m_data };
		}
		else if (location.m_type == SymbolLocation::Register)
		{
			m_globals[global->GetName()] = { Register, WriteReg(location.m_data) };
		}
		else if (location.m_type == SymbolLocation::XmmRegister)
		{
			if (global->GetType()->IsVector())
				m_globals[global->GetName()] = { Register, WriteXmmVectorReg(location.m_data) };
			else
				m_globals[global->GetName()] = { Register, WriteXmmReg(location.m_data) };
		}
	}

	std::array<uint8_t, 6> bytes =
	{
		// mov esi, imm32
		0xBE, 0x00, 0x00, 0x00, 0x00,
		// ret
		0xC3
	};

	std::memcpy(ObjectCursor(), &bytes[0], bytes.size());
	m_stackPointerSet = ((char*)ObjectCursor()) + 1;
	m_cursor += bytes.size();
}

void ShadyObject::WriteFunctions(const std::unordered_map<std::string, FunctionCode> & functions)
{
	for (auto && function : functions)
	{
		if (function.second.m_isExport)
			m_exports[function.first] = ObjectCursor();

		CallTrampoline();

		std::memcpy(ObjectCursor(), function.second.m_bytes.data(), function.second.m_bytes.size());

		m_cursor += static_cast<uint32_t>(function.second.m_bytes.size());
	}

	{
		// Update trampoline to set stack ptr
		void * stackStart = ObjectCursor();
		std::size_t space;

		std::align(16, 0x1000, stackStart, space);

		std::memcpy(m_stackPointerSet, &stackStart, sizeof(void*));
	}

	auto iter = m_exports.find("main");

	if (iter == m_exports.end())
		throw std::runtime_error("shader has no main() export");

	m_entryPoint = iter->second;
}

void * ShadyObject::ObjectCursor() const
{
	char * pointer = reinterpret_cast<char*>((void*)m_object);
	pointer += m_cursor;
	return pointer;
}

uint32_t ShadyObject::WriteReg(uint32_t reg)
{
	std::array<uint8_t, 6> bytes = { 0x8B, 0x05 + (reg * 8), 0x00, 0x00, 0x00, 0x00 };

	std::memcpy(ObjectCursor(), &bytes[0], bytes.size());

	uint32_t offset = m_cursor + 2;

	m_cursor += bytes.size();

	return offset;
}

uint32_t ShadyObject::WriteXmmReg(uint32_t reg)
{
	std::array<uint8_t, 8> bytes = { 0xF3, 0x0F, 0x10, 0x05 + (reg * 8), 0x00, 0x00, 0x00, 0x00 };

	std::memcpy(ObjectCursor(), &bytes[0], bytes.size());

	uint32_t offset = m_cursor + 4;

	m_cursor += bytes.size();

	return offset;
}

uint32_t ShadyObject::WriteXmmVectorReg(uint32_t reg)
{
	// uses movups because input might be unaligned
	std::array<uint8_t, 7> bytes = { 0x0F, 0x10, 0x05 + (reg * 8), 0x00, 0x00, 0x00, 0x00 };

	std::memcpy(ObjectCursor(), &bytes[0], bytes.size());

	uint32_t offset = m_cursor + 3;

	m_cursor += bytes.size();

	return offset;
}

void ShadyObject::CallTrampoline()
{
	uint32_t address = reinterpret_cast<uint32_t>(m_globalTrampoline);
	uint32_t cursor  = reinterpret_cast<uint32_t>(ObjectCursor()) + 5; // +5 == sizeof this instruction
	uint32_t offset = address - cursor;
	uint8_t * disp = (uint8_t*)&offset;

	std::array<uint8_t, 5> bytes = { 0xE8, disp[0], disp[1], disp[2], disp[3] };

	std::memcpy(ObjectCursor(), &bytes[0], bytes.size());

	m_cursor += bytes.size();
}
