#pragma once

#include <memory>
#include "Geometry.h"

class ObjModel
	: public geometry::Object
{
public:
	ObjModel(std::vector<geometry::Triangle> && faces);
};

class ObjReader
{
public:
	bool Read(const std::string & filename);

	std::unique_ptr<ObjModel> GetModel();

private:
	std::unique_ptr<ObjModel> m_model;
};
