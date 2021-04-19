#include "Geometry.h"

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
	// steep只需要判断一次，把steep的判断放在循环外可减少判断次数
	// 另外表达式(y1 > y0 ? 1 : -1)只需要计算一次，可以采用const int 替换以减少表达式计算次数。
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

Vector3f barycentric(Vertex_f* pts, Vector4f& P) {
	Vector3f vec1 = Vector3f(pts[1].position.x() - pts[0].position.x(), pts[2].position.x() - pts[0].position.x(), pts[0].position.x() - P.x());
	Vector3f vec2 = Vector3f(pts[1].position.y() - pts[0].position.y(), pts[2].position.y() - pts[0].position.y(), pts[0].position.y() - P.y());
	Vector3f crs = vec1.cross(vec2);
	if (std::abs(crs.z()) < 1e-2)   // 意味着crs.z == 0，即三角形的边共线，返回一个负值即可
		return Vector3f(-1, 1, 1);
	return Vector3f(1.0f - crs.x() / crs.z() - crs.y() / crs.z(), crs.x() / crs.z(), crs.y() / crs.z());
}

bool isInTriangle(Vertex_f* pts, Vector4f& P) {
	Vector3f baryCor = barycentric(pts, P);
	if (baryCor.x() < 0 || baryCor.y() < 0 || baryCor.z() < 0)
		return false;  // 点在三角形外
	else
		return true;
}

void DrawTriangle(Vertex_f* pts, std::vector<float>& zBuffer, TGAImage& image, const TGAImage& texture) {
	// 找到bounding box 大小
	Vector2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vector2f boundingBoxMax(0, 0);
	Vector2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x() = std::min(clamp.x(), std::max(boundingBoxMax.x(), std::ceil(pts[i].position.x())));
		boundingBoxMax.y() = std::min(clamp.x(), std::max(boundingBoxMax.y(), std::ceil(pts[i].position.y())));
		boundingBoxMin.x() = std::max(0.f, std::min(boundingBoxMin.x(), std::floor(pts[i].position.x())));
		boundingBoxMin.y() = std::max(0.f, std::min(boundingBoxMin.y(), std::floor(pts[i].position.y())));
	}

	Vector3f PointsZ(pts[0].position.w(), pts[1].position.w(), pts[2].position.w());
	Vector2f PointsUV[3]{ pts[0].DiffuseCoord, pts[1].DiffuseCoord, pts[2].DiffuseCoord };
	// 遍历bounding box的点
	Vector4f Point;
	for (Point.x() = boundingBoxMin.x(); Point.x() <= boundingBoxMax.x(); ++Point.x()) {
		for (Point.y() = boundingBoxMin.y(); Point.y() <= boundingBoxMax.y(); ++Point.y()) {
			if (!isInTriangle(pts, Point))
				continue;  // 点在三角形外
			// z插值
			Vector3f baryCor = barycentric(pts, Point);
			Point.z() = InterpolateDepth(baryCor, PointsZ);
			int zBufferInd = static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width;
			if (Point.z() > zBuffer[zBufferInd]) {
				zBuffer[zBufferInd] = Point.z();
				// UV插值
				Vector2f interpolateUV = interpolateVarings(PointsUV, baryCor, PointsZ);
				image.set(Point.x(), Point.y(), texture.get(interpolateUV[0] * texture.get_width(), (1 - interpolateUV[1]) * texture.get_height()));
			}
		}
	}
}

void DrawTriangle_MSAA(Vertex_f* pts, std::vector<float>& zBuffer, std::vector<Vector4f>& colorBuffer, TGAImage& image, const TGAImage& texture, size_t MSAAX) {
	if (MSAAX > 4)
	{
		"Error: MSAAX is more than 4. The program will fixed it to 4.";
		MSAAX = 4;
	}
	// 找到bounding box 大小
	Vector2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vector2f boundingBoxMax(0, 0);
	Vector2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x() = std::min(clamp.x(), std::max(boundingBoxMax.x(), std::ceil(pts[i].position.x())));
		boundingBoxMax.y() = std::min(clamp.x(), std::max(boundingBoxMax.y(), std::ceil(pts[i].position.y())));
		boundingBoxMin.x() = std::max(0.f, std::min(boundingBoxMin.x(), std::floor(pts[i].position.x())));
		boundingBoxMin.y() = std::max(0.f, std::min(boundingBoxMin.y(), std::floor(pts[i].position.y())));
	}
	// 初始化三个顶点的属性数组
	Vector3f PointsZ(pts[0].position.w(), pts[1].position.w(), pts[2].position.w());
	Vector2f PointsUV[3]{ pts[0].DiffuseCoord, pts[1].DiffuseCoord, pts[2].DiffuseCoord };
	// 遍历bounding box的点
	Vector4f Point;
	size_t MSAAX_square = MSAAX * MSAAX;
	const float fillUnitValue = 1.0f / MSAAX_square;
	for (Point.x() = boundingBoxMin.x(); Point.x() <= boundingBoxMax.x(); ++Point.x()) {
		for (Point.y() = boundingBoxMin.y(); Point.y() <= boundingBoxMax.y(); ++Point.y()) {
			std::vector<Vector4f> MSAAPoint(MSAAX_square); // MSAAPoint中每一个向量的第4个值代表是否在三角形内部，1.0f代表是，0.0f代表否
			float fillValue = .0f;
			for (size_t k = 0; k < MSAAX_square; ++k) {
				size_t xk = k % MSAAX, yk = k / MSAAX;
				MSAAPoint[k].x() = Point.x()  + (0.5f + xk) / MSAAX;
				MSAAPoint[k].y() = Point.y()  + (0.5f + yk) / MSAAX;
				if (isInTriangle(pts, MSAAPoint[k])) {
					fillValue += fillUnitValue;
					MSAAPoint[k].w() = 1.0f;
				}
				else
					MSAAPoint[k].w() = 0.0f;
			}
			if (fillValue < fillUnitValue)
				continue;
			// 对每个在三角形内的子采样点，更新zBuffer
			for (size_t k = 0; k < MSAAX_square; k++)
			{
				if (MSAAPoint[k].w() == 0.0f)
					continue;
				Vector3f baryCor = barycentric(pts, MSAAPoint[k]);
				MSAAPoint[k].z() = InterpolateDepth(baryCor, PointsZ);  // 插值得到像素点对应在空间中的z（透视投影矫正）
				int BufferInd = (static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width) * MSAAX_square + k;
				if (MSAAPoint[k].z() > zBuffer[BufferInd]) {
					zBuffer[BufferInd] = MSAAPoint[k].z();
					// uv插值(透视投影矫正)
					Vector2f interpolateUV = interpolateVarings(PointsUV, baryCor, PointsZ);
					colorBuffer[BufferInd] = texture.get(interpolateUV[0] * texture.get_width(), (1 - interpolateUV[1]) * texture.get_height()).Color2Vec4f();
				}
			}
			Vector4f blendColorVec(0, 0, 0, 0);
			for (size_t k = 0; k < MSAAX_square; k++) {
				int BufferInd = (static_cast<int>(Point.x()) + static_cast<int>(Point.y()) * width) * MSAAX_square + k;
				blendColorVec += colorBuffer[BufferInd];
			}
			image.set(static_cast<int>(Point.x()), static_cast<int>(Point.y()), TGAColor(blendColorVec * fillUnitValue));
		}
	}
}

static float InterpolateDepth(const Vector3f& weights, const Vector3f& depths) {
	return weights.dot(depths);
}

