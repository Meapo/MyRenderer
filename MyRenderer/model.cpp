#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include "model.h"

enum class TextureType {
    Diffuse, Normal, Unknown
};

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
    unordered_map<unsigned int, TGAImage*> textures;
    Material materials;
    unsigned int DiffuseTextureID = 0;
    unsigned int NormalTextureID = 0;

    if (pMesh == nullptr) {
        std::cout << "eMesh Node: " << pNode->GetName()
            << " dont have mesh." << std::endl;
        return;
    }

    const size_t CtrlPointCount = pMesh->GetControlPointsCount();
    const size_t PolygonCount = pMesh->GetPolygonCount();
    size_t PreVertexCounter = 0;
    vertsCount += CtrlPointCount;
    vector<Vertex_f> verts(CtrlPointCount);
    for (size_t ctrlPointInd = 0; ctrlPointInd < pMesh->GetControlPointsCount(); ctrlPointInd++)
    {
        // 读取顶点position
        ReadPosition(pMesh, ctrlPointInd, verts);
    }

    size_t faceSizeCounter = 0;
    for (size_t faceInd = 0; faceInd < PolygonCount; faceInd++) {
        faceSizeCounter += pMesh->GetPolygonSize(faceInd);
        faceSizeCounter -= 2;
    }

    faceCount += faceSizeCounter;
    vector<vector<unsigned int>> indices(faceSizeCounter, vector<unsigned int> (3, 0));
    size_t triangleCounter = 0;
    for (size_t PolygonInd = 0; PolygonInd < PolygonCount; PolygonInd++)
    {
        ReadPolygon(pMesh, PolygonInd, triangleCounter, PreVertexCounter, indices, verts);
    }

    LoadMaterial(pMesh, textures, DiffuseTextureID, NormalTextureID, materials);
    ConnectMaterialToMesh(pMesh, indices);
    _meshes.push_back(Mesh(verts, indices, textures, materials, DiffuseTextureID, NormalTextureID));
}

void Model::ReadPolygon(FbxMesh* pMesh, const size_t PolygonInd, size_t& triangleCounter, size_t& PreVertexCounter, 
    vector<vector<unsigned int> >& indices, vector<Vertex_f>& verts_) {
    const size_t faceSize = pMesh->GetPolygonSize(PolygonInd);
    // 读取0顶点
    size_t vertsInd_0 = pMesh->GetPolygonVertex(PolygonInd, 0);
    // 读取顶点UV
    ReadUV(pMesh, vertsInd_0, PreVertexCounter, 0, verts_);
    // 读取顶点normal
    ReadNormal(pMesh, vertsInd_0, PreVertexCounter, verts_);
    // 读取顶点tangent
    ReadTangent(pMesh, vertsInd_0, PreVertexCounter, verts_);
    for (size_t startInd = 1; startInd + 1 < faceSize; startInd++)
    {
        // 读取索引
        indices[triangleCounter][0] = vertsInd_0;

        for (size_t vertexIndInPolygon = startInd; vertexIndInPolygon < startInd + 2; vertexIndInPolygon++)
        {
            size_t triangleInd = vertexIndInPolygon - startInd + 1;
            size_t ctrlPointInd = pMesh->GetPolygonVertex(PolygonInd, vertexIndInPolygon);
            size_t vertecCounter = PreVertexCounter + vertexIndInPolygon;
            // 读取索引
            indices[triangleCounter][triangleInd] = ctrlPointInd;
            // 读取顶点UV
            ReadUV(pMesh, ctrlPointInd, vertecCounter, 0, verts_);
            // 读取顶点normal
            ReadNormal(pMesh, ctrlPointInd, vertecCounter, verts_);
            // 读取顶点tangent
            ReadTangent(pMesh, ctrlPointInd, vertecCounter, verts_);
        }
        triangleCounter++;
    }
    PreVertexCounter += faceSize;
}


void Model::ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd, vector<Vertex_f>& verts_) {
    FbxVector4 ctrlPoint = pMesh->GetControlPointAt(ctrlPointInd);
    verts_[ctrlPointInd].position = Vector4f(ctrlPoint[0], ctrlPoint[1], ctrlPoint[2], 1.0f);
    UpdateBoundingBox(verts_[ctrlPointInd].position);
}

void Model::ReadUV(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCounter, const size_t uvLayer, vector<Vertex_f>& verts_) {
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
            verts_[ctrlPointInd].texCoords.x() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[0];
            verts_[ctrlPointInd].texCoords.y() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[1];
        }
        break;

        case FbxGeometryElement::eIndexToDirect :
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(ctrlPointInd);
            verts_[ctrlPointInd].texCoords.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].texCoords.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
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
            verts_[ctrlPointInd].texCoords.x() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[0];
            verts_[ctrlPointInd].texCoords.y() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[1];
        }
        break;
        case FbxGeometryElement::eIndexToDirect:
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(vertexCounter);
            verts_[ctrlPointInd].texCoords.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].texCoords.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
        }
        break;

        default:
            break;
        }
    }
    break;
    }
}

