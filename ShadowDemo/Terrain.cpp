#include "Terrain.h"
using namespace DirectX;

Terrain::Terrain(float width, float height, UINT m, UINT n, float scale)
{
	if (!ReadRawFile("..\\Textures\\terrain_ps2.raw"))
		MessageBox(nullptr, L"error", L"read terrain file failed!", MB_OK);
	if(!InitTerrain(500, 500, 511, 511, 1.f))
		MessageBox(nullptr, L"error", L"init terrain  failed!", MB_OK);

}


Terrain::~Terrain()
{
}

bool Terrain::InitTerrain(float width, float height, UINT m, UINT n, float scale) 
{
	m_cellsPerRow = m;
	m_cellsPerCol = n;
	m_verticesPerRow = m + 1;
	m_verticesPerCol = n + 1;
	m_numsVertices = m_verticesPerRow*m_verticesPerCol;
	m_width = width;
	m_height = height;
	m_heightScale = scale;

	//得到缩放后的高度
	for (auto& item : m_heightInfos)
	{
		item *= m_heightScale;
	}

	//起始x z坐标
	float oX = -width * 0.5f;
	float oZ = height * 0.5f;
	//每一格坐标变化
	float dx = width / m;
	float dz = height / n;	

	//ground
	{
		//计算ground顶点
		UINT groundWidth = 225;
		UINT groundHeight = 512;
		m_vertexItems["ground"].resize(groundWidth * groundHeight);
		//共512行
		for (UINT i = 0; i < groundHeight; ++i)
		{
			float tempZ = oZ - dz * i;
			//共225列
			for (UINT j = 0; j < groundWidth; ++j)
			{
				UINT index = groundWidth * i + j;
				m_vertexItems["ground"][index].Pos.x = oX + dx * j;
				m_vertexItems["ground"][index].Pos.y = m_heightInfos[m_verticesPerRow*i + j];
				m_vertexItems["ground"][index].Pos.z = tempZ;
				m_vertexItems["ground"][index].TexC = XMFLOAT2(dx*i, dx*j);
				m_vertexItems["ground"][index].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}

		//计算ground索引
		m_indexItems["ground"].resize(6 * (groundWidth - 1)*(groundHeight - 1));
		UINT tmp = 0;
		for (UINT i = 0; i < groundHeight - 1; ++i)
		{
			for (UINT j = 0; j < groundWidth - 1; ++j)
			{
				m_indexItems["ground"][tmp] = i * groundWidth + j;
				m_indexItems["ground"][tmp + 1] = i * groundWidth + j + 1;
				m_indexItems["ground"][tmp + 2] = (i + 1) * groundWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["ground"][m_indexItems["ground"][tmp]],
					m_vertexItems["ground"][m_indexItems["ground"][tmp + 1]],
					m_vertexItems["ground"][m_indexItems["ground"][tmp + 2]], temp);
				m_vertexItems["ground"][m_indexItems["ground"][tmp]].Normal = temp;
				m_vertexItems["ground"][m_indexItems["ground"][tmp + 1]].Normal = temp;
				m_vertexItems["ground"][m_indexItems["ground"][tmp + 2]].Normal = temp;


				m_indexItems["ground"][tmp + 3] = i * groundWidth + j + 1;
				m_indexItems["ground"][tmp + 4] = (i + 1) * groundWidth + j + 1;
				m_indexItems["ground"][tmp + 5] = (i + 1) * groundWidth + j;
				ComputeNomal(m_vertexItems["ground"][m_indexItems["ground"][tmp + 3]],
					m_vertexItems["ground"][m_indexItems["ground"][tmp + 4]],
					m_vertexItems["ground"][m_indexItems["ground"][tmp + 5]], temp);
				m_vertexItems["ground"][m_indexItems["ground"][tmp + 3]].Normal = temp;
				m_vertexItems["ground"][m_indexItems["ground"][tmp + 4]].Normal = temp;
				m_vertexItems["ground"][m_indexItems["ground"][tmp + 5]].Normal = temp;

				tmp += 6;
			}
		}

	}

	//grass
	{
		//计算grass顶点
		UINT grassWidth = 225;
		UINT grassHeight = 512;
		m_vertexItems["grass"].resize(grassWidth * grassHeight);
		UINT offsetX = 287;
		//共512行
		for (UINT i = 0; i < grassHeight; ++i)
		{
			float tempZ = oZ - dz * i;
			//共225列
			for (UINT j = 0; j < grassWidth; ++j)
			{
				UINT index = grassWidth * i + j;
				m_vertexItems["grass"][index].Pos.x = oX + dx * (j + offsetX);
				m_vertexItems["grass"][index].Pos.y = m_heightInfos[m_verticesPerRow*i + (j + offsetX)];
				m_vertexItems["grass"][index].Pos.z = tempZ;
				m_vertexItems["grass"][index].TexC = XMFLOAT2(dx*i, dx*j);
				m_vertexItems["grass"][index].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}

		//计算grass索引
		m_indexItems["grass"].resize(6 * (grassWidth - 1)*(grassHeight - 1));
		UINT tmp = 0;
		for (UINT i = 0; i < grassHeight - 1; ++i)
		{
			for (UINT j = 0; j < grassWidth - 1; ++j)
			{
				m_indexItems["grass"][tmp] = i * grassWidth + j;
				m_indexItems["grass"][tmp + 1] = i * grassWidth + j + 1;
				m_indexItems["grass"][tmp + 2] = (i + 1) * grassWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["grass"][m_indexItems["grass"][tmp]],
					m_vertexItems["grass"][m_indexItems["grass"][tmp + 1]],
					m_vertexItems["grass"][m_indexItems["grass"][tmp + 2]], temp);
				m_vertexItems["grass"][m_indexItems["grass"][tmp]].Normal = temp;
				m_vertexItems["grass"][m_indexItems["grass"][tmp + 1]].Normal = temp;
				m_vertexItems["grass"][m_indexItems["grass"][tmp + 2]].Normal = temp;


				m_indexItems["grass"][tmp + 3] = i * grassWidth + j + 1;
				m_indexItems["grass"][tmp + 4] = (i + 1) * grassWidth + j + 1;
				m_indexItems["grass"][tmp + 5] = (i + 1) * grassWidth + j;
				ComputeNomal(m_vertexItems["grass"][m_indexItems["grass"][tmp + 3]],
					m_vertexItems["grass"][m_indexItems["grass"][tmp + 4]],
					m_vertexItems["grass"][m_indexItems["grass"][tmp + 5]], temp);
				m_vertexItems["grass"][m_indexItems["grass"][tmp + 3]].Normal = temp;
				m_vertexItems["grass"][m_indexItems["grass"][tmp + 4]].Normal = temp;
				m_vertexItems["grass"][m_indexItems["grass"][tmp + 5]].Normal = temp;

				tmp += 6;
			}
		}
	}

	//water
	{
		//计算water顶点
		UINT waterWidth = 64;
		UINT waterHeight = 512;
		m_vertexItems["waterBottom"].resize(waterWidth * waterHeight);
		UINT offsetX = 224;
		//共512行
		for (UINT i = 0; i < waterHeight; ++i)
		{
			float tempZ = oZ - dz * i;
			//共64列
			for (UINT j = 0; j < waterWidth; ++j)
			{
				UINT index = waterWidth * i + j;
				m_vertexItems["waterBottom"][index].Pos.x = oX + dx * (j + offsetX);
				m_vertexItems["waterBottom"][index].Pos.y = m_heightInfos[m_verticesPerRow*i + (j + offsetX)];
				m_vertexItems["waterBottom"][index].Pos.z = tempZ;
				m_vertexItems["waterBottom"][index].TexC = XMFLOAT2(dx*i, dx*j);
				m_vertexItems["waterBottom"][index].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}
		//计算waterBottom索引
		m_indexItems["waterBottom"].resize(6 * (waterWidth - 1)*(waterHeight - 1));
		UINT tmp = 0;
		for (UINT i = 0; i < waterHeight - 1; ++i)
		{
			for (UINT j = 0; j < waterWidth - 1; ++j)
			{
				m_indexItems["waterBottom"][tmp] = i * waterWidth + j;
				m_indexItems["waterBottom"][tmp + 1] = i * waterWidth + j + 1;
				m_indexItems["waterBottom"][tmp + 2] = (i + 1) * waterWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp]],
					m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 1]],
					m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 2]], temp);
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp]].Normal = temp;
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 1]].Normal = temp;
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 2]].Normal = temp;


				m_indexItems["waterBottom"][tmp + 3] = i * waterWidth + j + 1;
				m_indexItems["waterBottom"][tmp + 4] = (i + 1) * waterWidth + j + 1;
				m_indexItems["waterBottom"][tmp + 5] = (i + 1) * waterWidth + j;
				ComputeNomal(m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 3]],
					m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 4]],
					m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 5]], temp);
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 3]].Normal = temp;
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 4]].Normal = temp;
				m_vertexItems["waterBottom"][m_indexItems["waterBottom"][tmp + 5]].Normal = temp;

				tmp += 6;
			}
		}
	}

	//road
	{
		//计算road顶点 以water为界分为两块左边和右边
		UINT roadWidth = 224;//每一边宽度均为224
		UINT roadHeight = 64;
		m_vertexItems["road"].resize(2 * roadWidth * roadHeight);

		//oZ = height*0.5,这里需要修正为1/16*height,即offsetZ为-7/16*height;
		float offsetZ = -7.f / 16.f*height;
		UINT rightOffsetX = 288;
		UINT offsetZ_PixelNum = 224;
		//先计算左边
		for (UINT i = 0; i < roadHeight; ++i)
		{
			float tempZ = oZ - dz * i + offsetZ;
			for (UINT j = 0; j < roadWidth; ++j)
			{
				UINT index = roadWidth * i + j;
				m_vertexItems["road"][index].Pos.x = oX + dx * (j + rightOffsetX);
				m_vertexItems["road"][index].Pos.y = m_heightInfos[m_verticesPerRow*(i + offsetZ_PixelNum) + j] + 0.1f;
				m_vertexItems["road"][index].Pos.z = tempZ;
				m_vertexItems["road"][index].TexC = XMFLOAT2(dx*i, dx*j);
				m_vertexItems["road"][index].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}

		int startVertexIndex = roadWidth * (roadHeight - 1) + roadWidth + 1;
		//再计算右边
		for (UINT i = 0; i < roadHeight; ++i)
		{
			float tempZ = oZ - dz * i + offsetZ;
			for (UINT j = 0; j < roadWidth; ++j)
			{
				UINT index = startVertexIndex + roadWidth * i + j;
				m_vertexItems["road"][index].Pos.x = oX + dx * j;
				m_vertexItems["road"][index].Pos.y = m_heightInfos[m_verticesPerRow*(i + offsetZ_PixelNum) + j
					+ rightOffsetX] + 0.1f;
				m_vertexItems["road"][index].Pos.z = tempZ;
				m_vertexItems["road"][index].TexC = XMFLOAT2(dx*i, dx*j);
				m_vertexItems["road"][index].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
		}


		//计算road索引 
		m_indexItems["road"].resize(2 * 6 * (roadWidth - 1)*(roadHeight - 1));
		UINT tmp = 0;
		//左边
		for (UINT i = 0; i < roadHeight - 1; ++i)
		{
			for (UINT j = 0; j < roadWidth - 1; ++j)
			{
				m_indexItems["road"][tmp] = i * roadWidth + j;
				m_indexItems["road"][tmp + 1] = i * roadWidth + j + 1;
				m_indexItems["road"][tmp + 2] = (i + 1) * roadWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["road"][m_indexItems["road"][tmp]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 1]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 2]], temp);
				m_vertexItems["road"][m_indexItems["road"][tmp]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 1]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 2]].Normal = temp;


				m_indexItems["road"][tmp + 3] = i * roadWidth + j + 1;
				m_indexItems["road"][tmp + 4] = (i + 1) * roadWidth + j + 1;
				m_indexItems["road"][tmp + 5] = (i + 1) * roadWidth + j;
				ComputeNomal(m_vertexItems["road"][m_indexItems["road"][tmp + 3]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 4]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 5]], temp);
				m_vertexItems["road"][m_indexItems["road"][tmp + 3]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 4]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 5]].Normal = temp;

				tmp += 6;
			}
		}

		int startIndex = roadWidth * (roadHeight - 1) + roadWidth + 1;
		//右边
		for (UINT i = 0; i < roadHeight - 1; ++i)
		{
			for (UINT j = 0; j < roadWidth - 1; ++j)
			{
				m_indexItems["road"][tmp] = startIndex + i * roadWidth + j;
				m_indexItems["road"][tmp + 1] = startIndex + i * roadWidth + j + 1;
				m_indexItems["road"][tmp + 2] = startIndex + (i + 1) * roadWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["road"][m_indexItems["road"][tmp]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 1]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 2]], temp);
				m_vertexItems["road"][m_indexItems["road"][tmp]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 1]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 2]].Normal = temp;


				m_indexItems["road"][tmp + 3] = startIndex + i * roadWidth + j + 1;
				m_indexItems["road"][tmp + 4] = startIndex + (i + 1) * roadWidth + j + 1;
				m_indexItems["road"][tmp + 5] = startIndex + (i + 1) * roadWidth + j;
				ComputeNomal(m_vertexItems["road"][m_indexItems["road"][tmp + 3]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 4]],
					m_vertexItems["road"][m_indexItems["road"][tmp + 5]], temp);
				m_vertexItems["road"][m_indexItems["road"][tmp + 3]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 4]].Normal = temp;
				m_vertexItems["road"][m_indexItems["road"][tmp + 5]].Normal = temp;

				tmp += 6;
			}
		}
	}

	return true;
}

