#include "Common\D3d12Base.h"
#include "Common\MathHelper.h"
#include "Common\UploadBuffer.h"
#include "Common\GeometryGenerator.h"
#include "Common\Camera.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

struct RenderItem
{
	RenderItem() = default;
	//世界矩阵和纹理转换矩阵
	XMFLOAT4X4 world = MathHelper::Identity4x4();
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();

	int numFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT objCBIndex = -1;

	Material* mat = nullptr;
	MeshGeometry* geo = nullptr;

	//拓扑结构
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//索引等
	UINT indexCount = 0;
	UINT startIndexLocation = 0;
	int baseVertexLocation = 0;
};

class TerrainDemo : public D3d12Base
{
public:
	TerrainDemo(HINSTANCE hInstance);
	TerrainDemo(const TerrainDemo& rhs) = delete;
	TerrainDemo& operator=(const TerrainDemo& rhs) = delete;
	~TerrainDemo();

	virtual bool Initialize() override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	//void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

	std::vector<std::unique_ptr<FrameResource>> m_frameResources;
	FrameResource* m_pCurrFrameResource = nullptr;
	int m_currFrameResourceIndex = 0;

	UINT m_cbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> m_pSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	ComPtr<ID3D12PipelineState> m_pOpaquePSO = nullptr;

	//列举所有的render items.
	std::vector<std::unique_ptr<RenderItem>> m_allRenderItems;

	// Render items divided by PSO.
	std::vector<RenderItem*> m_opaqueRitems;

	PassConstants m_mainPassCB;

	XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4();


	POINT m_lastMousePos;
	//摄像机
	Camera m_camera;

	//高度图相关的变量函数
private:
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
	
	
	std::vector<Vertex>		m_vertices;		//顶点集合
	std::vector<UINT>		m_indices;		//索引集合
	std::unordered_map<std::string, std::vector<Vertex>>  m_vertexItems;	//不同地形的顶点集合
	std::unordered_map<std::string, std::vector<UINT>> m_indexItems;		//不同地形的索引集合

	bool ReadRawFile(std::string filePath);										//从高度图读取高度信息
	bool InitTerrain(float width, float height, UINT m, UINT n, float scale);	//初始化地形
	void ComputeNomal(Vertex& v1, Vertex& v2, Vertex& v3, XMFLOAT3& normal);	//计算法线
};

//main函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		TerrainDemo theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}