void Model::ReadNormal(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter, vector<Vertex_f>& verts_)
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

void Model::ReadTangent(FbxMesh* pMesh, size_t ctrlPointIndex, size_t vertexCounter, vector<Vertex_f>& verts_)
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

void Model::ConnectMaterialToMesh(FbxMesh* pMesh, vector<vector<unsigned int>>& indices)
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
                            indices[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
                        }
                        else if (pMesh->GetPolygonSize(polygonInd) == 4)
                        {
                            indices[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
                            indices[faceInd++][3] = pMesh->GetNode()->GetMaterial(materialIndex)->GetUniqueID();
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
                        indices[faceInd++][3] = id;
                    }
                    else if (pMesh->GetPolygonSize(polygonInd) == 4)
                    {
                        indices[faceInd++][3] = id;
                        indices[faceInd++][3] = id;
                    }
                }
            }
            }
        }
    }
}

void Model::LoadMaterial(FbxMesh* pMesh, unordered_map<unsigned int, TGAImage*>& textures_, 
    unsigned int& DiffuseTextureID, unsigned int& NormalTextureID, Material& materials_)
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
                LoadMaterialAttribute(pSurfaceMaterial, materials_);
                LoadMaterialTexture(pSurfaceMaterial, pNode->GetName(), textures_, DiffuseTextureID, NormalTextureID);
            }
        }
    }
}

void Model::LoadMaterialTexture(FbxSurfaceMaterial* pSurfaceMaterial, const std::string& nodeName, 
    unordered_map<unsigned int, TGAImage*>& textures_, unsigned int& DiffuseTextureID, unsigned int& NormalTextureID)
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
                        TextureType textureType = TextureType::Unknown;
                        if (!strcmp("DiffuseColor", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            fileName = modelFilePath_ + nodeName + "_color.tga";
                            textureType = TextureType::Diffuse;
                        }
                        else if (!strcmp("SpecularColor", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            // unknown
                        }
                        else if (!strcmp("Bump", FbxLayerElement::sTextureChannelNames[textureLayerIndex]))
                        {
                            fileName = modelFilePath_ + nodeName + "_normal.tga";
                            textureType = TextureType::Normal;
                        }
                        if (fileName != "") {
                            textures_[texInd] = new TGAImage();
                            textures_[texInd]->read_tga_file(fileName.c_str());
                            std::cout << textures_[texInd]->get_bytespp() << std::endl;

                            switch (textureType)
                            {
                            case TextureType::Diffuse:
                                DiffuseTextureID = texInd;
                                break;
                            case TextureType::Normal:
                                NormalTextureID = texInd;
                                break;
                            case TextureType::Unknown:
                                break;
                            default:
                                break;
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


void Model::LoadMaterialAttribute(FbxSurfaceMaterial* pSurfaceMaterial, Material& materials_)
{
    // Phong material
    if (pSurfaceMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        // Ambient Color
        materials_.Ambient() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Ambient);

        // Diffuse Color
        materials_.Diffuse() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Diffuse);

        // Specular Color
        materials_.Specular() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Specular);

        // Emissive Color
        materials_.Emissive() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Emissive);

        // Opacity
        materials_.TransparencyFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->TransparencyFactor;
      
        // Shininess
        materials_.Shininess() = ((FbxSurfacePhong*)pSurfaceMaterial)->Shininess;

        // Reflectivity
        materials_.ReflectionFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->ReflectionFactor;

        return;
    }

    // Lambert material
    if (pSurfaceMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
    {
        // Ambient Color
        materials_.Ambient() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Ambient);

        // Diffuse Color
        materials_.Diffuse() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Diffuse);

        // Emissive Color
        materials_.Emissive() = FbxDouble3ToVec3f(((FbxSurfacePhong*)pSurfaceMaterial)->Emissive);

        // Opacity
        materials_.TransparencyFactor() = ((FbxSurfacePhong*)pSurfaceMaterial)->TransparencyFactor;

        return;
    }
}
#pragma endregion

void Model::ProcessSkeleton(FbxNode* pNode) {
    
}

Model::Model(const char* path, const char *filename) : _meshes(), bBox(2, Vector4f(.0f, .0f, .0f, 1.0f)), modelFilePath_(path), vertsCount(0), faceCount(0) {
    std::string fullFileName(path);
    fullFileName += static_cast<std::string>(filename);
    std::string fileExtension = getFileExtension(filename);
    if (fileExtension == ".fbx") {
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
    else {
        std::cout << "File is not fbx." << endl;
    }
}

Model::~Model() {
}

const Vector4f& Model::MinBBox() {
    return bBox[0];
}

const Vector4f& Model::MaxBBox() {
    return bBox[1];
}







