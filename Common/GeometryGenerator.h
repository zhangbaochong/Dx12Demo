#ifndef _GEOMETRY_GENERATOR_H
#define _GEOMETRY_GENERATOR_H

#include<vector>

class GeometryGenerator
{
private:
	GeometryGenerator() {}
public:
	//单例模式
	static GeometryGenerator* GetInstance()
	{
		static GeometryGenerator instance;
		return &instance;
	}

	//定义一个通用的顶点结构：位置、法线、切线、纹理坐标
	struct Vertex
	{
		Vertex() {}
		Vertex(const XMFLOAT3 _pos, XMFLOAT3 _normal, XMFLOAT3 _tangent, XMFLOAT2 _tex) :
			pos(_pos), normal(_normal), tangent(_tangent), tex(_tex) {}

		XMFLOAT3	pos;
		XMFLOAT3	normal;
		XMFLOAT3	tangent;
		XMFLOAT2	tex;
	};

	//基本网络结构：顶点集合+索引集合
	struct MeshData
	{
		std::vector<Vertex>	vertices;
		std::vector<UINT>	indices;
	};

	//创建一个立方体：指定宽(X方向)、高(Y方向)、深(Z方向)
	void CreateBox(float width, float height, float depth, MeshData &mesh);

	//创建一个网络格子：指定总宽度(X方向)、总高度(Z方向); m、n为宽、高方向上的格子数量
	void CreateGrid(float width, float height, UINT m, UINT n, MeshData &mesh);

	//创建一个圆柱(不含上、下两个口)：指定上端半径(topRadius)、下端半径(bottomRadius)、高度(height)、
	//sllice、stack
	void CreateCylinder(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh);
	
	//给现成的圆柱添加上口
	void AddCylinderTopCap(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh);
	
	//给现成的圆柱添加下口
	void AddCylinderBottomCap(float topRadius, float bottomRadius, float height, int slice, int stack, MeshData &mesh);
	
	//创建一个球体：指定半径(radius)、slice和stack
	void CreateSphere(float radius, int slice, int stack, MeshData &mesh);
};

#endif//_GEOMETRY_GENERATOR_H
