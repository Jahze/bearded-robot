#include <cassert>
#include <sstream>
#include "br.h"
#include "BuiltinTypes.h"
#include "CodeGenerator.h"
#include "ProgramContext.h"
#include "ShadyObject.h"
#include "SymbolTable.h"
#include "SyntaxTree.h"

// when spilling registers of globals/locals we could
//   - allocate memory upfront for all of them (don't copy just reserve)
//   - when we spill copy to memory and update the SymbolLocation
//   - THEN don't have to put it back right afterwards

namespace
{
	class JumpPatcher
	{
	public:
		JumpPatcher(FunctionCode * code, int32_t offset)
			: m_code(code)
			, m_start(code->m_bytes.size())
			, m_offset(offset)
		{ }

		void PatchToHere()
		{
			uint32_t cursor = m_start + m_offset;

			assert(cursor < m_code->m_bytes.size());
			assert(m_start < m_code->m_bytes.size());

			assert(m_code->m_bytes.size() - m_start < 256);

			m_code->m_bytes[cursor] = m_code->m_bytes.size() - m_start;
		}

	private:
		FunctionCode * m_code;
		uint32_t m_start;
		int32_t m_offset;
	};

	class JumpPatcher32
	{
	public:
		JumpPatcher32(FunctionCode * code, int32_t offset)
			: m_code(code)
			, m_start(code->m_bytes.size())
			, m_offset(offset)
		{ }

		void PatchToHere()
		{
			uint32_t cursor = m_start + m_offset;

			assert(cursor < m_code->m_bytes.size());
			assert(m_start <= m_code->m_bytes.size());

			uint32_t offset = m_code->m_bytes.size() - m_start;

			std::memcpy(&m_code->m_bytes[cursor], &offset, 4);
		}

	private:
		FunctionCode * m_code;
		uint32_t m_start;
		int32_t m_offset;
	};

	// TODO remove
	std::string RegToStr(uint32_t r)
	{
		switch (r)
		{
		case 0: return "eax";
		case 1: return "ecx";
		case 2: return "edx";
		case 3: return "ebx";
		case 4: return "esp";
		case 5: return "ebp";
		case 6: return "esi";
		case 7: return "edi";
		}

		return "";
	}

	std::string XRegToStr(uint32_t r)
	{
		switch (r)
		{
		case 0: return "xmm0";
		case 1: return "xmm1";
		case 2: return "xmm2";
		case 3: return "xmm3";
		case 4: return "xmm4";
		case 5: return "xmm5";
		case 6: return "xmm6";
		case 7: return "xmm7";
		}

		return "";
	}

	std::string AsHex(uint32_t value)
	{
		std::ostringstream oss;
		oss << "0x" << std::hex << value;
		return oss.str();
	}

	BuiltinType * DominantScalarType(BuiltinType * type1, BuiltinType * type2)
	{
		assert(type1->IsScalar());
		assert(type2->IsScalar());

		if (type1->GetType() == BuiltinTypeType::Float)
			return type1;

		return type2;
	}

	SymbolLocation SelectOutputLocation(bool isAssign, Layout::StackLayout & stack, const SymbolLocation & lhs,
		BuiltinType *type)
	{
		if (isAssign)
			return lhs;

		return stack.PlaceTemporary(type);
	}

	bool IsAssignment(SyntaxNodeType type)
	{
		return type == SyntaxNodeType::Assign
			|| type == SyntaxNodeType::AddAssign
			|| type == SyntaxNodeType::SubtractAssign
			|| type == SyntaxNodeType::MultiplyAssign
			|| type == SyntaxNodeType::DivideAssign;
	}
}

namespace instruction
{
	Instruction Multiply
	{
		"imul",		{ 0x0F, 0xAF },
		"mulss",	{ 0xF3, 0x0F, 0x59 },
		"mulps",	{ 0x0F, 0x59 }
	};

	Instruction Divide
	{
		"",			{},
		"divss",	{ 0xF3, 0x0F, 0x5E },
		"",			{}
	};

	Instruction Add
	{
		"add",		{ 0x03 },
		"addss",	{ 0xF3, 0x0F, 0x58 },
		"addps",	{ 0x0F, 0x58 }
	};

	Instruction Subtract
	{
		"sub",		{ 0x2B },
		"subss",	{ 0xF3, 0x0F, 0x5C },
		"subps",	{ 0x0F, 0x5C }
	};
}

// RENAME : something about ensuring it's in register for the duration
// TODO : need to pass in unspillable locations
class OperandAssistant
{
public:
	OperandAssistant(CodeGenerator *generator, SymbolLocation & location, BuiltinType * type)
		: m_generator(generator)
		, m_oldLocation(location)
		, m_oldType(type)
	{
		if (! location.InMemory())
			return;

		const BuiltinTypeType builtinType = type->GetType();

		assert(br::one_of(builtinType,
			BuiltinTypeType::Bool, BuiltinTypeType::Int,
			BuiltinTypeType::Float, BuiltinTypeType::Vec3,
			BuiltinTypeType::Vec4));

		if (br::one_of(builtinType, BuiltinTypeType::Bool, BuiltinTypeType::Int))
		{
			m_register = generator->m_layout.GetFreeRegister();
		}
		else
		{
			m_register = generator->m_layout.GetFreeXmmRegister();
		}

		location = m_register->Location();

		SymbolLocation spiltLocation = m_register->SpiltLocation();

		if (spiltLocation.m_type != SymbolLocation::None)
			m_generator->GenerateWrite({ spiltLocation, type }, { location, type });
	}

	~OperandAssistant()
	{
		// write register back to memory
		if (bool(m_register))
		{
			m_generator->GenerateWrite({ m_oldLocation, m_oldType }, { m_register->Location(), m_oldType });

			SymbolLocation spiltLocation = m_register->SpiltLocation();

			if (spiltLocation.m_type != SymbolLocation::None)
				m_generator->GenerateWrite({ m_register->Location(), m_oldType }, { spiltLocation, m_oldType });
		}
	}

private:
	CodeGenerator *m_generator;
	std::unique_ptr<Layout::TemporaryRegister> m_register;
	SymbolLocation m_oldLocation;
	BuiltinType *m_oldType;
};

const Instruction::Variant & Instruction::GetVariant(BuiltinType * type) const
{
	if (type->IsVector())
	{
		assert(m_vector.bytes.size());
		return m_vector;
	}
	if (type->GetType() == BuiltinTypeType::Float)
	{
		assert(m_float.bytes.size());
		return m_float;
	}
	else
	{
		assert(m_integer.bytes.size());
		return m_integer;
	}
}

CodeGenerator::CodeGenerator(uint32_t globalMemory, const ProgramContext & context, SymbolTable & symbolTable,
	FunctionTable & functionTable)
	: m_globalMemory(globalMemory)
	, m_context(context)
	, m_symbolTable(symbolTable)
	, m_functionTable(functionTable)
{
	InitialLayout();
}

void CodeGenerator::Generate(ShadyObject * object, SyntaxNode * root)
{
	for (auto && node : root->m_nodes)
	{
		switch (node->m_type)
		{
		case SyntaxNodeType::GlobalVariable:
			// Ignore globals for now as they are already laid out and have no initializer
			break;

		case SyntaxNodeType::Function:
			ProcessFunction(node.get());
			break;

		default:
			throw std::runtime_error("malformed syntax tree");
		}
	}

	object->ReserveGlobalSize(m_layout.GlobalMemoryUsed());
	object->NoteGlobals(m_symbolTable);
	object->WriteConstants(m_constantFloats, m_constantVectors);
	object->WriteFunctions(m_functions);
}

