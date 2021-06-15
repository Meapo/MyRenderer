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
extern Vector4f LightDir;

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

class WhiteColorShader : public IShader {
public:
	WhiteColorShader() {};
	virtual Vector4f vertex(size_t faceInd, size_t vertInFace) {
		const Vertex_f& vertex = model->vert(faceInd, vertInFace);
		return Viewport * Projection * ModelView * vertex.position;
	}
	virtual bool fragment(const Vector3f& baryCor, TGAColor& color, const Vector3f& z, const size_t faceInd) {
		color = TGAColor( 255.0f,  255.0f,  255.0f, 255.0f);
		return false;
	}
};

class BlinnShader : public IShader
{
private:
	Vector3f _light;
	Vector2f _varying_uv[3];
	Vector4f _varying_norm[3];
	Matrix4f _uniform_M;    // View * Model
	Matrix4f _uniform_MIT;  // (View * Model).Inverse().transpose()
	Vector3f _clip_tri[3];      // triangle position in clip coordinate
	Matrix3f get_InverseTBNMatrix(const Vector3f& normal);
public:
	BlinnShader() : _light(), _varying_uv(), _uniform_M(), _uniform_MIT() { _light = Vec4f2Vec3f(ModelView * LightDir).normalized(); };
	Vector2f& varying_uv(size_t i) { return _varying_uv[i]; }
	Matrix4f& uniform_M() { return _uniform_M; }
	Matrix4f& uniform_MIT() { return _uniform_MIT; }
	virtual Vector4f vertex(size_t faceInd, size_t vertInFace) {
		const Vertex_f& vertex = model->vert(faceInd, vertInFace);
		_varying_uv[vertInFace] = vertex.texCoords;
		_varying_norm[vertInFace] = vertex.normal;
		Vector4f vertexCor = vertex.position;
		Vector4f clipCor = ModelView * vertexCor;
		_clip_tri[vertInFace] = Vec4f2Vec3f( clipCor / clipCor[3]);
		return Viewport * Projection * clipCor;
	}

	virtual bool fragment(const Vector3f& baryCor, TGAColor& color, const Vector3f& z, const size_t faceInd) {
		Vector2f interpolateUV = interpolateVarings(_varying_uv, baryCor, z);
		Vector3f interpolateNorm = Vec4f2Vec3f(_uniform_MIT * interpolateVarings(_varying_norm, baryCor, z));
		Vector3f interpolateCor = interpolateVarings(_clip_tri, baryCor, z);
		interpolateNorm.normalize(); 
		Matrix3f inverseTBN = get_InverseTBNMatrix(interpolateNorm);
		Vector3f normal(Vec4f2Vec3f(model->getTextureNormal(model->face(faceInd)[5], interpolateUV)));
	    normal =  normal.normalized();
		const float Shininess = model->getMaterial(model->face(faceInd)[3]).Shininess();
		Vector3f Ambient = model->getMaterial(model->face(faceInd)[3]).Ambient();
		Vector3f Diffuse = model->getMaterial(model->face(faceInd)[3]).Diffuse() * std::max(.0f, normal.dot(_light));
		Vector3f Specular = model->getMaterial(model->face(faceInd)[3]).Specular() * std::max(.0f, std::powf(normal.dot((_light - interpolateCor).normalized()), Shininess));
		Vector3f light = Ambient + Diffuse + Specular;

		TGAColor c = model->getTextureColor(model->face(faceInd)[4], interpolateUV);
		for (int i = 0; i < 3; i++)
			color[i] = std::min<int>(c[i] * light[i], 255); // (a bit of ambient light, diff + spec), clamp the result
		color[3] = 255;
		return false;
	}
};


