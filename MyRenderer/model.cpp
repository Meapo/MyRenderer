#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include "model.h"

std::string getFileExtension(const char* filename) {
    int pointInd = 0, chInd = 0;
    while (filename[chInd] != '\0') {
        if (filename[chInd] == '.')
            pointInd = chInd;
        chInd++;
    }
    std::string extesion(filename + pointInd);
    return extesion;
}

void Model::ReadNode(FbxNode* pNode) {
    if (!pNode) {
        return;
    }
    FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
    if (pNodeAttribute) {
        switch (pNodeAttribute->GetAttributeType())
        {
        case FbxNodeAttribute::eMesh:
            ProcessMesh(pNode);
            break;
        case FbxNodeAttribute::eSkeleton:
            ProcessSkeleton(pNode);
        default:
            break;
        }
    }
    
    for (int i = 0; i < pNode->GetChildCount(); ++i) {
        ReadNode(pNode->GetChild(i));
    }
}

#pragma region Test
bool isAtOnePlane(const Vector3f& vec1, const Vector3f& vec2, const Vector3f& vec3) {
    Matrix3f m;
    m << vec1, vec2, vec3;
    if (abs(m.determinant()) < 0.001f)
        return true;
    else
        return false;
}
#pragma endregion

std::ostream& operator<<(std::ostream& os, FbxAMatrix& m) {
    os << m[0][0] << " " << m[0][1] << " " << m[0][2] << std::endl
        << m[1][0] << " " << m[1][1] << " " << m[1][2] << std::endl
        << m[2][0] << " " << m[2][1] << " " << m[2][2] << std::endl;

    return os;
}


#pragma region ProcessMesh
void Model::ProcessMesh(FbxNode* pNode) {
    FbxMesh* pMesh = pNode->GetMesh();
    // Test Code
    /*{
        FbxGeometryElementUV* pVertexUV = pMesh->GetElementUV(0);
        std::cout << pVertexUV->GetDirectArray().GetCount() << std::endl;
        return; 
    }*/
    
    if (pMesh == nullptr) {
        std::cout << "eMesh Node: " << pNode->GetName()
            << " dont have mesh." << std::endl;
        return;
    }

    const size_t CtrlPointCount = pMesh->GetControlPointsCount();
    const size_t PolygonCount = pMesh->GetPolygonCount();
    const size_t beginVertsInd = verts_.size(), beginFacesInd = faces_.size();
    verts_.resize(beginVertsInd + CtrlPointCount);
    for (size_t ctrlPointInd = 0, VertsInd = beginVertsInd; ctrlPointInd < pMesh->GetControlPointsCount(); ctrlPointInd++, VertsInd++)
    {
        // 读取顶点position
        ReadPosition(pMesh, ctrlPointInd, VertsInd);
    }

    size_t faceSizeCounter = 0;
    for (size_t faceInd = 0; faceInd < PolygonCount; faceInd++) {
        faceSizeCounter += pMesh->GetPolygonSize(faceInd);
        faceSizeCounter -= 2;
    }

    faces_.resize(beginFacesInd + faceSizeCounter, std::vector<int>(6, 0));
    size_t PreVertexCounter = 0;
    size_t triangleCounter = beginFacesInd;
    for (size_t PolygonInd = 0; PolygonInd < PolygonCount; PolygonInd++)
    {
        ReadPolygon(pMesh, PolygonInd, triangleCounter, PreVertexCounter, beginVertsInd);
    }

    ConnectMaterialToMesh(pMesh);
    LoadMaterial(pMesh, beginFacesInd);
}

void Model::ReadPolygon(FbxMesh* pMesh, const size_t PolygonInd, size_t& triangleCounter, size_t& PreVertexCounter, const size_t beginVertsInd) {
    const size_t faceSize = pMesh->GetPolygonSize(PolygonInd);
    
    // 读取0顶点
    size_t vertsInd_0 = pMesh->GetPolygonVertex(PolygonInd, 0) + beginVertsInd;
    // 读取顶点UV
    ReadUV(pMesh, vertsInd_0, PreVertexCounter, 0);
    // 读取顶点normal
    ReadNormal(pMesh, vertsInd_0, PreVertexCounter);
    // 读取顶点tangent
    ReadTangent(pMesh, vertsInd_0, PreVertexCounter);
    for (size_t startInd = 1; startInd + 1 < faceSize; startInd++)
    {
        // 读取索引
        faces_[triangleCounter][0] = vertsInd_0;

        for (size_t vertexIndInPolygon = startInd; vertexIndInPolygon < startInd + 2; vertexIndInPolygon++)
        {
            size_t triangleInd = vertexIndInPolygon - startInd + 1;
            size_t ctrlPointInd = pMesh->GetPolygonVertex(PolygonInd, vertexIndInPolygon);
            size_t vertsInd = ctrlPointInd + beginVertsInd;
            size_t vertecCounter = PreVertexCounter + vertexIndInPolygon;
            // 读取索引
            faces_[triangleCounter][triangleInd] = vertsInd;
            // 读取顶点UV
            ReadUV(pMesh, vertsInd, vertecCounter, 0);
            // 读取顶点normal
            ReadNormal(pMesh, vertsInd, vertecCounter);
            // 读取顶点tangent
            ReadTangent(pMesh, vertsInd, vertecCounter);
        }
        triangleCounter++;
    }
    PreVertexCounter += faceSize;
}


