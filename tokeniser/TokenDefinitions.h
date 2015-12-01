#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace tokeniser
{

class TextStream;

struct SimpleTokenDefinition
{
	int m_type;
	std::string m_token;
};

using ComplexTokenFunction = std::function<bool(TextStream&)>;

struct ComplexTokenDefinition
{
	int m_type;
	ComplexTokenFunction m_function;
};

class TokenDefinitions
{
public:
	void Finalise()
	{
		std::sort(m_simpleTokens.begin(), m_simpleTokens.end(),
			[](const SimpleTokenDefinition & t1, const SimpleTokenDefinition & t2)
			{ return t1.m_token.length() > t2.m_token.length(); });
	}

	void Add(const SimpleTokenDefinition & definition)
	{
		m_simpleTokens.push_back(definition);
	}

	void Add(const ComplexTokenDefinition & definition)
	{
		m_complexTokens.push_back(definition);
	}

	const std::vector<SimpleTokenDefinition> & GetSimpleTokenDefinitions() const
	{
		return m_simpleTokens;
	}

	const std::vector<ComplexTokenDefinition> & GetComplexTokenDefinitions() const
	{
		return m_complexTokens;
	}

private:
	std::vector<SimpleTokenDefinition> m_simpleTokens;
	std::vector<ComplexTokenDefinition> m_complexTokens;
};

}
