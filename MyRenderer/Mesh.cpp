#include "Mesh.h"

unsigned int Mesh::nverts() {
    return _vertices.size();
}

unsigned int Mesh::nfaces() {
    return _indices.size();
}

std::vector<unsigned int> Mesh::face(size_t idx) {
    return _indices[idx];
}

Vertex_f& Mesh::vert(size_t i) {
    return _vertices[i];
}

Vertex_f& Mesh::vert(size_t faceInd, size_t vertInFace) {
    assert(vertInFace < 3);
    return _vertices[_indices[faceInd][vertInFace]];
}

const Vector4f const Mesh::getTextureColor(size_t texInd, const Vector2f& _UV) {
    assert(_textures.count(texInd));
    const TGAImage* texture = _textures[texInd];
    float x = _UV[0] * texture->get_width(), y = (1 - _UV[1]) * texture->get_height();
    Vector2i uv[4];
    uv[0] = { std::floor(x - .5f), std::floor(y - .5f) };
    uv[1] = { uv[0].x(), uv[0].y() + 1 };
    uv[2] = { uv[0].x() + 1, uv[0].y() + 1 };
    uv[3] = { uv[0].x() + 1, uv[0].y() };
    if (uv[0].x() >= 0 && uv[0].x() < (texture->get_width() - 1) && uv[0].y() >= 0 && uv[0].y() < (texture->get_height() - 1)) {
        // 在边界内使用bilinnear
        Vector4f color[4];
        for (size_t i = 0; i < 4; i++)
        {
            color[i] = texture->get(uv[i].x(), uv[i].y()).Color2Vec4f();
        }
        float t = x - uv[0].x() - .5f;
        Vector4f colorLerp0 = Lerp(color[0], color[3], t);
        Vector4f colorLerp1 = Lerp(color[1], color[2], t);
        float s = y - uv[0].y() - .5f;
        Vector4f result = Lerp(colorLerp0, colorLerp1, s);
        return result;
    }
    else {
        //在边界外使用最近的点
        Vector4f color = texture->get(std::round(x), std::round(y)).Color2Vec4f();
        return color;
    }
}

const Vector4f const Mesh::getTextureNormal(size_t texInd, const Vector2f& _UV) {
    assert(_textures.count(texInd));
    const TGAImage* NormalMap = _textures[texInd];
    float x = _UV[0] * NormalMap->get_width(), y = (1 - _UV[1]) * NormalMap->get_height();
    TGAColor color = NormalMap->get(std::round(x), std::round(y));
    Vector4f res;
    res[3] = .0f;
    for (size_t i = 0; i < 3; i++)
    {
        res[2 - i] = static_cast<float>(color[i]) / 255.f * 2.f - 1.f;
    }
    return res;
}


Material& Mesh::getMaterial() {
    return _materials;
}

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t) {
    return vec0 + t * (vec1 - vec0);
}