#pragma once
#include "tgaimage.h"
#include <Eigen>
#include "model.h"

extern const int height;
extern const int width;

using namespace Eigen;

void DrawLine(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color);

Vector3f barycentric(Vertex_f* pts, Vector4f& P);

bool isInTriangle(Vertex_f* pts, Vector4f& P);

void DrawTriangle(Vertex_f* pts, std::vector<float>& zBuffer, TGAImage& image, const TGAImage& texture);

void DrawTriangle_MSAA(Vertex_f* pts, std::vector<float>& zBuffer, std::vector<Vector4f>& colorBuffer, TGAImage& image, const TGAImage& texture, size_t MSAAX);

static float InterpolateDepth(const Vector3f& weights, const Vector3f& depths);


template<class T>
static T interpolateVarings(T src_varings[3], Vector3f& weights, const Vector3f& z) {
	T result;
	result.setZero();
	float sum = .0f;
	for (size_t i = 0; i < 3; i++)
	{
		float temp = weights[i] / z[i];
		result += src_varings[i] * temp;
		sum += temp;
	}
	return result / sum;
}
