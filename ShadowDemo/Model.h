#pragma once
#include <string>

#include "ModelImporter.h"

class Model
{
public:
	Model(std::string name, std::string path);
	~Model();
public:
	std::string m_name;
	std::string m_path;
	std::vector<ModelImporter::ModelMesh> m_meshes;
	ModelImporter* m_pModelImporter;
public:
	DirectX::XMMATRIX GetWorld();
	void SetScaleRotateOffset(DirectX::CXMMATRIX scale, DirectX::CXMMATRIX rotate, DirectX::CXMMATRIX offset);
private:
	DirectX::XMFLOAT4X4 m_scale;	//Ëõ·Å
	DirectX::XMFLOAT4X4 m_rotate;	//Ðý×ª
	DirectX::XMFLOAT4X4 m_offset;	//Æ½ÒÆ
};

