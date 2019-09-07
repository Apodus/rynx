
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/plane/plane.hpp>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

class FrustumR
{
public:
	void setCamInternals(float angle, float radius, float nearD, float farD);
	void setCamDef(const vec3<float>& p, const vec3<float>& l, const vec3<float>& u);

	enum FrustumResult { OUTSIDE, INTERSECT, INSIDE };
	FrustumResult pointInFrustum(const vec3<float>& p) const;
	FrustumResult sphereInFrustum(const vec3<float>& p, float radius) const;

	vec3<float> ntl,ntr,nbl,nbr;
	vec3<float> ftl,ftr,fbl,fbr;
private:

	enum
	{
		TOP = 0,
		BOTTOM,
		LEFT,
		RIGHT,
		NEARP,
		FARP
	};

	void setFrustum(float* m);

	Plane plane[6];

	vec3<float> X,Y,Z;
	vec3<float> camPos;
	float near;
	float far;
	float ratio;
	float angle;

	float sphereFactorX, sphereFactorY;
	float tang;
	float nw,nh,fw,fh;
};
