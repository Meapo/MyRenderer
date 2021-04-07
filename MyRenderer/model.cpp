#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"
#include <fbxsdk.h>

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

void ReadNode(FbxNode* root_node, std::vector<Vertex_f>& verts_, std::vector<std::vector<int> >& faces_) {
    if (!root_node)
        return;
    // MeshObject* ret = new MeshObject;
    int last_index = 0;//上一个模型的最大索引
    for (int i = 0; i < root_node->GetChildCount(); i++)
    {
        FbxNode* p_node = root_node->GetChild(i);
        for (int j = 0; j < p_node->GetNodeAttributeCount(); j++)
        {
            FbxNodeAttribute* p_attribute = p_node->GetNodeAttributeByIndex(j);
            if (p_attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                FbxMesh* mesh = p_attribute->GetNode()->GetMesh();
                if (mesh == NULL)
                {
                    return;
                }

                //数据是以形状存储的，会有重复的顶点，所以需要计算索引缓存
                int count_polygon = mesh->GetPolygonCount();
                //g_debug.Line(to_wstring(count_polygon));

                ret->_vecVertex.resize(last_index + count_polygon * 3);

                //已初始化标记
                vector<bool> vec_inited;
                vec_inited.resize(count_polygon * 3, false);

                int max_index = 0;
                for (int k = 0; k != count_polygon; ++k)
                {
                    if (mesh->GetPolygonSize(k) != 3)
                    {
                        g_debug.Line(L"模型数据未三角化！");
                        continue;
                    }

                    FbxVector4* ctrl_point = mesh->GetControlPoints();

                    for (int l = 0; l != 3; ++l)
                    {
                        int index = mesh->GetPolygonVertex(k, l);
                        if (index == -1)
                        {
                            g_debug.Line(L"获取顶点失败！");
                            continue;
                        }
                        max_index = max(index, max_index);
                        //依次记录顶点的索引，而顶点数据只存放一次
                        //g_debug.Line(to_wstring(index));
                        //g_debug.Line(String::Format(L"(%.0lf)(%.0lf)(%.0lf)", ctrl_point[index][0], ctrl_point[index][1], ctrl_point[index][2]));
                        ret->_vecIndex.push_back(last_index + index);
                        if (!vec_inited[index])
                        {
                            vec_inited[index] = true;
                            ret->_vecVertex[last_index + index].position.x = (float)(ctrl_point[index][0]) * 0.01f;
                            ret->_vecVertex[last_index + index].position.y = (float)(ctrl_point[index][1]) * 0.01f;
                            ret->_vecVertex[last_index + index].position.z = -(float)(ctrl_point[index][2]) * 0.01f;

                            ret->_vecVertex[last_index + index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
                        }

                    }

                }
                last_index += max_index + 1;
            }
        }
    }
    ret->_vecVertex.resize(last_index + 1);
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
                for (int i = 0; i < 2; i++) iss >> verts_[texCoordInd].texCoord[i];
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
            ReadNode(lRootNode, verts_, faces_);
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

