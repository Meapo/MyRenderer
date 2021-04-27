#pragma once
#include<Eigen>

using namespace Eigen;

class Material
{
public:
	Material() : _Ambient(0, 0, 0), _Diffuse(0, 0, 0), _Specular(0, 0, 0), _Emissive(0, 0, 0) {
		_TransparencyFactor = .0f;
		_Shininess = .0f;
		_ReflectionFactor = .0f;
	};

	Vector3f& Ambient() {
		return _Ambient;
	}
	Vector3f& Diffuse(){
		return _Diffuse; 
	}
	Vector3f& Specular() {
		return _Specular;
	}
	Vector3f& Emissive() {
		return _Emissive;
	}
	float& TransparencyFactor(){
		return _TransparencyFactor;
	}
	float& Shininess() {
		return _Shininess;
	}
	float& ReflectionFactor() {
		return _ReflectionFactor;
	}
private:
	Vector3f _Ambient;
	Vector3f _Diffuse;
	Vector3f _Specular;
	Vector3f _Emissive;
	float _TransparencyFactor;
	float _Shininess;
	float _ReflectionFactor;
};