TerrainDemo::TerrainDemo(HINSTANCE hInstance)
	:D3d12Base(hInstance)
{
	//设置相机
	m_lastMousePos = { 0,0 };
	XMVECTOR Eye = XMVectorSet(200.0f, 200.0f, 0.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_camera.LookAtXM(Eye, At, Up);
	//设置投影矩阵
	m_camera.SetLens(XM_PIDIV4, AspectRatio(), 0.1f, 1000.f);
}

TerrainDemo::~TerrainDemo()
{
	if (m_pD3dDevice != nullptr)
		FlushCommandQueue();
}

bool TerrainDemo::Initialize()
{
	if (!D3d12Base::Initialize())
		return false;

	//重置command list
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

	m_cbvSrvDescriptorSize = m_pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//初始化高度图地形
	//terrain.raw是最终地形
	if (!ReadRawFile("..\\Textures\\terrain_ps2.raw"))
		return false;
	if (!InitTerrain(500, 500, 511, 511, 1.f))
		return false;

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	

	//执行命令
	ThrowIfFailed(m_pCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//用fence确保等待直到初始化完毕
	FlushCommandQueue();

	return true;
}

void TerrainDemo::OnResize()
{
	D3d12Base::OnResize();
	//设置投影矩阵
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void TerrainDemo::OnMouseDown(WPARAM btnState, int x, int y){}
void TerrainDemo::OnMouseUp(WPARAM btnState, int x, int y){}
void TerrainDemo::OnMouseMove(WPARAM btnState, int x, int y) {}

void TerrainDemo::Update(const GameTimer& gt)
{
	UpdateCamera(gt);


	m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % gNumFrameResources;
	m_pCurrFrameResource = m_frameResources[m_currFrameResourceIndex].get();

	if (m_pCurrFrameResource->Fence != 0 && m_pFence->GetCompletedValue() < m_pCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_pFence->SetEventOnCompletion(m_pCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void TerrainDemo::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = m_pCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(m_pCommandList->Reset(cmdListAlloc.Get(), m_pOpaquePSO.Get()));

	m_pCommandList->RSSetViewports(1, &m_screenViewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_pCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_pSrvDescriptorHeap.Get() };
	m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	auto passCB = m_pCurrFrameResource->PassCB->Resource();
	m_pCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(m_pCommandList.Get(), m_opaqueRitems);

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_pCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(m_pSwapChain->Present(0, 0));
	m_currBackBuffer = (m_currBackBuffer + 1) % ms_swapChainBufferCount;

	m_pCurrFrameResource->Fence = ++m_currentFence;

	m_pCommandQueue->Signal(m_pFence.Get(), m_currentFence);
}

void TerrainDemo::UpdateCamera(const GameTimer& gt)
{
	//前后左右行走
	if (Input::GetInstance()->IsKeyDown('A'))
	{
		m_camera.Strafe(-60.f*gt.DeltaTime());
	}
	else if (Input::GetInstance()->IsKeyDown('D'))
	{
		m_camera.Strafe(60.f*gt.DeltaTime());
	}
	if (Input::GetInstance()->IsKeyDown('W'))
	{
		m_camera.Walk(60.f*gt.DeltaTime());
	}
	else if (Input::GetInstance()->IsKeyDown('S'))
	{
		m_camera.Walk(-60.f*gt.DeltaTime());
	}


	if (Input::GetInstance()->IsMouseMove())
	{
		float mouseX = Input::GetInstance()->GetMouseX();
		float mouseY = Input::GetInstance()->GetMouseY();
		if (Input::GetInstance()->IsLMouseDown())
		{
			float dx = XMConvertToRadians(0.25f*(mouseX - m_lastMousePos.x));
			float dy = XMConvertToRadians(0.25f*(mouseY - m_lastMousePos.y));

			OutputDebugString(L"left btn click");
			m_camera.Pitch(dy);
			m_camera.RotateY(dx);
		}
		m_lastMousePos.x = mouseX;
		m_lastMousePos.y = mouseY;
	}

	m_camera.UpdateViewMatrix();

	//根据camera更新视角、投影矩阵
	m_eyePos = m_camera.GetPosition();
	XMStoreFloat4x4(&m_view,m_camera.GetView());
	XMStoreFloat4x4(&m_proj ,m_camera.GetProj());
}

void TerrainDemo::AnimateMaterials(const GameTimer& gt)
{}

//更新各种矩阵等等
void TerrainDemo::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_pCurrFrameResource->ObjectCB.get();
	for (auto& e : m_allRenderItems)
	{	
		if (e->numFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->world);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->objCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->numFramesDirty--;
		}
	}
}

//更新材质
void TerrainDemo::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = m_pCurrFrameResource->MaterialCB.get();
	for (auto& e : m_materials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TerrainDemo::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);

	XMMATRIX viewProj = m_camera.GetViewProj();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&m_mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	m_mainPassCB.EyePosW = m_eyePos;
	m_mainPassCB.RenderTargetSize = XMFLOAT2(static_cast<float>(m_clientWidth), static_cast<float>(m_clientHeight));
	m_mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_clientWidth, 1.0f / m_clientHeight);
	m_mainPassCB.NearZ = 1.0f;
	m_mainPassCB.FarZ = 1000.0f;
	m_mainPassCB.TotalTime = gt.TotalTime();
	m_mainPassCB.DeltaTime = gt.DeltaTime();
	m_mainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_mainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_mainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	m_mainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_mainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	m_mainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	m_mainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = m_pCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_mainPassCB);
}

