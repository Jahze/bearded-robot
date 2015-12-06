#pragma once

#include <cstdint>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "CodeGenerator.h"
#include "ProgramContext.h"
#include "SymbolTable.h"

class ScopedAlloc
{
public:
	ScopedAlloc(uint32_t size);
	~ScopedAlloc();

	operator void*() const
	{
		return m_pointer;
	}

private:
	void * m_pointer;
};

class ShadyObject
{
public:
	ShadyObject(uint32_t size);

	uint32_t GetStart()
	{
		return reinterpret_cast<uint32_t>((void*)m_object);
	}

	void Execute();

	void ReserveGlobalSize(uint32_t size);

	void WriteConstants(
		const std::unordered_map<float, SymbolLocation> & floatConstants,
		const std::map<std::tuple<float, float, float, float>, SymbolLocation> & vectorConstants);

	void NoteGlobals(const SymbolTable & symbolTable);

	void WriteFunctions(const std::unordered_map<std::string, FunctionCode> & functions);

	class GlobalWriter
	{
	public:
		GlobalWriter(void * destination, bool byAddress)
			: m_destination(destination)
			, m_byAddress(byAddress)
		{ }

		template<typename T>
		void Write(const T & t) const
		{
			if (! m_byAddress)
			{
				std::memcpy(m_destination, &t, sizeof(T));
			}
			else
			{
				const void * p = &t;
				std::memcpy(m_destination, &p, sizeof(void*));
			}
		}

	private:
		void *const m_destination;
		bool m_byAddress;
	};

	template<typename T>
	void WriteGlobal(const std::string & name, const T & t)
	{
		auto iter = m_globals.find(name);

		if (iter == m_globals.end())
			throw std::runtime_error("couldn't find global '" + name + "'");

		char * pointer = reinterpret_cast<char*>((void*)m_object);
		pointer += iter->second.second;

		if (iter->second.first == Memory)
		{
			std::memcpy(pointer, &t, sizeof(T));
		}
		else
		{
			uint32_t address = reinterpret_cast<uint32_t>(&t);
			uint8_t * value = (uint8_t*)&address;

			std::memcpy(pointer, value, sizeof(address));
		}
	}

	class GlobalReader
	{
	public:
		GlobalReader(void * source)
			: m_source(source)
		{ }

		template<typename T>
		void Read(T & t) const
		{
			std:memcpy(&t, m_source, sizeof(T));
		}

	private:
		void *const m_source;
	};

	GlobalWriter GetGlobalLocation(const std::string & name)
	{
		auto iter = m_globals.find(name);

		if (iter == m_globals.end())
			throw std::runtime_error("couldn't find global '" + name + "'");

		char * pointer = reinterpret_cast<char*>((void*)m_object);
		pointer += iter->second.second;

		return GlobalWriter(pointer, iter->second.first != Memory);
	}

	GlobalReader GetGlobalReader(const std::string & name)
	{
		auto iter = m_globals.find(name);

		if (iter == m_globals.end())
			throw std::runtime_error("couldn't find global '" + name + "'");

		char * pointer = reinterpret_cast<char*>((void*)m_object);
		pointer += iter->second.second;

		assert(iter->second.first == Memory);

		return GlobalReader(pointer);
	}

private:
	void * ObjectCursor() const;
	uint32_t WriteReg(uint32_t reg);
	uint32_t WriteXmmReg(uint32_t reg);
	uint32_t WriteXmmVectorReg(uint32_t reg);
	void CallTrampoline();

private:
	enum GlobalType
	{
		Memory,
		Register,
	};

	std::unordered_map<std::string, void*> m_exports;
	std::unordered_map<std::string, std::pair<GlobalType,uint32_t>> m_globals;
	void * m_entryPoint = nullptr;
	void * m_globalTrampoline = nullptr;
	void * m_stackPointerSet = nullptr;
	ScopedAlloc m_object;
	uint32_t m_cursor = 0;
};
