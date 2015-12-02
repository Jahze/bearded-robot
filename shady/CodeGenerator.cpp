#include <cassert>
#include <sstream>
#include "br.h"
#include "BuiltinTypes.h"
#include "CodeGenerator.h"
#include "ProgramContext.h"
#include "SymbolTable.h"
#include "SyntaxTree.h"

// when spilling registers of globals/locals we could
//   - allocate memory upfront for all of them (don't copy just reserve)
//   - when we spill copy to memory and update the SymbolLocation
//   - THEN don't have to put it back right afterwards

namespace
{
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
}

namespace instruction
{
	Instruction Multiply
	{
		"imul",		{ 0x0F, 0xAF },
		"mulss",	{ 0xF3, 0x0F, 0x59 },
		"",			{}
	};

	Instruction Divide
	{
		"",			{},
		"divss",	{ 0xF3, 0x0F, 0x5E },
		"",			{}
	};
}

class OperandAssistant
{
public:
	OperandAssistant(CodeGenerator *generator, SymbolLocation & location, BuiltinType * type)
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
	}

	~OperandAssistant()
	{
		// TODO : write register back to memory
	}

private:
	std::unique_ptr<Layout::TemporaryRegister> m_register;
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

void CodeGenerator::Generate(SyntaxNode * root)
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

	assert(m_currentFunction);

	m_layout.PlaceParametersAndLocals(m_currentFunction);

	SyntaxNode * statements = functionNode->m_nodes[2].get();

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

		case SyntaxNodeType::Return:
		default:
			throw std::runtime_error("malformed syntax tree");
		}
	}

	m_layout.RelinquishParametersAndLocals(m_currentFunction);

	m_functions[m_currentFunction->GetName()] = m_currentFunctionCode;

	m_currentFunction = nullptr;
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
		return ProcessMultiply(stack, expression);

	case SyntaxNodeType::AddAssign:
	case SyntaxNodeType::SubtractAssign:
	case SyntaxNodeType::MultiplyAssign:
	case SyntaxNodeType::DivideAssign:
	case SyntaxNodeType::Divide:
	case SyntaxNodeType::Add:
	case SyntaxNodeType::Subtract:
	case SyntaxNodeType::Subscript:

	case SyntaxNodeType::Negate:
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
		SymbolLocation location;

		auto iter = m_constants.find(value);

		if (iter == m_constants.end())
		{
			location = m_layout.PlaceGlobalInMemory(value);
			m_constants[value] = location;
		}
		else
		{
			location = iter->second;
		}

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

CodeGenerator::ValueDescription CodeGenerator::ProcessMultiply(Layout::StackLayout & stack, SyntaxNode * multiply)
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
			SymbolLocation out = stack.PlaceTemporary(rhs.type);
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
			SymbolLocation out = stack.PlaceTemporary(lhs.type);
			GenerateMultiplyVectorScalar(lhs, rhs, out);
			return { out, lhs.type };
		}
	}
	else if (lhs.type->IsScalar())
	{
		SymbolLocation out = stack.PlaceTemporary(rhs.type);
		GenerateMultiplyScalarScalar(lhs, rhs, out);
		return { out, lhs.type };
	}

	throw std::runtime_error("malformed syntax tree");
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

	m_currentFunctionCode.m_asm += "mov " + TranslateValue(target) + ", " + std::to_string(literal) + "\n";
}

void CodeGenerator::GenerateWrite(const ValueDescription & target, const ValueDescription & source)
{
	if (target.type != source.type)
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	// TODO : global memory is not [rsi+x] but abs value that must be relocated

	if (target.location.InMemory() && source.location.InMemory())
	{
		// TODO : get layout->hasfreeregister and spill otherwise
		auto reg = m_layout.GetFreeRegister();

		const uint32_t size = source.type->GetSize();

		for (uint32_t i = 0; i < size; i += 4)
		{
			m_currentFunctionCode.m_asm += "mov " + RegToStr(reg->Register()) + ", [rsi+"
				+ std::to_string(source.location.m_data + i) + "]\n";

			m_currentFunctionCode.m_asm += "mov [rsi+" + std::to_string(target.location.m_data + i)
				+ "], " + std::to_string(reg->Register()) + "\n";
		}
	}
	else
	{
		BuiltinType *type = target.type;

		std::string instruction;
		std::vector<uint8_t> bytes;

		if (type->IsVector())
		{
			instruction = "vmovaps ";

			if (target.location.InMemory())
				CodeBytes({ 0x0f, 0x29 });
			else
				CodeBytes({ 0x0f, 0x28 });
		}
		else if (type->GetType() == BuiltinTypeType::Float)
		{
			instruction = "movss ";

			if (target.location.InMemory())
				CodeBytes({ 0xf3, 0x0f, 0x11 });
			else
				CodeBytes({ 0xf3, 0x0f, 0x10 });
		}
		else
		{
			instruction = "mov ";

			if (target.location.InMemory())
				CodeBytes({ 0x89 });
			else
				CodeBytes({ 0x8b });
		}

		CodeBytes(ConstructModRM(target.location, source.location));

		m_currentFunctionCode.m_asm += instruction + TranslateValue(target) + ", " + TranslateValue(source) + "\n";
	}
}

