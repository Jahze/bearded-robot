#pragma once

#include "Geometry.h"
#include "Matrix.h"
#include "Scene.h"

#include "ObjReader.h"

namespace scene
{

class Bunny
	: public IScene
{
public:
	Bunny()
	{
		ObjReader reader;

		if (reader.Read("models\\bunny.wfobj"))
		{
			m_teapot = reader.GetModel();
			m_teapot->SetModelMatrix(Matrix4::Translation({ 0.0, 0.0, -50.0 }) * Matrix4::Scale({ 50.0, 50.0, 50.0 }));
		}
	}

	void Update(long long ms)
	{
	}

	ObjectIterator GetObjects()
	{
		if (m_teapot)
			return ObjectIterator({ m_teapot.get() });

		return ObjectIterator({});
	}

private:
	std::unique_ptr<geometry::Object> m_teapot;
};

}
