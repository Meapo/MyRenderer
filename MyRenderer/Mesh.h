#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Material.h"
#include "tgaimage.h"

using namespace std;

struct Vertex_f
{
	Vector4f position;
	Vector2f texCoords;
	Vector4f normal;
	Vector4f tangent;
};


class Mesh {
public:
	unsigned int DiffuseTextureID;
	unsigned int NormalTextureID;

	Mesh() { DiffuseTextureID = 0; NormalTextureID = 0; };
	Mesh(const vector<Vertex_f>& verts, const vector<vector<unsigned int> >& inds,
		const unordered_map<unsigned int, TGAImage*>& textures,
		const Material& materials, unsigned int diffuseTextureID, unsigned int normalTextureID)
		: _vertices(verts), _indices(inds), _textures(textures), _materials(materials) 
	{
		DiffuseTextureID = diffuseTextureID; NormalTextureID = normalTextureID;
	}
	const vector<Vertex_f>& vertices() { 
		return _vertices; }
	const vector<vector<unsigned int> >& indices() {
		return _indices;
	}
	const unordered_map<unsigned int, TGAImage*>& textures() {
		return _textures;
	}
	Material& materials() {
		return _materials;
	}
	unsigned int nverts();
	unsigned int nfaces();
	Vertex_f& vert(size_t i);
	Vertex_f& vert(size_t faceInd, size_t vertInFace);
	std::vector<unsigned int> face(size_t idx);
	const Vector4f const getTextureColor(size_t texInd, const Vector2f& uv);
	const Vector4f const getTextureNormal(size_t texInd, const Vector2f& uv);
	Material& getMaterial();
private:
	vector<Vertex_f> _vertices;
	vector<vector<unsigned int> > _indices; 
	unordered_map<unsigned int, TGAImage*> _textures;
	Material _materials;
};

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t);
