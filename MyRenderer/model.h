#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <unordered_map>
#include <Eigen>
#include <fbxsdk.h>

using namespace Eigen;

struct Vertex_f
{
	Vector4f position;
	Vector2f DiffuseCoord;
	Vector3f normal;
	Vector3f tangent;
	// Vector4f color;
};

class Model {
private:
	std::vector<Vertex_f> verts_;
	std::vector<std::vector<int> > faces_;
	//std::unordered_map<
	void ReadNode(FbxNode* pNode);
	void ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd);
	// void ReadColor(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCount);
	void ProcessMesh(FbxNode* pNode);
	void ProcessSkeleton(FbxNode* pNode);
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vertex_f vert(int i);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