void TerrainDemo::LoadTextures()
{
	//加载不同贴图
	std::vector<std::string> texNames =
	{
		"groundTex",
		"grassTex",
		"roadTex",
		"waterBottomTex",
	};

	std::vector<std::wstring> texFilenames =
	{
		L"../Textures/tile.dds",	
		L"../Textures/grass1.dds",
		L"../Textures/road001.dds",
		L"../Textures/water1.dds"
	};

	for (int i = 0; i < (int)texNames.size(); ++i)
	{
		if (m_textures.find(texNames[i]) == std::end(m_textures))
		{
			auto texMap = std::make_unique<Texture>();
			texMap->Name = texNames[i];
			texMap->Filename = texFilenames[i];
			ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_pD3dDevice.Get(),
				m_pCommandList.Get(), texMap->Filename.c_str(),
				texMap->Resource, texMap->UploadHeap));
			m_textures[texMap->Name] = std::move(texMap);
		}
	}
}

void TerrainDemo::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature是root parameters组成的数组
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(m_pD3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf())));
}

//build 渲染目标描述符
void TerrainDemo::BuildDescriptorHeaps()
{
	// Create the SRV heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 64;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	//用actual descriptors填充heap 
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		m_textures["groundTex"]->Resource,
		m_textures["grassTex"]->Resource,
		m_textures["roadTex"]->Resource,
		m_textures["waterBottomTex"]->Resource
	};


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (UINT i = 0; i < (UINT)tex2DList.size(); ++i)
	{
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		m_pD3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, m_cbvSrvUavDescriptorSize);
	}

}

void TerrainDemo::BuildShadersAndInputLayout()
{
	m_shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	m_shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

//创建网格
void TerrainDemo::BuildShapeGeometry()
{	
	//create vertex offset
	UINT groundVertexOffset = 0;
	UINT grassVertexOffset = m_vertexItems["ground"].size();
	UINT roadVertexOffset = grassVertexOffset + m_vertexItems["grass"].size();
	UINT waterVertexOffset = roadVertexOffset + m_vertexItems["road"].size();

	//create start index
	UINT groundIndexOffset = 0;
	UINT grassIndexOffset = m_indexItems["ground"].size();
	UINT roadIndexOffset = grassIndexOffset + m_indexItems["grass"].size();
	UINT waterIndexOffset = roadIndexOffset + m_indexItems["road"].size();

	SubmeshGeometry groundSubmesh;
	groundSubmesh.IndexCount = m_indexItems["ground"].size();
	groundSubmesh.StartIndexLocation = groundIndexOffset;
	groundSubmesh.BaseVertexLocation = groundVertexOffset;
	SubmeshGeometry grassSubmesh;
	grassSubmesh.IndexCount = m_indexItems["grass"].size();
	grassSubmesh.StartIndexLocation = grassIndexOffset;
	grassSubmesh.BaseVertexLocation = grassVertexOffset;
	SubmeshGeometry roadSubmesh;
	roadSubmesh.IndexCount = m_indexItems["road"].size();
	roadSubmesh.StartIndexLocation = roadIndexOffset;
	roadSubmesh.BaseVertexLocation = roadVertexOffset;
	SubmeshGeometry waterSubmesh;
	waterSubmesh.IndexCount = m_indexItems["water"].size();
	waterSubmesh.StartIndexLocation = waterIndexOffset;
	waterSubmesh.BaseVertexLocation = waterVertexOffset;

	auto totalVertexCount =
		m_vertexItems["ground"].size() + m_vertexItems["grass"].size()
		+ m_vertexItems["road"].size() + m_vertexItems["water"].size();

	std::vector<Vertex> vertices(totalVertexCount);
	UINT k = 0;
	for (size_t i = 0; i < m_vertexItems["ground"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_vertexItems["ground"][i].Pos;
		vertices[k].Normal = m_vertexItems["ground"][i].Normal;
		vertices[k].TexC = m_vertexItems["ground"][i].TexC;
	}

	for (size_t i = 0; i < m_vertexItems["grass"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_vertexItems["grass"][i].Pos;
		vertices[k].Normal = m_vertexItems["grass"][i].Normal;
		vertices[k].TexC = m_vertexItems["grass"][i].TexC;
	}

	for (size_t i = 0; i < m_vertexItems["road"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_vertexItems["road"][i].Pos;
		vertices[k].Normal = m_vertexItems["road"][i].Normal;
		vertices[k].TexC = m_vertexItems["road"][i].TexC;
	}

	for (size_t i = 0; i < m_vertexItems["water"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_vertexItems["water"][i].Pos;
		vertices[k].Normal = m_vertexItems["water"][i].Normal;
		vertices[k].TexC = m_vertexItems["water"][i].TexC;
	}

	std::vector<UINT> indices;
	indices.insert(indices.end(),m_indexItems["ground"].begin(), m_indexItems["ground"].end());
	indices.insert(indices.end(), m_indexItems["grass"].begin(), m_indexItems["grass"].end());
	indices.insert(indices.end(), m_indexItems["road"].begin(), m_indexItems["road"].end());
	indices.insert(indices.end(), m_indexItems["water"].begin(), m_indexItems["water"].end());

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pD3dDevice.Get(),
		m_pCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pD3dDevice.Get(),
		m_pCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["ground"] = groundSubmesh;
	geo->DrawArgs["grass"] = grassSubmesh;
	geo->DrawArgs["road"] = roadSubmesh;
	geo->DrawArgs["water"] = waterSubmesh;

	m_geometries[geo->Name] = std::move(geo);	
}

void TerrainDemo::BuildPSOs()
{
	//pso描述符
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	opaquePsoDesc.pRootSignature = m_pRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["standardVS"]->GetBufferPointer()),
		m_shaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["opaquePS"]->GetBufferPointer()),
		m_shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_backBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = m_depthStencilFormat;
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_pOpaquePSO)));
}

