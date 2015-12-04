#include <cassert>
#include <cctype>
#include "tokeniser\TextStream.h"
#include "Tokens.h"

bool IntLiteral(tokeniser::TextStream & s)
{
	if (! std::isdigit(s.Peek()))
		return false;

	while (! s.AtEnd() && std::isdigit(s.Peek()))
		s.Next();

	return true;
}

bool FloatLiteral(tokeniser::TextStream & s)
{
	bool parsed = false;

	while (! s.AtEnd() && std::isdigit(s.Peek()))
	{
		s.Next();
		parsed = true;
	}

	if (s.AtEnd() || s.Peek() != '.')
	{
		return false;
	}

	assert(s.Peek() == '.');

	s.Next();

	while (! s.AtEnd() && std::isdigit(s.Peek()))
	{
		s.Next();
		parsed = true;
	}

	return parsed;
}

bool Identifier(tokeniser::TextStream & s)
{
	const char peeked = s.Peek();

	if (! std::isalpha(peeked) && peeked != '_')
		return false;

	s.Next();

	while (! s.AtEnd() && (std::isalnum(s.Peek()) || s.Peek() == '_'))
		s.Next();

	return true;
}

tokeniser::SimpleTokenDefinition Token(TokenType type, const char * text)
{
	static_assert(std::is_same<std::underlying_type<TokenType>::type, int>::value,
		"TokenType underlying type must be int");

	tokeniser::SimpleTokenDefinition definition;
	definition.m_type = static_cast<int>(type);
	definition.m_token = text;
	return definition;
}

tokeniser::ComplexTokenDefinition Token(TokenType type, tokeniser::ComplexTokenFunction f)
{
	static_assert(std::is_same<std::underlying_type<TokenType>::type, int>::value,
		"TokenType underlying type must be int");

	tokeniser::ComplexTokenDefinition definition;
	definition.m_type = static_cast<int>(type);
	definition.m_function = f;
	return definition;
}

tokeniser::TokenDefinitions CreateDefinitions()
{
	tokeniser::TokenDefinitions definitions;

	definitions.Add(Token(TokenType::Int, "int"));
	definitions.Add(Token(TokenType::Float, "float"));
	definitions.Add(Token(TokenType::Bool, "bool"));
	definitions.Add(Token(TokenType::Void, "void"));
	definitions.Add(Token(TokenType::Vec3, "vec3"));
	definitions.Add(Token(TokenType::Vec4, "vec4"));
	definitions.Add(Token(TokenType::Mat3x3, "mat3x3"));
	definitions.Add(Token(TokenType::Mat4x4, "mat4x4"));
	definitions.Add(Token(TokenType::LogicalAnd, "&&"));
	definitions.Add(Token(TokenType::LogicalOr, "||"));
	definitions.Add(Token(TokenType::LogicalNot, "!"));
	definitions.Add(Token(TokenType::LogicalEquals, "!="));
	definitions.Add(Token(TokenType::LogicalEquals, "=="));
	definitions.Add(Token(TokenType::Assign, "="));
	definitions.Add(Token(TokenType::Add, "+"));
	definitions.Add(Token(TokenType::AddAssign, "+="));
	definitions.Add(Token(TokenType::Subtract, "-"));
	definitions.Add(Token(TokenType::SubtractAssign, "-="));
	definitions.Add(Token(TokenType::Multiply, "*"));
	definitions.Add(Token(TokenType::MultiplyAssign, "*="));
	definitions.Add(Token(TokenType::Divide, "/"));
	definitions.Add(Token(TokenType::DivideAssign, "/="));
	definitions.Add(Token(TokenType::If, "if"));
	definitions.Add(Token(TokenType::Else, "else"));
	definitions.Add(Token(TokenType::Return, "return"));
	definitions.Add(Token(TokenType::For, "for"));
	definitions.Add(Token(TokenType::While, "while"));
	definitions.Add(Token(TokenType::Export, "export"));
	definitions.Add(Token(TokenType::Uniform, "uniform"));
	definitions.Add(Token(TokenType::Interpolated, "interpolated"));
	definitions.Add(Token(TokenType::SquareBracketLeft, "["));
	definitions.Add(Token(TokenType::SquareBracketRight, "]"));
	definitions.Add(Token(TokenType::RoundBracketLeft, "("));
	definitions.Add(Token(TokenType::RoundBracketRight, ")"));
	definitions.Add(Token(TokenType::CurlyBracketLeft, "{"));
	definitions.Add(Token(TokenType::CurlyBracketRight, "}"));
	definitions.Add(Token(TokenType::Comma, ","));
	definitions.Add(Token(TokenType::Semicolon, ";"));

	definitions.Add(Token(TokenType::FloatLiteral, FloatLiteral));
	definitions.Add(Token(TokenType::IntLiteral, IntLiteral));
	definitions.Add(Token(TokenType::Identifier, Identifier));

	definitions.Finalise();
	return definitions;
}
