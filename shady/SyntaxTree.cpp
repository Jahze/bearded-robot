#include <cassert>
#include <functional>
#include "BuiltinTypes.h"
#include "ProgramContext.h"
#include "SyntaxTree.h"
#include "Tokens.h"

namespace
{
	bool IsType(int tokenType)
	{
		return tokenType == static_cast<int>(TokenType::Float)
			|| tokenType == static_cast<int>(TokenType::Int)
			|| tokenType == static_cast<int>(TokenType::Bool)
			|| tokenType == static_cast<int>(TokenType::Void)
			|| tokenType == static_cast<int>(TokenType::Vec3)
			|| tokenType == static_cast<int>(TokenType::Vec4)
			|| tokenType == static_cast<int>(TokenType::Mat3x3)
			|| tokenType == static_cast<int>(TokenType::Mat4x4);
	}

	bool IsLiteral(int tokenType)
	{
		return tokenType == static_cast<int>(TokenType::FloatLiteral)
			|| tokenType == static_cast<int>(TokenType::BoolLiteral)
			|| tokenType == static_cast<int>(TokenType::IntLiteral);
	}

	bool IsTypeDecorator(int tokenType)
	{
		return tokenType == static_cast<int>(TokenType::Export)
			|| tokenType == static_cast<int>(TokenType::Interpolated)
			|| tokenType == static_cast<int>(TokenType::Uniform);
	}

	bool Is(int tokenType, TokenType tokenType_)
	{
		return tokenType == static_cast<int>(tokenType_);
	}

	bool CheckAdditiveExpressionCompatibility(BuiltinType * lhs, BuiltinType * rhs)
	{
		if (lhs->IsVector() && rhs->IsVector())
		{
			return lhs == rhs;
		}

		if (lhs->IsScalar() && rhs->IsScalar())
			return true;

		return false;
	}

	bool CheckMultiplicativeExpressionCompatibility(BuiltinType * lhs, BuiltinType * rhs)
	{
		if (lhs->IsVector() && rhs->IsVector())
		{
			// matrix * matrix
			if (lhs->IsMatrix() && lhs == rhs)
				return true;

			// matrix * vector
			if (lhs->GetElementType() == rhs)
				return true;

			return false;
		}

		// vector * scalar
		if (lhs->IsVector() && rhs->IsScalar())
		{
			if (lhs->GetElementType()->IsScalar())
				return true;

			return false;
		}

		// scalar * scalar
		if (lhs->IsScalar() && rhs->IsScalar())
			return true;

		return false;
	}

	bool CheckEqualityExpressionCompatibility(BuiltinType * lhs, BuiltinType * rhs)
	{
		if (lhs->IsScalar() && rhs->IsScalar())
			return true;

		// TODO : shouldn't allow compare of void/void function/function

		return lhs == rhs;
	}

	bool CheckAssignmentExpressionCompatibility(BuiltinType * lhs, BuiltinType * rhs)
	{
		if (lhs->IsVector() && rhs->IsVector())
		{
			return lhs == rhs;
		}

		if (lhs->IsScalar() && rhs->IsScalar())
			return true;

		if (lhs->GetType() == BuiltinTypeType::Bool && rhs->GetType() == BuiltinTypeType::Bool)
			return true;

		return false;
	}

	bool CheckExpressionsAreScalar(BuiltinType * lhs, BuiltinType * rhs)
	{
		return lhs->IsScalar() && rhs->IsScalar();
	}

	BuiltinType *ResultOf(BuiltinType * lhs, BuiltinType * rhs)
	{
		if (lhs->IsVector())
		{
			if (rhs->IsScalar())
				return lhs;

			if (lhs->GetElementType()->IsVector())
				return rhs;

			return lhs;
		}

		if (lhs->IsScalar())
		{
			if (lhs->GetType() == BuiltinTypeType::Float || rhs->GetType() == BuiltinTypeType::Float)
				return BuiltinType::Get(BuiltinTypeType::Float);
		}

		return lhs;
	}

	bool IsConstantExpression(SyntaxNode *node)
	{
		if (node->m_type == SyntaxNodeType::Name)
			return false;

		for (auto && child : node->m_nodes)
		{
			if (! IsConstantExpression(child.get()))
				return false;
		}

		return true;
	}

