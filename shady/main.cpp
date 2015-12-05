#include <cassert>
#include <iostream>
#include "CodeGenerator.h"
#include "ProgramContext.h"
#include "ShadyObject.h"
#include "SyntaxTree.h"
#include "tokeniser\TextStream.h"
#include "tokeniser\TokenStream.h"
#include "Tokens.h"

std::string FormatLine(const tokeniser::TextStream & text, uint32_t line)
{
	std::string lineText = text.GetLineText(line);
	std::string number = std::to_string(line);
	
	return number + std::string(6 - number.length(), ' ') + lineText + "\n";
}

int main()
{
	/*
	std::string test =
		"int main()\n"
		"{\n"
		"    int i = 1;\n"
		"    float f = 1.0;\n"
		"    vec4 v4;\n"
		"    i = 2;\n"
		"    i = 3 * 4;\n"
		"    v4 = v4 * 7.0;\n"
		//"    int main = 1;\n"
		//"    int i = 8;\n"
		//"    1 += 2;\n"
		"    v4 *= 1;\n"
		//"    i = j;\n"
		"    return i;\n"
		"}";
	*/

	std::string test =
		"export void main()\n"
		"{\n"
		"	g_projected_position = g_projection * g_view * g_model * g_position;\n"
		"	g_projected_position[0] /= g_projected_position[3];\n"
		"	g_projected_position[1] /= g_projected_position[3];\n"
		"	g_projected_position[2] /= g_projected_position[3];\n"
		"	g_world_position = normalize(g_model * g_normal);\n"
		"	return;\n"
		"}\n";

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

	ShadyObject object(0x1000);

	CodeGenerator generator(object.GetStart(),
		ProgramContext::VertexShaderContext(), tree.GetSymbolTable(), tree.GetFunctionTable());

	generator.Generate(&object, tree.GetRoot());

	return 0;
}
