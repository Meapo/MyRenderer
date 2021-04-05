#include<vector>
#include<cmath>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model* model = nullptr;
const int height = 800;
const int width = 800;

void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color) {
	bool steep = false;
	if (std::abs(x1 - x0) < std::abs(y1 - y0)) {
		steep = true;
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0, dy = y1 - y0;
	int _2dx = 2 * dx;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	const int yIncr = (y1 > y0) ? 1 : -1;
	int y = y0;
	if (steep) {
		for (int x = x0; x <= x1; ++x) {
			image.set(y, x, color);
			error2 += derror2;
			if (error2 > dx) {
				y += yIncr;
				error2 -= _2dx;
			}
		}
	}
	else {
		for (int x = x0; x <= x1; ++x) {
			image.set(x, y, color);
			error2 += derror2;
			if (error2 > dx) {
				y += yIncr;
				error2 -= _2dx;
			}
		}
	}
	// steep只需要判断一次，把steep的判断放在循环外可减少判断次数
	// 另外表达式(y1 > y0 ? 1 : -1)只需要计算一次，可以采用const int 替换以减少表达式计算次数。
	/*for (int x = x0; x <= x1; x++) {
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}*/
}


int main(int argc, char** argv) {
	// read model
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}
	// set image's width/height/colortype
	TGAImage image(width, height, TGAImage::RGB);
	// draw line
	for (int i = 0; i < model->nfaces(); ++i) {
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; ++j) {
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (v0.x + 1.) * width / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			line(x0, y0, x1, y1, image, white);
		}
	}


	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	// write output
	image.write_tga_file("output.tga");
	return 0;
}