void Model::ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t VertsInd) {
    FbxVector4 ctrlPoint = pMesh->GetControlPointAt(ctrlPointInd);
    verts_[VertsInd].position = Vector4f(ctrlPoint[0], ctrlPoint[1], ctrlPoint[2], 1.0f);
    UpdateBoundingBox(verts_[VertsInd].position);
}

void Model::ReadUV(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCounter, const size_t uvLayer) {
    if (pMesh->GetUVLayerCount() <= uvLayer) {
        std::cout << "uvLayer out of bound." << std::endl;
        return;
    }
    FbxGeometryElementUV* pVertexUV = pMesh->GetElementUV(uvLayer);
    switch (pVertexUV->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint :
    {
        switch (pVertexUV->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect :
        {
            verts_[ctrlPointInd].uv.x() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[0];
            verts_[ctrlPointInd].uv.y() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[1];
        }
        break;

        case FbxGeometryElement::eIndexToDirect :
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(ctrlPointInd);
            verts_[ctrlPointInd].uv.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].uv.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
        }
        break;

        default:
            break;
        }
    }
    break;

    case FbxGeometryElement::eByPolygonVertex :
    {
        switch (pVertexUV->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect: 
        {
            verts_[ctrlPointInd].uv.x() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[0];
            verts_[ctrlPointInd].uv.y() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[1];
        }
        break;
        case FbxGeometryElement::eIndexToDirect:
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(vertexCounter);
            verts_[ctrlPointInd].uv.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].uv.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
        }
        break;

        default:
            break;
        }
    }
    break;
    }
}

void Model::ReadNormal(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter)
{
    if (pMesh->GetElementNormalCount() < 1)
    {
        return;
    }
    verts_[ctrlPointIndex].normal.w() = .0f;
    FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(0);
    switch (leNormal->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint:
    {
        switch (leNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        {
            verts_[ctrlPointIndex].normal.x() = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[0];
            verts_[ctrlPointIndex].normal.y() = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[1];
            verts_[ctrlPointIndex].normal.z() = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[2];
        }
        break;

        case FbxGeometryElement::eIndexToDirect:
        {
            size_t ind = leNormal->GetIndexArray().GetAt(ctrlPointIndex);
            verts_[ctrlPointIndex].normal.x() = leNormal->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointIndex].normal.y() = leNormal->GetDirectArray().GetAt(ind)[1];
            verts_[ctrlPointIndex].normal.z() = leNormal->GetDirectArray().GetAt(ind)[2];
        }
        break;

        default:
            break;
        }
    }
    break;

    case FbxGeometryElement::eByPolygonVertex:
    {
        switch (leNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        {
            verts_[ctrlPointIndex].normal.x() = leNormal->GetDirectArray().GetAt(vertexCounter)[0];
            verts_[ctrlPointIndex].normal.y() = leNormal->GetDirectArray().GetAt(vertexCounter)[1];
            verts_[ctrlPointIndex].normal.z() = leNormal->GetDirectArray().GetAt(vertexCounter)[2];
        }
        break;

        case FbxGeometryElement::eIndexToDirect:
        {
            int ind = leNormal->GetIndexArray().GetAt(vertexCounter);
            verts_[ctrlPointIndex].normal.x() = leNormal->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointIndex].normal.y() = leNormal->GetDirectArray().GetAt(ind)[1];
            verts_[ctrlPointIndex].normal.z() = leNormal->GetDirectArray().GetAt(ind)[2];
        }
        break;

        default:
            break;
        }
    }
    break;
    }
}