	void CheckFunctionArgumentList(tokeniser::Token token, Function * function,
		std::vector<std::pair<std::unique_ptr<SyntaxNode>, BuiltinType*>> & arguments)
	{
		const std::vector<Symbol*> & parameters = function->GetParameters();

		if (parameters.size() > arguments.size())
			throw SyntaxException(token, "Too few arguments in function call");

		if (parameters.size() < arguments.size())
			throw SyntaxException(token, "Too many arguments in function call");

		std::size_t i = 0;

		for (auto && argument : arguments)
		{
			BuiltinType * type = parameters[i++]->GetType();

			if (! CheckAssignmentExpressionCompatibility(argument.second, type))
				throw SyntaxException(token, "Cannot convert argument " + std::to_string(i) + " from '" +
					argument.second->GetName() + "' to '" + type->GetName() + "'");
		}
	}
}

class TokenIterator
{
public:
	TokenIterator(const std::vector<tokeniser::Token> & tokens)
		: m_tokens(tokens)
		, m_cursor(0u)
	{ }

	bool HasMore() const
	{
		return m_cursor < m_tokens.size();
	}

	tokeniser::Token Next()
	{
		if (! HasMore())
			throw SyntaxException(EndOfInput(), "Unexpected end of input");

		return m_tokens[m_cursor++];
	}

	tokeniser::Token NextOrEndToken()
	{
		if (HasMore())
			return m_tokens[m_cursor++];

		return EndOfInput();
	}

	int Peek()
	{
		if (! HasMore())
			throw SyntaxException(EndOfInput(), "Unexpected end of input");

		return m_tokens[m_cursor].m_type;
	}

private:
	tokeniser::Token EndOfInput()
	{
		tokeniser::Token token;
		token.m_type = static_cast<int>(TokenType::EndOfInput);
		return token;
	}

private:
	const std::vector<tokeniser::Token> & m_tokens;
	uint32_t m_cursor;
};

SyntaxTree::SyntaxTree(const ProgramContext & context)
{
	context.ApplyToSymbolTable(m_symbolTable, m_functionTable);
}

bool SyntaxTree::Parse(const std::vector<tokeniser::Token> & tokens, std::string & error, tokeniser::Token & errorToken)
{
	TokenIterator iterator(tokens);
	m_iterator = &iterator;

	m_root.reset(new SyntaxNode(nullptr, SyntaxNodeType::Root));

	while (iterator.HasMore())
	{
		try
		{
			FunctionOrVariable();
		}
		catch (const SyntaxException & e)
		{
			error = e.m_message;
			errorToken = e.m_token;
			return false;
		}
	}

	return true;
}

Symbol * SyntaxTree::AddSymbol(const tokeniser::Token & typeToken, const tokeniser::Token & nameToken, ScopeType scopeType,
	SymbolType symbolType)
{
	BuiltinType *type;

	if (symbolType == SymbolType::Function)
		type = BuiltinType::Get(BuiltinTypeType::Function);
	else
		type = BuiltinType::Get(BuiltinType::FromName(typeToken.m_data));

	assert(type);

	Symbol *symbol = m_symbolTable.AddSymbol(nameToken.m_data, scopeType, symbolType, type, m_currentFunction);

	if (! symbol)
	{
		symbol = m_symbolTable.FindSymbol(nameToken.m_data, m_currentFunction);

		assert(symbol);

		if (symbol->IsIntrinsic())
			throw SyntaxException(nameToken, "Symbol '" + nameToken.m_data + "' shadows intrinsic");

		throw SyntaxException(nameToken, "Multiple definitions of symbol '" + nameToken.m_data + "'");
	}

	symbol->SetDeclarationLocation(nameToken.m_line, nameToken.m_column);

	if (symbolType == SymbolType::Function)
		m_currentFunction = m_functionTable.AddFunction(symbol);

	return symbol;
}

