#include <cassert>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include "Console.h"
#include "ShaderCache.h"

namespace
{
	class FragmentCommand
		: public ICommand
	{
	public:
		void Process(Console *console, const std::vector<std::string> & arguments) override
		{
			if (arguments.size() != 2)
			{
				console->Write("Syntax: $ <filename>\r\n", arguments[0]);
				return;
			}

			std::string error;
			if (! ShaderCache::Get().LoadFragmentShader(arguments[1], error))
			{
				console->Write(error + "\r\n");
			}
		}

		std::string GetName() const override
		{
			return "fragment";
		}
	};
}

Console::Console()
{
	::AllocConsole();

	// TODO: i don't think these need to be closed but double check

	m_stdin = ::GetStdHandle(STD_INPUT_HANDLE);
	m_stdout = ::GetStdHandle(STD_OUTPUT_HANDLE);

	m_commands.emplace_back(new FragmentCommand());
}

Console::~Console()
{
	::FreeConsole();
}

void Console::ProcessCommands()
{
	while (true)
	{
		Write("> ");

		std::string line = ReadLine();

		// Remove \r\n
		line = line.substr(0, line.length() - 2);

		std::vector<std::string> arguments;

		std::string::size_type pos = 0;
		std::string::size_type split = line.find(' ');

		while (split != std::string::npos)
		{
			arguments.push_back(line.substr(pos, split - pos));

			pos = split;

			while (line[pos] == ' ')
				++pos;

			split = line.find(' ', pos);
		}

		arguments.push_back(line.substr(pos));

		std::string command = arguments.front();

		if (command.empty())
			continue;

		if (command == "exit" || command == "quit")
			break;

		bool handled = false;

		for (auto && handler : m_commands)
		{
			if (handler->GetName() == command)
			{
				handler->Process(this, arguments);
				handled = true;
				break;
			}
		}

		if (! handled)
			Write("Unknown command '$'\r\n", command);
	}
}

template<typename T, typename... U>
void Console::Write(const std::string & format, T && value, U &&... args)
{
	std::ostringstream oss;

	std::string::size_type pos = format.find('$');

	if (pos != std::string::npos)
	{
		oss << format.substr(0, pos);
		oss << value;

		std::string out = oss.str();

		DWORD written;
		::WriteConsole(m_stdout, out.c_str(), out.length(), &written, nullptr);

		Write(format.substr(pos + 1), std::forward<U>(args)...);
		return;
	}

	DWORD written;
	::WriteConsole(m_stdout, format.c_str(), format.length(), &written, nullptr);
}

void Console::Write(const std::string & format)
{
	DWORD written;
	::WriteConsole(m_stdout, format.c_str(), format.length(), &written, nullptr);
}

std::string Console::ReadLine()
{
	char buffer[4096];

	DWORD nRead;

	::ReadConsole(m_stdin, buffer, sizeof(buffer), &nRead, nullptr);

	buffer[nRead] = 0;

	return buffer;
}
