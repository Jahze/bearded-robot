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

class Instruction
{
public:
	Instruction(const std::string & integerName, std::initializer_list<uint8_t> integerBytes,
		const std::string & floatName, std::initializer_list<uint8_t> floatBytes,
		const std::string & vectorName, std::initializer_list<uint8_t> vectorBytes)
		: m_integer({integerName, {integerBytes}})
		, m_float({floatName, {floatBytes}})
		, m_vector({vectorName, {vectorBytes}})
	{ }

private:
	struct Variant
	{
		std::string name;
		std::vector<uint8_t> bytes;
	};

	Variant m_integer;
	Variant m_float;
	Variant m_vector;
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

	ValueDescription ProcessMultiply(Layout::StackLayout & stack, SyntaxNode * multiply);

	void GenerateWrite(const ValueDescription & target, uint32_t literal);
	void GenerateWrite(const ValueDescription & target, const ValueDescription & source);

	void GenerateMultiplyMatrixMatrix(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateMultiplyMatrixVector(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateMultiplyVectorScalar(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateMultiplyScalarScalar(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);

	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, const SymbolLocation & source);
	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, uint8_t r);

	std::string TranslateValue(const ValueDescription & value);

	void CodeBytes(uint8_t byte);
	void CodeBytes(const std::initializer_list<uint8_t> & bytes);
	void CodeBytes(const std::vector<uint8_t> & bytes);

	friend class OperandAssistant;

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