void Model::ReadTangent(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter)
{
    if (pMesh->GetElementNormalCount() < 1)
    {
        return;
    }

    FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent(0);
    if (!leTangent) 
    {
        return;
    }
    else 
    {
        switch (leTangent->GetMappingMode())
        {
        case FbxGeometryElement::eByControlPoint:
        {
            switch (leTangent->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
            {
                verts_[ctrlPointIndex].tangent.x() = leTangent->GetDirectArray().GetAt(ctrlPointIndex)[0];
                verts_[ctrlPointIndex].tangent.y() = leTangent->GetDirectArray().GetAt(ctrlPointIndex)[1];
                verts_[ctrlPointIndex].tangent.z() = leTangent->GetDirectArray().GetAt(ctrlPointIndex)[2];
            }
            break;

            case FbxGeometryElement::eIndexToDirect:
            {
                size_t ind = leTangent->GetIndexArray().GetAt(ctrlPointIndex);
                verts_[ctrlPointIndex].tangent.x() = leTangent->GetDirectArray().GetAt(ind)[0];
                verts_[ctrlPointIndex].tangent.y() = leTangent->GetDirectArray().GetAt(ind)[1];
                verts_[ctrlPointIndex].tangent.z() = leTangent->GetDirectArray().GetAt(ind)[2];
            }
            break;

            default:
                break;
            }
        }
        break;

        case FbxGeometryElement::eByPolygonVertex:
        {
            switch (leTangent->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
            {
                verts_[ctrlPointIndex].tangent.x() = leTangent->GetDirectArray().GetAt(vertexCounter)[0];
                verts_[ctrlPointIndex].tangent.y() = leTangent->GetDirectArray().GetAt(vertexCounter)[1];
                verts_[ctrlPointIndex].tangent.z() = leTangent->GetDirectArray().GetAt(vertexCounter)[2];
            }
            break;

            case FbxGeometryElement::eIndexToDirect:
            {
                int ind = leTangent->GetIndexArray().GetAt(vertexCounter);
                verts_[ctrlPointIndex].tangent.x() = leTangent->GetDirectArray().GetAt(ind)[0];
                verts_[ctrlPointIndex].tangent.y() = leTangent->GetDirectArray().GetAt(ind)[1];
                verts_[ctrlPointIndex].tangent.z() = leTangent->GetDirectArray().GetAt(ind)[2];
            }
            break;

            default:
                break;
            }
        }
        break;
        }
    }
    
}

void Model::UpdateBoundingBox(const Vector4f& newPoint) {
    for (size_t i = 0; i < 3; ++i) {
        if (newPoint[i] > bBox[1][i])
            bBox[1][i] = newPoint[i];
        else if (newPoint[i] < bBox[0][i])
            bBox[0][i] = newPoint[i];
    }
}