void CodeGenerator::InitialLayout()
{
	// globals includes context variables which are added in syntax parsing stage

	// Order affects layout:
	//   - order they are applied to symtab (must be first)
	//   - order they are declared

	std::vector<Symbol*> globals = m_symbolTable.GetGlobalSymbols();

	for (auto && global : globals)
	{
		if (global->GetSymbolType() != SymbolType::Variable)
			continue;

		if (global->IsIntrinsic())
		{
			const ContextVariable * contextVariable = m_context.GetVariable(global->GetName());

			assert(contextVariable);

			// Put context out variables in memory as it's likely they're only being used once to write to
			if (contextVariable->m_type == ContextVariable::Output)
			{
				m_layout.PlaceGlobalInMemory(global);
				continue;
			}
		}

		m_layout.PlaceGlobal(global);
	}
}

void CodeGenerator::ProcessFunction(SyntaxNode * functionNode)
{
	assert(functionNode->m_nodes.size() == 3);
	assert(functionNode->m_nodes[2]->m_type == SyntaxNodeType::StatementList);

	m_currentFunctionCode.Reset();

	m_currentFunction = m_functionTable.FindFunction(functionNode->m_data);
	m_currentFunctionCode.m_isExport = m_currentFunction->IsExport();

	assert(m_currentFunction);

	m_layout.PlaceParametersAndLocals(m_currentFunction);

	ProcessStatements(functionNode->m_nodes[2].get());

	m_layout.RelinquishParametersAndLocals(m_currentFunction);

	m_functions[m_currentFunction->GetName()] = m_currentFunctionCode;

	m_currentFunction = nullptr;
}

void CodeGenerator::ProcessStatements(SyntaxNode * statements)
{
	for (auto && node : statements->m_nodes)
	{
		switch (node->m_type)
		{
		case SyntaxNodeType::LocalVariable:
		{
			if (node->m_nodes.size() > 1)
			{
				assert(node->m_nodes[1]->m_type == SyntaxNodeType::Initializer);

				SyntaxNode *initializer = node->m_nodes[1].get();

				assert(initializer->m_nodes.size() == 1);
				assert(initializer->m_nodes[0]->m_type == SyntaxNodeType::Expression);

				SyntaxNode *expression = initializer->m_nodes[0].get();

				assert(expression->m_nodes.size() == 1);

				Layout::StackLayout stack = m_layout.TemporaryLayout();

				ValueDescription value = ProcessExpression(stack, expression->m_nodes[0].get());

				Symbol * local = m_currentFunction->GetLocal(node->m_data);

				GenerateWrite({ local->GetLocation(), local->GetType() }, value);
			}
			break;
		}

		case SyntaxNodeType::Expression:
		{
			assert(node->m_nodes.size() == 1);

			Layout::StackLayout stack = m_layout.TemporaryLayout();

			ProcessExpression(stack, node->m_nodes[0].get());

			break;
		}

		case SyntaxNodeType::If:
		{
			Layout::StackLayout stack = m_layout.TemporaryLayout();

			ProcessIf(stack, node.get());

			break;
		}

		case SyntaxNodeType::Return:
			// TODO : return values
			CodeBytes({ 0xC3 });
			DebugAsm("ret");
			break;

		default:
			throw std::runtime_error("malformed syntax tree");
		}
	}
}

CodeGenerator::ValueDescription CodeGenerator::ProcessExpression(Layout::StackLayout & stack, SyntaxNode * expression)
{
	switch (expression->m_type)
	{
	case SyntaxNodeType::Literal:
		return ProcessLiteral(stack, expression);

	case SyntaxNodeType::Name:
		return ProcessName(stack, expression);

	case SyntaxNodeType::Assign:
		return ProcessAssign(stack, expression);

	case SyntaxNodeType::Multiply:
		return ProcessMultiply(stack, expression, false);

	case SyntaxNodeType::Divide:
		return ProcessDivide(stack, expression, false);

	case SyntaxNodeType::Add:
		return ProcessAdd(stack, expression, false);

	case SyntaxNodeType::Subtract:
		return ProcessSubtract(stack, expression, false);

	// TODO : assign instructions can't convert result to correct scalar type
	case SyntaxNodeType::MultiplyAssign:
		return ProcessMultiply(stack, expression, true);

	case SyntaxNodeType::DivideAssign:
		return ProcessDivide(stack, expression, true);

	case SyntaxNodeType::AddAssign:
		return ProcessAdd(stack, expression, true);

	case SyntaxNodeType::SubtractAssign:
		return ProcessSubtract(stack, expression, true);

	case SyntaxNodeType::Subscript:
		return ProcessSubscript(stack, expression);

	case SyntaxNodeType::FunctionCall:
		return ProcessFunctionCall(stack, expression);

	case SyntaxNodeType::Negate:
		return ProcessNegate(stack, expression);

	case SyntaxNodeType::LogicalNegate:
	case SyntaxNodeType::Equals:
	case SyntaxNodeType::NotEquals:
	case SyntaxNodeType::Less:
	case SyntaxNodeType::LessEquals:
	case SyntaxNodeType::Greater:
	case SyntaxNodeType::GreaterEquals:
	default:
		throw std::runtime_error("malformed syntax tree");
	}
}

CodeGenerator::ValueDescription CodeGenerator::ProcessLiteral(Layout::StackLayout & stack, SyntaxNode * literal)
{
	assert(literal->m_nodes.size() == 1);
	assert(literal->m_nodes[0]->m_type == SyntaxNodeType::Type);

	BuiltinType * type = BuiltinType::Get(BuiltinType::FromName(literal->m_nodes[0]->m_data));

	if (type->GetType() == BuiltinTypeType::Float)
	{
		float value = std::stof(literal->m_data);

		SymbolLocation location = GetConstantFloat(value);

		SymbolLocation temporary = stack.PlaceTemporary(type);

		GenerateWrite({ temporary, type }, { location, type });

		return { temporary, type };
	}
	else
	{
		SymbolLocation location = stack.PlaceTemporary(type);

		GenerateWrite({ location, type }, static_cast<uint32_t>(std::stoi(literal->m_data)));

		return { location, type };
	}
}

CodeGenerator::ValueDescription CodeGenerator::ProcessAssign(Layout::StackLayout & stack, SyntaxNode * assignment)
{
	assert(assignment->m_nodes.size() == 2);

	ValueDescription lhs = ProcessExpression(stack, assignment->m_nodes[0].get());

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		ValueDescription rhs = ProcessExpression(subStack, assignment->m_nodes[1].get());

		GenerateWrite(lhs, rhs);
	}

	return lhs;
}

CodeGenerator::ValueDescription CodeGenerator::ProcessName(Layout::StackLayout & stack, SyntaxNode * name)
{
	Symbol * symbol = m_symbolTable.FindSymbol(name->m_data, m_currentFunction);

	return { symbol->GetLocation(), symbol->GetType() };
}

