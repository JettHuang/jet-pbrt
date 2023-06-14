// \brief
//		color
//

#pragma once

#include "pbrt.h"


namespace pbrt
{

class FColor
{
public:
	Float  r, g, b;

	FColor() : r(0), g(0), b(0) {}
	FColor(Float v) : r(v), g(v), b(v) {}
	FColor(Float rr, Float gg, Float bb) : r(rr), g(gg), b(bb) {}

	FColor operator* (Float s) const { return FColor(r * s, g * s, b * s); }
	FColor operator/ (Float s) const { return FColor(r / s, g / s, b / s); }

	FColor& operator*= (Float s) { r *= s; g *= s; b *= s; return *this; }
	FColor& operator/= (Float s) { r /= s; g /= s; b /= s; return *this; }

	FColor operator+ (const FColor& c) const { return FColor(r + c.r, g + c.g, b + c.b); }
	FColor operator- (const FColor& c) const { return FColor(r - c.r, g - c.g, b - c.b); }
	FColor operator* (const FColor& c) const { return FColor(r * c.r, g * c.g, b * c.b); }

	FColor& operator+= (const FColor& c) { r += c.r; g += c.g; b += c.b; return *this; }
	FColor& operator*= (const FColor& c) { r *= c.r; g *= c.g; b *= c.b; return *this; }

	FColor operator/ (const FColor& rhs) const { return FColor(r / rhs.r, g / rhs.g, b / rhs.b); }

	FColor Sqrt() const 
	{
		return FColor(std::sqrt(r), std::sqrt(g), std::sqrt(b));
	}

	Float MaxComponentValue() const
	{
		return std::max(r, std::max(g, b));
	}

	Float Luminance() const
	{
		return 0.212671f * r + 0.715160f * g + 0.072169f * b;
	}

	bool IsBlack() const { return r == 0.f && g == 0.f && b == 0.f; }
	bool IsValid() const { return isValid(r) && isValid(g) && isValid(b); }

	friend std::ostream& operator << (std::ostream& os, const FColor& clr)
	{
		os << "{" << clr.r << "," << clr.g << "," << clr.b << "}";
		return os;
	}

	friend FColor operator* (Float s, const FColor& c)
	{
		return c * s;
	}

	static FColor Zero;
	static FColor Black;

};

}
