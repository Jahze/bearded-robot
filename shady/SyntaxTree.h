#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "FunctionTable.h"
#include "SymbolTable.h"
#include "tokeniser\TokenStream.h"

class ProgramContext;
class TokenIterator;

enum class SyntaxNodeType
{
	Root,
	Function,
	GlobalVariable,
	Type,
	FunctionParameters,
	FunctionParameter,
	StatementList,
	LocalVariable,
	Initializer,
	Expression,
	Assign,
	AddAssign,
	SubtractAssign,
	MultiplyAssign,
	DivideAssign,
	Multiply,
	Divide,
	Add,
	Subtract,
	Negate,
	LogicalNegate,
	Name,
	Literal,
	Equals,
	NotEquals,
	Less,
	LessEquals,
	Greater,
	GreaterEquals,
	Subscript,
	Return,
	If,
	Condition,
	Else,
	While,
	For,
};

namespace SyntaxNodeFlags
{
	enum SyntaxNodeFlags
	{
		Export = 0x1,
		Interpolated = 0x2,
		Uniform = 0x4,
	};
}

struct SyntaxNode
{
	SyntaxNode(SyntaxNode *parent, SyntaxNodeType type)
		: m_parent(parent)
		, m_type(type)
	{ }

	SyntaxNode * AddChild(SyntaxNodeType type)
	{
		std::unique_ptr<SyntaxNode> child = std::make_unique<SyntaxNode>(this, type);
		SyntaxNode *raw = child.get();
		m_nodes.push_back(std::move(child));
		return raw;
	}

	void AddChild(std::unique_ptr<SyntaxNode> node)
	{
		node->m_parent = this;
		m_nodes.push_back(std::move(node));
	}

	SyntaxNodeType m_type;
	uint32_t m_flags = 0;
	std::string m_data;
	std::vector<std::unique_ptr<SyntaxNode>> m_nodes;
	SyntaxNode * m_parent;
};

class SyntaxTree
{
public:
	SyntaxTree(const ProgramContext & context);

	bool Parse(const std::vector<tokeniser::Token> & tokens, std::string & error, tokeniser::Token & errorToken);

	SymbolTable & GetSymbolTable()
	{
		return m_symbolTable;
	}

	FunctionTable & GetFunctionTable()
	{
		return m_functionTable;
	}

	SyntaxNode * GetRoot() const
	{
		return m_root.get();
	}

private:
	Symbol * AddSymbol(
		const tokeniser::Token & typeToken,
		const tokeniser::Token & nameToken,
		ScopeType scopeType,
		SymbolType symbolType);

	uint32_t CollectDeclarationDecorators();

	void FunctionOrVariable();
	void FunctionBody(SyntaxNode *function);
	void FunctionArguments(SyntaxNode *function);
	SyntaxNode * TypeAndIdentifier(SyntaxNode *parent, SyntaxNodeType type);
	SyntaxNode * TypeAndIdentifier(
		SyntaxNode *parent,
		SyntaxNodeType type,
		tokeniser::Token &typeToken,
		tokeniser::Token &nameToken);
	void StatementList(SyntaxNode *parent);
	void LocalVariable(SyntaxNode *parent);
	void Expression(SyntaxNode *parent);
	std::unique_ptr<SyntaxNode> AssignmentExpression();
	std::unique_ptr<SyntaxNode> RelationalExpression();
	std::unique_ptr<SyntaxNode> MultipicativeExpression();
	std::unique_ptr<SyntaxNode> AdditiveExpression();
	std::unique_ptr<SyntaxNode> UnaryOperatorExpression();
	std::unique_ptr<SyntaxNode> ExpressionAtom();
	void IfStatement(SyntaxNode *parent);
	void Condition(SyntaxNode *parent);
	void WhileStatement(SyntaxNode *parent);
	void ForStatement(SyntaxNode *parent);
	void ReturnStatement(SyntaxNode *parent);

private:
	std::unique_ptr<SyntaxNode> m_root;
	TokenIterator * m_iterator;

	SymbolTable m_symbolTable;
	FunctionTable m_functionTable;

	BuiltinType * m_currentExpressionType;
	Function * m_currentFunction;
};

struct SyntaxException
{
	SyntaxException(const tokeniser::Token & token, const std::string & message)
		: m_token(token)
		, m_message(message)
	{ }

	std::string m_message;
	tokeniser::Token m_token;
};
