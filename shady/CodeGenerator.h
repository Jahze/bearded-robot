#pragma once

#include <functional>
#include <map>
#include <unordered_map>
#include "Layout.h"
#include "SymbolTable.h"

class BuiltinType;
class FunctionTable;
class ProgramContext;
class ShadyObject;
class SymbolTable;
struct SyntaxNode;

struct FunctionCode
{
	std::vector<uint8_t> m_bytes;
	std::string m_asm;
	bool m_isExport = false;

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

	struct Variant
	{
		std::string name;
		std::vector<uint8_t> bytes;
	};

	const Variant & GetVariant(BuiltinType * type) const;

private:
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

	void Generate(ShadyObject * object, SyntaxNode * root);

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

	ValueDescription ProcessMultiply(Layout::StackLayout & stack, SyntaxNode * multiply, bool isAssign);
	ValueDescription ProcessDivide(Layout::StackLayout & stack, SyntaxNode * divide, bool isAssign);
	ValueDescription ProcessAdd(Layout::StackLayout & stack, SyntaxNode * add, bool isAssign);
	ValueDescription ProcessSubtract(Layout::StackLayout & stack, SyntaxNode * subtract, bool isAssign);

	ValueDescription ProcessSubscript(Layout::StackLayout & stack, SyntaxNode * subscript);
	ValueDescription ProcessFunctionCall(Layout::StackLayout & stack, SyntaxNode * function);

	ValueDescription ResolveRegisterPart(Layout::StackLayout & stack, ValueDescription value);

	void GenerateWrite(const ValueDescription & target, uint32_t literal);
	void GenerateWrite(const ValueDescription & target, const ValueDescription & source);

	void GenerateNormalize(ValueDescription value, SymbolLocation out);
	void GenerateLength(ValueDescription value, SymbolLocation out);
	void GenerateMax(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateDot3(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateClamp(ValueDescription value, ValueDescription min, ValueDescription max, SymbolLocation out);

	void GenerateMultiplyMatrixMatrix(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateMultiplyMatrixVector(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);
	void GenerateMultiplyVectorScalar(ValueDescription lhs, ValueDescription rhs, SymbolLocation out);

	void GenerateInstruction(
		bool commutitive,
		const Instruction & instruction,
		ValueDescription lhs,
		ValueDescription rhs,
		SymbolLocation out);

	void GenerateVectorInstruction(
		bool commutitive,
		const Instruction & instruction,
		ValueDescription lhs,
		ValueDescription rhs,
		SymbolLocation out);

	SymbolLocation GenerateExplodeFloat(Layout::StackLayout & stack, ValueDescription & float_);

	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, const SymbolLocation & source);
	std::vector<uint8_t> ConstructModRM(const SymbolLocation & target, uint8_t r);

	std::string TranslateValue(const ValueDescription & value);

	void CodeBytes(uint8_t byte);
	void CodeBytes(const std::initializer_list<uint8_t> & bytes);
	void CodeBytes(const std::vector<uint8_t> & bytes);

	template<typename T, typename... U>
	void DebugAsm(const std::string & format, T && value, U &&... args);
	void DebugAsm(const std::string & format);

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

	std::unordered_map<float, SymbolLocation> m_constantFloats;

	// XXX : not std::hash for tuple so can't use unordered_map :(
	std::map<std::tuple<float,float,float,float>, SymbolLocation> m_constantVectors;
};