void TerrainDemo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResource>(m_pD3dDevice.Get(),
			1, (UINT)m_allRenderItems.size(), (UINT)m_materials.size()));
	}
}

void TerrainDemo::BuildMaterials()
{
	auto groudMat = std::make_unique<Material>();
	groudMat->Name = "ground";
	groudMat->MatCBIndex = 0;
	groudMat->DiffuseSrvHeapIndex = 0;
	groudMat->NormalSrvHeapIndex = 0;
	groudMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	groudMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	groudMat->Roughness = 0.3f;

	auto grassMat = std::make_unique<Material>();
	grassMat->Name = "grass";
	grassMat->MatCBIndex = 1;
	grassMat->DiffuseSrvHeapIndex = 1;
	grassMat->NormalSrvHeapIndex = 1;
	grassMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grassMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	grassMat->Roughness = 1.0f;

	auto roadMat = std::make_unique<Material>();
	roadMat->Name = "road";
	roadMat->MatCBIndex = 2;
	roadMat->DiffuseSrvHeapIndex = 2;
	roadMat->NormalSrvHeapIndex = 2;
	roadMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	roadMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	roadMat->Roughness = 0.1f;

	auto waterMat = std::make_unique<Material>();
	waterMat->Name = "water";
	waterMat->MatCBIndex = 3;
	waterMat->DiffuseSrvHeapIndex = 3;
	waterMat->NormalSrvHeapIndex = 3;
	waterMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	waterMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	waterMat->Roughness = 0.3f;

	m_materials["ground"] = std::move(groudMat);
	m_materials["grass"] = std::move(grassMat);
	m_materials["road"] = std::move(roadMat);
	m_materials["water"] = std::move(waterMat);
}