uint32_t SyntaxTree::CollectDeclarationDecorators()
{
	TokenType tokenType = static_cast<TokenType>(m_iterator->Peek());

	switch (tokenType)
	{
	case TokenType::Export:
		m_iterator->Next();
		return SyntaxNodeFlags::Export | CollectDeclarationDecorators();
	case TokenType::Interpolated:
		m_iterator->Next();
		return SyntaxNodeFlags::Interpolated | CollectDeclarationDecorators();
	case TokenType::Uniform:
		m_iterator->Next();
		return SyntaxNodeFlags::Uniform | CollectDeclarationDecorators();
	}

	return 0u;
}

void SyntaxTree::FunctionOrVariable()
{
	uint32_t flags = CollectDeclarationDecorators();

	tokeniser::Token typeToken = m_iterator->Next();

	if (! IsType(typeToken.m_type))
		throw SyntaxException(typeToken,
			flags == 0 ?
			"Expecting variable or function declaration" :
			"Expecting type name");

	tokeniser::Token nameToken = m_iterator->Next();

	if (! Is(nameToken.m_type, TokenType::Identifier))
		throw SyntaxException(nameToken, "Expecting identifier");

	int peeked = m_iterator->Peek();

	if (Is(peeked, TokenType::RoundBracketLeft))
	{
		if (flags & SyntaxNodeFlags::Uniform)
			throw SyntaxException(nameToken, "'uniform' cannot be applied to functions");

		if (flags & SyntaxNodeFlags::Interpolated)
			throw SyntaxException(nameToken, "'interpolated' cannot be applied to functions");

		SyntaxNode *function = m_root->AddChild(SyntaxNodeType::Function);
		function->m_flags = flags;
		function->m_data = nameToken.m_data;

		SyntaxNode *typeChild = function->AddChild(SyntaxNodeType::Type);
		typeChild->m_data = typeToken.m_data;

		AddSymbol(typeToken, nameToken, ScopeType::Global, SymbolType::Function);

		m_currentFunction->SetReturnType(BuiltinType::FromName(typeToken.m_data));
		m_currentFunction->SetExport(flags & SyntaxNodeFlags::Export);

		FunctionBody(function);

		return;
	}

	if (! Is(peeked, TokenType::Semicolon))
		throw SyntaxException(m_iterator->NextOrEndToken(), "Missing semicolon");

	if (Is(typeToken.m_type, TokenType::Void))
		throw SyntaxException(typeToken, "Variables cannot have a type of 'void'");

	SyntaxNode *variable = m_root->AddChild(SyntaxNodeType::GlobalVariable);
	variable->m_flags = flags;
	variable->m_data = nameToken.m_data;

	SyntaxNode *typeChild = variable->AddChild(SyntaxNodeType::Type);
	typeChild->m_data = typeToken.m_data;

	AddSymbol(typeToken, nameToken, ScopeType::Global, SymbolType::Variable);
}

void SyntaxTree::FunctionBody(SyntaxNode *function)
{
	assert(Is(m_iterator->Peek(), TokenType::RoundBracketLeft));

	m_iterator->Next();

	FunctionArguments(function);

	StatementList(function);

	m_currentFunction = nullptr;
}

void SyntaxTree::FunctionArguments(SyntaxNode *function)
{
	SyntaxNode *parameters = function->AddChild(SyntaxNodeType::FunctionParameters);

	if (! Is(m_iterator->Peek(), TokenType::RoundBracketRight))
	{
		while (true)
		{
			tokeniser::Token typeToken;
			tokeniser::Token nameToken;

			TypeAndIdentifier(parameters, SyntaxNodeType::FunctionParameter, typeToken, nameToken);

			Symbol * symbol = AddSymbol(typeToken, nameToken, ScopeType::Local, SymbolType::Variable);

			m_currentFunction->AddParameter(symbol);

			tokeniser::Token next = m_iterator->Next();

			if (Is(next.m_type, TokenType::Comma))
				continue;

			if (Is(next.m_type, TokenType::RoundBracketRight))
				break;

			throw SyntaxException(next, "Unexpected token in function parameter list");
		}
	}
	else
	{
		m_iterator->Next();
	}
}