CodeGenerator::ValueDescription CodeGenerator::ProcessMultiply(Layout::StackLayout & stack, SyntaxNode * multiply,
	bool isAssign)
{
	assert(multiply->m_nodes.size() == 2);

	ValueDescription lhs;
	ValueDescription rhs;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		lhs = ProcessExpression(subStack, multiply->m_nodes[0].get());
		rhs = ProcessExpression(subStack, multiply->m_nodes[1].get());

		// temporary locations get given back here so destination and source can be the same
	}

	if (lhs.type->IsMatrix())
	{
		if (rhs.type->IsMatrix())
		{
			SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, rhs.type);
			GenerateMultiplyMatrixMatrix(lhs, rhs, out);
			return { out, rhs.type };
		}
		else if (rhs.type->IsVector())
		{
			SymbolLocation out = stack.PlaceTemporary(rhs.type);
			GenerateMultiplyMatrixVector(lhs, rhs, out);
			return { out, rhs.type };
		}
	}
	else if (lhs.type->IsVector())
	{
		if (rhs.type->IsScalar())
		{
			SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, lhs.type);
			GenerateMultiplyVectorScalar(lhs, rhs, out);
			return { out, lhs.type };
		}
	}
	else if (lhs.type->IsScalar())
	{
		BuiltinType * resultType = DominantScalarType(lhs.type, rhs.type);
		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, resultType);
		GenerateInstruction(true, instruction::Multiply, lhs, rhs, out);
		return { out, resultType };
	}

	throw std::runtime_error("malformed syntax tree");
}

CodeGenerator::ValueDescription CodeGenerator::ProcessDivide(Layout::StackLayout & stack, SyntaxNode * divide,
	bool isAssign)
{
	assert(divide->m_nodes.size() == 2);

	ValueDescription lhs;
	ValueDescription rhs;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		lhs = ProcessExpression(subStack, divide->m_nodes[0].get());
		rhs = ProcessExpression(subStack, divide->m_nodes[1].get());

		// temporary locations get given back here so destination and source can be the same
	}

	// TODO : integer division - idiv is weird only one operand

	if (lhs.type->IsScalar() && rhs.type->IsScalar())
	{
		BuiltinType * resultType = DominantScalarType(lhs.type, rhs.type);
		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, resultType);
		GenerateInstruction(false, instruction::Divide, lhs, rhs, out);
		return { out, resultType };
	}

	throw std::runtime_error("malformed syntax tree");
}

CodeGenerator::ValueDescription CodeGenerator::ProcessAdd(Layout::StackLayout & stack, SyntaxNode * add, bool isAssign)
{
	assert(add->m_nodes.size() == 2);

	ValueDescription lhs;
	ValueDescription rhs;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		lhs = ProcessExpression(subStack, add->m_nodes[0].get());
		rhs = ProcessExpression(subStack, add->m_nodes[1].get());

		// temporary locations get given back here so destination and source can be the same
	}

	if (lhs.type->IsVector())
	{
		assert(lhs.type == rhs.type);

		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, lhs.type);
		GenerateVectorInstruction(true, instruction::Add, lhs, rhs, out);
		return { out, lhs.type };
	}
	else if (lhs.type->IsScalar())
	{
		assert(rhs.type->IsScalar());

		BuiltinType * resultType = DominantScalarType(lhs.type, rhs.type);
		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, resultType);
		GenerateInstruction(true, instruction::Add, lhs, rhs, out);
		return { out, resultType };
	}

	throw std::runtime_error("malformed syntax tree");
}

CodeGenerator::ValueDescription CodeGenerator::ProcessSubtract(Layout::StackLayout & stack, SyntaxNode * subtract,
	bool isAssign)
{
	assert(subtract->m_nodes.size() == 2);

	ValueDescription lhs;
	ValueDescription rhs;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		lhs = ProcessExpression(subStack, subtract->m_nodes[0].get());
		rhs = ProcessExpression(subStack, subtract->m_nodes[1].get());

		// temporary locations get given back here so destination and source can be the same
	}

	if (lhs.type->IsVector())
	{
		assert(lhs.type == rhs.type);

		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, lhs.type);
		GenerateVectorInstruction(false, instruction::Subtract, lhs, rhs, out);
		return { out, lhs.type };
	}
	else if (lhs.type->IsScalar())
	{
		assert(rhs.type->IsScalar());

		BuiltinType * resultType = DominantScalarType(lhs.type, rhs.type);
		SymbolLocation out = SelectOutputLocation(isAssign, stack, lhs.location, resultType);
		GenerateInstruction(false, instruction::Subtract, lhs, rhs, out);
		return { out, resultType };
	}

	throw std::runtime_error("malformed syntax tree");
}

CodeGenerator::ValueDescription CodeGenerator::ProcessNegate(Layout::StackLayout & stack, SyntaxNode * negate)
{
	assert(negate->m_nodes.size() == 1);

	ValueDescription value;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		value = ProcessExpression(subStack, negate->m_nodes[0].get());
	}

	assert(value.type->IsScalar() || (value.type->IsVector() && ! value.type->IsMatrix()));

	SymbolLocation out = stack.PlaceTemporary(value.type);

	if (out != value.location)
		GenerateWrite({ out, value.type }, value);

	if (value.type->GetType() == BuiltinTypeType::Int)
	{
		CodeBytes(0xF7);
		CodeBytes(ConstructModRM(out, 3));

		DebugAsm("neg $",
			TranslateValue({ out, value.type }));
	}
	else
	{
		// XXX: This does vectors and floats
		// there is no xorss instruction

		uint32_t xor = 0x80000000;
		float mask = *reinterpret_cast<float*>(&xor);
		std::tuple<float, float, float, float> constant { mask, mask, mask, mask };

		SymbolLocation constantLocation = GetConstantVector(constant);

		CodeBytes({ 0x0F, 0x57 });
		CodeBytes(ConstructModRM(out, constantLocation));

		DebugAsm("xorps $,$",
			TranslateValue({ out, value.type }),
			TranslateValue({ constantLocation, value.type }));
	}

	return { out, value.type };
}