void TerrainDemo::BuildRenderItems()
{		
	auto groundRitem = std::make_unique<RenderItem>();
	groundRitem->objCBIndex = 0;
	groundRitem->mat = m_materials["ground"].get();
	groundRitem->geo = m_geometries["shapeGeo"].get();
	//纹理变换矩阵，针对地形贴图具体大小做调整
	XMStoreFloat4x4(&groundRitem->texTransform, XMMatrixScaling(.02f, 0.02f, 1.f));
	groundRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	groundRitem->indexCount = groundRitem->geo->DrawArgs["ground"].IndexCount;
	groundRitem->startIndexLocation = groundRitem->geo->DrawArgs["ground"].StartIndexLocation;
	groundRitem->baseVertexLocation = groundRitem->geo->DrawArgs["ground"].BaseVertexLocation;
	m_allRenderItems.push_back(std::move(groundRitem));
	
	auto grassRitem = std::make_unique<RenderItem>();
	grassRitem->objCBIndex = 1;
	grassRitem->mat = m_materials["grass"].get();
	grassRitem->geo = m_geometries["shapeGeo"].get();
	XMStoreFloat4x4(&grassRitem->texTransform, XMMatrixScaling(0.03f, 0.03f, 1.f));
	grassRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	grassRitem->indexCount = grassRitem->geo->DrawArgs["grass"].IndexCount;
	grassRitem->startIndexLocation = grassRitem->geo->DrawArgs["grass"].StartIndexLocation;
	grassRitem->baseVertexLocation = grassRitem->geo->DrawArgs["grass"].BaseVertexLocation;
	m_allRenderItems.push_back(std::move(grassRitem));
	
	auto roadRitem = std::make_unique<RenderItem>();
	roadRitem->objCBIndex = 2;
	roadRitem->mat = m_materials["road"].get();
	roadRitem->geo = m_geometries["shapeGeo"].get();
	XMStoreFloat4x4(&roadRitem->texTransform, XMMatrixScaling(0.03f, 0.03f, 1.f));
	roadRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	roadRitem->indexCount = roadRitem->geo->DrawArgs["road"].IndexCount;
	roadRitem->startIndexLocation = roadRitem->geo->DrawArgs["road"].StartIndexLocation;
	roadRitem->baseVertexLocation = roadRitem->geo->DrawArgs["road"].BaseVertexLocation;
	m_allRenderItems.push_back(std::move(roadRitem));

	auto waterRitem = std::make_unique<RenderItem>();
	waterRitem->objCBIndex = 3;
	waterRitem->mat = m_materials["water"].get();
	waterRitem->geo = m_geometries["shapeGeo"].get();
	XMStoreFloat4x4(&waterRitem->texTransform, XMMatrixScaling(0.02f, 0.02f, 1.f));
	waterRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	waterRitem->indexCount = waterRitem->geo->DrawArgs["water"].IndexCount;
	waterRitem->startIndexLocation = waterRitem->geo->DrawArgs["water"].StartIndexLocation;
	waterRitem->baseVertexLocation = waterRitem->geo->DrawArgs["water"].BaseVertexLocation;
	m_allRenderItems.push_back(std::move(waterRitem));

	// All the render items are opaque.
	for (auto& e : m_allRenderItems)
		m_opaqueRitems.push_back(e.get());
}

void TerrainDemo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = m_pCurrFrameResource->ObjectCB->Resource();
	auto matCB = m_pCurrFrameResource->MaterialCB->Resource();

	// 对于每个render item给出命令
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->primitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->mat->DiffuseSrvHeapIndex, m_cbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->objCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->indexCount, 1, ri->startIndexLocation, ri->baseVertexLocation, 0);
	}
}

//从.raw文件读取高度图信息
bool TerrainDemo::ReadRawFile(std::string filePath)
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

