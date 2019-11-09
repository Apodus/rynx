
#include "plane.hpp"

#include <cstdio>


Plane::Plane(const vec3<float>& v1, const vec3<float>& v2, const vec3<float>& v3)
{
	set3Points(v1,v2,v3);
}


Plane::Plane()
{
}


void Plane::set3Points(const vec3<float>& v1, const vec3<float>& v2, const vec3<float>& v3)
{
	vec3<float> aux1 = v1 - v2;
	vec3<float> aux2 = v3 - v2;

	normal = aux2 * aux1;
	normal.normalize();
	point = v2;
	d = -(normal.dot(point));
}

void Plane::setNormalAndPoint(const vec3<float>& normal_, const vec3<float>& point_)
{
	normal = normal_;
	normal.normalize();
	d = -normal.dot(point_);
}

void Plane::setCoefficients(float a, float b, float c, float d_)
{
	// set the normal vector
	normal.set(a,b,c);
	//compute the lenght of the vector
	float l = normal.length();
	// normalize the vector
	normal.set(a/l,b/l,c/l);
	// and divide d by th length as well
	d = d_/l;
}


float Plane::distance(const vec3<float>& p) const
{
	return (d + normal.dot(p));
}

