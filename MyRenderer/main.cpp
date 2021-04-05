#include<iostream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model* model = nullptr;
const int height = 200;
const int width = 200;

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

Vec3f barycentric(Vec2i* pts, Vec2i P) {
	Vec3f crs = cross(Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x),
		Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y));
	if (std::abs(crs.z) < 1)   // 意味着crs.z == 0，即三角形的边共线，返回一个负值即可
		return Vec3f(-1, 1, 1);
	return Vec3f(1.0f - crs.x / crs.z - crs.y / crs.z, crs.x / crs.z, crs.y / crs.z);
}

void triangle(Vec2i* pts, TGAImage& image, TGAColor color) {
	// 找到bounding box 大小
	Vec2i boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vec2i boundingBoxMax(0, 0);
	Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x = std::min(clamp.x, std::max(boundingBoxMax.x, pts[i].x));
		boundingBoxMax.y = std::min(clamp.x, std::max(boundingBoxMax.y, pts[i].y));
		boundingBoxMin.x = std::max(0, std::min(boundingBoxMin.x, pts[i].x));
		boundingBoxMin.y = std::max(0, std::min(boundingBoxMin.y, pts[i].y));
	}
	// 遍历bounding box的点
	Vec2i Point;
	for (Point.x = boundingBoxMin.x; Point.x <= boundingBoxMax.x; ++Point.x) {
		for (Point.y = boundingBoxMin.y; Point.y <= boundingBoxMax.y; ++Point.y) {
			Vec3f baryCor = barycentric(pts, Point);
			if (baryCor.x < 0 || baryCor.y < 0 || baryCor.z < 0)
				continue;  // 点在三角形外
			image.set(Point.x, Point.y, color);
		}
	}
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
	// draw Model
	for (int i = 0; i < model->nfaces(); ++i) {
		std::vector<int> face = model->face(i);
		Vec2i Points[3];
		for (int j = 0; j < 3; ++j) {
			Vec3f worldCoords = model->vert(face[j]);
			Points[j] = Vec2i((worldCoords.x + 1) * width / 2, (worldCoords.y + 1) * height / 2);
		}
		triangle(Points, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
	}
	// draw triangle
	/*Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
	triangle(pts, image, red);*/

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	// write output
	image.write_tga_file("output.tga");
	return 0;
}

