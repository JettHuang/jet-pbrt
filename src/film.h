// \brief
//		film: pixels rectangle
//

#pragma once

#include "pbrt.h"
#include "color.h"
#include "geometry.h"


namespace pbrt
{

enum class EImageType
{
	PPM,
	BMP,
	HDR
};

inline Float Clamp01(Float x) { return Clamp<Float, Float, Float>(x, 0, 1); }
inline FColor Clamp01(const FColor& c) { return FColor(Clamp01(c.r), Clamp01(c.g), Clamp01(c.b)); }
inline uint8_t gamma_encoding(Float x) { return (uint8_t)(std::pow(Clamp01(x), (Float)(1 / 2.2)) * 255.0); }

// film
class FFilm
{
public:
	FFilm(int w, int h)
		: width(w)
		, height(h)
	{
		pixels = std::make_unique<FColor[]>(GetPixelsNum());
	}

	int Width() const { return width; }
	int Height() const { return height; }
	int Channels() const { return 3; }
	int GetPixelsNum() const { return width * height; }
	
	FVector2 GetResolution() const { return FVector2((Float)width, (Float)height); }
	virtual FColor& operator()(int x, int y)
	{
		DOCHECK(x >=0 && x < width && y >=0 && y < height);

		int offset = width * y + x;
		return *(pixels.get() + offset);
	}
	
	void SetColor(int x, int y, const FColor& clr)
	{
		operator()(x, y) = clr;
	}

	void AddColor(int x, int y, const FColor& clr)
	{
		FColor& pixel = operator()(x, y);
		pixel += clr;
	}

	void ClearColor(int x, int y)
	{
		operator()(x, y) = FColor::Black;
	}

	void Clear()
	{
		int Count = GetPixelsNum();
		for (int i=0; i<Count; ++i)
		{
			pixels[i] = FColor::Black;
		}
	}

	virtual bool SaveAsImage(const std::string& filename, EImageType imgType) const;
protected:
	static bool SaveAsPPM(const std::string& filename, int width, int height, int channels, const FColor* pColors);
	static bool SaveAsBMP(const std::string& filename, int width, int height, int channels, const FColor* pColors);
	static bool SaveAsHDR(const std::string& filename, int width, int height, int channels, const FColor* pColors);

protected:
	int width;
	int height;
	std::unique_ptr<FColor[]> pixels;
};

} // namespace pbrt
