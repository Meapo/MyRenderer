#include<iostream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "Geometry.h"
#include "model.h"
#include "Transform.h"

/*  1. 向量统一为列向量，即变换矩阵左乘向量。
    2. Model变换过程定义为把Model的boudingbox的中心放到原点，且大小除以boundingbox最大边的长度的一半。	
*/


const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor black = TGAColor(0, 0, 0, 255);
Model* model = nullptr;
const int height = 800;
const int width = 800;

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
	const char* DiffuseTexFileName;
	if (4 == argc)
	{
		lFileName = argv[1];
		DiffuseTexFileName = argv[2];
	}
	else {
		lFileName = "obj/morphling_econ.fbx";
		DiffuseTexFileName = "obj/__morphling_base_color.tga";
	}
	model = new Model(lFileName);
	TGAImage DiffuseTex;
	DiffuseTex.read_tga_file(DiffuseTexFileName);
	// set image's width/height/colortype
	TGAImage image(width, height, TGAImage::RGB);

	bool MSAA = true;
	const size_t MSAAX = 2;
	// std::vector<float> zBuffer(width * height, std::numeric_limits<float>::min());
	std::vector<float> zBuffer(width * height, -std::numeric_limits<float>::max());  // 注意：上面注释部分得到的值是正数中float的最小值！
	std::vector<Vector4f> colorBuffer;
	if (MSAA) {
		zBuffer.resize(zBuffer.size() * MSAAX * MSAAX, -std::numeric_limits<float>::max());
		colorBuffer.resize(zBuffer.size(), Vector4f(.0f , .0f , .0f, 255.0f));
	}
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
	std::vector<Vector4f> afterTransPoints(model->nfaces()); // afterTransPoints的w代表插值
	for (size_t i = 0; i < model->nverts(); i++)
	{
		afterTransPoints[i] = FinalTransMatrix * model->vert(i).position;
		afterTransPoints[i][0] /= afterTransPoints[i][3];
		afterTransPoints[i][1] /= afterTransPoints[i][3];
	}
	if (MSAA)
	{
		for (int i = 0; i < model->nfaces(); ++i) {
			std::vector<int> face = model->face(i);
			Vertex_f Points[3];
			Vector4f colors[3];
			// 视口变换
			for (int j = 0; j < 3; ++j) {
				Points[j] = model->vert(face[j]);
				Points[j].position = afterTransPoints[face[j]];  // Points[j].position中的w代表投影变换前的z值
			}
			//DrawTriangle(Points, zBuffer, image, colors);
			DrawTriangle_MSAA(Points, zBuffer, colorBuffer, image, DiffuseTex, MSAAX);
		}
	}
	else {
		for (int i = 0; i < model->nfaces(); ++i) {
			std::vector<int> face = model->face(i);
			Vertex_f Points[3];
			TGAColor colors[3];
			// 视口变换
			for (int j = 0; j < 3; ++j) {
				Points[j] = model->vert(face[j]);
				Points[j].position = afterTransPoints[face[j]];
				
			}
			DrawTriangle(Points, zBuffer, image, DiffuseTex);
		}
	}
	

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	// write output
	image.write_tga_file("output.tga");

	return 0;
}
#pragma endregion





