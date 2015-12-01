#include <cctype>
#include "TextStream.h"
#include "TokenDefinitions.h"
#include "TokenStream.h"

namespace tokeniser
{

TokenStream::TokenStream(TextStream & text, const TokenDefinitions & tokens)
	: m_text(text)
	, m_tokenDefinitions(tokens)
{
}

bool TokenStream::Parse(std::string & error)
{
	error.clear();

	while (! m_text.AtEnd())
	{
		uint32_t line = m_text.GetLine();
		uint32_t column = m_text.GetColumn();

		if (std::isspace(m_text.Peek()))
		{
			m_text.Next();
			continue;
		}

		auto marker = m_text.Mark();
		bool found = false;

		if (! MatchSimpleTokenDefinitions(marker.get(), error))
		{
			if (! error.empty())
				return false;
		}
		else
		{
			continue;
		}

		if (! MatchComplexTokenDefinitions(marker.get(), error))
		{
			if (error.empty())
			{
				error = "Unable to match token at line " + std::to_string(line)
					+ ", column " + std::to_string(column) + ": ";
			}

			return false;
		}
	}

	return true;
}

bool TokenStream::MatchSimpleTokenDefinitions(TextStreamMarker * marker, std::string & error)
{
	auto simpleDefinitions = m_tokenDefinitions.GetSimpleTokenDefinitions();

	uint32_t line = m_text.GetLine();
	uint32_t column = m_text.GetColumn();

	for (auto && definition : simpleDefinitions)
	{
		if (m_text.MatchAndAdvance(definition.m_token))
		{
			Token token;

			token.m_line = line;
			token.m_column = column;
			token.m_type = definition.m_type;
			token.m_data = definition.m_token;

			m_tokens.push_back(token);
			return true;
		}

		// TODO : match and advance means this isn't necessary
		// neither is error arg
		m_text.Reset(marker);
	}

	return false;
}

bool TokenStream::MatchComplexTokenDefinitions(TextStreamMarker * marker, std::string & error)
{
	auto complexDefinitions = m_tokenDefinitions.GetComplexTokenDefinitions();

	uint32_t line = m_text.GetLine();
	uint32_t column = m_text.GetColumn();

	for (auto && definition : complexDefinitions)
	{
		try
		{
			if (definition.m_function(m_text))
			{
				if (m_text.GetLine() == line && m_text.GetColumn() == column)
				{
					error = "Token definition for " + std::to_string(definition.m_type)
						+ " did not advance stream after matching";

					return false;
				}

				Token token;

				token.m_line = line;
				token.m_column = column;
				token.m_type = definition.m_type;
				token.m_data = m_text.TextFrom(marker);

				m_tokens.push_back(token);
				return true;
			}

			m_text.Reset(marker);
		}
		catch (const std::logic_error & e)
		{
			error = "Token definition for " + std::to_string(definition.m_type)
				+ ": " + std::string(e.what());

			return false;
		}
	}

	return false;
}

}
