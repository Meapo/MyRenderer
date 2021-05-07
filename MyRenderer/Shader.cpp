#include "Shader.h"


IShader::~IShader() {}

Matrix3f BlinnShader::get_InverseTBNMatrix(const Vector3f& normal) {
	Matrix3f A;
	A << (_clip_tri[1] - _clip_tri[0]), (_clip_tri[2] - _clip_tri[0]), normal;
	Matrix3f AI = A.inverse();
	Vector3f i(AI * Vector3f(_varying_uv[1][0] - _varying_uv[0][0], _varying_uv[2][0] - _varying_uv[0][0], 0));
	Vector3f j(AI * Vector3f(_varying_uv[1][1] - _varying_uv[0][1], _varying_uv[2][1] - _varying_uv[0][1], 0));
	i.normalize(); j.normalize();
	Matrix3f B;
	B << i, j, normal;
	return B;
}
