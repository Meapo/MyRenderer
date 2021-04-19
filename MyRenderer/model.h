#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <unordered_map>
#include <Eigen>
#include <fbxsdk.h>
#include "tgaimage.h"

using namespace Eigen;

struct Vertex_f
{
	Vector4f position;
	Vector2f DiffuseCoord;
	Vector3f normal;
	Vector3f tangent;
};

class Model {
private:
	std::vector<Vertex_f> verts_;
	std::vector<std::vector<int> > faces_;
	std::vector<Vector4f> bBox;

	void ReadNode(FbxNode* pNode);
	void ReadTriangle(FbxMesh* pMesh, size_t startInd, size_t faceInd, size_t triangleCounter, size_t vertexCounter, const size_t faceSize);
	void ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd);
	void ReadUV(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCounter, const size_t uvLayer);
	void ReadNormal(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter);
	void ReadTangent(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter);
	void UpdateBoundingBox(const Vector4f& newPoint);
	void ProcessMesh(FbxNode* pNode);
	void ProcessSkeleton(FbxNode* pNode);
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vertex_f vert(int i);
	std::vector<int> face(int idx);
	Vector4f MinBBox();
	Vector4f MaxBBox();
};

#endif //__MODEL_H__