void Model::ConnectMaterialToMesh(FbxMesh* pMesh)
{
    // Get the material index list of current mesh
    FbxLayerElementArrayTemplate<int>* pMaterialIndices;
    FbxGeometryElement::EMappingMode   materialMappingMode = FbxGeometryElement::eNone;
    int polygonCount = pMesh->GetPolygonCount();
    if (pMesh->GetElementMaterial())
    {
        pMaterialIndices = &pMesh->GetElementMaterial()->GetIndexArray();
        materialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
        if (pMaterialIndices)
        {
            switch (materialMappingMode)
            {
            case FbxGeometryElement::eByPolygon:
            {
                if (pMaterialIndices->GetCount() == polygonCount)
                {
                    int faceInd = 0;
                    for (int polygonInd = 0; polygonInd < polygonCount; ++polygonInd)
                    {
                        int materialIndex = pMaterialIndices->GetAt(polygonInd);
                        if (pMesh->GetPolygonSize(polygonInd) == 3)
                        {
                            faces_[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
                        }
                        else if (pMesh->GetPolygonSize(polygonInd) == 4)
                        {
                            faces_[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
                            faces_[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
                        }
                    }
                }
            }
            break;

            case FbxGeometryElement::eAllSame:
            {
                int lMaterialIndex = pMaterialIndices->GetAt(0);
                int id = pMesh->GetNode()->GetMaterial(lMaterialIndex)->GetUniqueID();
                int faceInd = 0;
                for (int polygonInd = 0; polygonInd < polygonCount; ++polygonInd)
                {
                    if (pMesh->GetPolygonSize(polygonInd) == 3)
                    {
                        faces_[faceInd++][3] = id;
                    }
                    else if (pMesh->GetPolygonSize(polygonInd) == 4)
                    {
                        faces_[faceInd++][3] = id;
                        faces_[faceInd++][3] = id;
                    }
                }
            }
            }
        }
    }
}

void Model::LoadMaterial(FbxMesh* pMesh, const size_t beginFaceInd)
{
    size_t materialCount = 0;
    FbxNode* pNode;

    if (pMesh && pMesh->GetNode())
    {
        pNode = pMesh->GetNode();
        materialCount = pNode->GetMaterialCount();
        if (materialCount > 0)
        {
            for (int materialIndex = 0; materialIndex < materialCount; materialIndex++)
            {
                FbxSurfaceMaterial* pSurfaceMaterial = pNode->GetMaterial(materialIndex);
                LoadMaterialAttribute(pSurfaceMaterial, beginFaceInd);
                LoadMaterialTexture(pSurfaceMaterial, pNode->GetName(), beginFaceInd);
            }
        }
    }
}

void Model::LoadMaterialTexture(FbxSurfaceMaterial* pSurfaceMaterial, const std::string& nodeName, const size_t beginFaceInd)
{
    int textureLayerIndex;
    FbxProperty pProperty;
    int texID;
    for (textureLayerIndex = 0; textureLayerIndex < FbxLayerElement::sTypeTextureCount; ++textureLayerIndex)
    {
        pProperty = pSurfaceMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[textureLayerIndex]);
        if (pProperty.IsValid())
        {
            int textureCount = pProperty.GetSrcObjectCount();

            for (int j = 0; j < textureCount; ++j)
            {
                FbxTexture* pTexture = FbxCast<FbxTexture>(pProperty.GetSrcObject(j));
                if (pTexture)
                {
                    size_t texInd = pTexture->GetUniqueID();
                    // Use pTexture to load the attribute of current texture...
                    if (!textures_.count(texInd))
                    {
                        std::string fileName;
                        size_t ind = -1;
                        if (!strcmp("DiffuseColor", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            fileName = modelFilePath_ + nodeName + "_color.tga";
                            ind = 4;
                        }
                        else if (!strcmp("SpecularColor", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            // unknown
                        }
                        else if (!strcmp("Bump", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            fileName = modelFilePath_ + nodeName + "_normal.tga";
                            ind = 5;
                        }
                        if (fileName != "") {
                            textures_[texInd] = new TGAImage();
                            textures_[texInd]->read_tga_file(fileName.c_str());
                            std::cout << textures_[texInd]->get_bytespp() << std::endl;
                            for (size_t i = beginFaceInd; i < faces_.size(); i++)
                            {
                                faces_[i][ind] = texInd;
                            }
                        }
                    }
                }
            }
        }
    }
}

Vector3f FbxDouble3ToVec3f(const FbxDouble3& d) {
    return Vector3f(d[0], d[1], d[2]);
}

void Model::LoadMaterialAttribute(FbxSurfaceMaterial* pSurfaceMaterial, const size_t beginFaceInd) 
{
    int uniqueId = pSurfaceMaterial->GetUniqueID();
    for (size_t i = beginFaceInd; i < faces_.size(); i++)
    {
        faces_[i][3] = uniqueId;  // 记录面的材质索引
    }
    // Phong material
    if (pSurfaceMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        // Ambient Color
        materials_[uniqueId].Ambient() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Ambient);

        // Diffuse Color
        materials_[uniqueId].Diffuse() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Diffuse);

        // Specular Color
        materials_[uniqueId].Specular() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Specular);

        // Emissive Color
        materials_[uniqueId].Emissive() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Emissive);

        // Opacity
        materials_[uniqueId].TransparencyFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->TransparencyFactor;
      
        // Shininess
        materials_[uniqueId].Shininess() = ((FbxSurfacePhong*)pSurfaceMaterial)->Shininess;

        // Reflectivity
        materials_[uniqueId].ReflectionFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->ReflectionFactor;

        return;
    }

    // Lambert material
    if (pSurfaceMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
    {
        // Ambient Color
        materials_[uniqueId].Ambient() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Ambient);

        // Diffuse Color
        materials_[uniqueId].Diffuse() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Diffuse);

        // Emissive Color
        materials_[uniqueId].Emissive() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Emissive);

        // Opacity
        materials_[uniqueId].TransparencyFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->TransparencyFactor;

        return;
    }
}
#pragma endregion

void Model::ProcessSkeleton(FbxNode* pNode) {
    
}