SyntaxNode * SyntaxTree::TypeAndIdentifier(SyntaxNode *parent, SyntaxNodeType type)
{
	tokeniser::Token typeToken;
	tokeniser::Token nameToken;
	return TypeAndIdentifier(parent, type, typeToken, nameToken);
}

SyntaxNode * SyntaxTree::TypeAndIdentifier(SyntaxNode *parent, SyntaxNodeType type, tokeniser::Token &typeToken,
	tokeniser::Token &nameToken)
{
	typeToken = m_iterator->Next();

	if (! IsType(typeToken.m_type))
		throw SyntaxException(typeToken, "Expecting type name");

	nameToken = m_iterator->Next();

	if (! Is(nameToken.m_type, TokenType::Identifier))
		throw SyntaxException(nameToken, "Expecting identifier");

	if (Is(typeToken.m_type, TokenType::Void))
		throw SyntaxException(typeToken, "Variables cannot have a type of 'void'");

	SyntaxNode *node = parent->AddChild(type);
	node->m_data = nameToken.m_data;

	SyntaxNode *typeNode = node->AddChild(SyntaxNodeType::Type);
	typeNode->m_data = typeToken.m_data;

	return node;
}

void SyntaxTree::StatementList(SyntaxNode *parent)
{
	tokeniser::Token next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::CurlyBracketLeft))
		throw SyntaxException(next, "Expecting start of statement list '{'");

	bool isStatement = true;

	SyntaxNode *statements = parent->AddChild(SyntaxNodeType::StatementList);

	do
	{
		int peeked = m_iterator->Peek();

		if (IsType(peeked))
		{
			LocalVariable(statements);
		}
		else if (Is(peeked, TokenType::Identifier)
			|| Is(peeked, TokenType::Subtract)
			|| Is(peeked, TokenType::LogicalNot)
			|| Is(peeked, TokenType::RoundBracketLeft)
			|| Is(peeked, TokenType::FloatLiteral)
			|| Is(peeked, TokenType::BoolLiteral)
			|| Is(peeked, TokenType::IntLiteral))
		{
			Expression(statements);

			next = m_iterator->Next();

			if (! Is(next.m_type, TokenType::Semicolon))
				throw SyntaxException(next, "Missing semicolon");
		}
		else if (Is(peeked, TokenType::If))
		{
			IfStatement(statements);
		}
		else if (Is(peeked, TokenType::While))
		{
			WhileStatement(statements);
		}
		else if (Is(peeked, TokenType::For))
		{
			ForStatement(statements);
		}
		else if (Is(peeked, TokenType::Return))
		{
			ReturnStatement(statements);
		}
		else if (Is(peeked, TokenType::Semicolon))
		{
			m_iterator->Next();
		}
		else
		{
			isStatement = false;
		}
	}
	while (isStatement);

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::CurlyBracketRight))
		throw SyntaxException(next, "Expecting end of statement list '}'");
}

void SyntaxTree::LocalVariable(SyntaxNode *parent)
{
	tokeniser::Token typeToken;
	tokeniser::Token nameToken;

	SyntaxNode * variable = TypeAndIdentifier(parent, SyntaxNodeType::LocalVariable, typeToken, nameToken);

	if (Is(m_iterator->Peek(), TokenType::Assign))
	{
		tokeniser::Token next = m_iterator->Next();

		SyntaxNode * initializer = variable->AddChild(SyntaxNodeType::Initializer);

		Expression(initializer);

		BuiltinType *lhsType = BuiltinType::Get(BuiltinType::FromName(typeToken.m_data));

		if (lhsType != nullptr)
		{
			if (! CheckAssignmentExpressionCompatibility(lhsType, m_currentExpressionType))
				throw SyntaxException(next, "Cannot initialise '" + lhsType->GetName() + "' with '" +
					m_currentExpressionType->GetName() + "'");
		}
	}

	tokeniser::Token next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::Semicolon))
		throw SyntaxException(next, "Missing semicolon");

	Symbol * symbol = AddSymbol(typeToken, nameToken, ScopeType::Local, SymbolType::Variable);

	m_currentFunction->AddLocal(symbol);
}

