#pragma once

#include <memory>
#include <string>
#include "ShadyObject.h"

class ProgramContext;

class ShaderCompiler
{
public:
	std::unique_ptr<ShadyObject> CompileVertexShader(const std::string & source, std::string & error);
	std::unique_ptr<ShadyObject> CompileFragmentShader(const std::string & source, std::string & error);

private:
	std::unique_ptr<ShadyObject> Compile(
		const ProgramContext & context,
		const std::string & source,
		std::string & error);
};