Model::Model(const char* path, const char *filename) : verts_(), faces_(), bBox(2, Vector4f(.0f, .0f, .0f, 1.0f)), materials_(), textures_(), modelFilePath_(path) {
    std::string fullFileName(path);
    fullFileName += static_cast<std::string>(filename);
    std::string fileExtension = getFileExtension(filename);
    if (fileExtension == ".obj") {
        std::ifstream in;
        in.open(fullFileName, std::ifstream::in);
        if (in.fail()) return;
        std::string line;
        int VertsCount = 0;
        int texCoordInd = 0, normalInd = 0;
        while (!in.eof()) {
            std::getline(in, line);
            std::istringstream iss(line.c_str());
            char trash;
            if (!line.compare(0, 2, "v ")) {
                iss >> trash;
                Vertex_f v;
                for (int i = 0; i < 3; i++) iss >> v.position[i];
                verts_.push_back(v);
                VertsCount++;
            }
            else if (!line.compare(0, 2, "f ")) {
                std::vector<int> f;
                int itrash, idx;
                iss >> trash;
                while (iss >> idx >> trash >> itrash >> trash >> itrash) {
                    idx--; // in wavefront obj all indices start at 1, not zero
                    f.push_back(idx);
                }
                faces_.push_back(f);
            }
            else if (!line.compare(0, 3, "vt ")) {
                iss >> trash;
                for (int i = 0; i < 2; i++) iss >> verts_[texCoordInd].uv[i];
                float fTrash;
                iss >> fTrash;
                texCoordInd++;
            }
            else if (!line.compare(0, 3, "vn ")) {
                iss >> trash;
                for (int i = 0; i < 3; i++) iss >> verts_[normalInd].normal[i];
                normalInd++;
            }
        }
        std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
    }
    else if (fileExtension == ".fbx") {
        // Initialize the SDK manager. This object handles all our memory management.
        FbxManager* lSdkManager = FbxManager::Create();

        // Create the IO settings object.
        FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
        lSdkManager->SetIOSettings(ios);

        // Create an importer using the SDK manager.
        FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");

        // Use the first argument as the filename for the importer.
        if (!lImporter->Initialize(fullFileName.c_str(), -1, lSdkManager->GetIOSettings())) {
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

        // Read the nodes of the scene and their attributes recursively.
        FbxNode* lRootNode = lScene->GetRootNode();
        if (lRootNode) {
            ReadNode(lRootNode);
        }
        else {
            std::cout << "File root node is null." << std::endl;
        }
        // Destroy the SDK manager and all the other objects it was handling.
        lSdkManager->Destroy();
    }
}

Model::~Model() {

}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(size_t idx) {
    return faces_[idx];
}

Vertex_f& Model::vert(size_t i) {
    return verts_[i];
}

Vertex_f& Model::vert(size_t faceInd, size_t vertInFace) {
    assert(vertInFace < 3);
    return verts_[faces_[faceInd][vertInFace]];
}


const Vector4f& Model::MinBBox() {
    return bBox[0];
}

const Vector4f& Model::MaxBBox() {
    return bBox[1];
}

const Vector4f const Model::getTextureColor(size_t texInd, const Vector2f& _UV) {
    const TGAImage* texture = textures_[texInd];
    float x = _UV[0] * texture->get_width(), y = (1 - _UV[1]) * texture->get_height();
    //Vector2i uv[4];
    //uv[0] = { std::floor(x - .5f), std::floor(y - .5f) };
    //uv[1] = { uv[0].x(), uv[0].y() + 1 };
    //uv[2] = { uv[0].x() + 1, uv[0].y() + 1 };
    //uv[3] = { uv[0].x() + 1, uv[0].y() };
    //if (uv[0].x() >= 0 && uv[0].x() < (texture->get_width() - 1) && uv[0].y() >= 0 && uv[0].y() < (texture->get_height() - 1)) {
    //    // 在边界内使用bilinnear
    //    Vector4f color[4];
    //    for (size_t i = 0; i < 4; i++)
    //    {
    //        color[i] = texture->get(uv[i].x(), uv[i].y()).Color2Vec4f();
    //    }
    //    float t = x - uv[0].x() - .5f;
    //    Vector4f colorLerp0 = Lerp(color[0], color[3], t);
    //    Vector4f colorLerp1 = Lerp(color[1], color[2], t);
    //    float s = y - uv[0].y() - .5f;
    //    Vector4f result = Lerp(colorLerp0, colorLerp1, s);
    //    return result;
    //}
    //else {
        // 在边界外使用最近的点
        Vector4f color = texture->get(std::round(x), std::round(y)).Color2Vec4f();
        return color;
    // }
}

const Vector4f const Model::getTextureNormal(size_t texInd, const Vector2f& _UV) {
    const TGAImage* NormalMap = textures_[texInd];
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


const Material& Model::getMaterial(size_t ind) {
    return materials_[ind];
}

static Vector4f Lerp(const Vector4f& vec0, const Vector4f& vec1, float t) {
    return vec0 + t * (vec1 - vec0);
}



