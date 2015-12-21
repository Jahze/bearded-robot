#pragma once

#include <memory>
#include "ShadyObject.h"

class ShaderCache
{
public:
	static ShaderCache & Get();

	ShadyObject * VertexShader();
	ShadyObject * FragmentShader();

	bool LoadVertexShader(const std::string & filename, std::string & error);
	bool LoadFragmentShader(const std::string & filename, std::string & error);

private:
	//LoadShader

private:
	std::unique_ptr<ShadyObject> m_vertexShader;
	std::unique_ptr<ShadyObject> m_fragmentShader;
};
