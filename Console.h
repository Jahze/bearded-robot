#pragma once

#include <memory>
#include <string>
#include <vector>
#include <Windows.h>

class ICommand;

class Console
{
public:
	Console();
	~Console();

	void ProcessCommands();

	template<typename T, typename... U>
	void Write(const std::string & format, T && value, U &&... args);
	void Write(const std::string & format);

private:
	std::string ReadLine();

private:
	HANDLE m_stdin;
	HANDLE m_stdout;
	std::vector<std::unique_ptr<ICommand>> m_commands;
};

class ICommand
{
public:
	virtual void Process(Console * console, const std::vector<std::string> & arguments) = 0;
	virtual std::string GetName() const = 0;
};