void SyntaxTree::Expression(SyntaxNode *parent)
{
	std::unique_ptr<SyntaxNode> node = AssignmentExpression();
	SyntaxNode * expression = parent->AddChild(SyntaxNodeType::Expression);
	expression->AddChild(std::move(node));
}

std::unique_ptr<SyntaxNode> SyntaxTree::AssignmentExpression()
{
	std::unique_ptr<SyntaxNode> node = AdditiveExpression();

	auto ParseRhs = [&] (SyntaxNodeType type, std::function<bool(BuiltinType*, BuiltinType*)> check, bool mult)
	{
		tokeniser::Token next = m_iterator->Next();

		if (IsConstantExpression(node.get()))
			throw SyntaxException(next, "Assignment to constant expression");

		BuiltinType *lhsType = m_currentExpressionType;

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, type));
		operation->AddChild(std::move(node));
		operation->AddChild(RelationalExpression());

		if (! check(lhsType, m_currentExpressionType))
			throw SyntaxException(next, "Incompatible types for assignment '" + lhsType->GetName() +
				"' and '" + m_currentExpressionType->GetName() + "'");

		// TODO: the result of "x = y" is always typeof(x) ?

		BuiltinType *rhsType = ResultOf(lhsType, m_currentExpressionType);

		if (! CheckAssignmentExpressionCompatibility(lhsType, rhsType))
			throw SyntaxException(next, "Incompatible types for assignment '" + lhsType->GetName() +
				"' and '" + rhsType->GetName() + "'");

		m_currentExpressionType = rhsType;

		return operation;
	};

	switch (static_cast<TokenType>(m_iterator->Peek()))
	{
	case TokenType::Assign:
		return ParseRhs(SyntaxNodeType::Assign, CheckAssignmentExpressionCompatibility, false);

	case TokenType::AddAssign:
		return ParseRhs(SyntaxNodeType::AddAssign, CheckAdditiveExpressionCompatibility, false);

	case TokenType::SubtractAssign:
		return ParseRhs(SyntaxNodeType::SubtractAssign, CheckAdditiveExpressionCompatibility, false);

	case TokenType::MultiplyAssign:
		return ParseRhs(SyntaxNodeType::MultiplyAssign, CheckMultiplicativeExpressionCompatibility, true);

	case TokenType::DivideAssign:
		return ParseRhs(SyntaxNodeType::DivideAssign, CheckExpressionsAreScalar, false);
	}

	return node;
}

std::unique_ptr<SyntaxNode> SyntaxTree::RelationalExpression()
{
	std::unique_ptr<SyntaxNode> node = AdditiveExpression();

	auto ParseRhs = [&](SyntaxNodeType type, std::function<bool(BuiltinType*,BuiltinType*)> check)
	{
		tokeniser::Token next = m_iterator->Next();

		BuiltinType *lhsType = m_currentExpressionType;

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, type));
		operation->AddChild(std::move(node));
		operation->AddChild(AdditiveExpression());

		if (! check(lhsType, m_currentExpressionType))
			throw SyntaxException(next, "Cannot compare '" + lhsType->GetName() +
				"' and '" + m_currentExpressionType->GetName() + "'");

		m_currentExpressionType = BuiltinType::Get(BuiltinTypeType::Bool);

		return operation;
	};

	switch (static_cast<TokenType>(m_iterator->Peek()))
	{
	case TokenType::LogicalEquals:
		return ParseRhs(SyntaxNodeType::Equals, CheckEqualityExpressionCompatibility);

	case TokenType::LogicalNotEquals:
		return ParseRhs(SyntaxNodeType::NotEquals, CheckEqualityExpressionCompatibility);

	case TokenType::LogicalLess:
		return ParseRhs(SyntaxNodeType::Less, CheckExpressionsAreScalar);

	case TokenType::LogicalLessEquals:
		return ParseRhs(SyntaxNodeType::LessEquals, CheckExpressionsAreScalar);

	case TokenType::LogicalGreater:
		return ParseRhs(SyntaxNodeType::Greater, CheckExpressionsAreScalar);

	case TokenType::LogicalGreaterEquals:
		return ParseRhs(SyntaxNodeType::GreaterEquals, CheckExpressionsAreScalar);
	}

	return node;
}

