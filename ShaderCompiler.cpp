#include "CodeGenerator.h"
#include "ShaderCompiler.h"
#include "ProgramContext.h"
#include "SyntaxTree.h"
#include "tokeniser\TextStream.h"
#include "tokeniser\TokenStream.h"
#include "Tokens.h"

namespace
{
	std::string FormatLine(const tokeniser::TextStream & text, uint32_t line)
	{
		std::string lineText = text.GetLineText(line);
		std::string number = std::to_string(line);

		return number + std::string(6 - number.length(), ' ') + lineText + "\n";
	}
}

std::unique_ptr<ShadyObject> ShaderCompiler::CompileVertexShader(const std::string & source, std::string & error)
{
	return Compile(ProgramContext::VertexShaderContext(), source, error);
}

std::unique_ptr<ShadyObject> ShaderCompiler::CompileFragmentShader(const std::string & source, std::string & error)
{
	return Compile(ProgramContext::FragmentShaderContext(), source, error);
}

std::unique_ptr<ShadyObject> ShaderCompiler::Compile(const ProgramContext & context, const std::string & source,
	std::string & error)
{
	tokeniser::TextStream text(source);
	tokeniser::TokenDefinitions definitions(CreateDefinitions());

	tokeniser::TokenStream tokens(text, definitions);

	if (! tokens.Parse(error))
		return nullptr;

	tokeniser::Token errorToken;
	SyntaxTree tree(context);

	if (! tree.Parse(tokens.GetTokens(), error, errorToken))
	{
		if (errorToken.m_type == static_cast<int>(TokenType::EndOfInput))
		{
			error += " at end of input\n";
			return nullptr;
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

		error = message;
		return nullptr;
	}

	std::unique_ptr<ShadyObject> object = std::make_unique<ShadyObject>(0x1000);

	CodeGenerator generator(object->GetStart(), context, tree.GetSymbolTable(), tree.GetFunctionTable());

	generator.Generate(object.get(), tree.GetRoot());

	return object;
}
