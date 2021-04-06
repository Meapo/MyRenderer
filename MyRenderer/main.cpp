#include<iostream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <fbxsdk.h>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model* model = nullptr;
const int height = 800;
const int width = 800;

void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color) {
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

Vec3f barycentric(Vec3f* pts, Vec3f& P) {
	Vec3f crs = cross(Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x),
		Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y));
	if (std::abs(crs.z) < 1e-2)   // 意味着crs.z == 0，即三角形的边共线，返回一个负值即可
		return Vec3f(-1, 1, 1);
	return Vec3f(1.0f - crs.x / crs.z - crs.y / crs.z, crs.x / crs.z, crs.y / crs.z);
}

//float Interpolation(Vec3f& baryCor, Vec3f& interpolateVec) {
//	return baryCor * interpolateVec;
//}

void triangle(Vec3f* pts, float* zBuffer, TGAImage& image, TGAColor color) {
	// 找到bounding box 大小
	Vec2f boundingBoxMin(image.get_width() - 1, image.get_height() - 1);
	Vec2f boundingBoxMax(0, 0);
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; ++i) {
		boundingBoxMax.x = std::min(clamp.x, std::max(boundingBoxMax.x, std::ceil(pts[i].x)));
		boundingBoxMax.y = std::min(clamp.x, std::max(boundingBoxMax.y, std::ceil(pts[i].y)));
		boundingBoxMin.x = std::max(0.f, std::min(boundingBoxMin.x, std::floor(pts[i].x)));
		boundingBoxMin.y = std::max(0.f, std::min(boundingBoxMin.y, std::floor(pts[i].y)));
	}
	// 遍历bounding box的点
	Vec3f Point;
	for (Point.x = boundingBoxMin.x; Point.x <= boundingBoxMax.x; ++Point.x) {
		for (Point.y = boundingBoxMin.y; Point.y <= boundingBoxMax.y; ++Point.y) {
			Vec3f baryCor = barycentric(pts, Point);
			if (baryCor.x < 0 || baryCor.y < 0 || baryCor.z < 0)
				continue;  // 点在三角形外
			// z插值
			Point.z = baryCor * Vec3f(pts[0].z, pts[1].z, pts[2].z);
			int zBufferInd = static_cast<int>(Point.x) + static_cast<int>(Point.y) * width;
			if (Point.z > zBuffer[zBufferInd]) {
				zBuffer[zBufferInd] = Point.z;
				image.set(static_cast<int>(Point.x), static_cast<int>(Point.y), color);
			}
		}
	}
}

#pragma region drawmain
//int main(int argc, char** argv) {
//	// read model
//	if (2 == argc)
//	{
//		model = new Model(argv[1]);
//	}
//	else {
//		model = new Model("obj/african_head.obj");
//	}
//	// set image's width/height/colortype
//	TGAImage image(width, height, TGAImage::RGB);
//	// draw Model
//	Vec3f lightDir(0, 0, -1);
//	float* zBuffer = new float[width * height];
//	for (int i = 0; i < width * height; ++i)
//		zBuffer[i] = std::numeric_limits<float>::min();
//	for (int i = 0; i < model->nfaces(); ++i) {
//		std::vector<int> face = model->face(i);
//		Vec3f Points[3];
//		Vec3f worldCoords[3];
//		for (int j = 0; j < 3; ++j) {
//			worldCoords[j] = model->vert(face[j]);
//			Points[j] = Vec3f((worldCoords[j].x + 1) * width / 2, (worldCoords[j].y + 1) * height / 2, worldCoords[j].z);
//		}
//		Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
//		n.normalize();
//		float lightIntense = lightDir * n;
//		if (lightIntense > 0)
//			triangle(Points, zBuffer, image, TGAColor(255 * lightIntense, 255 * lightIntense, 255 * lightIntense, 255));
//	}
//	// draw triangle
//	/*Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
//	triangle(pts, image, red);*/
//
//	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
//	// write output
//	image.write_tga_file("output.tga");
//
//
//	delete[] zBuffer;
//	return 0;
//}
#pragma endregion

#pragma region fbxtest
 /* Tab character ("\t") counter */
int numTabs = 0;

/**
 * Print the required number of tabs.
 */
void PrintTabs() {
	for (int i = 0; i < numTabs; i++)
		printf("\t");
}

/**
 * Return a string-based representation based on the attribute type.
 */
FbxString GetAttributeTypeName(FbxNodeAttribute::EType type) {
	switch (type) {
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}

/**
 * Print an attribute.
 */
void PrintAttribute(FbxNodeAttribute* pAttribute) {
	if (!pAttribute) return;

	FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
	FbxString attrName = pAttribute->GetName();
	PrintTabs();
	// Note: to retrieve the character array of a FbxString, use its Buffer() method.
	printf("<attribute type='%s' name='%s'/>\n", typeName.Buffer(), attrName.Buffer());
}

/**
 * Print a node, its attributes, and all its children recursively.
 */
void PrintNode(FbxNode* pNode) {
	PrintTabs();
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	// Print the contents of the node.
	printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);
	numTabs++;

	// Print the node's attributes.
	for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
		PrintAttribute(pNode->GetNodeAttributeByIndex(i));

	// Recursively print the children.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		PrintNode(pNode->GetChild(j));

	numTabs--;
	PrintTabs();
	printf("</node>\n");
}

/**
 * Main function - loads the hard-coded fbx file,
 * and prints its contents in an xml format to stdout.
 */
int main(int argc, char** argv) {
	const char* lFileName;
	if (2 == argc)
	{
		lFileName = argv[1];
	}
	else {
		lFileName = "obj/morphling_econ.fbx";
	}

	// Initialize the SDK manager. This object handles all our memory management.
	FbxManager* lSdkManager = FbxManager::Create();

	// Create the IO settings object.
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer using the SDK manager.
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(lFileName, -1, lSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported; so get rid of the importer.
	lImporter->Destroy();

	// Print the nodes of the scene and their attributes recursively.
	// Note that we are not printing the root node because it should
	// not contain any attributes.
	FbxNode* lRootNode = lScene->GetRootNode();
	if (lRootNode) {
		for (int i = 0; i < lRootNode->GetChildCount(); i++)
			PrintNode(lRootNode->GetChild(i));
	}
	// Destroy the SDK manager and all the other objects it was handling.
	lSdkManager->Destroy();
	return 0;


	return 0;
}
#pragma endregion





