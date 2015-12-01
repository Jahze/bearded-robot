#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace tokeniser
{

class TextStreamMarker
{
private:
	const char * m_cursor;
	uint32_t m_line;
	uint32_t m_column;

	friend class TextStream;
};

class TextStream
{
public:
	TextStream(const char *const begin, const char *const end);
	TextStream(const std::string & text);

	std::unique_ptr<TextStreamMarker> Mark() const;
	void Reset(TextStreamMarker *marker);

	bool AtEnd() const;
	char Next();
	char Peek() const;
	bool MatchAndAdvance(const std::string & text);

	std::string TextFrom(TextStreamMarker *marker) const;
	std::string GetLineText(uint32_t line) const;

	uint32_t GetLine() const
	{
		return m_line;
	}

	uint32_t GetColumn() const
	{
		return m_column;
	}

	static TextStream CreateFromFile(const std::string & filename);

private:
	void Initialise(const char *const begin, const char *const end);

private:
	uint32_t m_line = 1;
	uint32_t m_column = 1;
	std::unique_ptr<char[]> m_buffer;
	const char * m_begin;
	const char * m_end;
	const char * m_cursor;
};

}
