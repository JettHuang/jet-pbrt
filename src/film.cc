// \brief
//		film.cc
//

#include "film.h"


namespace pbrt
{

bool FFilm::SaveAsImage(const std::string& filename, EImageType imgType) const
{
	PBRT_DOCHECK(Channels() == 3);

	switch (imgType)
	{
	case pbrt::EImageType::PPM:
	{
		const std::string realname = filename + ".ppm";

		return SaveAsPPM(realname, Width(), Height(), Channels(), pixels);
	}
		break;
	case pbrt::EImageType::BMP:
	{
		const std::string realname = filename + ".bmp";

		return SaveAsBMP(realname, Width(), Height(), Channels(), pixels);
	}
		break;
	case pbrt::EImageType::HDR:
	{
		const std::string realname = filename + ".hdr";

		return SaveAsHDR(realname, Width(), Height(), Channels(), pixels);
	}
		break;
	default:
		break;
	}

	return false;
}

bool FFilm::SaveAsPPM(const std::string& filename, int width, int height, int channels, const FColor* pColors)
{
	std::fstream img_file(filename, std::ios::binary | std::ios::out);

	img_file << "P3" << std::endl << width << " " << height <<  std::endl << 255 << std::endl;

	int pixels_num = width * height;
	for (int index = 0; index < pixels_num; ++index)
	{
		img_file << gamma_encoding(pColors[index].r) << "  "
			     << gamma_encoding(pColors[index].g) << "  "
			     << gamma_encoding(pColors[index].b) << std::endl;
	} // end for index

	return true;
}

bool FFilm::SaveAsBMP(const std::string& filename, int width, int height, int channels, const FColor* pColors)
{
	// https://github.com/SmallVCM/SmallVCM/blob/master/src/framebuffer.hxx#L149-L215
	// https://github.com/skywind3000/RenderHelp/blob/master/RenderHelp.h#L937-L1018

	std::fstream img_file(filename, std::ios::binary | std::ios::out);


	// 1.write file header & 2.write info header

	uint32_t padding_line_bytes = (width * channels + 3) & (~3);
	uint32_t padding_image_bytes = padding_line_bytes * height;

	const uint32_t FILE_HEADER_SIZE = 14;
	const uint32_t INFO_HEADER_SIZE = 40;

#pragma pack(push)
#pragma pack(1)
	struct BITMAP_FILE_HEADER_INFO_HEADER
	{
		// file header
		uint16_t type;
		uint32_t file_size{};
		uint32_t reserved{ 0 };
		uint32_t databody_offset{ FILE_HEADER_SIZE + INFO_HEADER_SIZE };

		// info header
		uint32_t	info_header_size{ INFO_HEADER_SIZE };

		int32_t     width{};
		int32_t		height{};
		int16_t	    color_planes{ 1 };
		int16_t	    per_pixel_bits{};
		uint32_t	compression{ 0 };
		uint32_t	image_bytes{ 0 };

		uint32_t	x_pixels_per_meter{ 0 };
		uint32_t	y_pixels_per_meter{ 0 };
		uint32_t	color_used{ 0 };
		uint32_t	color_important{ 0 };
	} bmp_header;
#pragma pack(pop)

	bmp_header.type = 0x4d42; // BM
	bmp_header.file_size = FILE_HEADER_SIZE + INFO_HEADER_SIZE + padding_image_bytes;
	bmp_header.width = width;
	bmp_header.height = height;
	bmp_header.color_planes = 1;
	bmp_header.per_pixel_bits = (int16_t)(channels * 8);

	img_file.write((char*)&bmp_header, sizeof(bmp_header));

	// 3.without color table

	// 4.write data body 
	// gamma encoding
	int pixels_num = width * height;
	int bytes_num = pixels_num * channels;
	auto bytes = std::make_unique<uint8_t[]>(padding_image_bytes);
	for (int y = 0; y < height; ++y)
	{
		uint8_t* pLine = &bytes[0] + (padding_line_bytes * y);
		for (int x = 0; x < width; ++x)
		{
			int pixel_index = width * y + x;
			int byte_index = x * channels;
			// BGR
			pLine[byte_index + 0] = gamma_encoding(pColors[pixel_index].b);
			pLine[byte_index + 1] = gamma_encoding(pColors[pixel_index].g);
			pLine[byte_index + 2] = gamma_encoding(pColors[pixel_index].r);
		}

	}

	int line_num = width * channels;
	// bmp is stored from bottom to up
	for (int y = height - 1; y >= 0; --y)
	{
		img_file.write((const char*)(bytes.get() + y * line_num), line_num);
	}

	return true;
}


bool FFilm::SaveAsHDR(const std::string& filename, int width, int height, int channels, const FColor* pColors)
{
	// https://github.com/SmallVCM/SmallVCM/blob/master/src/framebuffer.hxx#L218-L251

	std::ofstream img_file(filename, std::ios::binary | std::ios::out);

	img_file << "#?RADIANCE\n"
		"FORMAT=32-bit_rle_rgbe\n\n"
		"-Y " << height << " +X " << width << std::endl;

	int pixel_num = width * height;
	for (int index = 0; index < pixel_num; index++)
	{
		uint8_t rgbe[4];

		const FColor &color = pColors[index];
		float v = std::max({ color.r, color.g, color.b });

		if (v >= 1e-32f)
		{
			/*
			   write:
					v = m * 2 ^ e ( 0 < m < 1)
					r = R * m * 256.0/v
			   read:
					R = r * 2^(e ¨C 128 - 8);
			*/

			int e;
			float m = float(std::frexp(v, &e) * 256.f / v);

			rgbe[0] = uint8_t(color.r * m);
			rgbe[1] = uint8_t(color.g * m);
			rgbe[2] = uint8_t(color.b * m);
			rgbe[3] = uint8_t(e + 128);
		}

		img_file.write((const char*)&rgbe[0], 4);
	} // end for index

	return true;
}


}