std::unique_ptr<SyntaxNode> SyntaxTree::AdditiveExpression()
{
	std::unique_ptr<SyntaxNode> node = MultipicativeExpression();

	BuiltinType *lhsType = m_currentExpressionType;

	if (Is(m_iterator->Peek(), TokenType::Add))
	{
		tokeniser::Token next = m_iterator->Next();

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::Add));
		operation->AddChild(std::move(node));
		operation->AddChild(MultipicativeExpression());

		if (! CheckAdditiveExpressionCompatibility(lhsType, m_currentExpressionType))
			throw SyntaxException(next, "Cannot add '" + lhsType->GetName() +
				"' and '" + m_currentExpressionType->GetName() + "'");

		m_currentExpressionType = ResultOf(lhsType, m_currentExpressionType);

		return operation;
	}
	else if (Is(m_iterator->Peek(), TokenType::Subtract))
	{
		tokeniser::Token next = m_iterator->Next();

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::Subtract));
		operation->AddChild(std::move(node));
		operation->AddChild(MultipicativeExpression());

		if (! CheckAdditiveExpressionCompatibility(lhsType, m_currentExpressionType))
			throw SyntaxException(next, "Cannot subtract '" + lhsType->GetName() +
				"' and '" + m_currentExpressionType->GetName() + "'");

		m_currentExpressionType = ResultOf(lhsType, m_currentExpressionType);

		return operation;
	}

	return node;
}

std::unique_ptr<SyntaxNode> SyntaxTree::MultipicativeExpression()
{
	std::unique_ptr<SyntaxNode> node = UnaryOperatorExpression();

	BuiltinType * lhsType = m_currentExpressionType;

	while (Is(m_iterator->Peek(), TokenType::Multiply) || Is(m_iterator->Peek(), TokenType::Divide))
	{
		if (Is(m_iterator->Peek(), TokenType::Multiply))
		{
			tokeniser::Token next = m_iterator->Next();

			std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::Multiply));
			operation->AddChild(std::move(node));
			operation->AddChild(UnaryOperatorExpression());

			if (! CheckMultiplicativeExpressionCompatibility(lhsType, m_currentExpressionType))
				throw SyntaxException(next, "Cannot multiply '" + lhsType->GetName() +
					"' by '" + m_currentExpressionType->GetName() + "'");

			m_currentExpressionType = ResultOf(lhsType, m_currentExpressionType);

			node = std::move(operation);
		}
		else if (Is(m_iterator->Peek(), TokenType::Divide))
		{
			tokeniser::Token next = m_iterator->Next();

			std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::Divide));
			operation->AddChild(std::move(node));
			operation->AddChild(UnaryOperatorExpression());

			if (! CheckExpressionsAreScalar(lhsType, m_currentExpressionType))
				throw SyntaxException(next, "Cannot divide '" + lhsType->GetName() +
					"' by '" + m_currentExpressionType->GetName() + "'");

			m_currentExpressionType = ResultOf(lhsType, m_currentExpressionType);

			node = std::move(operation);
		}
	}

	return node;
}

std::unique_ptr<SyntaxNode> SyntaxTree::UnaryOperatorExpression()
{
	if (Is(m_iterator->Peek(), TokenType::Subtract))
	{
		tokeniser::Token next = m_iterator->Next();

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::Negate));
		operation->AddChild(PostfixExpression());

		if (! m_currentExpressionType->IsScalar() && ! m_currentExpressionType->IsVector())
			throw SyntaxException(next, "Unary '-' applied to invalid expression");

		return operation;
	}
	else if (Is(m_iterator->Peek(), TokenType::LogicalNot))
	{
		tokeniser::Token next = m_iterator->Next();

		std::unique_ptr<SyntaxNode> operation(new SyntaxNode(nullptr, SyntaxNodeType::LogicalNegate));
		operation->AddChild(PostfixExpression());

		if (m_currentExpressionType->GetType() != BuiltinTypeType::Bool)
			throw SyntaxException(next, "Unary '!' applied to non-boolean expression");

		return operation;
	}

	return PostfixExpression();
}

