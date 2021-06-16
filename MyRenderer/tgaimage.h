#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <cstdint>
#include <fstream>
#include <vector>
#include <Eigen>

using namespace Eigen;

#pragma pack(push,1)
struct TGA_Header {
	std::uint8_t  idlength{};
	std::uint8_t  colormaptype{};
	std::uint8_t  datatypecode{};
	std::uint16_t colormaporigin{};
	std::uint16_t colormaplength{};
	std::uint8_t  colormapdepth{};
	std::uint16_t x_origin{};
	std::uint16_t y_origin{};
	std::uint16_t width{};
	std::uint16_t height{};
	std::uint8_t  bitsperpixel{};
	std::uint8_t  imagedescriptor{};
};
#pragma pack(pop)

struct TGAColor {
	std::uint8_t bgra[4] = { 0,0,0,0 };
	std::uint8_t bytespp = { 0 };

	TGAColor() = default;
	TGAColor(const std::uint8_t R, const std::uint8_t G, const std::uint8_t B, const std::uint8_t A = 255) : bgra{ B,G,R,A }, bytespp(4) { }
	TGAColor(const std::uint8_t v) : bgra{ v,0,0,0 }, bytespp(1) { }
	TGAColor(const Vector4f& vec) : bgra{ static_cast<unsigned char>(vec[0]), static_cast<unsigned char>(vec[1]), static_cast<unsigned char>(vec[2]), static_cast<unsigned char>(vec[3]) }, bytespp(4) {}
	TGAColor(const std::uint8_t* p, const std::uint8_t bpp) : bgra{ 0,0,0,0 }, bytespp(bpp) {
		for (int i = 0; i < bpp; i++)
			bgra[i] = p[i];
	}

	std::uint8_t& operator[](const int i) { return bgra[i]; }

	TGAColor operator *(const double intensity) const {
		TGAColor res = *this;
		double clamped = std::max(0., std::min(intensity, 1.));
		for (int i = 0; i < 4; i++) res.bgra[i] = bgra[i] * clamped;
		return res;
	}
	TGAColor& operator +=(const TGAColor& c) {
		for (size_t i = 0; i < 4; i++)
		{
			bgra[i] += c.bgra[i];
		}
		return *this;
	}


	TGAColor operator+(const TGAColor& c) {
		TGAColor result;
		for (size_t i = 0; i < 4; i++)
		{
			result = bgra[i] + c.bgra[i];
		}
		return result;
	}

	Vector4f Color2Vec4f() const {
		Vector4f res;
		for (size_t i = 0; i < 4; i++)
		{
			res[i] = bgra[i];
		}
		return res;
	}

	Vector3f Color2Vec3f() const {
		Vector3f res;
		for (size_t i = 0; i < 3; i++)
		{
			res[i] = bgra[i];
		}
		return res;
	}
};

class TGAImage {
protected:
	std::vector<std::uint8_t> data;
	size_t width;
	size_t height;
	int bytespp;

	bool   load_rle_data(std::ifstream& in);
	bool unload_rle_data(std::ofstream& out) const;
public:
	enum Format { GRAYSCALE = 1, RGB = 3, RGBA = 4 };

	TGAImage();
	TGAImage(const size_t w, const size_t h, const size_t bpp);
	bool  read_tga_file(const std::string filename);
	bool write_tga_file(const std::string filename, const bool vflip = true, const bool rle = true) const;
	void flip_horizontally();
	void flip_vertically();
	void scale(const size_t w, const size_t h);
	TGAColor get(const size_t x, const size_t y) const;
	void set(const size_t x, const size_t y, const TGAColor& c);
	int get_width() const;
	int get_height() const;
	int get_bytespp();
	std::uint8_t* buffer();
	void clear();
};

#endif //__IMAGE_H__