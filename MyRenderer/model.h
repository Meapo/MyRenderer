#pragma once
#include <fbxsdk.h>
#include "Mesh.h";

class Model {
private:
	vector<Mesh> _meshes;
	vector<Vector4f> bBox;
	string modelFilePath_;

	void ReadNode(FbxNode* pNode);
	void ReadPolygon(FbxMesh* pMesh, const size_t PolygonInd, size_t& triangleCounter, size_t& PreVertexCounter,
		vector<vector<unsigned int> >& indices, vector<Vertex_f>& verts_);
	void ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd, vector<Vertex_f>& verts_);
	void ReadUV(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCounter, const size_t uvLayer, vector<Vertex_f>& verts_);
	void ReadNormal(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter, vector<Vertex_f>& verts_);
	void ReadTangent(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter, vector<Vertex_f>& verts_);
	void ConnectMaterialToMesh(FbxMesh* pMesh, vector<vector<unsigned int>>& indices);
	void LoadMaterial(FbxMesh* pMesh, unordered_map<unsigned int, TGAImage*>& textures_,
		unsigned int& DiffuseTextureID, unsigned int& NormalTextureID, Material& materials_);
	void LoadMaterialAttribute(FbxSurfaceMaterial* pSurfaceMaterial, Material& materials_);
	void LoadMaterialTexture(FbxSurfaceMaterial* pSurfaceMaterial, const std::string& nodeName,
		unordered_map<unsigned int, TGAImage*>& textures_, unsigned int& DiffuseTextureID, unsigned int& NormalTextureID);
	void UpdateBoundingBox(const Vector4f& newPoint);
	void ProcessMesh(FbxNode* pNode);
	void ProcessSkeleton(FbxNode* pNode);
public:
	unsigned int vertsCount;
	unsigned int faceCount;
	vector<Mesh>& meshes() { return _meshes; }
	Model(const char* path, const char *filename);
	~Model();
	const Vector4f& MinBBox();
	const Vector4f& MaxBBox();
	const std::string& modelFilePath() { return modelFilePath_; }
};


