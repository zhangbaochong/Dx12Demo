#include "Model.h"

using namespace DirectX;

Model::Model(std::string name, std::string path)
{
	m_name = name;
	m_path = path;
	m_pModelImporter = new ModelImporter(name);
	m_pModelImporter->LoadModel(path);
	m_meshes = m_pModelImporter->m_meshes;
}

Model::~Model()
{

}

void Model::SetScaleRotateOffset(DirectX::CXMMATRIX scale, DirectX::CXMMATRIX rotate, DirectX::CXMMATRIX offset)
{
	XMStoreFloat4x4(&m_scale, scale);
	XMStoreFloat4x4(&m_rotate, rotate);
	XMStoreFloat4x4(&m_offset, offset);
}

DirectX::XMMATRIX Model::GetWorld()
{
	XMMATRIX scale = XMLoadFloat4x4(&m_scale);
	XMMATRIX rotate = XMLoadFloat4x4(&m_rotate);
	XMMATRIX offset = XMLoadFloat4x4(&m_offset);
	return scale * rotate * offset;
}


