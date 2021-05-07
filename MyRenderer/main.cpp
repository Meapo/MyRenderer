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
const int height = 800;
const int width = 800;

Matrix4f ModelView;
Matrix4f Projection;
Matrix4f Viewport;


int main(int argc, char** argv) {
	const char* lFileName;
	const char* path;
	if (4 == argc)
	{
		path = argv[1];
		lFileName = argv[2];
	}
	else {
		path = "obj/morphling/";
		lFileName = "morphling_econ.fbx";
	}
	model = new Model(path, lFileName);
	// set image's width/height/colortype
	TGAImage image(width, height, TGAImage::RGB);

	
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

	std::vector<Vector4f> afterTransPoints(model->nfaces()); // afterTransPoints��w�����ֵ

	// shader
	BlinnShader shader;
	shader.uniform_M() = ModelView;
	// �ο���fundamentals of computer graphics����6.2.2��,�����˷������ڽ�������任ʱΪʲô���ǳ���ModelView��������������ת��
	shader.uniform_MIT() = ModelView.inverse().transpose(); 

	for (int i = 0; i < model->nfaces(); ++i) {
		Vector4f ScreenCor[3];
		for (int j = 0; j < 3; ++j) {
			ScreenCor[j] = shader.vertex(i, j);
		}
		DrawTriangle(ScreenCor, zBuffer, image, shader, i);
	}
	

	//image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	// write output
	image.write_tga_file("output.tga");

	return 0;
}





