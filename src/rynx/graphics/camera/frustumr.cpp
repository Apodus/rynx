
#include "frustumr.hpp"
#include <cmath>
#include <cstdio>

#define m(col,row)  m[row*4+col]

void FrustumR::setFrustum(float *m)
{
	plane[NEARP].setCoefficients(m(2,0) + m(3,0),
	                             m(2,1) + m(3,1),
	                             m(2,2) + m(3,2),
	                             m(2,3) + m(3,3));
	plane[FARP].setCoefficients( -m(2,0) + m(3,0),
	                             -m(2,1) + m(3,1),
	                             -m(2,2) + m(3,2),
	                             -m(2,3) + m(3,3));
	plane[BOTTOM].setCoefficients(m(1,0) + m(3,0),
	                              m(1,1) + m(3,1),
	                              m(1,2) + m(3,2),
	                              m(1,3) + m(3,3));
	plane[TOP].setCoefficients(  -m(1,0) + m(3,0),
	                             -m(1,1) + m(3,1),
	                             -m(1,2) + m(3,2),
	                             -m(1,3) + m(3,3));
	plane[LEFT].setCoefficients(  m(0,0) + m(3,0),
	                              m(0,1) + m(3,1),
	                              m(0,2) + m(3,2),
	                              m(0,3) + m(3,3));
	plane[RIGHT].setCoefficients(-m(0,0) + m(3,0),
	                             -m(0,1) + m(3,1),
	                             -m(0,2) + m(3,2),
	                             -m(0,3) + m(3,3));
}

#undef m


void FrustumR::setCamInternals(float angle_, float ratio_, float near_, float far_)
{
	ratio = ratio_;
	angle = angle_;
	near = near_;
	far = far_;

	// compute width and height of the near and far planeane sections
	tang = tan(angle);
	sphereFactorY = 1.0f/cos(angle);//tang * sin(this->angle) + cos(this->angle);

	float anglex = atan(tang*ratio);
	sphereFactorX = 1.0f/cos(anglex); //tang*ratio * sin(anglex) + cos(anglex);

	nh = near * tang;
	nw = nh * ratio;

	fh = far * tang;
	fw = fh * ratio;
}


void FrustumR::setCamDef(const vec3<float>& p, const vec3<float>& l, const vec3<float>& u)
{
	vec3<float> dir,nc,fc;

	camPos = p;

	// compute the Z axis of camera
	Z = p - l;
	Z.normalizeAccurate();

	// X axis of camera of given "up" vector and Z axis
	X = u * Z;
	X.normalizeAccurate();

	// the real "up" vector is the cross product of Z and X
	Y = Z * X;

	// compute the center of the near and far planeanes
	nc = p - Z * near;
	fc = p - Z * far;

	// compute the 8 corners of the frustum
	ntl = nc + Y * nh - X * nw;
	ntr = nc + Y * nh + X * nw;
	nbl = nc - Y * nh - X * nw;
	nbr = nc - Y * nh + X * nw;

	ftl = fc + Y * fh - X * fw;
	fbr = fc - Y * fh + X * fw;
	ftr = fc + Y * fh + X * fw;
	fbl = fc - Y * fh - X * fw;

	// compute the six planes
	// the function set3Points asssumes that the points
	// are given in counter clockwise order
	plane[TOP].set3Points(ntr,ntl,ftl);
	plane[BOTTOM].set3Points(nbl,nbr,fbr);
	plane[LEFT].set3Points(ntl,nbl,fbl);
	plane[RIGHT].set3Points(nbr,ntr,fbr);
//	plane[NEARP].set3Points(ntl,ntr,nbr);
//	plane[FARP].set3Points(ftr,ftl,fbl);

	plane[NEARP].setNormalAndPoint(-Z,nc);
	plane[FARP].setNormalAndPoint(Z,fc);

	vec3<float> aux, normal;

	aux = (nc + Y*nh) - p;
	normal = aux * X;
	plane[TOP].setNormalAndPoint(normal,nc+Y*nh);

	aux = (nc - Y*nh) - p;
	normal = X * aux;
	plane[BOTTOM].setNormalAndPoint(normal,nc-Y*nh);
	
	aux = (nc - X*nw) - p;
	normal = aux * Y;
	plane[LEFT].setNormalAndPoint(normal,nc-X*nw);

	aux = (nc + X*nw) - p;
	normal = Y * aux;
	plane[RIGHT].setNormalAndPoint(normal,nc+X*nw);
}

FrustumR::FrustumResult FrustumR::pointInFrustum(const vec3<float>& p) const
{
	float pcz,pcx,pcy,aux;

	// compute vector from camera position to p
	vec3<float> v = p-camPos;

	// compute and test the Z coordinate
	pcz = v.dot(-Z);
	if(pcz > far || pcz < near)
		return OUTSIDE;

	// compute and test the Y coordinate
	pcy = v.dot(Y);
	aux = pcz * tang;
	if(pcy > aux || pcy < -aux)
		return OUTSIDE;

	// compute and test the X coordinate
	pcx = v.dot(X);
	aux = aux * ratio;
	if(pcx > aux || pcx < -aux)
		return OUTSIDE;

	return INSIDE;
}

FrustumR::FrustumResult FrustumR::sphereInFrustum(const vec3<float>& p, float radius) const
{
	float d1,d2;
	float az, ax, ay, zz1, zz2;
	FrustumResult result = INSIDE;

	vec3<float> v = p - camPos;

	az = v.dot(-Z);
	if (az > far + radius || az < near-radius)
		return OUTSIDE;

	ax = v.dot(X);
	zz1 = az * tang * ratio;
	d1 = sphereFactorX * radius;
	if (ax > zz1+d1 || ax < -zz1-d1)
		return OUTSIDE;

	ay = v.dot(Y);
	zz2 = az * tang;
	d2 = sphereFactorY * radius;
	if (ay > zz2+d2 || ay < -zz2-d2)
		return OUTSIDE;

	if (az > far - radius || az < near+radius)
		result = INTERSECT;
	if (ay > zz2-d2 || ay < -zz2+d2)
		result = INTERSECT;
	if (ax > zz1-d1 || ax < -zz1+d1)
		result = INTERSECT;

	return result;
}

