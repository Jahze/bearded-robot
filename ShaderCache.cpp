#include <fstream>
#include "ShaderCache.h"
#include "ShaderCompiler.h"

ShaderCache & ShaderCache::Get()
{
	static ShaderCache instance;
	return instance;
}

ShadyObject * ShaderCache::GetVertexShader(const std::string & filename)
{
	auto iter = m_vertexShaders.find(filename);

	if (iter != m_vertexShaders.end())
		return iter->second.get();

	std::ifstream file(filename);

	std::string source{ std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>() };

	std::string error;
	ShaderCompiler compiler;

	std::unique_ptr<ShadyObject> object = compiler.CompileVertexShader(source, error);

	assert(error.empty());

	ShadyObject * r = object.get();
	m_vertexShaders.emplace(filename, std::move(object));

	return r;
}

ShadyObject * ShaderCache::GetFragmentShader(const std::string & filename)
{
	auto iter = m_fragmentShaders.find(filename);

	if (iter != m_fragmentShaders.end())
		return iter->second.get();

	std::ifstream file(filename);

	std::string source{ std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>() };

	std::string error;
	ShaderCompiler compiler;

	std::unique_ptr<ShadyObject> object = compiler.CompileFragmentShader(source, error);

	assert(error.empty());

	ShadyObject * r = object.get();
	m_fragmentShaders.emplace(filename, std::move(object));

	return r;
}
