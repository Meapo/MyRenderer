#include "EigenExtension.h"

Vector3f Vec4f2Vec3f(const Vector4f& vec) {
	return Vector3f(vec[0], vec[1], vec[2]);
}