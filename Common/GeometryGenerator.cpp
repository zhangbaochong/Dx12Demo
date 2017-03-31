#include "GeometryGenerator.h"


void GeometryGenerator::CreateBox(float width, float height, float depth, MeshData &mesh)
{
	mesh.vertices.clear();
	mesh.indices.clear();

	//一共24个顶点(每面4个)
	mesh.vertices.resize(24);
	//一共36个索引(每面6个)
	mesh.indices.resize(36);

	float halfW = width * 0.5f;
	float halfH = height * 0.5f;
	float halfD = depth * 0.5f;

	//眼睛面向z轴正方向
	//构建顶点
	//前面
	mesh.vertices[0].pos = XMFLOAT3(-halfW, -halfH, -halfD);
	mesh.vertices[0].normal = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[0].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[0].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[1].pos = XMFLOAT3(-halfW, halfH, -halfD);
	mesh.vertices[1].normal = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[1].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[1].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[2].pos = XMFLOAT3(halfW, halfH, -halfD);
	mesh.vertices[2].normal = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[2].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[2].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[3].pos = XMFLOAT3(halfW, -halfH, -halfD);
	mesh.vertices[3].normal = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[3].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[3].tex = XMFLOAT2(1.f, 1.f);
	//左侧面
	mesh.vertices[4].pos = XMFLOAT3(-halfW, -halfH, halfD);
	mesh.vertices[4].normal = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[4].tangent = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[4].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[5].pos = XMFLOAT3(-halfW, halfH, halfD);
	mesh.vertices[5].normal = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[5].tangent = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[5].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[6].pos = XMFLOAT3(-halfW, halfH, -halfD);
	mesh.vertices[6].normal = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[6].tangent = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[6].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[7].pos = XMFLOAT3(-halfW, -halfH, -halfD);
	mesh.vertices[7].normal = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[7].tangent = XMFLOAT3(0.f, 0.f, -1.f);
	mesh.vertices[7].tex = XMFLOAT2(1.f, 1.f);
	//背面
	mesh.vertices[8].pos = XMFLOAT3(halfW, -halfH, halfD);
	mesh.vertices[8].normal = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[8].tangent = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[8].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[9].pos = XMFLOAT3(halfW, halfH, halfD);
	mesh.vertices[9].normal = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[9].tangent = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[9].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[10].pos = XMFLOAT3(-halfW, halfH, halfD);
	mesh.vertices[10].normal = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[10].tangent = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[10].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[11].pos = XMFLOAT3(-halfW, -halfH, halfD);
	mesh.vertices[11].normal = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[11].tangent = XMFLOAT3(-1.f, 0.f, 0.f);
	mesh.vertices[11].tex = XMFLOAT2(1.f, 1.f);
	//右侧面
	mesh.vertices[12].pos = XMFLOAT3(halfW, -halfH, -halfD);
	mesh.vertices[12].normal = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[12].tangent = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[12].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[13].pos = XMFLOAT3(halfW, halfH, -halfD);
	mesh.vertices[13].normal = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[13].tangent = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[13].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[14].pos = XMFLOAT3(halfW, halfH, halfD);
	mesh.vertices[14].normal = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[14].tangent = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[14].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[15].pos = XMFLOAT3(halfW, -halfH, halfD);
	mesh.vertices[15].normal = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[15].tangent = XMFLOAT3(0.f, 0.f, 1.f);
	mesh.vertices[15].tex = XMFLOAT2(1.f, 1.f);
	//上面
	mesh.vertices[16].pos = XMFLOAT3(-halfW, halfH, -halfD);
	mesh.vertices[16].normal = XMFLOAT3(0.f, 1.f, 0.f);
	mesh.vertices[16].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[16].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[17].pos = XMFLOAT3(-halfW, halfH, halfD);
	mesh.vertices[17].normal = XMFLOAT3(0.f, 1.f, 0.f);
	mesh.vertices[17].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[17].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[18].pos = XMFLOAT3(halfW, halfH, halfD);
	mesh.vertices[18].normal = XMFLOAT3(0.f, 1.f, 0.f);
	mesh.vertices[18].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[18].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[19].pos = XMFLOAT3(halfW, halfH, -halfD);
	mesh.vertices[19].normal = XMFLOAT3(0.f, 1.f, 0.f);
	mesh.vertices[19].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[19].tex = XMFLOAT2(1.f, 1.f);
	//底面
	mesh.vertices[20].pos = XMFLOAT3(-halfW, -halfH, halfD);
	mesh.vertices[20].normal = XMFLOAT3(0.f, -1.f, 0.f);
	mesh.vertices[20].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[20].tex = XMFLOAT2(0.f, 1.f);
	mesh.vertices[21].pos = XMFLOAT3(-halfW, -halfH, -halfD);
	mesh.vertices[21].normal = XMFLOAT3(0.f, -1.f, 0.f);
	mesh.vertices[21].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[21].tex = XMFLOAT2(0.f, 0.f);
	mesh.vertices[22].pos = XMFLOAT3(halfW, -halfH, -halfD);
	mesh.vertices[22].normal = XMFLOAT3(0.f, -1.f, 0.f);
	mesh.vertices[22].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[22].tex = XMFLOAT2(1.f, 0.f);
	mesh.vertices[23].pos = XMFLOAT3(halfW, -halfH, halfD);
	mesh.vertices[23].normal = XMFLOAT3(0.f, -1.f, 0.f);
	mesh.vertices[23].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[23].tex = XMFLOAT2(1.f, 1.f);

	//构建索引
	mesh.indices[0] = 0;
	mesh.indices[1] = 1;
	mesh.indices[2] = 2;
	mesh.indices[3] = 0;
	mesh.indices[4] = 2;
	mesh.indices[5] = 3;

	mesh.indices[6] = 4;
	mesh.indices[7] = 5;
	mesh.indices[8] = 6;
	mesh.indices[9] = 4;
	mesh.indices[10] = 6;
	mesh.indices[11] = 7;

	mesh.indices[12] = 8;
	mesh.indices[13] = 9;
	mesh.indices[14] = 10;
	mesh.indices[15] = 8;
	mesh.indices[16] = 10;
	mesh.indices[17] = 11;

	mesh.indices[18] = 12;
	mesh.indices[19] = 13;
	mesh.indices[20] = 14;
	mesh.indices[21] = 12;
	mesh.indices[22] = 14;
	mesh.indices[23] = 15;

	mesh.indices[24] = 16;
	mesh.indices[25] = 17;
	mesh.indices[26] = 18;
	mesh.indices[27] = 16;
	mesh.indices[28] = 18;
	mesh.indices[29] = 19;

	mesh.indices[30] = 20;
	mesh.indices[31] = 21;
	mesh.indices[32] = 22;
	mesh.indices[33] = 20;
	mesh.indices[34] = 22;
	mesh.indices[35] = 23;
}