CodeGenerator::ValueDescription CodeGenerator::ProcessSubscript(Layout::StackLayout & stack, SyntaxNode * subscript)
{
	assert(subscript->m_nodes.size() == 2);

	ValueDescription lhs;
	ValueDescription rhs;

	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		lhs = ProcessExpression(subStack, subscript->m_nodes[0].get());
		rhs = ProcessExpression(subStack, subscript->m_nodes[1].get());

		// temporary locations get given back here so destination and source can be the same
	}

	assert(lhs.type->IsVector());

	if (lhs.type->IsMatrix())
	{
		// TODO : can optimise if index is constant

		const uint32_t stride = 4 * 4;

		SymbolLocation indexLocation = stack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));
		ValueDescription indexValue = { indexLocation, BuiltinType::Get(BuiltinTypeType::Int) };

		if (indexLocation == rhs.location)
		{
			OperandAssistant assistant(this, indexLocation, BuiltinType::Get(BuiltinTypeType::Int));
			CodeBytes(0x69);
			CodeBytes(ConstructModRM(indexLocation, indexLocation));
			CodeBytes(br::as_bytes(stride));

			DebugAsm("mul $,$,$",
				TranslateValue(indexValue),
				TranslateValue(indexValue),
				stride);
		}
		else
		{
			GenerateWrite(indexValue, stride);
			GenerateInstruction(true, instruction::Multiply, indexValue, rhs, indexValue.location);
		}

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			SymbolLocation addressLocation = subStack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));
			ValueDescription addressValue = { addressLocation, BuiltinType::Get(BuiltinTypeType::Int) };

			assert(lhs.location.InMemory());

			if (lhs.location.m_type == SymbolLocation::GlobalMemory)
			{
				GenerateWrite(addressValue, lhs.location.m_data + m_globalMemory);
			}
			else if (lhs.location.m_type == SymbolLocation::LocalMemory)
			{
				SymbolLocation sp;
				sp.m_type = SymbolLocation::Register;
				sp.m_data = Register::Esi;
				GenerateWrite(addressValue, lhs.location.m_data);
				GenerateInstruction(false, instruction::Add, addressValue, { sp, addressValue.type }, addressLocation);
			}
			else
			{
				throw std::runtime_error("unimplemented");
			}

			GenerateInstruction(true, instruction::Add, indexValue, addressValue, indexLocation);

			indexLocation.MakeIndirect();

			return { indexLocation, lhs.type->GetElementType() };
		}
	}
	else if (lhs.type->IsVector())
	{
		Layout::StackLayout subStack = m_layout.TemporaryLayout();

		if (lhs.location.InMemory())
		{
			const uint32_t stride = 4;

			SymbolLocation indexLocation = stack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));
			ValueDescription indexValue = { indexLocation, BuiltinType::Get(BuiltinTypeType::Int) };

			if (indexLocation == rhs.location)
			{
				OperandAssistant assistant(this, indexLocation, BuiltinType::Get(BuiltinTypeType::Int));
				CodeBytes(0x69);
				CodeBytes(ConstructModRM(indexLocation, indexLocation));
				CodeBytes(br::as_bytes(stride));

				DebugAsm("mul $,$,$",
					TranslateValue(indexValue),
					TranslateValue(indexValue),
					stride);
			}
			else
			{
				GenerateWrite(indexValue, stride);
				GenerateInstruction(true, instruction::Multiply, indexValue, rhs, indexValue.location);
			}

			SymbolLocation addressLocation = subStack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));
			ValueDescription addressValue = { addressLocation, BuiltinType::Get(BuiltinTypeType::Int) };

			if (lhs.location.m_type == SymbolLocation::GlobalMemory)
			{
				GenerateWrite(addressValue, lhs.location.m_data + m_globalMemory);
			}
			else if (lhs.location.m_type == SymbolLocation::LocalMemory)
			{
				SymbolLocation sp;
				sp.m_type = SymbolLocation::Register;
				sp.m_data = Register::Esi;
				GenerateWrite(addressValue, lhs.location.m_data);
				GenerateInstruction(false, instruction::Add, addressValue, { sp, addressValue.type }, addressLocation);
			}
			else
			{
				throw std::runtime_error("unimplemented");
			}

			GenerateInstruction(true, instruction::Add, indexValue, addressValue, indexLocation);

			indexLocation.MakeIndirect();

			return { indexLocation, lhs.type->GetElementType() };
		}
		else
		{
			SymbolLocation shiftLocation = stack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));

			if (! shiftLocation.InMemory())
			{
				shiftLocation = subStack.PlaceTemporaryInMemory(rhs.type);
				GenerateWrite({ shiftLocation, rhs.type }, rhs);
			}
			else if (shiftLocation != rhs.location)
			{
				GenerateWrite({ shiftLocation, rhs.type }, rhs);
			}

			ValueDescription shiftValue = { shiftLocation, rhs.type };

			{
				Layout::StackLayout subStack2 = m_layout.TemporaryLayout();

				SymbolLocation temp = subStack2.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Int));
				ValueDescription tempValue = { temp, BuiltinType::Get(BuiltinTypeType::Int) };

				const uint32_t stride = 4;

				GenerateWrite(tempValue, 4 * 8);
				GenerateInstruction(true, instruction::Multiply, shiftValue, tempValue, shiftValue.location);
			}

			assert(lhs.location.m_type == SymbolLocation::XmmRegister);

			SymbolLocation out =
				stack.PlaceRegisterPart(static_cast<XmmRegister>(lhs.location.m_data), shiftLocation.m_data);

			if (! IsAssignment(subscript->m_parent->m_type))
				return ResolveRegisterPart(stack, { out, lhs.type->GetElementType() });

			return { out, lhs.type->GetElementType() };
		}
	}

	throw std::runtime_error("malformed syntax tree");
}

CodeGenerator::ValueDescription CodeGenerator::ProcessFunctionCall(Layout::StackLayout & stack, SyntaxNode * function)
{
	assert(function->m_nodes.size());

	SyntaxNode * name = function->m_nodes[0].get();

	assert(name->m_type == SyntaxNodeType::Name);

	if (name->m_data == "normalize" || name->m_data == "length")
	{
		assert(function->m_nodes.size() == 2);

		ValueDescription lhs;

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			lhs = ProcessExpression(subStack, function->m_nodes[1].get());
		}

		BuiltinType * returnType = m_functionTable.FindFunction(name->m_data)->GetReturnType();

		SymbolLocation out = stack.PlaceTemporary(returnType);

		if (name->m_data == "normalize")
			GenerateNormalize(lhs, out);
		else
			GenerateLength(lhs, out);

		return { out, returnType };
	}
	else if (name->m_data == "dot3")
	{
		assert(function->m_nodes.size() == 3);

		ValueDescription lhs;
		ValueDescription rhs;

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			lhs = ProcessExpression(subStack, function->m_nodes[1].get());
			rhs = ProcessExpression(subStack, function->m_nodes[2].get());
		}

		SymbolLocation out = stack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Float));

		GenerateDot3(lhs, rhs, out);

		return { out, BuiltinType::Get(BuiltinTypeType::Float) };
	}
	else if (name->m_data == "clamp")
	{
		assert(function->m_nodes.size() == 4);

		BuiltinType *floatType = BuiltinType::Get(BuiltinTypeType::Float);

		SymbolLocation temp;

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			ValueDescription value = ProcessExpression(subStack, function->m_nodes[1].get());
			ValueDescription min = ProcessExpression(subStack, function->m_nodes[2].get());
			ValueDescription max = ProcessExpression(subStack, function->m_nodes[3].get());

			temp = subStack.PlaceTemporary(floatType);

			GenerateClamp(value, min, max, temp);
		}

		SymbolLocation out = stack.PlaceTemporary(floatType);

		if (out != temp)
			GenerateWrite({ out, floatType }, { temp, floatType });

		return { out, floatType };
	}

	throw std::runtime_error("function call not implemented");
}

