#pragma once

#include <vector>
#include <unordered_map>
#include <fbxsdk.h>
#include "tgaimage.h"
#include "Material.h"
#include "Mesh.h";

class Model {
private:
	std::vector<Vertex_f> verts_;
	std::vector<std::vector<int> > faces_; // 前三个代表顶点索引，第四个代表材质索引，第五个代表Diffuse纹理索引, 第六个代表Normal纹理索引
	std::vector<Vector4f> bBox;
	std::unordered_map<int, Material> materials_;
	std::unordered_map<int, TGAImage*> textures_;
	std::string modelFilePath_;


	void ReadNode(FbxNode* pNode);
	void ReadPolygon(FbxMesh* pMesh, const size_t faceInd, size_t& triangleCounter, size_t& PreVertexCounter, const size_t beginVertsInd);
	void ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t VertsInd);
	void ReadUV(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCounter, const size_t uvLayer);
	void ReadNormal(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter);
	void ReadTangent(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter);
	void ConnectMaterialToMesh(FbxMesh* pMesh);
	void LoadMaterial(FbxMesh* pMesh, const size_t beginFaceInd);
	void LoadMaterialAttribute(FbxSurfaceMaterial* pSurfaceMaterial, const size_t beginFaceInd);
	void LoadMaterialTexture(FbxSurfaceMaterial* pSurfaceMaterial, const std::string& nodeName, const size_t begingFaceInd);
	void UpdateBoundingBox(const Vector4f& newPoint);
	void ProcessMesh(FbxNode* pNode);
	void ProcessSkeleton(FbxNode* pNode);
public:
	Model(const char* path, const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vertex_f& vert(size_t i);
	Vertex_f& vert(size_t faceInd, size_t vertInFace);
	std::vector<int> face(size_t idx);
	const Vector4f& MinBBox();
	const Vector4f& MaxBBox();
	const Vector4f const getTextureColor(size_t texInd, const Vector2f& uv);
	const Vector4f const getTextureNormal(size_t texInd, const Vector2f& uv);
	Material& getMaterial(size_t);
	const std::string& modelFilePath() { return modelFilePath_; }
};

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t);