void GeometryGenerator::CreateGrid(float width, float height, UINT m, UINT n, MeshData &mesh)
{
	mesh.vertices.clear();
	mesh.indices.clear();
	//每行顶点数、每列顶点数
	UINT nVertsRow = m + 1;
	UINT nVertsCol = n + 1;
	//起始x、z坐标
	float oX = -width * 0.5f;
	float oZ = height * 0.5f;
	//每一格坐标变化
	float dx = width / m;
	float dz = height / n;

	//顶点总数量：nVertsRow * nVertsCol
	mesh.vertices.resize(nVertsRow * nVertsCol);

	//逐个添加顶点
	for (UINT i = 0; i < nVertsCol; ++i)
	{
		float tmpZ = oZ - dz * i;
		for (UINT j = 0; j < nVertsRow; ++j)
		{
			UINT index = nVertsRow * i + j;
			mesh.vertices[index].pos.x = oX + dx * j;
			mesh.vertices[index].pos.y = 0.f;
			mesh.vertices[index].pos.z = tmpZ;

			mesh.vertices[index].normal = XMFLOAT3(0.f, 1.f, 0.f);
			mesh.vertices[index].tangent = XMFLOAT3(1.f, 0.f, 0.f);

			mesh.vertices[index].tex = XMFLOAT2(dx*i, dx*j);
		}
	}

	//总格子数量:m * n
	//因此总索引数量: 6 * m * n
	UINT nIndices = m * n * 6;
	mesh.indices.resize(nIndices);
	UINT tmp = 0;
	for (UINT i = 0; i < n; ++i)
	{
		for (UINT j = 0; j < m; ++j)
		{
			mesh.indices[tmp] = i * nVertsRow + j;
			mesh.indices[tmp + 1] = i * nVertsRow + j + 1;
			mesh.indices[tmp + 2] = (i + 1) * nVertsRow + j;
			mesh.indices[tmp + 3] = i * nVertsRow + j + 1;
			mesh.indices[tmp + 4] = (i + 1) * nVertsRow + j + 1;
			mesh.indices[tmp + 5] = (i + 1) * nVertsRow + j;

			tmp += 6;
		}
	}
}