void CodeGenerator::GenerateMultiplyMatrixMatrix(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
}

void CodeGenerator::GenerateMultiplyMatrixVector(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
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
	SymbolLocation scalars = stack.PlaceTemporary(vectorType);

	if (scalars.InMemory())
	{
		SymbolLocation l = scalars;

		for (int i = 0; i < 4; ++i)
		{
			GenerateWrite({ l, rhs.type }, rhs);
			l.m_data += 4;
		}
	}
	else
	{
		GenerateWrite({ scalars, rhs.type }, rhs);

		CodeBytes({ 0x0F, 0xC6 });
		CodeBytes(ConstructModRM(scalars, scalars));
		CodeBytes(0x00);

		m_currentFunctionCode.m_asm += "shufps " + TranslateValue({ scalars, rhs.type }) +
			", " + TranslateValue({ scalars, rhs.type }) + ", 0\n";
	}

	// out contains lhs
	// XXX: it's important the exploding happens before as rhs might use the same temporary as out
	if (lhs.location != out)
	{
		GenerateWrite({ out, vectorType }, lhs);
	}

	CodeBytes({ 0x0F, 0x59 });
	CodeBytes(ConstructModRM(out, scalars));

	m_currentFunctionCode.m_asm += "mulps " + TranslateValue({ out, vectorType }) +
		", " + TranslateValue({ scalars, vectorType }) + ", 0\n";
}

void CodeGenerator::GenerateMultiplyScalarScalar(ValueDescription lhs, ValueDescription rhs, SymbolLocation out)
{
	if (lhs.type != rhs.type)
	{
		// TODO : convert between float and int
		throw std::runtime_error("malformed syntax tree");
	}

	BuiltinType * type = lhs.type;

	// make sure out is in a register
	OperandAssistant assistant(this, out, type);

	// make out equal to one side of the multiplication (if it isn't)
	if (lhs.location != out && rhs.location != out)
	{
		GenerateWrite({ out, type }, lhs);
	}
	else if (lhs.location != out && rhs.location == out)
	{
		// multiplication is commutitive
		std::swap(lhs, rhs);
	}

	std::string instruction;

	if (type->GetType() == BuiltinTypeType::Float)
	{
		instruction = "mulss ";

		CodeBytes({0xF3, 0x0F, 0x59});
	}
	else
	{
		instruction = "imul ";

		CodeBytes({ 0x0F, 0xAF });
	}

	CodeBytes(ConstructModRM(out, rhs.location));

	m_currentFunctionCode.m_asm += instruction + TranslateValue({out, type}) + ", " + TranslateValue(rhs) + "\n";
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

	m_currentFunctionCode.m_asm += variant.name + TranslateValue({ out, type }) + ", " + TranslateValue(rhs) + "\n";
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
		uint8_t * disp = (uint8_t*)&target.m_data;
		return { MakeModRM(0x0, source.m_data, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&target.m_data;
		return { MakeModRM(0x2, source.m_data, 0x6), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (source.m_type == SymbolLocation::GlobalMemory)
	{
		uint8_t * disp = (uint8_t*)&source.m_data;
		return { MakeModRM(0x0, target.m_data, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (source.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&source.m_data;
		return { MakeModRM(0x2, target.m_data, 0x6), disp[0], disp[1], disp[2], disp[3] };
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
		uint8_t * disp = (uint8_t*)&target.m_data;
		return{ MakeModRM(0x0, r, 0x5), disp[0], disp[1], disp[2], disp[3] };
	}
	else if (target.m_type == SymbolLocation::LocalMemory)
	{
		uint8_t * disp = (uint8_t*)&target.m_data;
		return{ MakeModRM(0x2, r, 0x6), disp[0], disp[1], disp[2], disp[3] };
	}
	else
	{
		return{ MakeModRM(0x3, r, target.m_data) };
	}
}

std::string CodeGenerator::TranslateValue(const ValueDescription & value)
{
	if (value.location.InMemory())
	{
		if (value.location.m_type == SymbolLocation::GlobalMemory)
		{
			return "[" + AsHex(value.location.m_data) + "]";
		}
		else
		{
			return "[rsi + " + std::to_string(value.location.m_data) + "]";
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
