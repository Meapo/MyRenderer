#pragma once
#include <Eigen>

using namespace Eigen;

struct Vertex_f
{
	Vector4f position;
	Vector2f texCoords;
	Vector4f normal;
	Vector4f tangent;
};

