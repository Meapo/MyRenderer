#include<iostream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "Geometry.h"
#include "model.h"
#include "Transform.h"
#include "EigenExtension.h"
#include "PointLight.h"
#include "Shader.h"

/*  1. ����ͳһΪ�����������任�������������
    2. Model�任���̶���Ϊ��Model��boudingbox�����ķŵ�ԭ�㣬�Ҵ�С����boundingbox���ߵĳ��ȵ�һ�롣	
*/


const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor black = TGAColor(0, 0, 0, 255);
Model* model = nullptr;
Mesh* mesh = nullptr;
const int height = 800;
const int width = 800;

Matrix4f ModelView;
Matrix4f Projection;
Matrix4f Viewport;

Vector4f LightDir(1.0f, -0.7f, -1.0f, .0f);

int main(int argc, char** argv) {
	const char* lFileName;
	const char* path;
	if (4 == argc)
	{
		path = argv[1];
		lFileName = argv[2];
	}
	else {
		path = "obj/slark/";
		lFileName = "slark_econ.fbx";
	}
	model = new Model(path, lFileName);
	// set image's width/height/colortype
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage depthImage(width, height, TGAImage::RGB);
	
	// std::vector<float> zBuffer(width * height, std::numeric_limits<float>::min());
	std::vector<float> zBuffer(width * height, -std::numeric_limits<float>::max());  // ע�⣺����ע�Ͳ��ֵõ���ֵ��������float����Сֵ��
	std::vector<Vector4f> colorBuffer;
	// Model
	Matrix4f ModelTransMatrix = get_ModelTransformMatrix(model->MinBBox(), model->MaxBBox());
	// View and Light
	const float zNear = -2.0f;
	const float zFar = -4.0f;
	Vector4f CamPos(.0f, .0f, 1.0f - zNear, 1.0f);
	Vector4f CamLookAtDir(.0f, .0f, -1.0f, .0f);
	Vector4f CamUpDir(.0f, 1.0f, .0f, .0f);
	Matrix4f ViewTransMatrix = get_ViewTransformMatrix(CamPos, CamLookAtDir, CamUpDir);
	ModelView = ViewTransMatrix * ModelTransMatrix;

	// Projection(Perspective)
	Projection = get_PerspectiveProjectionMatrix(2 * atanf(1 / zNear), 1.0f, zNear, zFar);

	// MVP + �ӿڱ任
	Viewport = get_ViewportMatrix(width, height);

	// shader
	BlinnShader shader;
	shader.uniform_M() = ModelView;
	// �ο���fundamentals of computer graphics����6.2.2��,�����˷������ڽ�������任ʱΪʲô���ǳ���ModelView��������������ת��
	shader.uniform_MIT() = ModelView.inverse().transpose(); 

	for (size_t meshInd = 0; meshInd < model->meshes().size(); ++meshInd) {
		mesh = &model->meshes()[meshInd];
		for (size_t faceInd = 0; faceInd < mesh->nfaces(); faceInd++)
		{
			Vector4f ScreenCor[3];
			for (int vertInd = 0; vertInd < 3; ++vertInd) {
				ScreenCor[vertInd] = shader.vertex(faceInd, vertInd);
			}
			DrawTriangle(ScreenCor, zBuffer, image, shader, faceInd);
		}
	}
	
	for (size_t i = 0; i < zBuffer.size(); i++)
	{
		int x = i % width, y = i / width;
		float gray = 1.0f - zBuffer[i] / (zFar - zNear);
		depthImage.set(x, y, TGAColor(255.0f * gray, 255.0f * gray, 255.0f * gray, 255));
	}

	delete model;
	model = nullptr;
	// write output
	image.write_tga_file("output.tga");
	depthImage.write_tga_file("depthImage.tga");
	return 0;
}





