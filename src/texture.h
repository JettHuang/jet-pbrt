// \brief
//		texture
//

#pragma once

#include "pbrt.h"
#include "geometry.h"


namespace pbrt
{

// texture
class FTexture
{
public:
	virtual FColor Sample(Float u, Float v, const FPoint3& pt) const = 0;
};


// solid color
class FSolidColor : public FTexture
{
public:
	FSolidColor() {}
	FSolidColor(const FColor& c) : color(c) {}
	FSolidColor(Float r, Float g, Float b)
		: color(r, g, b)
	{}

	FColor Sample(Float u, Float v, const FPoint3& pt) const override
	{
		return color;
	}

private:
	FColor	color;
};


// checker texture
class FCheckerTexture : public FTexture
{
public:
	FCheckerTexture(const FColor& t0, const FColor& t1) : odd(t0), even(t1) {}

	FColor Sample(Float u, Float v, const FPoint3& pt) const override;

public:
	FColor odd, even;
};


// image texture
class FImageTexture : public FTexture
{
public:
	const static int bytes_per_pixel = 3;

	FImageTexture()
		: data(nullptr), width(0), height(0), bytes_per_scanline(0) {}

	FImageTexture(const char* filename);

	~FImageTexture() {
		delete data;
	}

	FColor Sample(Float u, Float v, const FPoint3& pt) const override;

private:
	unsigned char* data;
	int width, height;
	int bytes_per_scanline;
};


} // namespace pbrt