std::unique_ptr<SyntaxNode> SyntaxTree::PostfixExpression()
{
	std::unique_ptr<SyntaxNode> atom = ExpressionAtom();

	while (true)
	{
		int peeked = m_iterator->Peek();

		if (Is(peeked, TokenType::SquareBracketLeft))
		{
			tokeniser::Token token = m_iterator->Next();

			if (! m_currentExpressionType->IsVector())
				throw SyntaxException(token, "Type '" + m_currentExpressionType->GetName() + "' cannot be subscripted");

			BuiltinType * vectorType = m_currentExpressionType;

			std::unique_ptr<SyntaxNode> subscript = std::make_unique<SyntaxNode>(nullptr, SyntaxNodeType::Subscript);
			subscript->AddChild(std::move(atom));
			subscript->AddChild(AssignmentExpression());

			if (m_currentExpressionType->GetType() != BuiltinTypeType::Int)
				throw SyntaxException(token, "Subscript is not integral");

			token = m_iterator->Next();

			if (! Is(token.m_type, TokenType::SquareBracketRight))
				throw SyntaxException(token, "Expecting subscript end ']'");

			m_currentExpressionType = vectorType->GetElementType();
			atom = std::move(subscript);
		}
		else if (Is(peeked, TokenType::RoundBracketLeft))
		{
			tokeniser::Token start = m_iterator->Next();

			if (m_currentExpressionType->GetType() != BuiltinTypeType::Function)
				throw SyntaxException(start, "Cannot call expression of type '" +
					m_currentExpressionType->GetName() + "'");

			std::vector<std::pair<std::unique_ptr<SyntaxNode>, BuiltinType*>> arguments;

			Function * function = m_functionTable.FindFunction(atom->m_data);

			std::unique_ptr<SyntaxNode> call = std::make_unique<SyntaxNode>(nullptr, SyntaxNodeType::FunctionCall);
			call->AddChild(std::move(atom));

			if (Is(m_iterator->Peek(), TokenType::RoundBracketRight))
			{
				m_iterator->Next();
				CheckFunctionArgumentList(start, function, arguments);
				atom = std::move(call);
				m_currentExpressionType = function->GetReturnType();
				continue;
			}

			while (true)
			{
				std::unique_ptr<SyntaxNode> argument = AssignmentExpression();

				arguments.push_back(std::make_pair(std::move(argument), m_currentExpressionType));

				tokeniser::Token token = m_iterator->Next();

				if (Is(token.m_type, TokenType::RoundBracketRight))
				{
					break;
				}

				if (Is(token.m_type, TokenType::Comma))
				{
					continue;
				}

				throw SyntaxException(start, "Unfinished function call parameter list");
			}

			CheckFunctionArgumentList(start, function, arguments);

			for (auto && argument : arguments)
				call->AddChild(std::move(argument.first));

			atom = std::move(call);
			m_currentExpressionType = function->GetReturnType();
		}
		else
		{
			break;
		}
	}

	return atom;
}

std::unique_ptr<SyntaxNode> SyntaxTree::ExpressionAtom()
{
	tokeniser::Token token = m_iterator->Next();

	if (Is(token.m_type, TokenType::RoundBracketLeft))
	{
		std::unique_ptr<SyntaxNode> expression = AssignmentExpression();

		token = m_iterator->Next();

		if (! Is(token.m_type, TokenType::RoundBracketRight))
			throw SyntaxException(token, "Unclosed bracketed expression");

		return expression;
	}
	else if (Is(token.m_type, TokenType::Identifier))
	{
		std::unique_ptr<SyntaxNode> name = std::make_unique<SyntaxNode>(nullptr, SyntaxNodeType::Name);
		name->m_data = token.m_data;

		Symbol *symbol = m_symbolTable.FindSymbol(token.m_data, m_currentFunction);

		if (! symbol)
			throw SyntaxException(token, "Undefined symbol '" + token.m_data + "'");

		m_currentExpressionType = symbol->GetType();

		return name;
	}
	else if (IsLiteral(token.m_type))
	{
		std::unique_ptr<SyntaxNode> literal = std::make_unique<SyntaxNode>(nullptr, SyntaxNodeType::Literal);
		literal->m_data = token.m_data;

		SyntaxNode * literalType = literal->AddChild(SyntaxNodeType::Type);

		switch (static_cast<TokenType>(token.m_type))
		{
		case TokenType::FloatLiteral:
			literalType->m_data = "float";
			break;
		case TokenType::BoolLiteral:
			literalType->m_data = "bool";
			break;
		case TokenType::IntLiteral:
			literalType->m_data = "int";
			break;
		default:
			assert(false);
			break;
		}

		m_currentExpressionType = BuiltinType::Get(BuiltinType::FromName(literalType->m_data));

		return literal;
	}

	throw SyntaxException(token, "Expected expression");
}

