#include "Geometry.h"
#include <iostream>
void DrawWireFrame(Vector4f ScreenCor[3], TGAImage& image, const TGAColor& color) {
	for (size_t i = 0; i < 3; i++)
	{
		ScreenCor[i] /= ScreenCor[i].w();
	}
	DrawLine(ScreenCor[0].x(), ScreenCor[0].y(), ScreenCor[1].x(), ScreenCor[1].y(), image, color);
	DrawLine(ScreenCor[1].x(), ScreenCor[1].y(), ScreenCor[2].x(), ScreenCor[2].y(), image, color);
	DrawLine(ScreenCor[2].x(), ScreenCor[2].y(), ScreenCor[0].x(), ScreenCor[0].y(), image, color);
}

void DrawLine(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color) {
	bool steep = false;
	if (std::abs(x1 - x0) < std::abs(y1 - y0)) {
		steep = true;
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0, dy = y1 - y0;
	int _2dx = 2 * dx;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	const int yIncr = (y1 > y0) ? 1 : -1;
	int y = y0;
	if (steep) {
		for (int x = x0; x <= x1; ++x) {
			image.set(y, x, color);
			error2 += derror2;
			if (error2 > dx) {
				y += yIncr;
				error2 -= _2dx;
			}
		}
	}
	else {
		for (int x = x0; x <= x1; ++x) {
			image.set(x, y, color);
			error2 += derror2;
			if (error2 > dx) {
				y += yIncr;
				error2 -= _2dx;
			}
		}
	}
	// steepֻ��Ҫ�ж�һ�Σ���steep���жϷ���ѭ����ɼ����жϴ���
	// ������ʽ(y1 > y0 ? 1 : -1)ֻ��Ҫ����һ�Σ����Բ���const int �滻�Լ��ٱ��ʽ���������
	/*for (int x = x0; x <= x1; x++) {
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}*/
}

Vector3f barycentric(const Vector4f* pts, const Vector4f& P) {
	Vector3f vec1 = Vector3f(pts[1].x() - pts[0].x(), pts[2].x() - pts[0].x(), pts[0].x() - P.x());
	Vector3f vec2 = Vector3f(pts[1].y() - pts[0].y(), pts[2].y() - pts[0].y(), pts[0].y() - P.y());
	Vector3f crs = vec1.cross(vec2);
	if (std::abs(crs.z()) < 1e-2)   // ��ζ��crs.z == 0���������εı߹��ߣ�����һ����ֵ����
		return Vector3f(-1, 1, 1);
	return Vector3f(1.0f - crs.x() / crs.z() - crs.y() / crs.z(), crs.x() / crs.z(), crs.y() / crs.z());
}

bool isInTriangle(const Vector4f* pts, const Vector4f& P) {
	Vector3f baryCor = barycentric(pts, P);
	if (baryCor.x() < 0 || baryCor.y() < 0 || baryCor.z() < 0)
		return false;  // ������������
	else
		return true;
}

bool isInTriangleByBary(const Vector3f& baryCor) {
	if (baryCor.x() < 0 || baryCor.y() < 0 || baryCor.z() < 0)
		return false;  // ������������
	else
		return true;
}

void DrawTriangle(Vector4f pts[3], std::vector<float>& zBuffer, TGAImage& image, IShader& shader, size_t faceInd)
{
	Vector3f PointsZInCCS(pts[0].w(), pts[1].w(), pts[2].w()); // ��¼�������������ϵ�е�Zֵ, ���ڽ���͸��ͶӰ����ȷ
	for (size_t i = 0; i < 3; i++)
	{
		pts[i] /= pts[i].w();
	}
	Vector3f PointsZInFinal(pts[0].z(), pts[1].z(), pts[2].z()); // �������յ�����ϵ�е�zֵ
	// �ҵ�bounding box ��С
	Vector2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vector2f boundingBoxMax(0, 0);
	Vector2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x() = std::min(clamp.x(), std::max(boundingBoxMax.x(), std::ceil(pts[i].x())));
		boundingBoxMax.y() = std::min(clamp.y(), std::max(boundingBoxMax.y(), std::ceil(pts[i].y())));
		boundingBoxMin.x() = std::max(0.f, std::min(boundingBoxMin.x(), std::floor(pts[i].x())));
		boundingBoxMin.y() = std::max(0.f, std::min(boundingBoxMin.y(), std::floor(pts[i].y())));
	}
	// ����bounding box�ĵ�
	Vector4f Point;
	for (Point.x() = boundingBoxMin.x(); Point.x() <= boundingBoxMax.x(); ++Point.x()) {
		for (Point.y() = boundingBoxMin.y(); Point.y() <= boundingBoxMax.y(); ++Point.y()) {
			Vector3f baryCor = barycentric(pts, Point);
			if (!isInTriangleByBary(baryCor))
				continue;  // ������������
			// z��ֵ
			Point.z() = InterpolateDepth(baryCor, PointsZInFinal);
			float zInCCS = InterpolateDepth(baryCor, PointsZInCCS);
			int zBufferInd = static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width;
			if (zInCCS > zBuffer[zBufferInd]) {
				zBuffer[zBufferInd] = zInCCS;
				TGAColor color;
				if (!shader.fragment(baryCor, color, PointsZInCCS))
					image.set(Point.x(), Point.y(), color);
			}
		}
	}
}

