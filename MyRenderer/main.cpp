#include<iostream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "Transform.h"

/*  1. 向量统一为列向量，即变换矩阵左乘向量。
    2. Model变换过程定义为把Model的boudingbox的中心放到原点，且大小除以boundingbox最大边的长度的一半。	
*/


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

Vector3f barycentric(Vector4f* pts, Vector4f& P) {
	Vector3f vec1 = Vector3f(pts[1].x() - pts[0].x(), pts[2].x() - pts[0].x(), pts[0].x() - P.x());
	Vector3f vec2 = Vector3f(pts[1].y() - pts[0].y(), pts[2].y() - pts[0].y(), pts[0].y() - P.y());
	Vector3f crs = vec1.cross(vec2);
	if (std::abs(crs.z()) < 1e-2)   // 意味着crs.z == 0，即三角形的边共线，返回一个负值即可
		return Vector3f(-1, 1, 1);
	return Vector3f(1.0f - crs.x() / crs.z() - crs.y() / crs.z(), crs.x() / crs.z(), crs.y() / crs.z());
}

void triangle(Vector4f* pts, float* zBuffer, TGAImage& image, TGAColor color) {
	// 找到bounding box 大小
	Vector2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vector2f boundingBoxMax(0, 0);
	Vector2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x() = std::min(clamp.x(), std::max(boundingBoxMax.x(), std::ceil(pts[i].x())));
		boundingBoxMax.y() = std::min(clamp.x(), std::max(boundingBoxMax.y(), std::ceil(pts[i].y())));
		boundingBoxMin.x() = std::max(0.f, std::min(boundingBoxMin.x(), std::floor(pts[i].x())));
		boundingBoxMin.y() = std::max(0.f, std::min(boundingBoxMin.y(), std::floor(pts[i].y())));
	}
	// 遍历bounding box的点
	Vector4f Point;
	for (Point.x() = boundingBoxMin.x(); Point.x() <= boundingBoxMax.x(); ++Point.x()) {
		for (Point.y() = boundingBoxMin.y(); Point.y() <= boundingBoxMax.y(); ++Point.y()) {
			Vector3f baryCor = barycentric(pts, Point);
			if (baryCor.x() < 0 || baryCor.y() < 0 || baryCor.z() < 0)
				continue;  // 点在三角形外
			// z插值
			Point.z() = baryCor.dot(Vector3f(pts[0].z(), pts[1].z(), pts[2].z()));
			int zBufferInd = static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width;
			if (Point.z() > zBuffer[zBufferInd]) {
				zBuffer[zBufferInd] = Point.z();
				image.set(static_cast<int>(Point.x()), static_cast<int>(Point.y()), color);
			}
		}
	}
}

#pragma region drawmain
//int main(int argc, char** argv) {
//	// read model
//	if (2 == argc)
//	{
//		model = new Model(argv[1]);
//	}
//	else {
//		model = new Model("obj/african_head.obj");
//	}
//	// set image's width/height/colortype
//	TGAImage image(width, height, TGAImage::RGB);
//	// draw Model
//	Vec3f lightDir(0, 0, -1);
//	float* zBuffer = new float[width * height];
//	for (int i = 0; i < width * height; ++i)
//		zBuffer[i] = std::numeric_limits<float>::min();
//	for (int i = 0; i < model->nfaces(); ++i) {
//		std::vector<int> face = model->face(i);
//		Vec3f Points[3];
//		Vec3f worldCoords[3];
//		for (int j = 0; j < 3; ++j) {
//			worldCoords[j] = model->vert(face[j]);
//			Points[j] = Vec3f((worldCoords[j].x + 1) * width / 2, (worldCoords[j].y + 1) * height / 2, worldCoords[j].z);
//		}
//		Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
//		n.normalize();
//		float lightIntense = lightDir * n;
//		if (lightIntense > 0)
//			triangle(Points, zBuffer, image, TGAColor(255 * lightIntense, 255 * lightIntense, 255 * lightIntense, 255));
//	}
//	// draw triangle
//	/*Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
//	triangle(pts, image, red);*/
//
//	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
//	// write output
//	image.write_tga_file("output.tga");
//
//
//	delete[] zBuffer;
//	return 0;
//}
#pragma endregion

#pragma region fbxtest



int main(int argc, char** argv) {
	const char* lFileName;
	if (2 == argc)
	{
		lFileName = argv[1];
	}
	else {
		lFileName = "obj/morphling_econ.fbx";
	}
	model = new Model(lFileName);
	// set image's width/height/colortype
	TGAImage image(width, height, TGAImage::RGB);
	// draw
	float* zBuffer = new float[width * height]; 
	for (int i = 0; i < width * height; ++i)
		zBuffer[i] = std::numeric_limits<float>::min();
	// Model
	Matrix4f ModelTransMatrix = get_ModelTransformMatrix(model->MinBBox(), model->MaxBBox());
	// View
	const float zNear = -2.0f;
	const float zFar = -4.0f;
	Vector4f CamPos(.0f, .0f, 1.0f - zNear, 1.0f);
	Vector4f CamLookAtDir(.0f, .0f, -1.0f, .0f);
	Vector4f CamUpDir(.0f, 1.0f, .0f, .0f);
	Matrix4f ViewTransMatrix = get_ViewTransformMatrix(CamPos, CamLookAtDir, CamUpDir);
	// Projection(Perspective)
	Matrix4f PerspecProjectMatrix = get_PerspectiveProjectionMatrix(2 * atanf(1 / zNear), 1.0f, zNear, zFar);

	// MVP + 视口变换
	Matrix4f ViewportMatrix = get_ViewportMatrix(width, height);
	Matrix4f FinalTransMatrix = ViewportMatrix * PerspecProjectMatrix * ViewTransMatrix * ModelTransMatrix;
	std::vector<Vector4f> afterTransPoints(model->nfaces());
	for (size_t i = 0; i < model->nverts(); i++)
	{
		afterTransPoints[i] = FinalTransMatrix * model->vert(i).position;
		afterTransPoints[i] /= afterTransPoints[i][3];
	}
	for (int i = 0; i < model->nfaces(); ++i) {
		std::vector<int> face = model->face(i);
		Vector4f Points[3];
		// 视口变换
		for (int j = 0; j < 3; ++j) {
			Points[j] = afterTransPoints[face[j]];
		}
		triangle(Points, zBuffer, image, TGAColor(255 * std::rand(), 255 * std::rand(), 255 * std::rand(), 255));
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	// write output
	image.write_tga_file("output.tga");

	return 0;
}
#pragma endregion





