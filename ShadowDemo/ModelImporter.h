#pragma once
#include <string>
#include <vector>
#include "assimp\Importer.hpp"
#include "assimp\scene.h"
#include "assimp\postprocess.h"
#include "Common\d3dUtil.h"

class ModelImporter
{
public:
	ModelImporter(std::string name);
	~ModelImporter();

public:
	class ModelVertex
	{
	public:
		ModelVertex() = default;

		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexC;
		DirectX::XMFLOAT3 TangentU;
	};

	class ModelMaterial
	{
	public:
		ModelMaterial() = default;
		ModelMaterial& operator=(const ModelMaterial& rhs)
		{
			this->Name = rhs.Name;
			this->DiffuseAlbedo = rhs.DiffuseAlbedo;
			this->FresnelR0 = rhs.FresnelR0;
			this->Roughness = rhs.Roughness;
			this->DiffuseMapName = rhs.DiffuseMapName;
			this->NormalMapName = rhs.NormalMapName;
			return *this;
		
		}

		std::string Name;

		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.8f;

		std::string DiffuseMapName;
		std::string NormalMapName;
	};


	class ModelMesh
	{
	public:
		std::vector<ModelVertex> vertices;
		std::vector<UINT> indices;
		ModelMaterial material;

		ModelMesh() = default;
		ModelMesh(std::vector<ModelVertex> vertices, std::vector<UINT> indices, ModelMaterial material)
		{
			this->vertices = vertices;
			this->indices = indices;
			this->material = material;
		}
	};

	bool LoadModel(const std::string filepath);
	std::vector<ModelMesh> m_meshes;
	std::string m_name;
	std::string filepath;
private:	
	void ProcessNode(aiNode* node, const aiScene* scene);	//解析每个节点
	ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);	//解析每个Mesh

};

