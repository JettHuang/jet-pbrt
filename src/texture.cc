// \brief
//		texture.cc
//

#include "texture.h"

// Disable pedantic warnings for this external library.
#ifdef _MSC_VER
	// Microsoft Visual C++ Compiler
#pragma warning (push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

// Restore warning levels.
#ifdef _MSC_VER
	// Microsoft Visual C++ Compiler
#pragma warning (pop)
#endif

namespace pbrt
{


FColor FCheckerTexture::Sample(Float u, Float v, const FPoint3& pt) const
{
	auto sines = std::sin(10 * pt.x) * std::sin(10 * pt.y) * sin(10 * pt.z);
	if (sines < 0.0f) {
		return odd;
	}
	else {
		return even;
	}
}

// image texture
FImageTexture::FImageTexture(const char* filename)
{
	auto components_per_pixel = bytes_per_pixel;

	data = stbi_load(filename, &width, &height, &components_per_pixel, components_per_pixel);

	if (!data) {
		std::cerr << "ERROR: Could not load texture image file" << filename << ".\n";
		width = height = 0;
	}

	bytes_per_scanline = bytes_per_pixel * width;
}

FColor FImageTexture::Sample(Float u, Float v, const FPoint3& pt) const
{
	// if we have no texture data, then return solid cyan as a debugging aid
	if (data == nullptr) {
		return FColor(0, 1, 1);
	}

	// Clamp input texture coordinates to [0,1]x[1,0]
	u = Clamp<Float, Float, Float>(u, 0, 1);
	v = 1 - Clamp<Float, Float, Float>(v, 0, 1); // flip v to image coordinates

	auto i = static_cast<int>(u * width);
	auto j = static_cast<int>(v * height);

	// clamp integer mapping, sine actual coordinates should be less than 1.0
	if (i >= width) i = width - 1;
	if (j >= height) j = height - 1;

	const Float color_scale = 1.0f / 255.0f;
	auto pixel = data + (j * bytes_per_scanline + i * bytes_per_pixel);

	return FColor(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
}



} // namespace pbrt
