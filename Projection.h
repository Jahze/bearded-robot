#pragma once

#include "Types.h"
#include "Matrix4.h"
#include "Vector.h"

class Projection
{
public:
	Projection(Real fovx, Real fovy, Real znear, Real zfar);

	Matrix4 GetProjectionMatrix() const;

	void Project(Vector3 & v);
	void ToScreen(unsigned width, unsigned height, Vector3 & v);
	//bool ShouldClip(const Vector<4> & v);

private:
	Real m_fovxCoeff;
	Real m_fovyCoeff;
	Real m_zClip1;
	Real m_zClip2;
};
