#pragma once

#include <memory>
#include <string>
#include "ShadyObject.h"

class ShaderCompiler
{
public:
	std::unique_ptr<ShadyObject> Compile(const std::string & source, std::string & error);

private:
};