void SyntaxTree::IfStatement(SyntaxNode *parent)
{
	assert(Is(m_iterator->Peek(), TokenType::If));

	SyntaxNode *ifStatement = parent->AddChild(SyntaxNodeType::If);

	Condition(ifStatement);

	StatementList(ifStatement);

	bool parsed;

	do
	{
		int peeked = m_iterator->Peek();

		parsed = false;

		if (Is(peeked, TokenType::Else))
		{
			m_iterator->Next();

			parsed = true;

			peeked = m_iterator->Peek();

			SyntaxNode *elseStatement = ifStatement->AddChild(SyntaxNodeType::Else);

			if (Is(peeked, TokenType::If))
			{
				IfStatement(elseStatement);
			}
			else
			{
				StatementList(elseStatement);
			}
		}
	}
	while (parsed);
}

void SyntaxTree::Condition(SyntaxNode *parent)
{
	tokeniser::Token next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::RoundBracketLeft))
		throw SyntaxException(next, "Expecting condition start '('");

	SyntaxNode * condition = parent->AddChild(SyntaxNodeType::Condition);

	Expression(condition);

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::RoundBracketRight))
		throw SyntaxException(next, "Expecting condition end ')'");
}

void SyntaxTree::WhileStatement(SyntaxNode *parent)
{
	assert(Is(m_iterator->Peek(), TokenType::While));

	m_iterator->Next();

	SyntaxNode *whileStatement = parent->AddChild(SyntaxNodeType::While);

	Condition(whileStatement);

	StatementList(whileStatement);
}

void SyntaxTree::ForStatement(SyntaxNode *parent)
{
	assert(Is(m_iterator->Peek(), TokenType::While));

	m_iterator->Next();

	tokeniser::Token next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::RoundBracketLeft))
		throw SyntaxException(next, "Expecting '('");

	SyntaxNode * forStatement = parent->AddChild(SyntaxNodeType::For);

	Expression(forStatement);

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::Semicolon))
		throw SyntaxException(next, "Expecting condition end ';'");

	Expression(forStatement);

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::Semicolon))
		throw SyntaxException(next, "Expecting condition end ';'");

	Expression(forStatement);

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::RoundBracketRight))
		throw SyntaxException(next, "Expecting ')'");

	StatementList(parent);
}

void SyntaxTree::ReturnStatement(SyntaxNode *parent)
{
	assert(Is(m_iterator->Peek(), TokenType::Return));

	tokeniser::Token next = m_iterator->Next();

	SyntaxNode *statement = parent->AddChild(SyntaxNodeType::Return);

	BuiltinType * returnType = m_currentFunction->GetReturnType();

	if (! Is(m_iterator->Peek(), TokenType::Semicolon))
	{
		Expression(statement);

		if (returnType->GetType() == BuiltinTypeType::Void ||
			! CheckAssignmentExpressionCompatibility(returnType, m_currentExpressionType))
		{
			throw SyntaxException(next, "Cannot return '" + m_currentExpressionType->GetName() + "' from a '" +
				returnType->GetName() + " function");
		}
	}
	else
	{
		if (returnType->GetType() != BuiltinTypeType::Void)
			throw SyntaxException(next, "Expecting return value of type '" + m_currentExpressionType->GetName() + "'");
	}

	next = m_iterator->Next();

	if (! Is(next.m_type, TokenType::Semicolon))
		throw SyntaxException(next, "Missing semicolon");
}
