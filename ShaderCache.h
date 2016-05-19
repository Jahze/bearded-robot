#pragma once

#include <memory>
#include <unordered_map>

class ShadyObject;

class ShaderCache
{
public:
	static ShaderCache & Get();

	ShadyObject * GetVertexShader(const std::string & filename);
	ShadyObject * GetFragmentShader(const std::string & filename);

	ShadyObject * DefaultVertexShader()
	{
		return GetVertexShader("vertex.shader");
	}

	ShadyObject * DefaultFragmentShader()
	{
		return GetFragmentShader("fragment.shader");
	}

private:
	std::unordered_map<std::string, std::unique_ptr<ShadyObject>> m_vertexShaders;
	std::unordered_map<std::string, std::unique_ptr<ShadyObject>> m_fragmentShaders;
};