void GeometryGenerator::CreateCylinder(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh)
{
	mesh.vertices.clear();
	mesh.indices.clear();

	//从上到下每个stack半径变化量:dRadius
	float dRadius = (bottomRadius - topRadius) / stack;
	//每个stack高度:dHeight
	float dHeight = height / stack;

	//每个圆周上顶点数量:slice+1
	int vertsPerRow = slice + 1;
	//顶点行数:stack+1
	int nRows = stack + 1;

	//总顶点数
	int nVerts = vertsPerRow * nRows;
	//总索引数
	int nIndices = slice * stack * 6;

	mesh.vertices.resize(nVerts);
	mesh.indices.resize(nIndices);

	//顶部Y坐标
	float topY = height * 0.5f;

	for (int i = 0; i < nRows; ++i)
	{
		float tmpY = topY - dHeight * i;
		float tmpRadius = topRadius + i * dRadius;

		for (int j = 0; j < vertsPerRow; ++j)
		{
			float theta = XM_2PI * j / slice;
			int index = i * vertsPerRow + j;
			mesh.vertices[index].pos = XMFLOAT3(tmpRadius*cos(theta), tmpY, tmpRadius*sin(theta));
		}
	}

	UINT tmp(0);
	for (int i = 0; i < stack; ++i)
	{
		for (int j = 0; j < slice; ++j)
		{
			mesh.indices[tmp] = i * vertsPerRow + j;
			mesh.indices[tmp + 1] = (i + 1) * vertsPerRow + j + 1;
			mesh.indices[tmp + 2] = (i + 1) * vertsPerRow + j;
			mesh.indices[tmp + 3] = i * vertsPerRow + j;
			mesh.indices[tmp + 4] = i * vertsPerRow + j + 1;
			mesh.indices[tmp + 5] = (i + 1) * vertsPerRow + j + 1;

			tmp += 6;
		}
	}
}

