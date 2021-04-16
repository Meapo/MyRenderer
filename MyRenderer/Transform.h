#pragma once
#include <Eigen>

using namespace Eigen;


Matrix4f get_ModelTransformMatrix(const Vector4f& MinBBox, const Vector4f& MaxBBox);

Matrix4f get_ViewTransformMatrix(const Vector4f& camPos, const Vector4f& camLookAt, const Vector4f& camUp);

Matrix4f get_PerspectiveProjectionMatrix(float eye_fov, float aspect_ratio, float zNear, float zFar);

Matrix4f get_OrthogonalProjectionMatrix(float eye_fov, float aspect_ratio, float zNear, float zFar);

Matrix4f get_ViewportMatrix(const float width, const float height);

Matrix4f get_TranslateMatrix(const Vector4f& vec);

Matrix4f get_ScaleMatrix(const Vector4f& vec);

Matrix4f get_RotateMatrixWithAxisValue(const Vector4f& axisX, const Vector4f& axisY, const Vector4f& axisZ);