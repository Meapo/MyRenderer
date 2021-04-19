#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
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


#pragma region ProcessMesh
void Model::ProcessMesh(FbxNode* pNode) {
    FbxMesh* pMesh = pNode->GetMesh();
    // Test Code
    /*{
        return; 
    }*/
    
    if (pMesh == nullptr) {
        std::cout << "eMesh Node: " << pNode->GetName()
            << " dont have mesh." << std::endl;
        return;
    }
    const size_t CtrlPointCount = pMesh->GetControlPointsCount();
    const size_t PolygonCount = pMesh->GetPolygonCount();
    verts_.resize(CtrlPointCount);
    faces_.resize(PolygonCount * 2, std::vector<int>(3, 0));
    size_t PreVertexCounter = 0;
    size_t triangleCounter = 0;
    for (size_t faceInd = 0; faceInd < PolygonCount; faceInd++)
    {
        const int faceSize = pMesh->GetPolygonSize(faceInd);
        // 如果模型存在四边形面
        if (faceSize == 4) {
            // 判读点1，3是否在向量02的两侧(不需要判断，因为只给出四个点，两种空间四边形都有可能，在这里就采用012，230的方式划分)
            /*FbxVector4 ctrlPoint_0 = pMesh->GetControlPointAt(
                pMesh->GetPolygonVertex(faceInd, 0));
            FbxVector4 ctrlPoint_1 = pMesh->GetControlPointAt(
                pMesh->GetPolygonVertex(faceInd, 1));
            FbxVector4 ctrlPoint_2 = pMesh->GetControlPointAt(
                pMesh->GetPolygonVertex(faceInd, 2));
            FbxVector4 ctrlPoint_3 = pMesh->GetControlPointAt(
                pMesh->GetPolygonVertex(faceInd, 3));
            FbxVector4 Vector_01 = ctrlPoint_1 - ctrlPoint_0;
            FbxVector4 Vector_02 = ctrlPoint_2 - ctrlPoint_0;
            FbxVector4 Vector_03 = ctrlPoint_3 - ctrlPoint_0;
            Vector3f vec01(Vector_01[0], Vector_01[1], Vector_01[2]);
            Vector3f vec02(Vector_02[0], Vector_02[1], Vector_02[2]);          
            Vector3f vec03(Vector_03[0], Vector_03[1], Vector_03[2]);
            Vector3f cross21 = vec02.cross(vec01);
            Vector3f cross23 = vec02.cross(vec03);
            if ((cross21[2] > 0 && cross23[2] < 0) || (cross21[2] < 0 && cross23[2] > 0)) {*/
            ReadTriangle(pMesh, 0, faceInd, triangleCounter, PreVertexCounter, faceSize);
            ++triangleCounter;
            ReadTriangle(pMesh, 2, faceInd, triangleCounter, PreVertexCounter, faceSize);
            ++triangleCounter;
            /*}
            else {
                ReadTriangle(pMesh, 1, faceInd, triangleCounter, PreVertexCounter, faceSize);
                ++triangleCounter;
                ReadTriangle(pMesh, 3, faceInd, triangleCounter, PreVertexCounter, faceSize);
                ++triangleCounter;
            }*/
            PreVertexCounter += 4;
        }
        else if (faceSize == 3){
            ReadTriangle(pMesh, 0, faceInd, triangleCounter, PreVertexCounter, faceSize);
            ++triangleCounter;
            PreVertexCounter += 3;
        }
        else {
            std::cout << "Exist face size not 3 or 4!" << std::endl;
        }
    }
}

void Model::ReadTriangle(FbxMesh* pMesh, size_t startInd, size_t faceInd, size_t triangleCounter, size_t PreVertexCounter, const size_t faceSize) {
    for (size_t vertexIndInPolygon = startInd; vertexIndInPolygon < startInd + 3; vertexIndInPolygon++)
    {
        size_t triangleInd = vertexIndInPolygon - startInd;
        size_t polygonInd = vertexIndInPolygon % faceSize;
        size_t ctrlPointInd = pMesh->GetPolygonVertex(faceInd, polygonInd);
        // 读取索引
        faces_[triangleCounter][triangleInd] = ctrlPointInd;
        // 读取顶点position
        ReadPosition(pMesh, ctrlPointInd);
        // 读取顶点Diffuse UV
        ReadUV(pMesh, ctrlPointInd, PreVertexCounter + polygonInd, 0);
        // 读取顶点normal
        ReadNormal(pMesh, ctrlPointInd, PreVertexCounter + polygonInd);
        // 读取顶点tangent
        ReadTangent(pMesh, ctrlPointInd, PreVertexCounter + polygonInd);
    }
}


void Model::ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd) {
    FbxVector4 ctrlPoint = pMesh->GetControlPointAt(ctrlPointInd);
    verts_[ctrlPointInd].position = Vector4f(ctrlPoint[0], ctrlPoint[1], ctrlPoint[2], 1.0f);
    UpdateBoundingBox(verts_[ctrlPointInd].position);
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
            verts_[ctrlPointInd].DiffuseCoord.x() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[0];
            verts_[ctrlPointInd].DiffuseCoord.y() = pVertexUV->GetDirectArray().GetAt(ctrlPointInd)[1];
        }
        break;

        case FbxGeometryElement::eIndexToDirect :
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(ctrlPointInd);
            verts_[ctrlPointInd].DiffuseCoord.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].DiffuseCoord.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
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
            verts_[ctrlPointInd].DiffuseCoord.x() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[0];
            verts_[ctrlPointInd].DiffuseCoord.y() = pVertexUV->GetDirectArray().GetAt(vertexCounter)[1];
        }
        break;
        case FbxGeometryElement::eIndexToDirect:
        {
            size_t ind = pVertexUV->GetIndexArray().GetAt(vertexCounter);
            verts_[ctrlPointInd].DiffuseCoord.x() = pVertexUV->GetDirectArray().GetAt(ind)[0];
            verts_[ctrlPointInd].DiffuseCoord.y() = pVertexUV->GetDirectArray().GetAt(ind)[1];
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


#pragma endregion

void Model::ProcessSkeleton(FbxNode* pNode) {
    
}

Model::Model(const char *filename) : verts_(), faces_(), bBox(2, Vector4f(.0f, .0f, .0f, 1.0f)) {
    std::string fileExtension = getFileExtension(filename);
    if (fileExtension == ".obj") {
        std::ifstream in;
        in.open(filename, std::ifstream::in);
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
                for (int i = 0; i < 2; i++) iss >> verts_[texCoordInd].DiffuseCoord[i];
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
        if (!lImporter->Initialize(filename, -1, lSdkManager->GetIOSettings())) {
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

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vertex_f Model::vert(int i) {
    return verts_[i];
}

Vector4f Model::MinBBox() {
    return bBox[0];
}

Vector4f Model::MaxBBox() {
    return bBox[1];
}

