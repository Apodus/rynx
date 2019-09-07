#ifndef TEXTURECOORDINATE_H
#define TEXTURECOORDINATE_H

struct TextureCoordinate
{
	TextureCoordinate(): x(0.0f), y(0.0f) {}
	TextureCoordinate(float x_, float y_): x(x_), y(y_) {}
	float x;
	float y;
};

#endif

