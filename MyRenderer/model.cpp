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

#pragma region ProcessMesh
void Model::ProcessMesh(FbxNode* pNode) {
    FbxMesh* pMesh = pNode->GetMesh();
    std::cout << "UV Name: " <<
        pMesh->GetElementUV()->GetName() << std::endl;
    if (pMesh == nullptr) {
        std::cout << "eMesh Node: " << pNode->GetName()
            << " dont have mesh." << std::endl;
        return;
    }
    const size_t CtrlPointCount = pMesh->GetControlPointsCount();
    const size_t TriangleCount = pMesh->GetPolygonCount();
    verts_.resize(CtrlPointCount);
    faces_.resize(TriangleCount, std::vector<int>(3, 0));
    size_t vertexCount = 0;
    size_t nonTriangleCount = 0;
    for (size_t faceInd = 0; faceInd < TriangleCount; faceInd++)
    {
        int faceSize = pMesh->GetPolygonSize(faceInd);
        // 如果模型存在多边形不为三角形
        if (faceSize > 3) {
            ++nonTriangleCount;
            continue;
        }

        for (size_t vertexIndInTriangle = 0u; vertexIndInTriangle < 3u; vertexIndInTriangle++)
        {
            size_t ctrlPointInd  = 
                pMesh->GetPolygonVertex(faceInd, vertexIndInTriangle);
            // 读取索引
            faces_[faceInd][vertexIndInTriangle] = ctrlPointInd;
            // 读取顶点position
            ReadPosition(pMesh, ctrlPointInd);
            // 读取顶点Color
            // ReadColor(pMesh, ctrlPointInd, vertexCount);

            ++vertexCount;
        }
    }
    std::cout << "NonTriangle conut: " << nonTriangleCount << std::endl;
}

void Model::ReadPosition(FbxMesh* pMesh, const size_t ctrlPointInd) {
    FbxVector4 ctrlPoint = pMesh->GetControlPointAt(ctrlPointInd);
    verts_[ctrlPointInd].position = Vector4f(ctrlPoint[0], ctrlPoint[1], ctrlPoint[2], ctrlPoint[3]);
}

// void Model::ReadColor(FbxMesh* pMesh, const size_t ctrlPointInd, const size_t vertexCount) {
//
//}

#pragma endregion






void Model::ProcessSkeleton(FbxNode* pNode) {

}

Model::Model(const char *filename) : verts_(), faces_() {
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

