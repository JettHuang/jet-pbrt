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
		pixels = new FColor[GetPixelsNum()];
	}

	~FFilm()
	{
		if (pixels) {
			delete[] pixels;
		}
		pixels = nullptr;
	}

	int Width() const { return width; }
	int Height() const { return height; }
	int Channels() const { return 3; }
	int GetPixelsNum() const { return width * height; }
	
	FVector2 GetResolution() const { return FVector2((Float)width, (Float)height); }
	virtual FColor& operator()(int x, int y)
	{
		PBRT_DOCHECK(x >=0 && x < width && y >=0 && y < height);

		int offset = width * y + x;
		return *(pixels + offset);
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
	FColor *pixels;
};

// Film View
//   +--------------+
//   |              |
//   |              |
//   +--------------+
//
//   [sx  ex)  [sy  ey)
class FFilmView
{
public:
	FFilmView(FFilm* inFilm, int sx, int sy, int ex, int ey)
		: pFilm(inFilm)
		, start_x(sx)
		, start_y(sy)
		, end_x(ex)
		, end_y(ey)
	{}

	void GetViewport(int& startx, int& starty, int& endx, int& endy)
	{
		startx = start_x;
		starty = start_y;
		endx = end_x;
		endy = end_y;
	}

	void SetColor(int x, int y, const FColor& clr)
	{
		pFilm->SetColor(x, y, clr);
	}

	void AddColor(int x, int y, const FColor& clr)
	{
		pFilm->AddColor(x, y, clr);
	}

protected:
	FFilm* pFilm;
	int start_x, start_y;
	int end_x, end_y;
};

} // namespace pbrt
