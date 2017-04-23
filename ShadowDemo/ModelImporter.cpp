#include "ModelImporter.h"

using namespace std;
using namespace DirectX;

ModelImporter::ModelImporter(std::string name)
{
	this->m_name = name;
}

ModelImporter::~ModelImporter()
{

}

bool ModelImporter::LoadModel(const std::string filepath)
{
	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(filepath, 
		aiProcess_Triangulate | //三角化
		aiProcess_FlipUVs | //翻转
		aiProcess_CalcTangentSpace | //计算切线空间
		aiProcess_FixInfacingNormals | //修正正反面
		aiProcess_ConvertToLeftHanded |//变为左手系
		aiProcess_GenNormals
	);

	if (!pScene)
	{
		MessageBoxA(NULL, importer.GetErrorString(), "Error", MB_OK);
		return false;
	}

	this->filepath = filepath;
	this->ProcessNode(pScene->mRootNode, pScene);
}

void ModelImporter::ProcessNode(aiNode* node, const aiScene* scene)
{
	// 添加当前节点中的所有Mesh
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(this->ProcessMesh(mesh, scene));
	}
	// 递归处理该节点的子孙节点
	for (int i = 0; i < node->mNumChildren; i++)
	{
		this->ProcessNode(node->mChildren[i], scene);
	}
}

ModelImporter::ModelMesh ModelImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	vector<ModelVertex> vertices;
	vector<UINT> indices;
	ModelMaterial material;

	//解析顶点
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		ModelVertex vertex;

		vertex.Pos = XMFLOAT3
		{
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};
		
		vertex.Normal = XMFLOAT3
		{
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			 mesh->mNormals[i].z
		};
		
		if (mesh->mTextureCoords[0]) // 是否包含uv
		{
			vertex.TexC = XMFLOAT2
			{
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			};
		}
		else
			vertex.TexC = XMFLOAT2{ 0.0f, 0.0f };

		//是否有切线
		if (mesh->mTangents)
		{
			vertex.TangentU = XMFLOAT3
			{
				mesh->mTangents[i].x,
				mesh->mTangents[i].y,
				mesh->mTangents[i].z
			};
		}
		
		vertices.push_back(vertex);
	}
	
	//解析索引
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	int lastPos = filepath.find_last_of('\\');
	std::string basePath = filepath.substr(0, lastPos + 1);
	std::string tempStr;
	//解析材质
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

		material.Name = m_name + to_string(m_meshes.size());
		//加载漫反射纹理,只加载一张
		for (int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
			
			char path[100];			
			memcpy(path, str.data, str.length + 1);

			tempStr = path;
			int startPos = tempStr.find_last_of('\\');
			std::string realPath = basePath + tempStr.substr(startPos + 1, 1000);

			int tempPos = realPath.find_last_of('.');
			std::string strForm = realPath.substr(tempPos + 1, 100);
			//如果后缀不是dds转化为dds
			if (strForm != "dds")
			{
				realPath = realPath.substr(0, tempPos + 1) + "dds";
			}
			material.DiffuseMapName = realPath;
			
		}

		for (int i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS); ++i)
		{
			aiString str;
			mat->GetTexture(aiTextureType_NORMALS, i, &str);

			char path[100];
			memcpy(path, str.data, str.length + 1);

			tempStr = path;
			int startPos = tempStr.find_last_of('\\');
			std::string realPath = basePath + tempStr.substr(startPos + 1, 1000);

			int tempPos = realPath.find_last_of('.');
			std::string strForm = realPath.substr(tempPos + 1, 100);
			//如果后缀不是dds转化为dds
			if (strForm != "dds")
			{
				realPath = realPath.substr(0, tempPos + 1) + "dds";
			}
			material.NormalMapName = realPath;
		}

	}

	return ModelMesh(vertices, indices, material);
}