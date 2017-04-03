#pragma once
#include <vector>
#include <DirectXMath.h>
#include "Common\d3dUtil.h"
#include "Common\MathHelper.h"
#include "FrameResource.h"

class Terrain
{
public:
	Terrain(float width, float height, UINT m, UINT n, float scale);
	Terrain(const Terrain& rhs) = delete;
	Terrain& operator =(const Terrain& rhs) = delete;
	~Terrain();

private:
	bool InitTerrain(float width, float height, UINT m, UINT n, float scale);
	bool ReadRawFile(std::string filePath);										//从高度图读取高度信息
	void ComputeNomal(Vertex& v1, Vertex& v2, Vertex& v3, DirectX::XMFLOAT3& normal);	//计算法线
	void XMFloat3Add(DirectX::XMFLOAT3& dst, DirectX::XMFLOAT3& vec3);
	void XMFloat3Normalize(DirectX::XMFLOAT3& vec3);
	void CalcTangent(DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& normal);


	std::vector<float>	m_heightInfos;		//高度图高度信息
	int		m_cellsPerRow;					//每行单元格数
	int		m_cellsPerCol;					//每列单元格数
	int		m_verticesPerRow;				//每行顶点数
	int		m_verticesPerCol;				//每列顶点数
	int		m_numsVertices;					//顶点总数
	float	m_width;						//地形宽度
	float	m_height;						//地形高度
	float	m_heightScale;					//高度缩放系数

	//不同地形对应的高度，以便给予不同的材质贴图shader等等
	float   m_waterHeight;
	float	m_groundHeight;
	float   m_roadHeight;
	float   m_grassHeight;

public:
	std::unordered_map<std::string, std::vector<Vertex>>  m_vertexItems;	//不同地形的顶点集合
	std::unordered_map<std::string, std::vector<UINT>> m_indexItems;		//不同地形的索引集合
};

