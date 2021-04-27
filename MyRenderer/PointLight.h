#pragma once
#include <Eigen>

using namespace Eigen;

class PointLight
{
private:
	Vector4f _position;
	float _intensity;
	float _attenuation;
public:
	PointLight() : _position(.0f, .0f, .0f, .0f), _intensity(1.0f), _attenuation(1.0f){};
	PointLight(const Vector4f& pos, float inten, float atten) : _position(pos), _intensity(inten), _attenuation(atten) {};
	~PointLight();
	Vector4f positon() { return _position; }
	const float intensity() { return _intensity; }
	const float attenuation() { return _attenuation; }
};

