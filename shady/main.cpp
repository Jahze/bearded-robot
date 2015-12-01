#include <cassert>
#include <cctype>
#include <iostream>
#include "CodeGenerator.h"
#include "ProgramContext.h"
#include "SyntaxTree.h"
#include "tokeniser\TextStream.h"
#include "tokeniser\TokenDefinitions.h"
#include "tokeniser\TokenStream.h"
#include "TokenType.h"

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
	if (! std::isalpha(s.Peek()))
		return false;

	s.Next();

	while (! s.AtEnd() && std::isalnum(s.Peek()))
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

std::string FormatLine(const tokeniser::TextStream & text, uint32_t line)
{
	std::string lineText = text.GetLineText(line);
	std::string number = std::to_string(line);
	
	return number + std::string(6 - number.length(), ' ') + lineText + "\n";
}

int main()
{
	std::string test =
		"int main()\n"
		"{\n"
		"    int i = 1;\n"
		"    float f = 1.0;\n"
		"    vec4 v4;\n"
		"    i = 2;\n"
		//"    int main = 1;\n"
		//"    int i = 8;\n"
		//"    1 += 2;\n"
		"    v4 *= 1;\n"
		//"    i = j;\n"
		"    return i;\n"
		"}";

	tokeniser::TextStream text(test);
	tokeniser::TokenDefinitions definitions(CreateDefinitions());

	tokeniser::TokenStream tokens(text, definitions);

	std::string error;

	if (! tokens.Parse(error))
		return 1;

	tokeniser::Token errorToken;
	SyntaxTree tree(ProgramContext::VertexShaderContext());

	if (! tree.Parse(tokens.GetTokens(), error, errorToken))
	{
		if (errorToken.m_type == static_cast<int>(TokenType::EndOfInput))
		{
			std::cout << error << " at end of input\n";
			return 1;
		}

		std::string line = text.GetLineText(errorToken.m_line);
		std::string spaces(6, ' ');

		for (uint32_t i = 0; i < (errorToken.m_column - 1); ++i)
		{
			if (line[i] == '\t')
				spaces.append(1, '\t');
			else
				spaces.append(1, ' ');
		}

		std::string message;

		if (errorToken.m_line > 1)
		{
			message += FormatLine(text, errorToken.m_line - 1);
		}

		message += FormatLine(text, errorToken.m_line);
		message += spaces + "^ " + error + "\n";

		std::string after = FormatLine(text, errorToken.m_line + 1);

		if (! after.empty())
		{
			message += "\n";
			message += after;
		}

		std::cout << message;
	}

	CodeGenerator generator(0x10000000,
		ProgramContext::VertexShaderContext(), tree.GetSymbolTable(), tree.GetFunctionTable());

	generator.Generate(tree.GetRoot());

	return 0;
}