void GeometryGenerator::AddCylinderTopCap(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh)
{
	UINT start = mesh.vertices.size();

	for (int i = 0; i < slice + 1; ++i)
	{
		float theta = XM_2PI * i / slice;

		float x = topRadius*cosf(theta);
		float y = height * 0.5f;
		float z = topRadius*sinf(theta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mesh.vertices.push_back(Vertex(XMFLOAT3(x, y, z), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT2(u, v)));
	}

	mesh.vertices.push_back(Vertex(XMFLOAT3(0.f, height*0.5f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT2(0.5f, 0.5f)));

	UINT center = mesh.vertices.size() - 1;
	for (int i = 0; i < slice; ++i)
	{
		mesh.indices.push_back(center);
		mesh.indices.push_back(start + i + 1);
		mesh.indices.push_back(start + i);
	}
}

void GeometryGenerator::AddCylinderBottomCap(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh)
{
	UINT start = mesh.vertices.size();

	for (int i = 0; i < slice + 1; ++i)
	{
		float theta = XM_2PI * i / slice;

		float x = bottomRadius*cosf(theta);
		float y = -height * 0.5f;
		float z = bottomRadius*sinf(theta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mesh.vertices.push_back(Vertex(XMFLOAT3(x, y, z), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT2(u, v)));
	}

	mesh.vertices.push_back(Vertex(XMFLOAT3(0.f, -height*0.5f, 0.f), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT2(0.5f, 0.5f)));

	UINT center = mesh.vertices.size() - 1;
	for (int i = 0; i < slice; ++i)
	{
		mesh.indices.push_back(center);
		mesh.indices.push_back(start + i);
		mesh.indices.push_back(start + i + 1);
	}
}

void GeometryGenerator::CreateSphere(float radius, int slice, int stack, MeshData &mesh)
{

	mesh.vertices.clear();
	mesh.indices.clear();

	int vertsPerRow = slice + 1;
	int nRows = stack - 1;

	int nVerts = vertsPerRow * nRows + 2;
	int nIndices = (nRows - 1)*slice * 6 + slice * 6;

	mesh.vertices.resize(nVerts);
	mesh.indices.resize(nIndices);

	for (int i = 1; i <= nRows; ++i)
	{
		float phy = XM_PI * i / stack;
		float tmpRadius = radius * sin(phy);
		for (int j = 0; j < vertsPerRow; ++j)
		{
			float theta = XM_2PI * j / slice;
			UINT index = (i - 1)*vertsPerRow + j;

			float x = tmpRadius*cos(theta);
			float y = radius*cos(phy);
			float z = tmpRadius*sin(theta);

			//位置坐标
			mesh.vertices[index].pos = XMFLOAT3(x, y, z);
			//法线
			XMVECTOR N = XMVectorSet(x, y, z, 0.f);
			XMStoreFloat3(&mesh.vertices[index].normal, XMVector3Normalize(N));
			//切线
			XMVECTOR T = XMVectorSet(-sin(theta), 0.f, cos(theta), 0.f);
			XMStoreFloat3(&mesh.vertices[index].tangent, XMVector3Normalize(T));
			//纹理坐标
			mesh.vertices[index].tex = XMFLOAT2(j*1.f / slice, i*1.f / stack);
		}
	}

	int size = vertsPerRow * nRows;
	//添加顶部和底部两个顶点信息
	mesh.vertices[size].pos = XMFLOAT3(0.f, radius, 0.f);
	mesh.vertices[size].normal = XMFLOAT3(0.f, 1.f, 0.f);
	mesh.vertices[size].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[size].tex = XMFLOAT2(0.f, 0.f);

	mesh.vertices[size + 1].pos = XMFLOAT3(0.f, -radius, 0.f);
	mesh.vertices[size + 1].normal = XMFLOAT3(0.f, -1.f, 0.f);
	mesh.vertices[size + 1].tangent = XMFLOAT3(1.f, 0.f, 0.f);
	mesh.vertices[size + 1].tex = XMFLOAT2(0.f, 1.f);

	UINT tmp(0);
	int start1 = 0;
	int start2 = mesh.vertices.size() - vertsPerRow - 2;
	int top = size;
	int bottom = size + 1;
	for (int i = 0; i < slice; ++i)
	{
		mesh.indices[tmp] = top;
		mesh.indices[tmp + 1] = start1 + i + 1;
		mesh.indices[tmp + 2] = start1 + i;

		tmp += 3;
	}

	for (int i = 0; i < slice; ++i)
	{
		mesh.indices[tmp] = bottom;
		mesh.indices[tmp + 1] = start2 + i;
		mesh.indices[tmp + 2] = start2 + i + 1;

		tmp += 3;
	}

	for (int i = 0; i < nRows - 1; ++i)
	{
		for (int j = 0; j < slice; ++j)
		{
			mesh.indices[tmp] = i * vertsPerRow + j;
			mesh.indices[tmp + 1] = (i + 1) * vertsPerRow + j + 1;
			mesh.indices[tmp + 2] = (i + 1) * vertsPerRow + j;
			mesh.indices[tmp + 3] = i * vertsPerRow + j;
			mesh.indices[tmp + 4] = i * vertsPerRow + j + 1;
			mesh.indices[tmp + 5] = (i + 1) * vertsPerRow + j + 1;

			tmp += 6;
		}
	}
}
