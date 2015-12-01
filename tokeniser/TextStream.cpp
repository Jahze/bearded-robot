#define _SCL_SECURE_NO_WARNINGS

#include <cassert>
#include <fstream>
#include "TextStream.h"

namespace tokeniser
{

TextStream::TextStream(const char * begin, const char * end)
{
	Initialise(begin, end);
}

TextStream::TextStream(const std::string & text)
{
	Initialise(text.c_str(), text.c_str() + text.length());
}

std::unique_ptr<TextStreamMarker> TextStream::Mark() const
{
	TextStreamMarker m;

	m.m_cursor = m_cursor;
	m.m_line = m_line;
	m.m_column = m_column;

	return std::make_unique<TextStreamMarker>(m);
}

void TextStream::Reset(TextStreamMarker * marker)
{
	m_cursor = marker->m_cursor;
	m_line = marker->m_line;
	m_column = marker->m_column;
}

bool TextStream::AtEnd() const
{
	return m_cursor == m_end;
}

char TextStream::Next()
{
	if (AtEnd())
		throw std::logic_error("text stream is empty");

	if (*m_cursor == '\n')
	{
		m_column = 1;
		++m_line;
	}
	else
	{
		++m_column;
	}

	return *m_cursor++;
}

char TextStream::Peek() const
{
	return *m_cursor;
}

bool TextStream::MatchAndAdvance(const std::string & text)
{
	std::size_t left = static_cast<std::size_t>(std::distance(m_cursor, m_end));

	const std::string::size_type length = text.length();

	if (left < length)
		return false;

	if (std::string(m_cursor, m_cursor + length) != text)
		return false;

	for (std::string::size_type i = 0; i < length; ++i)
		Next();

	return true;
}

std::string TextStream::TextFrom(TextStreamMarker *marker) const
{
	assert(marker->m_cursor < m_cursor);
	return std::string(marker->m_cursor, m_cursor);
}

std::string TextStream::GetLineText(uint32_t line) const
{
	const char *start = m_begin;

	for (uint32_t count = 0; count < (line-1); ++count)
	{
		while (*start != '\n')
		{
			if (start >= m_end)
				return std::string();

			++start;
		}

		++start;
	}

	const char *end = start;

	while (*end != '\n')
	{
		if (end >= m_end)
			break;

		++end;
	}

	return std::string(start, end);
}

void TextStream::Initialise(const char *const begin, const char *const end)
{
	assert(begin < end);

	std::size_t length = static_cast<std::size_t>(std::distance(begin, end));

	m_buffer.reset(new char[length]);

	std::copy(begin, end, m_buffer.get());

	m_begin = m_buffer.get();
	m_end = m_buffer.get() + length;
	m_cursor = m_begin;
}

TextStream TextStream::CreateFromFile(const std::string & filename)
{
	std::ifstream file(filename, std::ios::binary);

	if (! file.is_open())
		throw std::runtime_error("could not open " + filename);

	std::string contents{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

	return TextStream(contents);
}

}
