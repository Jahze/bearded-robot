#pragma once

#include <unordered_map>
#include "Layout.h"
#include "SymbolTable.h"

class BuiltinType;
class FunctionTable;
class ProgramContext;
class SymbolTable;
struct SyntaxNode;

struct FunctionCode
{
	std::vector<uint8_t> m_bytes;
	std::string m_asm;

	void Reset()
	{
		m_bytes.clear();
	}
};

class CodeGenerator
{
public:
	CodeGenerator(
		uint32_t globalMemory,
		const ProgramContext & context,
		SymbolTable & symbolTable,
		FunctionTable & functionTable);

	void Generate(SyntaxNode * root);

private:
	struct ValueDescription
	{
		SymbolLocation location;
		BuiltinType * type;
	};

	void InitialLayout();

	void ProcessFunction(SyntaxNode * function);
	ValueDescription ProcessExpression(Layout::StackLayout & stack, SyntaxNode * expression);
	ValueDescription ProcessLiteral(Layout::StackLayout & stack, SyntaxNode * literal);
	ValueDescription ProcessAssign(Layout::StackLayout & stack, SyntaxNode * assignment);
	ValueDescription ProcessName(Layout::StackLayout & stack, SyntaxNode * name);

	void GenerateWrite(const ValueDescription & target, uint32_t literal);
	void GenerateWrite(const ValueDescription & target, const ValueDescription & source);

	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, const SymbolLocation & source);
	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, uint8_t r);

	std::string TranslateValue(const ValueDescription & value);

private:
	const uint32_t m_globalMemory;
	const ProgramContext & m_context;
	SymbolTable & m_symbolTable;
	FunctionTable & m_functionTable;
	Layout m_layout;

	std::unordered_map<std::string, FunctionCode> m_functions;
	FunctionCode m_currentFunctionCode;
	Function * m_currentFunction = nullptr;

	std::unordered_map<float, SymbolLocation> m_constants;
};
