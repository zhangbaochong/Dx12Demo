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
	ModelImporter();
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
			ModelMaterial mat;
			mat.Name = this->Name;
			mat.DiffuseAlbedo = this->DiffuseAlbedo;
			mat.FresnelR0 = this->FresnelR0;
			mat.Roughness = this->Roughness;
			mat.DiffuseMapName = this->DiffuseMapName;
			mat.NormalMapName = this->NormalMapName;
			return mat;
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

private:	
	void ProcessNode(aiNode* node, const aiScene* scene);	//解析每个节点
	ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);	//解析每个Mesh

};

