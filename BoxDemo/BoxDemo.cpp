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

class BoxDemo : public D3d12Base
{
public:
	BoxDemo(HINSTANCE hInstance);
	BoxDemo(const BoxDemo& rhs) = delete;
	BoxDemo& operator=(const BoxDemo& rhs) = delete;
	~BoxDemo();

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
		BoxDemo theApp(hInstance);
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


BoxDemo::BoxDemo(HINSTANCE hInstance)
	:D3d12Base(hInstance)
{
	//设置相机
	//设置相机
	m_lastMousePos = { 0,0 };
	XMVECTOR Eye = XMVectorSet(0.0f, 10.0f, 0.1f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_camera.LookAtXM(Eye, At, Up);
	//设置投影矩阵
	m_camera.SetLens(XM_PIDIV4, AspectRatio(), 1.f, 1000.f);
}

BoxDemo::~BoxDemo()
{
	if (m_pD3dDevice != nullptr)
		FlushCommandQueue();
}

bool BoxDemo::Initialize()
{
	if (!D3d12Base::Initialize())
		return false;

	//重置command list
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

	m_cbvSrvDescriptorSize = m_pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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

void BoxDemo::OnResize()
{
	D3d12Base::OnResize();
	//设置投影矩阵
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void BoxDemo::OnMouseDown(WPARAM btnState, int x, int y){}
void BoxDemo::OnMouseUp(WPARAM btnState, int x, int y){}
void BoxDemo::OnMouseMove(WPARAM btnState, int x, int y) {}

void BoxDemo::Update(const GameTimer& gt)
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

void BoxDemo::Draw(const GameTimer& gt)
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

void BoxDemo::UpdateCamera(const GameTimer& gt)
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

void BoxDemo::AnimateMaterials(const GameTimer& gt)
{}

//更新各种矩阵等等
void BoxDemo::UpdateObjectCBs(const GameTimer& gt)
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
void BoxDemo::UpdateMaterialCBs(const GameTimer& gt)
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

void BoxDemo::UpdateMainPassCB(const GameTimer& gt)
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

void BoxDemo::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../Textures/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_pD3dDevice.Get(),
		m_pCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

	m_textures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void BoxDemo::BuildRootSignature()
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
void BoxDemo::BuildDescriptorHeaps()
{
	// Create the SRV heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	//用actual descriptors填充heap 
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto woodCrateTex = m_textures["woodCrateTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = woodCrateTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//同d3d11的shaderResourceView
	m_pD3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
}

void BoxDemo::BuildShadersAndInputLayout()
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
void BoxDemo::BuildShapeGeometry()
{
	//创建一个立方体
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;

	//顶点和索引缓冲
	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

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
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;

	m_geometries[geo->Name] = std::move(geo);
}

void BoxDemo::BuildPSOs()
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

void BoxDemo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResource>(m_pD3dDevice.Get(),
			1, (UINT)m_allRenderItems.size(), (UINT)m_materials.size()));
	}
}

void BoxDemo::BuildMaterials()
{
	auto woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	m_materials["woodCrate"] = std::move(woodCrate);
}

void BoxDemo::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->objCBIndex = 0;
	boxRitem->mat = m_materials["woodCrate"].get();
	boxRitem->geo = m_geometries["boxGeo"].get();
	boxRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->indexCount = boxRitem->geo->DrawArgs["box"].IndexCount;
	boxRitem->startIndexLocation = boxRitem->geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->baseVertexLocation = boxRitem->geo->DrawArgs["box"].BaseVertexLocation;
	m_allRenderItems.push_back(std::move(boxRitem));//采用移动操作

	// All the render items are opaque.
	for (auto& e : m_allRenderItems)
		m_opaqueRitems.push_back(e.get());
}

void BoxDemo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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

//定义一些常用的纹理采样方式
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> BoxDemo::GetStaticSamplers()
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