bool Terrain::ReadRawFile(std::string filePath) 
{
	std::ifstream inFile;
	//二进制方式打开文件
	inFile.open(filePath.c_str(), std::ios::binary);
	//文件指针移动到末尾
	inFile.seekg(0, std::ios::end);
	//大小为当前缓冲区大小
	std::vector<BYTE> inData(inFile.tellg());
	//文件指针移动到开头
	inFile.seekg(std::ios::beg);
	//读取高度信息
	inFile.read((char*)&inData[0], inData.size());
	inFile.close();

	m_heightInfos.resize(inData.size());
	for (unsigned int i = 0; i < inData.size(); ++i)
	{
		m_heightInfos[i] = inData[i];
	}

	return true;
}

void Terrain::ComputeNomal(Vertex& v1, Vertex& v2, Vertex& v3, DirectX::XMFLOAT3& normal) 
{
	XMFLOAT3 f1(v2.Pos.x - v1.Pos.x, v2.Pos.y - v1.Pos.y, v2.Pos.z - v1.Pos.z);
	XMFLOAT3 f2(v3.Pos.x - v1.Pos.x, v3.Pos.y - v1.Pos.y, v3.Pos.z - v1.Pos.z);
	XMVECTOR vec1 = XMLoadFloat3(&f1);
	XMVECTOR vec2 = XMLoadFloat3(&f2);
	XMVECTOR temp = XMVector3Normalize(XMVector3Cross(vec1, vec2));
	XMStoreFloat3(&normal, temp);
}