//初始化地形,重点算出不同地形对应的顶点索引等等
bool TerrainDemo::InitTerrain(float width, float height, UINT m, UINT n, float scale)
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

	m_vertices.resize(m_numsVertices);

	//计算顶点
	for (UINT i = 0; i < m_verticesPerCol; ++i)
	{
		float tempZ = oZ - dz * i;
		for (UINT j = 0; j < m_verticesPerRow; ++j)
		{
			UINT index = m_verticesPerRow * i + j;
			m_vertices[index].Pos.x = oX + dx * j;
			m_vertices[index].Pos.y = m_heightInfos[index];
			m_vertices[index].Pos.z = tempZ;
			m_vertices[index].TexC = XMFLOAT2(dx*i, dx*j);
					
		}
	}
	
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
		m_vertexItems["water"].resize(waterWidth * waterHeight);
		UINT offsetX = 224;
		//共512行
		for (UINT i = 0; i < waterHeight; ++i)
		{
			float tempZ = oZ - dz * i;
			//共64列
			for (UINT j = 0; j < waterWidth; ++j)
			{
				UINT index = waterWidth * i + j;
				m_vertexItems["water"][index].Pos.x = oX + dx * (j + offsetX);
				m_vertexItems["water"][index].Pos.y = m_heightInfos[m_verticesPerRow*i + (j + offsetX)];
				m_vertexItems["water"][index].Pos.z = tempZ;
				m_vertexItems["water"][index].TexC = XMFLOAT2(dx*i, dx*j);
			}
		}
		//计算water索引
		m_indexItems["water"].resize(6 * (waterWidth - 1)*(waterHeight - 1));
		UINT tmp = 0;
		for (UINT i = 0; i < waterHeight - 1; ++i)
		{
			for (UINT j = 0; j < waterWidth - 1; ++j)
			{
				m_indexItems["water"][tmp] = i * waterWidth + j;
				m_indexItems["water"][tmp + 1] = i * waterWidth + j + 1;
				m_indexItems["water"][tmp + 2] = (i + 1) * waterWidth + j;
				//计算法线
				XMFLOAT3 temp;
				ComputeNomal(m_vertexItems["water"][m_indexItems["water"][tmp]],
					m_vertexItems["water"][m_indexItems["water"][tmp + 1]],
					m_vertexItems["water"][m_indexItems["water"][tmp + 2]], temp);
				m_vertexItems["water"][m_indexItems["water"][tmp]].Normal = temp;
				m_vertexItems["water"][m_indexItems["water"][tmp + 1]].Normal = temp;
				m_vertexItems["water"][m_indexItems["water"][tmp + 2]].Normal = temp;


				m_indexItems["water"][tmp + 3] = i * waterWidth + j + 1;
				m_indexItems["water"][tmp + 4] = (i + 1) * waterWidth + j + 1;
				m_indexItems["water"][tmp + 5] = (i + 1) * waterWidth + j;
				ComputeNomal(m_vertexItems["water"][m_indexItems["water"][tmp + 3]],
					m_vertexItems["water"][m_indexItems["water"][tmp + 4]],
					m_vertexItems["water"][m_indexItems["water"][tmp + 5]], temp);
				m_vertexItems["water"][m_indexItems["water"][tmp + 3]].Normal = temp;
				m_vertexItems["water"][m_indexItems["water"][tmp + 4]].Normal = temp;
				m_vertexItems["water"][m_indexItems["water"][tmp + 5]].Normal = temp;

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
					+ rightOffsetX ] + 0.1f;
				m_vertexItems["road"][index].Pos.z = tempZ;
				m_vertexItems["road"][index].TexC = XMFLOAT2(dx*i, dx*j);
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

//计算法线
void TerrainDemo::ComputeNomal(Vertex& v1, Vertex& v2, Vertex& v3, XMFLOAT3& normal)
{
	XMFLOAT3 f1(v2.Pos.x - v1.Pos.x, v2.Pos.y - v1.Pos.y, v2.Pos.z - v1.Pos.z);
	XMFLOAT3 f2(v3.Pos.x - v1.Pos.x, v3.Pos.y - v1.Pos.y, v3.Pos.z - v1.Pos.z);
	XMVECTOR vec1 = XMLoadFloat3(&f1);
	XMVECTOR vec2 = XMLoadFloat3(&f2);
	XMVECTOR temp = XMVector3Normalize(XMVector3Cross(vec1, vec2));
	XMStoreFloat3(&normal, temp);
}

//定义一些常用的纹理采样方式
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TerrainDemo::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return{
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

