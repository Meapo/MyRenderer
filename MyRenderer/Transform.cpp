#include "Transform.h"


/*
* Summary: 把模型的bouding box映射到中心在世界坐标系原点的2x2x2的矩阵下
*/
Matrix4f get_ModelTransformMatrix(const Vector4f& MinBBox, const Vector4f& MaxBBox) {
	// translate
	Vector4f BBoxMidPoint = (MinBBox + MaxBBox) / 2;
	Matrix4f TranslateMatrix = get_TranslateMatrix(-BBoxMidPoint);
	// scale
	float maxLength = 0.0f;
	for (size_t i = 0; i < 3; i++)
	{
		float temp = MaxBBox[i] - MinBBox[i];
		if (temp > maxLength)
		{
			maxLength = temp;
		}
	}
	if (maxLength == 0.0f)
		throw "Model bounding box max length is equal to 0.0f.";
	float scale = 2 / maxLength;
	Matrix4f ScaleMatrix = get_ScaleMatrix(Vector4f(scale, scale, scale, .0f));
	return ScaleMatrix * TranslateMatrix;
}

Vector4f get_CrossProduct(const Vector4f& vec1, const Vector4f& vec2) {
	Vector3f tempVec(Vector3f(vec1[0], vec1[1], vec1[2]).cross(Vector3f(vec2[0], vec2[1], vec2[2])));
	return Vector4f(tempVec[0], tempVec[1], tempVec[2], .0f);
}

Matrix4f get_ViewTransformMatrix(const Vector4f& camPos, const Vector4f& camLookAt, const Vector4f& camUp) {
	// translate
	Matrix4f TranslateMatrix = get_TranslateMatrix(-camPos);
	// Rotate
	Vector4f camZ = -camLookAt.normalized();
	Vector4f camY = camUp.normalized();
	Vector4f camX = get_CrossProduct(camY, camZ);
	Matrix4f RotateMatrix = get_RotateMatrixWithAxisValue(camX, camY, camZ);
	return RotateMatrix * TranslateMatrix;
}

/*
* Summary: 获取透视投影矩阵
* Parameter:
*	eye_fov: 视野上下边界的夹角角度
*	aspect_ratio: aspect_ratio = width / height ,其中width、height分别为近平面的宽度和高度
*	zNear: 近平面的坐标（负值）
*	zFar: 远平面的坐标（负值）
*/
Matrix4f get_PerspectiveProjectionMatrix(float eye_fov, float aspect_ratio, float zNear, float zFar) {
	Matrix4f PerspectToOrth;
	PerspectToOrth << zNear, .0f, .0f, .0f,
		.0f, zNear, .0f, .0f,
		.0f, .0f, zNear + zFar, -zNear * zFar,
		.0f, .0f, 1.0f, .0f;
	Matrix4f Orth = get_OrthogonalProjectionMatrix(eye_fov, aspect_ratio, zNear, zFar);
	return Orth * PerspectToOrth;
}

Matrix4f get_OrthogonalProjectionMatrix(float eye_fov, float aspect_ratio, float zNear, float zFar) {
	float halfHeight = zNear * tanf(eye_fov / 2);
	float halfWidth = halfHeight * aspect_ratio;
	float zMid = (zNear + zFar) / 2;
	Matrix4f translateMatrix = get_TranslateMatrix(Vector4f(.0f, .0f, -zMid, .0f));
	Matrix4f scaleMatrix = get_ScaleMatrix(Vector4f(1 / halfWidth, 1 / halfHeight, 2 / (zNear - zFar), .0f));
	return scaleMatrix * translateMatrix;
}

Matrix4f get_ViewportMatrix(const float width, const float height) {
	Matrix4f ViewportMatrix;
	float halfW = width / 2, halfH = height / 2;
	ViewportMatrix << halfW, .0f, .0f, halfW,
		.0f, halfH, .0f, halfH,
		.0f, .0f, 1.0f, .0f,
		.0f, .0f, .0f, 1.0f;
	return ViewportMatrix;
}

Matrix4f get_TranslateMatrix(const Vector4f& vec) {
	Matrix4f result;
	result << 1.0f, .0f, .0f, vec.x(),
		.0f, 1.0f, .0f, vec.y(),
		.0f, .0f, 1.0f, vec.z(),
		.0f, .0f, .0f, 1.0f;
	return result;
}

Matrix4f get_ScaleMatrix(const Vector4f& vec) {
	Matrix4f result;
	result << vec[0], .0f, .0f, .0f,
		.0f, vec[1], .0f, .0f,
		.0f, .0f, vec[2], .0f,
		.0f, .0f, .0f, 1.0f;
	return result;
}

Matrix4f get_RotateMatrixWithAxisValue(const Vector4f& axisX, const Vector4f& axisY, const Vector4f& axisZ) {
	Matrix4f RotateInverse;
	RotateInverse << axisX, axisY, axisZ, Vector4f(.0f, .0f, .0f, 1.0f);  // 按照列向量构成RoateInverse矩阵
	return RotateInverse.transpose();
}