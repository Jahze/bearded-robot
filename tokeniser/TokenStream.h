#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tokeniser
{

class TextStream;
class TextStreamMarker;
class TokenDefinitions;

struct Token
{
	uint32_t m_line;
	uint32_t m_column;
	std::string m_data;
	int m_type;
};

class TokenStream
{
public:
	TokenStream(TextStream & text, const TokenDefinitions & tokens);

	bool Parse(std::string & error);

	const std::vector<Token> & GetTokens()
	{
		return m_tokens;
	}

private:
	bool MatchSimpleTokenDefinitions(TextStreamMarker * marker, std::string & error);
	bool MatchComplexTokenDefinitions(TextStreamMarker * marker, std::string & error);

private:
	TextStream & m_text;
	const TokenDefinitions & m_tokenDefinitions;
	std::vector<Token> m_tokens;
};

}
