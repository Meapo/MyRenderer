#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <Eigen>

using namespace Eigen;

struct Vertex_f
{
	Vector4f position;
	Vector2f texCoord;
	Vector3f normal;
};

class Model {
private:
	std::vector<Vertex_f> verts_;
	std::vector<std::vector<int> > faces_;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vertex_f vert(int i);
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__