//void DrawTriangle_MSAA(Vertex_f* pts, std::vector<float>& zBuffer, std::vector<Vector4f>& colorBuffer, TGAImage& image, const TGAImage& texture, size_t MSAAX) {
//	if (MSAAX > 4)
//	{
//		"Error: MSAAX is more than 4. The program will fixed it to 4.";
//		MSAAX = 4;
//	}
//	// �ҵ�bounding box ��С
//	Vector2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
//	Vector2f boundingBoxMax(0, 0);
//	Vector2f clamp(image.get_width() - 1, image.get_height() - 1);
//	for (int i = 0; i < 3; ++i) {
//		boundingBoxMax.x() = std::min(clamp.x(), std::max(boundingBoxMax.x(), std::ceil(pts[i].position.x())));
//		boundingBoxMax.y() = std::min(clamp.x(), std::max(boundingBoxMax.y(), std::ceil(pts[i].position.y())));
//		boundingBoxMin.x() = std::max(0.f, std::min(boundingBoxMin.x(), std::floor(pts[i].position.x())));
//		boundingBoxMin.y() = std::max(0.f, std::min(boundingBoxMin.y(), std::floor(pts[i].position.y())));
//	}
//	// ��ʼ�������������������
//	Vector3f PointsZ(pts[0].position.w(), pts[1].position.w(), pts[2].position.w());
//	Vector2f PointsUV[3]{ pts[0].uv, pts[1].uv, pts[2].uv };
//	// ����bounding box�ĵ�
//	Vector4f Point;
//	size_t MSAAX_square = MSAAX * MSAAX;
//	const float fillUnitValue = 1.0f / MSAAX_square;
//	for (Point.x() = boundingBoxMin.x(); Point.x() <= boundingBoxMax.x(); ++Point.x()) {
//		for (Point.y() = boundingBoxMin.y(); Point.y() <= boundingBoxMax.y(); ++Point.y()) {
//			std::vector<Vector4f> MSAAPoint(MSAAX_square); // MSAAPoint��ÿһ�������ĵ�4��ֵ�����Ƿ����������ڲ���1.0f�����ǣ�0.0f�����
//			float fillValue = .0f;
//			for (size_t k = 0; k < MSAAX_square; ++k) {
//				size_t xk = k % MSAAX, yk = k / MSAAX;
//				MSAAPoint[k].x() = Point.x()  + (0.5f + xk) / MSAAX;
//				MSAAPoint[k].y() = Point.y()  + (0.5f + yk) / MSAAX;
//				if (isInTriangle(pts, MSAAPoint[k])) {
//					fillValue += fillUnitValue;
//					MSAAPoint[k].w() = 1.0f;
//				}
//				else
//					MSAAPoint[k].w() = 0.0f;
//			}
//			if (fillValue < fillUnitValue)
//				continue;
//			// ��ÿ�����������ڵ��Ӳ����㣬����zBuffer
//			for (size_t k = 0; k < MSAAX_square; k++)
//			{
//				if (MSAAPoint[k].w() == 0.0f)
//					continue;
//				Vector3f baryCor = barycentric(pts, MSAAPoint[k]);
//				MSAAPoint[k].z() = InterpolateDepth(baryCor, PointsZ);  // ��ֵ�õ����ص��Ӧ�ڿռ��е�z��͸��ͶӰ������
//				int BufferInd = (static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width) * MSAAX_square + k;
//				if (MSAAPoint[k].z() > zBuffer[BufferInd]) {
//					zBuffer[BufferInd] = MSAAPoint[k].z();
//					// uv��ֵ(͸��ͶӰ����)
//					Vector2f interpolateUV = interpolateVarings(PointsUV, baryCor, PointsZ);
//					colorBuffer[BufferInd] = BilinearInterpolate(texture, interpolateUV[0], interpolateUV[1]);
//				}
//			}
//			Vector4f blendColorVec(0, 0, 0, 0);
//			for (size_t k = 0; k < MSAAX_square; k++) {
//				int BufferInd = (static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width) * MSAAX_square + k;
//				blendColorVec += colorBuffer[BufferInd];
//			}
//			image.set(static_cast<int>(Point.x()), static_cast<int>(Point.y()), TGAColor(blendColorVec * fillUnitValue));
//		}
//	}
//}

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t) {
	return vec0 + t * (vec1 - vec0);
}




static float InterpolateDepth(const Vector3f& weights, const Vector3f& depths) {
	Vector3f vec(1 / depths.x(), 1 / depths.y(), 1 / depths.z());
	return 1 / weights.dot(vec);
}

