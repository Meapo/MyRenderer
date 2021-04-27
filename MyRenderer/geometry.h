#pragma once
#include "tgaimage.h"
#include <Eigen>
#include "model.h"
#include "PointLight.h"
#include "Shader.h"

extern const int height;
extern const int width;

using namespace Eigen;

void DrawLine(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color);

Vector3f barycentric(const Vertex_f* pts, const Vector4f& P);

bool isInTriangle(const Vertex_f* pts, const Vector4f& P);

bool isInTriangleByBary(const Vector3f& baryCor);

void DrawTriangle(Vector4f pts[3], std::vector<float>& zBuffer, TGAImage& image, IShader& shader, const size_t faceInd);

void DrawTriangle_MSAA(Vertex_f* pts, std::vector<float>& zBuffer, std::vector<Vector4f>& colorBuffer, TGAImage& image, const TGAImage& texture, size_t MSAAX);

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t);

static float InterpolateDepth(const Vector3f& weights, const Vector3f& depths);



