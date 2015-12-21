#include <fstream>
#include "ShaderCache.h"
#include "ShaderCompiler.h"

namespace
{
	bool Slurp(const std::string & filename, std::string & contents)
	{
		std::ifstream file(filename);

		if (! file.good())
			return false;

		contents = std::string { std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>() };

		return true;
	}
}

ShaderCache & ShaderCache::Get()
{
	static ShaderCache cache;
	return cache;
}

ShadyObject * ShaderCache::VertexShader()
{
	return m_vertexShader.get();
}

ShadyObject * ShaderCache::FragmentShader()
{
	return m_fragmentShader.get();
}

bool ShaderCache::LoadVertexShader(const std::string & filename, std::string & error)
{
	ShaderCompiler compiler;

	std::string source;

	if (! Slurp(filename, source))
	{
		error = "Unable to open file " + filename;
		return false;
	}

	m_vertexShader = compiler.CompileVertexShader(source, error);

	return error.empty();
}

bool ShaderCache::LoadFragmentShader(const std::string & filename, std::string & error)
{
	ShaderCompiler compiler;

	std::string source;

	if (! Slurp(filename, source))
	{
		error = "Unable to open file " + filename;
		return false;
	}

	m_fragmentShader = compiler.CompileFragmentShader(source, error);

	return error.empty();
}
