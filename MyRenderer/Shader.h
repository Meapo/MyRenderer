#pragma once
#include <Eigen>
#include "tgaimage.h"
#include "model.h"
#include "EigenExtension.h"

using namespace Eigen;

extern Model* model;
extern Matrix4f ModelView;
extern Matrix4f Projection;
extern Matrix4f Viewport;

template<class T>
static T interpolateVarings(T src_varings[3], const Vector3f& weights, const Vector3f& z) {
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

class IShader
{
public:
	virtual ~IShader();
	virtual Vector4f vertex(size_t faceInd, size_t vertInFace) = 0;
	virtual bool fragment(const Vector3f& baryCor, TGAColor& color, const Vector3f& z, const size_t faceInd) = 0;
};

class BlinnShader : public IShader
{
private:
	Vector2f _varying_uv[3];
	Vector4f _varying_norm[3];
	Matrix4f _uniform_M;    // View * Model
	Matrix4f _uniform_MIT;  // (View * Model).Inverse().transpose()
	Vector3f _clip_tri[3];      // triangle position in clip coordinate
	Matrix3f get_InverseTBNMatrix(const Vector3f& normal);
public:
	BlinnShader() : _varying_uv(), _uniform_M(), _uniform_MIT() {};
	Vector2f& varying_uv(size_t i) { return _varying_uv[i]; }
	Matrix4f& uniform_M() { return _uniform_M; }
	Matrix4f& uniform_MIT() { return _uniform_MIT; }
	virtual Vector4f vertex(size_t faceInd, size_t vertInFace) {
		const Vertex_f& vertex = model->vert(faceInd, vertInFace);
		_varying_uv[vertInFace] = vertex.uv;
		_varying_norm[vertInFace] = vertex.normal;
		Vector4f vertexCor = vertex.position;
		Vector4f clipCor = ModelView * vertexCor;
		_clip_tri[vertInFace] = Vec4f2Vec3f( clipCor / clipCor[3]);
		return Viewport * Projection * clipCor;
	}

	virtual bool fragment(const Vector3f& baryCor, TGAColor& color, const Vector3f& z, const size_t faceInd) {
		Vector2f interpolateUV = interpolateVarings(_varying_uv, baryCor, z);
		Vector3f interpolateNorm = Vec4f2Vec3f(_uniform_MIT * interpolateVarings(_varying_norm, baryCor, z));
		interpolateNorm.normalize();
		Matrix3f inverseTBN = get_InverseTBNMatrix(interpolateNorm);
		Vector3f normal(Vec4f2Vec3f(_uniform_MIT * model->getTextureNormal(model->face(faceInd)[5], interpolateUV)));
		normal = (inverseTBN * normal).normalized();
		Vector3f light(0, 0, 1);
		light.normalize();
		float intensity = std::max(.0f, normal.dot(light));
		// color = TGAColor(model->getTextureColor(model->face(faceInd)[4], interpolateUV) * intensity);
		color = TGAColor(model->getTextureColor(model->face(faceInd)[4], interpolateUV));

		return false;
	}
};