void CodeGenerator::ProcessRelational(Layout::StackLayout & stack, SyntaxNode * relational)
{
	assert(br::one_of(relational->m_type, SyntaxNodeType::Equals, SyntaxNodeType::NotEquals,
		SyntaxNodeType::Greater, SyntaxNodeType::GreaterEquals, SyntaxNodeType::Less,
		SyntaxNodeType::LessEquals));

	assert(relational->m_nodes.size() == 2);

	Layout::StackLayout subStack = m_layout.TemporaryLayout();

	ValueDescription lhs = ProcessExpression(subStack, relational->m_nodes[0].get());
	ValueDescription rhs = ProcessExpression(subStack, relational->m_nodes[1].get());

	if (lhs.type != rhs.type)
	{
		// TODO: convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	// TODO: == and != allow any type

	if (lhs.type->GetType() == BuiltinTypeType::Int)
	{
		uint8_t opcode = 0x39;

		std::unique_ptr<Layout::TemporaryRegister> reg;

		if (lhs.location.InMemory())
		{
			if (rhs.location.InMemory())
			{
				reg = m_layout.GetFreeRegister();

				GenerateWrite({ reg->Location(), rhs.type }, rhs);

				rhs.location = reg->Location();
			}
		}
		else
		{
			opcode = 0x3B;
		}

		CodeBytes(opcode);
		CodeBytes(ConstructModRM(lhs.location, rhs.location));

		DebugAsm("cmp $,$",
			TranslateValue(lhs),
			TranslateValue(rhs));
	}
	else
	{
		std::unique_ptr<Layout::TemporaryRegister> reg;

		if (lhs.location.InMemory())
		{
			reg = m_layout.GetFreeXmmRegister();

			GenerateWrite({ reg->Location(), lhs.type }, lhs);

			lhs.location = reg->Location();
		}

		CodeBytes({ 0x0F, 0x2F });
		CodeBytes(ConstructModRM(lhs.location, rhs.location));

		DebugAsm("comiss $,$",
			TranslateValue(lhs),
			TranslateValue(rhs));
	}
}

void CodeGenerator::ProcessIf(Layout::StackLayout & stack, SyntaxNode * if_)
{
	assert(if_->m_nodes.size() > 1);

	SyntaxNode * condition = if_->m_nodes[0].get();

	assert(condition->m_nodes.size() == 1);
	assert(condition->m_type == SyntaxNodeType::Condition);

	SyntaxNode * expression = condition->m_nodes[0].get();

	assert(expression->m_nodes.size() == 1);
	assert(expression->m_type == SyntaxNodeType::Expression);

	ProcessRelational(stack, expression->m_nodes[0].get());

	switch (expression->m_nodes[0]->m_type)
	{
	case SyntaxNodeType::Less:
		CodeBytes({ 0x0F, 0x83, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("jae x");
		break;
	case SyntaxNodeType::LessEquals:
		CodeBytes({ 0x0F, 0x87, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("ja x");
		break;
	case SyntaxNodeType::Greater:
		CodeBytes({ 0x0F, 0x86, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("jbe x");
		break;
	case SyntaxNodeType::GreaterEquals:
		CodeBytes({ 0x0F, 0x82, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("jb x");
		break;
	case SyntaxNodeType::Equals:
		CodeBytes({ 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("jne x");
		break;
	case SyntaxNodeType::NotEquals:
		CodeBytes({ 0x0F, 0x84, 0x00, 0x00, 0x00, 0x00 });
		DebugAsm("je x");
		break;
	default:
		throw std::runtime_error("malformed syntax tree");
	}

	JumpPatcher32 after(&m_currentFunctionCode, -4);

	SyntaxNode * statements = if_->m_nodes[1].get();

	assert(statements->m_type == SyntaxNodeType::StatementList);

	ProcessStatements(statements);

	CodeBytes({ 0xE9, 0x00, 0x00, 0x00, 0x00 });
	DebugAsm("jmp x");

	JumpPatcher32 end(&m_currentFunctionCode, -4);

	after.PatchToHere();

	if (if_->m_nodes.size() > 2)
	{
		SyntaxNode * next = if_->m_nodes[2].get();

		if (next->m_type == SyntaxNodeType::ElseIf)
		{
			assert(next->m_nodes.size() > 0);
			ProcessIf(stack, next->m_nodes[0].get());
		}
		else
		{
			assert(next->m_type == SyntaxNodeType::Else);
			assert(next->m_nodes.size() > 0);
			assert(next->m_nodes[0]->m_type == SyntaxNodeType::StatementList);
			ProcessStatements(next->m_nodes[0].get());
		}
	}

	end.PatchToHere();
}

CodeGenerator::ValueDescription CodeGenerator::ResolveRegisterPart(Layout::StackLayout & stack, ValueDescription value)
{
	if (value.location.m_type == SymbolLocation::RegisterPart)
	{
		SymbolLocation out = stack.PlaceTemporary(value.type);

		SymbolLocation valueLocation;
		valueLocation.m_type = SymbolLocation::XmmRegister;
		valueLocation.m_data = value.location.m_data;

		SymbolLocation shift;
		SymbolLocation::LocalMemory;
		shift.m_data = value.location.m_shift;

		// TODO : reuse value register location for out if temporary

		assert(out.m_type == SymbolLocation::XmmRegister);

		GenerateWrite({ out, value.type }, { valueLocation, value.type });

		CodeBytes({ 0x66, 0x0F, 0xD2 });
		CodeBytes(ConstructModRM(out, shift));

		DebugAsm("psrld $,$",
			TranslateValue({ out, value.type }),
			TranslateValue({ shift, value.type }));

		const int one = 1;
		std::tuple<float, float, float, float> constant { 0.0f, 0.0f, 0.0f, *reinterpret_cast<const float*>(&one) };

		SymbolLocation constantLocation = GetConstantVector(constant);

		// TODO : move this to the part where it gets written
		// this would allow any operation to leave crap in the upper bits

		CodeBytes({ 0x0F, 0x54 });
		CodeBytes(ConstructModRM(out, constantLocation));

		DebugAsm("andps $,$",
			TranslateValue({ out, value.type }),
			TranslateValue({ constantLocation, value.type }));

		return { out, value.type };
	}

	return value;
}

void CodeGenerator::GenerateWrite(const ValueDescription & target, uint32_t literal)
{
	if (br::none_of(target.type->GetType(), BuiltinTypeType::Int, BuiltinTypeType::Bool))
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	CodeBytes(0xC7);
	CodeBytes(ConstructModRM(target.location, 0x0));
	CodeBytes(br::as_bytes(literal));

	DebugAsm("mov $,$",
		TranslateValue(target),
		std::to_string(literal));
}

void CodeGenerator::GenerateWrite(const ValueDescription & target, const ValueDescription & source)
{
	if (target.type != source.type)
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	if (target.location.InMemory() && source.location.InMemory())
	{
		// TODO : get layout->hasfreeregister and spill otherwise
		auto reg = m_layout.GetFreeRegister();

		const uint32_t size = source.type->GetSize();

		SymbolLocation copySource = source.location;
		SymbolLocation copyTarget = target.location;

		for (uint32_t i = 0; i < size; i += 4)
		{
			CodeBytes(0x8B);
			CodeBytes(ConstructModRM(reg->Location(), copySource));

			DebugAsm("mov $,$",
				TranslateValue({ reg->Location(), BuiltinType::Get(BuiltinTypeType::Int) }),
				TranslateValue({ copySource, BuiltinType::Get(BuiltinTypeType::Int) }));

			CodeBytes(0x89);
			CodeBytes(ConstructModRM(copyTarget, reg->Location()));

			DebugAsm("mov $,$",
				TranslateValue({ copyTarget, BuiltinType::Get(BuiltinTypeType::Int) }),
				TranslateValue({ reg->Location(), BuiltinType::Get(BuiltinTypeType::Int) }));

			copySource.m_data += 4;
			copyTarget.m_data += 4;
		}
	}
	else
	{
		BuiltinType *type = target.type;

		std::string instruction;
		std::vector<uint8_t> bytes;

		SymbolLocation out = target.location;

		if (out.m_type == SymbolLocation::RegisterPart)
		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();
			out = ResolveRegisterPart(subStack, target).location;
		}

		if (type->IsVector())
		{
			instruction = "movaps";

			if (out.InMemory())
				CodeBytes({ 0x0f, 0x29 });
			else
				CodeBytes({ 0x0f, 0x28 });
		}
		else if (type->GetType() == BuiltinTypeType::Float)
		{
			instruction = "movss";

			if (out.InMemory())
				CodeBytes({ 0xf3, 0x0f, 0x11 });
			else
				CodeBytes({ 0xf3, 0x0f, 0x10 });
		}
		else
		{
			instruction = "mov";

			if (out.InMemory())
				CodeBytes({ 0x89 });
			else
				CodeBytes({ 0x8b });
		}

		CodeBytes(ConstructModRM(out, source.location));

		DebugAsm(instruction + " $,$",
			TranslateValue({ out, type }),
			TranslateValue(source));

		if (target.location.m_type == SymbolLocation::RegisterPart)
		{
			SymbolLocation shift;
			SymbolLocation::LocalMemory;
			shift.m_data = target.location.m_shift;

			CodeBytes({ 0x66, 0x0F, 0xF2 });
			CodeBytes(ConstructModRM(out, shift));

			DebugAsm("pslld $,$",
				TranslateValue({ out, type }),
				TranslateValue({ shift, type }));

			CodeBytes({ 0x0F, 0x58 });
			CodeBytes(ConstructModRM(target.location, out));

			DebugAsm("addps $,$",
				TranslateValue(target),
				TranslateValue({ out, type }));
		}
	}
}

void CodeGenerator::GenerateNormalize(ValueDescription value, SymbolLocation out)
{
	// http://fastcpp.blogspot.co.uk/2012/02/calculating-length-of-3d-vector-using.html
	// XXX : this is a "fast" normalize that gets the magnitude close to 1 but not exactly
	// does that matter?

	assert(value.type->GetType() == BuiltinTypeType::Vec4);

	OperandAssistant assitant(this, out, value.type);

	bool writeToOut = true;

	std::unique_ptr<Layout::TemporaryRegister> reg;

	if (out == value.location)
	{
		reg = m_layout.GetFreeXmmRegister();
		out = reg->Location();
		writeToOut = false;
	}

	GenerateWrite({ out, value.type }, value);

	CodeBytes({ 0x66, 0x0F, 0x3A, 0x40 });
	CodeBytes(ConstructModRM(out, out));
	CodeBytes(0x77);

	DebugAsm("dpps $,$,0x77",
		TranslateValue({ out, value.type }),
		TranslateValue({ out, value.type }));

	CodeBytes({ 0x0F, 0x52 });
	CodeBytes(ConstructModRM(out, out));

	DebugAsm("rsqrtps $,$",
		TranslateValue({ out, value.type }),
		TranslateValue({ out, value.type }));

	CodeBytes({ 0x0F, 0x59 });

	if (writeToOut)
	{
		CodeBytes(ConstructModRM(out, value.location));

		DebugAsm("mulps $,$",
			TranslateValue({ out, value.type }),
			TranslateValue(value));
	}
	else
	{
		CodeBytes(ConstructModRM(value.location, out));

		DebugAsm("mulps $,$",
			TranslateValue(value),
			TranslateValue({ out, value.type }));
	}
}

void CodeGenerator::GenerateLength(ValueDescription value, SymbolLocation out)
{
	assert(value.type->GetType() == BuiltinTypeType::Vec4);

	OperandAssistant assitant(this, out, BuiltinType::Get(BuiltinTypeType::Float));

	if (out != value.location)
		GenerateWrite({ out, value.type }, value);

	CodeBytes({ 0x66, 0x0F, 0x3A, 0x40 });
	CodeBytes(ConstructModRM(out, out));
	CodeBytes(0x77);

	DebugAsm("dpps $,$,0x77",
		TranslateValue({ out, value.type }),
		TranslateValue({ out, value.type }));

	CodeBytes({ 0xF3, 0x0F, 0x51 });
	CodeBytes(ConstructModRM(out, out));

	DebugAsm("sqrtss $,$",
		TranslateValue({ out, value.type }),
		TranslateValue({ out, value.type }));
}

void CodeGenerator::GenerateDot3(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
	assert(lhs.type->GetType() == BuiltinTypeType::Vec4);
	assert(rhs.type->GetType() == BuiltinTypeType::Vec4);

	OperandAssistant assitant(this, out, BuiltinType::Get(BuiltinTypeType::Float));

	if (out != lhs.location)
	{
		if (out == rhs.location)
			std::swap(lhs, rhs);
		else
			GenerateWrite({ out, lhs.type }, lhs);
	}

	CodeBytes({ 0x66, 0x0F, 0x3A, 0x40 });
	CodeBytes(ConstructModRM(out, rhs.location));
	CodeBytes(0x77);

	DebugAsm("dpps $,$,0x77",
		TranslateValue({ out, lhs.type }),
		TranslateValue(rhs));
}

void CodeGenerator::GenerateClamp(ValueDescription value, ValueDescription min, ValueDescription max,
	SymbolLocation out)
{
	assert(value.type->GetType() == BuiltinTypeType::Float);
	assert(min.type->GetType() == BuiltinTypeType::Float);
	assert(max.type->GetType() == BuiltinTypeType::Float);

	OperandAssistant assitant(this, out, value.type);

	GenerateWrite({ out, value.type }, value);

	CodeBytes({ 0x0F, 0x2F });
	CodeBytes(ConstructModRM(out, min.location));

	DebugAsm("comiss $,$",
		TranslateValue({ out, value.type }),
		TranslateValue(min));

	CodeBytes({ 0x73, 0x00 });

	JumpPatcher maxTest(&m_currentFunctionCode, -1);

	DebugAsm("jae x");

	GenerateWrite({ out, value.type }, min);

	CodeBytes({ 0xEB, 0x00 });

	JumpPatcher endClamp(&m_currentFunctionCode, -1);

	DebugAsm("jmp x");

	maxTest.PatchToHere();

	CodeBytes({ 0x0F, 0x2F });
	CodeBytes(ConstructModRM(out, max.location));

	DebugAsm("comiss $,$",
		TranslateValue({ out, value.type }),
		TranslateValue(max));

	CodeBytes({ 0x76, 0x00 });

	JumpPatcher isValue(&m_currentFunctionCode, -1);

	DebugAsm("jbe x");

	GenerateWrite({ out, value.type }, max);

	CodeBytes({ 0xEB, 0x00 });

	JumpPatcher endClamp2(&m_currentFunctionCode, -1);

	DebugAsm("jmp x");

	isValue.PatchToHere();

	GenerateWrite({ out, value.type }, value);

	endClamp.PatchToHere();
	endClamp2.PatchToHere();
}

void CodeGenerator::GenerateMultiplyMatrixMatrix(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
	assert(lhs.location.InMemory() && rhs.location.InMemory());
	assert(lhs.type->IsMatrix() && rhs.type->IsMatrix());
	assert(lhs.type->GetElementType() == rhs.type->GetElementType());

	std::unique_ptr<Layout::TemporaryRegister> xmm0 = m_layout.GetFreeXmmRegister();
	std::unique_ptr<Layout::TemporaryRegister> xmm1 = m_layout.GetFreeXmmRegister();
	std::unique_ptr<Layout::TemporaryRegister> xmm2 = m_layout.GetFreeXmmRegister();
	std::unique_ptr<Layout::TemporaryRegister> xmm3 = m_layout.GetFreeXmmRegister();

	BuiltinType * vectorType = rhs.type->GetElementType();
	ValueDescription currentRow = { rhs.location, vectorType };

	GenerateWrite({ xmm0->Location(), vectorType }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateWrite({ xmm1->Location(), vectorType }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateWrite({ xmm2->Location(), vectorType }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateWrite({ xmm3->Location(), vectorType }, currentRow);
	currentRow.location.m_data += 4 * 4;

	currentRow = { lhs.location, vectorType->GetElementType() };

	for (int i = 0; i < 4; ++i)
	{
		Layout::StackLayout stack = m_layout.TemporaryLayout();

		SymbolLocation lhsRow0 = GenerateExplodeFloat(stack, currentRow);
		currentRow.location.m_data += 4;

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			SymbolLocation lhsRow1 = GenerateExplodeFloat(subStack, currentRow);
			currentRow.location.m_data += 4;

			GenerateInstruction(true, instruction::Multiply,
				{ lhsRow0, vectorType }, { xmm0->Location(), vectorType }, lhsRow0);

			GenerateInstruction(true, instruction::Multiply,
				{ lhsRow1, vectorType }, { xmm1->Location(), vectorType }, lhsRow1);

			GenerateInstruction(true, instruction::Add, { lhsRow0, vectorType }, { lhsRow1, vectorType }, lhsRow0);
		}

		{
			Layout::StackLayout subStack = m_layout.TemporaryLayout();

			SymbolLocation lhsRow1 = GenerateExplodeFloat(subStack, currentRow);
			currentRow.location.m_data += 4;

			SymbolLocation lhsRow2 = GenerateExplodeFloat(subStack, currentRow);
			currentRow.location.m_data += 4;

			GenerateInstruction(true, instruction::Multiply,
				{ lhsRow1, vectorType }, { xmm2->Location(), vectorType }, lhsRow1);

			GenerateInstruction(true, instruction::Multiply,
				{ lhsRow2, vectorType }, { xmm3->Location(), vectorType }, lhsRow2);

			GenerateInstruction(true, instruction::Add, { lhsRow1, vectorType }, { lhsRow2, vectorType }, lhsRow1);
			GenerateInstruction(true, instruction::Add, { lhsRow0, vectorType }, { lhsRow1, vectorType }, lhsRow0);
		}

		GenerateWrite({ out, vectorType }, { lhsRow0, vectorType });
		out.m_data += 4 * 4;
	}
}

void CodeGenerator::GenerateMultiplyMatrixVector(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
	assert(lhs.location.InMemory());
	assert(lhs.type->IsMatrix());
	assert(rhs.type->IsVector());
	assert(lhs.type->GetElementType() == rhs.type);

	std::unique_ptr<Layout::TemporaryRegister> xmm0;
	std::unique_ptr<Layout::TemporaryRegister> xmm1 = m_layout.GetFreeXmmRegister();
	std::unique_ptr<Layout::TemporaryRegister> xmm2 = m_layout.GetFreeXmmRegister();
	std::unique_ptr<Layout::TemporaryRegister> xmm3;

	SymbolLocation xmm0Location;
	bool skipOutWrite = false;

	if (out.m_type == SymbolLocation::XmmRegister)
	{
		xmm0Location = out;
		skipOutWrite = true;
	}
	else
	{
		xmm0 = m_layout.GetFreeXmmRegister();
		xmm0Location = xmm0->Location();
	}

	SymbolLocation vectorLocation;

	if (rhs.location.InMemory())
	{
		xmm3 = m_layout.GetFreeXmmRegister();
		vectorLocation = xmm3->Location();
		GenerateWrite({ vectorLocation, rhs.type }, rhs);
	}
	else
	{
		vectorLocation = rhs.location;
	}

	ValueDescription vectorValue = { vectorLocation, rhs.type };
	ValueDescription currentRow = { lhs.location, rhs.type };

	GenerateWrite({ xmm0Location, rhs.type }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateWrite({ xmm1->Location(), rhs.type }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateInstruction(true, instruction::Multiply, { xmm0Location, rhs.type }, vectorValue, xmm0Location);
	GenerateInstruction(true, instruction::Multiply, { xmm1->Location(), rhs.type }, vectorValue, xmm1->Location());

	auto WriteHadd = [this, &rhs](SymbolLocation first, SymbolLocation second)
	{
		CodeBytes({ 0xF2, 0x0F, 0x7C });
		CodeBytes(ConstructModRM(first, second));

		DebugAsm("haddps $,$",
			TranslateValue({ first, rhs.type }),
			TranslateValue({ second, rhs.type }));
	};

	WriteHadd(xmm0Location, xmm1->Location());

	GenerateWrite({ xmm1->Location(), rhs.type }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateWrite({ xmm2->Location(), rhs.type }, currentRow);
	currentRow.location.m_data += 4 * 4;

	GenerateInstruction(true, instruction::Multiply, { xmm1->Location(), rhs.type }, vectorValue, xmm1->Location());
	GenerateInstruction(true, instruction::Multiply, { xmm2->Location(), rhs.type }, vectorValue, xmm2->Location());

	WriteHadd(xmm1->Location(), xmm2->Location());
	WriteHadd(xmm0Location, xmm1->Location());

	if (! skipOutWrite)
	{
		GenerateWrite({ out, rhs.type }, { xmm0Location, rhs.type });
	}
}

void CodeGenerator::GenerateMultiplyVectorScalar(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
	if (rhs.type->GetType() != BuiltinTypeType::Float)
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	BuiltinType * vectorType = lhs.type;

	// make sure out is in register
	OperandAssistant assistant(this, out, vectorType);

	Layout::StackLayout stack = m_layout.TemporaryLayout();

	// expode rhs into 4
	SymbolLocation scalars = GenerateExplodeFloat(stack, rhs);

	// out contains lhs
	// XXX: it's important the exploding happens before as rhs might use the same temporary as out
	if (lhs.location != out)
	{
		GenerateWrite({ out, vectorType }, lhs);
	}

	CodeBytes({ 0x0F, 0x59 });
	CodeBytes(ConstructModRM(out, scalars));

	DebugAsm("mulps $,$",
		TranslateValue({ out, vectorType }),
		TranslateValue({ scalars, vectorType }));
}

void CodeGenerator::GenerateInstruction(bool commutitive, const Instruction & instruction, ValueDescription lhs,
	ValueDescription rhs, SymbolLocation out)
{
	if (lhs.type != rhs.type)
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	BuiltinType * type = lhs.type;

	// make sure out is in a register
	OperandAssistant assistant(this, out, type);

	// make out equal to lhs (or rhs if commutitive)
	if (lhs.location != out)
	{
		if (commutitive && rhs.location == out)
		{
			std::swap(lhs, rhs);
		}
		else
		{
			GenerateWrite({ out, type }, lhs);
		}
	}

	const Instruction::Variant variant = instruction.GetVariant(type);

	CodeBytes(variant.bytes);
	CodeBytes(ConstructModRM(out, rhs.location));

	DebugAsm(variant.name + " $,$",
		TranslateValue({ out, type }),
		TranslateValue(rhs));
}

void CodeGenerator::GenerateVectorInstruction(bool commutitive, const Instruction & instruction, ValueDescription lhs,
	ValueDescription rhs, SymbolLocation out)
{
	if (lhs.type != rhs.type)
	{
		throw std::runtime_error("malformed syntax tree");
	}

	BuiltinType * type = lhs.type;

	const uint32_t size = type->GetSize();

	assert(size % 4 == 0);

	for (uint32_t i = 0; i < size; i += 4 * 4)
	{
		SymbolLocation sourceLhs = lhs.location;
		sourceLhs.m_data = lhs.location.m_data + i;

		SymbolLocation sourceRhs = rhs.location;
		sourceRhs.m_data = rhs.location.m_data + i;

		assert(i == 0 || (sourceLhs.InMemory() && sourceRhs.InMemory()));

		BuiltinType * elementType = type->GetElementType();

		// XXX: get the basic vector type
		BuiltinType * vectorType = (elementType->IsVector() ? elementType : type);

		SymbolLocation location = out;
		location.m_data = out.m_data + i;

		assert(i == 0 || location.InMemory());

		OperandAssistant assistant(this, location, vectorType);

		GenerateInstruction(commutitive, instruction, { sourceLhs, vectorType }, { sourceRhs, vectorType }, location);
	}
}

SymbolLocation CodeGenerator::GenerateExplodeFloat(Layout::StackLayout & stack, ValueDescription & float_)
{
	assert(float_.type->GetType() == BuiltinTypeType::Float);

	SymbolLocation scalars = stack.PlaceTemporary(BuiltinType::Get(BuiltinTypeType::Vec4));

	if (scalars.InMemory())
	{
		SymbolLocation l = scalars;

		for (int i = 0; i < 4; ++i)
		{
			GenerateWrite({ l, float_.type }, float_);
			l.m_data += 4;
		}
	}
	else
	{
		GenerateWrite({ scalars, float_.type }, float_);

		CodeBytes({ 0x0F, 0xC6 });
		CodeBytes(ConstructModRM(scalars, scalars));
		CodeBytes(0x00);

		DebugAsm("shufps $,$,0",
			TranslateValue({ scalars, float_.type }),
			TranslateValue({ scalars, float_.type }));
	}

	return scalars;
}

SymbolLocation CodeGenerator::GetConstantFloat(float value)
{
	auto iter = m_constantFloats.find(value);

	if (iter == m_constantFloats.end())
	{
		SymbolLocation location = m_layout.PlaceGlobalFloatInMemory();

		m_constantFloats[value] = location;

		return location;
	}

	return iter->second;
}

SymbolLocation CodeGenerator::GetConstantVector(std::tuple<float, float, float, float> value)
{
	auto iter = m_constantVectors.find(value);

	if (iter == m_constantVectors.end())
	{
		SymbolLocation location = m_layout.PlaceGlobalVectorInMemory();

		m_constantVectors[value] = location;

		return location;
	}

	return iter->second;
}

namespace
{
	uint8_t MakeModRM(uint8_t mod, uint8_t r, uint8_t rm)
	{
		uint8_t modrm = (mod << 6);
		modrm |= (r & 7) << 3;
		modrm |= (rm & 7);
		return modrm;
	}
}

std::vector<uint8_t> CodeGenerator::ConstructModRM(const SymbolLocation & target, const SymbolLocation & source)
{
	if (target.m_type == SymbolLocation::GlobalMemory)
	{
		uint32_t value = target.m_data + m_globalMemory;
		uint8_t * disp = (uint8_t*)&value;
		return { MakeModRM(0x0, source.m_data, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&target.m_data;
		return { MakeModRM(0x2, source.m_data, 0x6), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::IndirectRegister)
	{
		return { MakeModRM(0x0, source.m_data, target.m_data) };
	}
	else if (source.m_type == SymbolLocation::GlobalMemory)
	{
		uint32_t value = source.m_data + m_globalMemory;
		uint8_t * disp = (uint8_t*)&value;
		return { MakeModRM(0x0, target.m_data, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (source.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&source.m_data;
		return { MakeModRM(0x2, target.m_data, 0x6), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (source.m_type == SymbolLocation::IndirectRegister)
	{
		return { MakeModRM(0x0, target.m_data, source.m_data) };
	}
	else
	{
		return { MakeModRM(0x3, target.m_data, source.m_data) };
	}
}

std::vector<uint8_t> CodeGenerator::ConstructModRM(const SymbolLocation & target, uint8_t r)
{
	if (target.m_type == SymbolLocation::GlobalMemory)
	{
		uint32_t value = target.m_data + m_globalMemory;
		uint8_t * disp = (uint8_t*)&value;
		return { MakeModRM(0x0, r, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&target.m_data;
		return { MakeModRM(0x2, r, 0x6), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::IndirectRegister)
	{
		return{ MakeModRM(0x0, r, target.m_data) };
	}
	else
	{
		return { MakeModRM(0x3, r, target.m_data) };
	}
}

std::string CodeGenerator::TranslateValue(const ValueDescription & value)
{
	if (value.location.InMemory())
	{
		if (value.location.m_type == SymbolLocation::GlobalMemory)
		{
			Symbol * symbol = m_symbolTable.ResolveAddress(value.location.m_data, nullptr);

			if (symbol)
				return "[" + symbol->GetName() + "]";
			else
				return "[" + AsHex(value.location.m_data) + "]";
		}
		else if (value.location.m_type == SymbolLocation::LocalMemory)
		{
			Symbol * symbol = m_symbolTable.ResolveAddress(value.location.m_data, m_currentFunction);

			if (symbol)
				return "[" + symbol->GetName() + "]";
			else
				return "[rsi + " + std::to_string(value.location.m_data) + "]";
		}
		else if (value.location.m_type == SymbolLocation::IndirectRegister)
		{
			return "[" + RegToStr(value.location.m_data) + "]";
		}
	}

	if (value.type->IsVector() || value.type->GetType() == BuiltinTypeType::Float)
	{
			return XRegToStr(value.location.m_data);
	}

	return RegToStr(value.location.m_data);
}

void CodeGenerator::CodeBytes(uint8_t byte)
{
	m_currentFunctionCode.m_bytes.push_back(byte);
}

void CodeGenerator::CodeBytes(const std::initializer_list<uint8_t> & bytes)
{
	m_currentFunctionCode.m_bytes.insert(m_currentFunctionCode.m_bytes.end(), bytes);
}

void CodeGenerator::CodeBytes(const std::vector<uint8_t> & bytes)
{
	m_currentFunctionCode.m_bytes.insert(m_currentFunctionCode.m_bytes.end(), bytes.begin(), bytes.end());
}

template<typename T, typename... U>
void CodeGenerator::DebugAsm(const std::string & format, T && value, U &&... args)
{
	std::ostringstream oss;

	std::string::size_type pos = format.find('$');

	if (pos != std::string::npos)
	{
		oss << format.substr(0, pos);
		oss << value;
		m_currentFunctionCode.m_asm += oss.str();
		DebugAsm(format.substr(pos+1), std::forward<U>(args)...);
		return;
	}

	m_currentFunctionCode.m_asm += format + "\n";
}

void CodeGenerator::DebugAsm(const std::string & format)
{
	m_currentFunctionCode.m_asm += format + "\n";
}
