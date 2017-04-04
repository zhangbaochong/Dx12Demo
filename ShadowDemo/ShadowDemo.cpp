#include "Common\D3d12Base.h"
#include "Common\MathHelper.h"
#include "Common\UploadBuffer.h"
#include "Common\GeometryGenerator.h"
#include "Common\Camera.h"
#include "FrameResource.h"
#include "Terrain.h"
#include "waves.h"
#include "ShadowMap.h"
#include "ModelImporter.h"

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

//层次
enum class RenderLayer : int
{
	Opaque = 0,
	Sky,
	Transparent,
	Debug,
	Count
};

class ShadowDemo : public D3d12Base
{
public:
	ShadowDemo(HINSTANCE hInstance);
	ShadowDemo(const ShadowDemo& rhs) = delete;
	ShadowDemo& operator=(const ShadowDemo& rhs) = delete;
	~ShadowDemo();

	virtual bool Initialize() override;

private:
	virtual void CreateRtvAndDsvDescriptorHeaps()override;
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
	void UpdateMaterialBuffer(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildTerrainGeometry();
	void BuildShapeGeometry();
	void BuildWavesGeometry();
	void BuildModelsGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawSceneToShadowMap();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

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
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	RenderItem* m_wavesRitem = nullptr;

	//列举所有的render items.
	std::vector<std::unique_ptr<RenderItem>> m_allRenderItems;

	// Render items divided by PSO.
	std::vector<RenderItem*> m_ritemLayer[(int)RenderLayer::Count];

	UINT m_skyTexHeapIndex = 0;
	UINT m_shadowMapHeapIndex = 0;

	UINT m_nullCubeSrvIndex = 0;
	UINT m_nullTexSrvIndex = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	PassConstants m_mainPassCB;// index 0 of pass cbuffer.
	PassConstants mShadowPassCB;// index 1 of pass cbuffer.

	//地形
	std::unique_ptr<Terrain> m_pTerrain;

	//波纹
	std::unique_ptr<Waves> m_waves;

	//阴影
	std::unique_ptr<ShadowMap> m_pShadowMap;

	//模型
	std::unique_ptr<ModelImporter> m_pModelImporter;
	UINT m_modelSrvHeapIndex = 0;
	std::vector<std::string> m_modelTexNames;

	DirectX::BoundingSphere m_sceneBounds;

	//light相关变量
	float m_lightNearZ = 0.0f;
	float m_lightFarZ = 0.0f;
	XMFLOAT3 m_lightPosW;
	XMFLOAT4X4 m_lightView = MathHelper::Identity4x4();
	XMFLOAT4X4 m_lightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 m_shadowTransform = MathHelper::Identity4x4();

	float m_lightRotationAngle = 0.0f;
	XMFLOAT3 m_baseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 m_rotatedLightDirections[3];

	//摄像机相关变量
	XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4();
	POINT m_lastMousePos;
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
		ShadowDemo theApp(hInstance);
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


ShadowDemo::ShadowDemo(HINSTANCE hInstance)
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

	m_sceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//m_sceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
	//包围球的半径，包围球应该包含整个场景,所以球的直径应该等于场地对角线 场地长宽现在为500，对角线为500*1.414
	m_sceneBounds.Radius = 500.f*1.414f / 2 + 2.f;
}

ShadowDemo::~ShadowDemo()
{
	if (m_pD3dDevice != nullptr)
		FlushCommandQueue();
}

bool ShadowDemo::Initialize()
{
	if (!D3d12Base::Initialize())
		return false;

	//重置command list
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

	m_cbvSrvDescriptorSize = m_pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pTerrain = std::make_unique<Terrain>(500, 500, 511, 511, 1.f);

	m_waves = std::make_unique<Waves>(500, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	m_pShadowMap = std::make_unique<ShadowMap>(m_pD3dDevice.Get(), 2048, 2048);

	//加载模型
	m_pModelImporter = std::make_unique<ModelImporter>("fox");
	m_pModelImporter->LoadModel("..\\Models\\fox\\file.fbx");

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildTerrainGeometry();
	BuildShapeGeometry();
	BuildWavesGeometry();
	BuildModelsGeometry();
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

void ShadowDemo::CreateRtvAndDsvDescriptorHeaps()
{
	// Add +6 RTV for cube render target.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = ms_swapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_pD3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_pRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_pD3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())));
}

void ShadowDemo::OnResize()
{
	D3d12Base::OnResize();
	//设置投影矩阵
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void ShadowDemo::OnMouseDown(WPARAM btnState, int x, int y){}
void ShadowDemo::OnMouseUp(WPARAM btnState, int x, int y){}
void ShadowDemo::OnMouseMove(WPARAM btnState, int x, int y) {}

void ShadowDemo::Update(const GameTimer& gt)
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

	//改变灯光角度
	m_lightRotationAngle += 0.1f*gt.DeltaTime();
	XMMATRIX R = XMMatrixRotationY(m_lightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&m_baseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&m_rotatedLightDirections[i], lightDir);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	UpdateShadowPassCB(gt);
	UpdateWaves(gt);
}

void ShadowDemo::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = m_pCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(m_pCommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_pSrvDescriptorHeap.Get() };
	m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// Bind all the materials used in this scene
	auto matBuffer = m_pCurrFrameResource->MaterialBuffer->Resource();
	m_pCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// Bind null SRV for shadow map pass.
	m_pCommandList->SetGraphicsRootDescriptorTable(3, mNullSrv);

	// Bind all the textures used in this scene. 
	m_pCommandList->SetGraphicsRootDescriptorTable(4, m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	DrawSceneToShadowMap();


	m_pCommandList->RSSetViewports(1, &m_screenViewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_pCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	/**
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_pSrvDescriptorHeap.Get() };
	m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());*/

	auto passCB = m_pCurrFrameResource->PassCB->Resource();
	m_pCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	/* Bind all the materials used in this scene.
	auto matBuffer = m_pCurrFrameResource->MaterialBuffer->Resource();
	m_pCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());*/

	//Bind the sky cube map
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(m_skyTexHeapIndex, m_cbvSrvDescriptorSize);
	m_pCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	/* Bind all the textures used in this scene
	m_pCommandList->SetGraphicsRootDescriptorTable(4, m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	*/
	
	//render opaque
	m_pCommandList->SetPipelineState(m_PSOs["opaque"].Get());
	DrawRenderItems(m_pCommandList.Get(), m_ritemLayer[(int)RenderLayer::Opaque]);
	//render sky
	m_pCommandList->SetPipelineState(m_PSOs["sky"].Get());
	DrawRenderItems(m_pCommandList.Get(), m_ritemLayer[(int)RenderLayer::Sky]);

	/*render debug 暂时没添加debug层 可以用于在屏幕上上显示东西
	m_pCommandList->SetPipelineState(m_PSOs["debug"].Get());
	DrawRenderItems(m_pCommandList.Get(), m_ritemLayer[(int)RenderLayer::Debug]);*/

	//render transparent
	m_pCommandList->SetPipelineState(m_PSOs["transparent"].Get());
	DrawRenderItems(m_pCommandList.Get(), m_ritemLayer[(int)RenderLayer::Transparent]);

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

void ShadowDemo::UpdateCamera(const GameTimer& gt)
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

void ShadowDemo::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	//改变water纹理坐标
	auto waterMat = m_materials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void ShadowDemo::UpdateObjectCBs(const GameTimer& gt)
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
			objConstants.MaterialIndex = e->mat->MatCBIndex;

			currObjectCB->CopyData(e->objCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->numFramesDirty--;
		}
	}
}

//更新材质
void ShadowDemo::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialBuffer = m_pCurrFrameResource->MaterialBuffer.get();
	for (auto& e : m_materials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			matData.NormalMapIndex = mat->NormalSrvHeapIndex;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void ShadowDemo::UpdateShadowTransform(const GameTimer& gt)
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&m_rotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f*m_sceneBounds.Radius*lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&m_sceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&m_lightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - m_sceneBounds.Radius;
	float b = sphereCenterLS.y - m_sceneBounds.Radius;
	float n = sphereCenterLS.z - m_sceneBounds.Radius;
	float r = sphereCenterLS.x + m_sceneBounds.Radius;
	float t = sphereCenterLS.y + m_sceneBounds.Radius;
	float f = sphereCenterLS.z + m_sceneBounds.Radius;

	m_lightNearZ = n;
	m_lightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView*lightProj*T;
	XMStoreFloat4x4(&m_lightView, lightView);
	XMStoreFloat4x4(&m_lightProj, lightProj);
	XMStoreFloat4x4(&m_shadowTransform, S);
}

void ShadowDemo::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);

	XMMATRIX viewProj = m_camera.GetViewProj();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMMATRIX shadowTransform = XMLoadFloat4x4(&m_shadowTransform);

	XMStoreFloat4x4(&m_mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&m_mainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	m_mainPassCB.EyePosW = m_eyePos;
	m_mainPassCB.RenderTargetSize = XMFLOAT2(static_cast<float>(m_clientWidth), static_cast<float>(m_clientHeight));
	m_mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_clientWidth, 1.0f / m_clientHeight);
	m_mainPassCB.NearZ = 1.0f;
	m_mainPassCB.FarZ = 1000.0f;
	m_mainPassCB.TotalTime = gt.TotalTime();
	m_mainPassCB.DeltaTime = gt.DeltaTime();
	m_mainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	/**
	m_mainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_mainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	m_mainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_mainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	m_mainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	m_mainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };*/
	m_mainPassCB.Lights[0].Direction = m_rotatedLightDirections[0];
	m_mainPassCB.Lights[0].Strength = { 0.9f, 0.8f, 0.7f };
	m_mainPassCB.Lights[1].Direction = m_rotatedLightDirections[1];
	m_mainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	m_mainPassCB.Lights[2].Direction = m_rotatedLightDirections[2];
	m_mainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = m_pCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_mainPassCB);
}

//update波纹纹理
void ShadowDemo::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((m_timer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(8, m_waves->RowCount() - 5);
		int j = MathHelper::Rand(8, m_waves->ColumnCount() - 5);

		float r = MathHelper::RandF(1.f, 1.5f);

		m_waves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	m_waves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = m_pCurrFrameResource->WavesVB.get();
	for (int i = 0; i < m_waves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = m_waves->Position(i);
		v.Normal = m_waves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / m_waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / m_waves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	m_wavesRitem->geo->VertexBufferGPU = currWavesVB->Resource();
}

void ShadowDemo::UpdateShadowPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_lightView);
	XMMATRIX proj = XMLoadFloat4x4(&m_lightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = m_pShadowMap->Width();
	UINT h = m_pShadowMap->Height();

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mShadowPassCB.EyePosW = m_lightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = m_lightNearZ;
	mShadowPassCB.FarZ = m_lightFarZ;

	auto currPassCB = m_pCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mShadowPassCB);
}

void ShadowDemo::LoadTextures()
{
	//加载不同贴图
	std::vector<std::string> texNames =
	{
		"blankNormalMap",
		"groundDiffuseMap",
		"groundNormalMap",
		"grassDiffuseMap",
		"grassNormalMap",
		"roadDiffuseMap",
		"roadNormalMap",
		"waterBottomDiffuseMap",
		"waterBottomNormalMap",
		"waterDiffuseMap",
		"waterNormalMap",
		"skyCubeMap"
	};

	std::vector<std::wstring> texFilenames =
	{
		L"../Textures/blank_nrm.dds",
		L"../Textures/tile.dds",
		L"../Textures/tile_nmap.dds",
		L"../Textures/grass_dif.dds",
		L"../Textures/grass_nrm.dds",
		L"../Textures/road_dif.dds",
		L"../Textures/road_nrm.dds",
		L"../Textures/water_bottom_dif.dds",
		L"../Textures/water_bottom_nrm.dds",
		L"../Textures/water002_dif.dds",
		L"../Textures/water002_nrm.dds",
		L"../Textures/grasscube1024.dds"
	};

	//添加模型里的贴图
	for (int i = 0; i < m_pModelImporter->m_meshes.size(); ++i)
	{
		std::string diffuseFileName = m_pModelImporter->m_meshes[i].material.DiffuseMapName;
		std::string normalFileName = m_pModelImporter->m_meshes[i].material.NormalMapName;
		if (diffuseFileName != "")
		{
			m_modelTexNames.push_back(m_pModelImporter->m_meshes[i].material.Name + "DiffuseMap");
			texNames.push_back(m_pModelImporter->m_meshes[i].material.Name + "DiffuseMap");
			texFilenames.push_back(AnsiToWString(diffuseFileName));
		}
		if (normalFileName != "")
		{
			m_modelTexNames.push_back(m_pModelImporter->m_meshes[i].material.Name + "NormalMap");
			texNames.push_back(m_pModelImporter->m_meshes[i].material.Name + "NormalMap");
			texFilenames.push_back(AnsiToWString(normalFileName));
		}
	}


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

void ShadowDemo::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2, 0);//原先是20

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	// A root signature是root parameters组成的数组
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
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
void ShadowDemo::BuildDescriptorHeaps()
{
	// Create the SRV heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 64; //图片数量*2,？？？
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvDescriptorHeap)));

	//用actual descriptors填充heap 
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		m_textures["blankNormalMap"]->Resource,
		m_textures["groundDiffuseMap"]->Resource,
		m_textures["groundNormalMap"]->Resource,
		m_textures["grassDiffuseMap"]->Resource,
		m_textures["grassNormalMap"]->Resource,
		m_textures["roadDiffuseMap"]->Resource,
		m_textures["roadNormalMap"]->Resource,
		m_textures["waterBottomDiffuseMap"]->Resource,
		m_textures["waterBottomNormalMap"]->Resource,
		m_textures["waterDiffuseMap"]->Resource,
		m_textures["waterNormalMap"]->Resource
	};

	m_modelSrvHeapIndex = (UINT)tex2DList.size();

	for (UINT i = 0; i < (UINT)m_modelTexNames.size(); ++i)
	{
		auto texResource = m_textures[m_modelTexNames[i]]->Resource;
		assert(texResource != nullptr);
		tex2DList.push_back(texResource);
	}

	auto skyTex = m_textures["skyCubeMap"]->Resource;
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

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyTex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyTex->GetDesc().Format;
	m_pD3dDevice->CreateShaderResourceView(skyTex.Get(), &srvDesc, hDescriptor);

	//天空贴图索引
	m_skyTexHeapIndex = (UINT)tex2DList.size();
	//shadow索引
	m_shadowMapHeapIndex = m_skyTexHeapIndex + 1;

	m_nullCubeSrvIndex = m_shadowMapHeapIndex + 1;
	m_nullTexSrvIndex = m_nullCubeSrvIndex + 1;

	auto srvCpuStart = m_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = m_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_nullCubeSrvIndex, m_cbvSrvUavDescriptorSize);
	mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_nullCubeSrvIndex, m_cbvSrvUavDescriptorSize);

	m_pD3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, m_cbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_pD3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	m_pShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_shadowMapHeapIndex, m_cbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_shadowMapHeapIndex, m_cbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, m_dsvDescriptorSize));

}

void ShadowDemo::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	m_shaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	m_shaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

	//m_shaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	//m_shaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	m_shaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

//创建网格
void ShadowDemo::BuildTerrainGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);

	//create vertex offset
	UINT groundVertexOffset = 0;
	UINT grassVertexOffset = m_pTerrain->m_vertexItems["ground"].size();
	UINT roadVertexOffset = grassVertexOffset + m_pTerrain->m_vertexItems["grass"].size();
	UINT waterVertexOffset = roadVertexOffset + m_pTerrain->m_vertexItems["road"].size();
	UINT sphereVertexOffset = waterVertexOffset + m_pTerrain->m_vertexItems["waterBottom"].size();

	//create start index
	UINT groundIndexOffset = 0;
	UINT grassIndexOffset = m_pTerrain->m_indexItems["ground"].size();
	UINT roadIndexOffset = grassIndexOffset + m_pTerrain->m_indexItems["grass"].size();
	UINT waterIndexOffset = roadIndexOffset + m_pTerrain->m_indexItems["road"].size();
	UINT sphereIndexOffset = waterIndexOffset + m_pTerrain->m_indexItems["waterBottom"].size();

	SubmeshGeometry groundSubmesh;
	groundSubmesh.IndexCount = m_pTerrain->m_indexItems["ground"].size();
	groundSubmesh.StartIndexLocation = groundIndexOffset;
	groundSubmesh.BaseVertexLocation = groundVertexOffset;

	SubmeshGeometry grassSubmesh;
	grassSubmesh.IndexCount = m_pTerrain->m_indexItems["grass"].size();
	grassSubmesh.StartIndexLocation = grassIndexOffset;
	grassSubmesh.BaseVertexLocation = grassVertexOffset;

	SubmeshGeometry roadSubmesh;
	roadSubmesh.IndexCount = m_pTerrain->m_indexItems["road"].size();
	roadSubmesh.StartIndexLocation = roadIndexOffset;
	roadSubmesh.BaseVertexLocation = roadVertexOffset;

	SubmeshGeometry waterSubmesh;
	waterSubmesh.IndexCount = m_pTerrain->m_indexItems["waterBottom"].size();
	waterSubmesh.StartIndexLocation = waterIndexOffset;
	waterSubmesh.BaseVertexLocation = waterVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	auto totalVertexCount =
		m_pTerrain->m_vertexItems["ground"].size() + m_pTerrain->m_vertexItems["grass"].size()
		+ m_pTerrain->m_vertexItems["road"].size() + m_pTerrain->m_vertexItems["waterBottom"].size() + sphere.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);
	UINT k = 0;
	for (size_t i = 0; i < m_pTerrain->m_vertexItems["ground"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_pTerrain->m_vertexItems["ground"][i].Pos;
		vertices[k].Normal = m_pTerrain->m_vertexItems["ground"][i].Normal;
		vertices[k].TexC = m_pTerrain->m_vertexItems["ground"][i].TexC;
		vertices[k].TangentU = m_pTerrain->m_vertexItems["ground"][i].TangentU;
	}

	for (size_t i = 0; i < m_pTerrain->m_vertexItems["grass"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_pTerrain->m_vertexItems["grass"][i].Pos;
		vertices[k].Normal = m_pTerrain->m_vertexItems["grass"][i].Normal;
		vertices[k].TexC = m_pTerrain->m_vertexItems["grass"][i].TexC;
		vertices[k].TangentU = m_pTerrain->m_vertexItems["grass"][i].TangentU;
	}

	for (size_t i = 0; i < m_pTerrain->m_vertexItems["road"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_pTerrain->m_vertexItems["road"][i].Pos;
		vertices[k].Normal = m_pTerrain->m_vertexItems["road"][i].Normal;
		vertices[k].TexC = m_pTerrain->m_vertexItems["road"][i].TexC;
		vertices[k].TangentU = m_pTerrain->m_vertexItems["road"][i].TangentU;
	}

	for (size_t i = 0; i < m_pTerrain->m_vertexItems["waterBottom"].size(); ++i, ++k)
	{
		vertices[k].Pos = m_pTerrain->m_vertexItems["waterBottom"][i].Pos;
		vertices[k].Normal = m_pTerrain->m_vertexItems["waterBottom"][i].Normal;
		vertices[k].TexC = m_pTerrain->m_vertexItems["waterBottom"][i].TexC;
		vertices[k].TangentU = m_pTerrain->m_vertexItems["waterBottom"][i].TangentU;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	std::vector<UINT> indices;
	indices.insert(indices.end(), m_pTerrain->m_indexItems["ground"].begin(), m_pTerrain->m_indexItems["ground"].end());
	indices.insert(indices.end(), m_pTerrain->m_indexItems["grass"].begin(), m_pTerrain->m_indexItems["grass"].end());
	indices.insert(indices.end(), m_pTerrain->m_indexItems["road"].begin(), m_pTerrain->m_indexItems["road"].end());
	indices.insert(indices.end(), m_pTerrain->m_indexItems["waterBottom"].begin(), m_pTerrain->m_indexItems["waterBottom"].end());
	indices.insert(indices.end(), std::begin(sphere.Indices32), std::end(sphere.Indices32));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "terrainGeo";

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
	geo->DrawArgs["waterBottom"] = waterSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;

	m_geometries[geo->Name] = std::move(geo);
}

void ShadowDemo::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(10.0f, 10.0f, 1.0f, 3);
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	UINT boxVertexOffset = 0;
	UINT quadVertexOffset = boxVertexOffset + (UINT)box.Vertices.size();
	UINT boxIndexOffset = 0;
	UINT quadIndexOffset = boxIndexOffset + (UINT)box.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubmesh.StartIndexLocation = quadIndexOffset;
	quadSubmesh.BaseVertexLocation = quadVertexOffset;

	auto totalVertexCount = box.Vertices.size() + quad.Vertices.size();


	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}
	for (int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TexC = quad.Vertices[i].TexC;
		vertices[k].TangentU = quad.Vertices[i].TangentU;
	}

	std::vector<UINT> indices;
	indices.insert(indices.end(), std::begin(box.Indices32), std::end(box.Indices32));
	indices.insert(indices.end(), std::begin(quad.Indices32), std::end(quad.Indices32));

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

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["quad"] = quadSubmesh;

	m_geometries[geo->Name] = std::move(geo);
}

void ShadowDemo::BuildWavesGeometry()
{
		std::vector<UINT> indices(3 * m_waves->TriangleCount()); // 3 indices per face
		assert(m_waves->VertexCount() < 0x0000ffff);

		// Iterate over each quad.
		int m = m_waves->RowCount();
		int n = m_waves->ColumnCount();
		int k = 0;
		for (int i = 0; i < m - 1; ++i)
		{
			for (int j = 0; j < n - 1; ++j)
			{
				indices[k] = i*n + j;
				indices[k + 1] = i*n + j + 1;
				indices[k + 2] = (i + 1)*n + j;
				indices[k + 3] = (i + 1)*n + j;
				indices[k + 4] = i*n + j + 1;
				indices[k + 5] = (i + 1)*n + j + 1;

				k += 6; // next quad
			}
		}

		UINT vbByteSize = m_waves->VertexCount() * sizeof(Vertex);
		UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "waterGeo";

		// Set dynamically.
		geo->VertexBufferCPU = nullptr;
		geo->VertexBufferGPU = nullptr;

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pD3dDevice.Get(),
			m_pCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R32_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["waterGrid"] = submesh;

		m_geometries["waterGeo"] = std::move(geo);
}

void ShadowDemo::BuildModelsGeometry()
{
	GeometryGenerator geoGen;
	std::vector<ModelImporter::ModelVertex> vertices;
	std::vector<UINT> indices;

	for (int i = 0; i < m_pModelImporter->m_meshes.size(); ++i)
	{
		vertices.insert(vertices.end(),
			m_pModelImporter->m_meshes[i].vertices.begin(),
			m_pModelImporter->m_meshes[i].vertices.end());
		indices.insert(indices.end(),
			m_pModelImporter->m_meshes[i].indices.begin(),
			m_pModelImporter->m_meshes[i].indices.end());
	}
	
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(ModelImporter::ModelVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = m_pModelImporter->m_name;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pD3dDevice.Get(),
		m_pCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pD3dDevice.Get(),
		m_pCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(ModelImporter::ModelVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	int startOffset = 0;
	for (UINT i = 0; i < m_pModelImporter->m_meshes.size(); ++i)
	{
		SubmeshGeometry submesh;
		std::string name = "sm_" + std::to_string(i);

		submesh.IndexCount = (UINT)m_pModelImporter->m_meshes[i].indices.size();
		submesh.StartIndexLocation = startOffset;
		submesh.BaseVertexLocation = 0;
		startOffset += submesh.IndexCount;

		geo->DrawArgs[name] = submesh;
	}

	m_geometries[geo->Name] = std::move(geo);
}

void ShadowDemo::BuildPSOs()
{
	//pso for opaque
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
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	// PSO for shadow map pass.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = m_pRootSignature.Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["shadowVS"]->GetBufferPointer()),
		m_shaders["shadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["shadowOpaquePS"]->GetBufferPointer()),
		m_shaders["shadowOpaquePS"]->GetBufferSize()
	};

	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&m_PSOs["shadow_opaque"])));

	/* PSO for debug layer.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaquePsoDesc;
	debugPsoDesc.pRootSignature = m_pRootSignature.Get();
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["debugVS"]->GetBufferPointer()),
		m_shaders["debugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["debugPS"]->GetBufferPointer()),
		m_shaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&m_PSOs["debug"])));*/

	//pso for sky
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = m_pRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["skyVS"]->GetBufferPointer()),
		m_shaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["skyPS"]->GetBufferPointer()),
		m_shaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&m_PSOs["sky"])));

	// pso for transparent object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(m_pD3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&m_PSOs["transparent"])));
}

void ShadowDemo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResource>(m_pD3dDevice.Get(),
			2, (UINT)m_allRenderItems.size(), (UINT)m_materials.size(), m_waves->VertexCount()));
		//2个frame pass -- mainPass和shadowPass
	}
}

void ShadowDemo::BuildMaterials()
{
	auto groudMat = std::make_unique<Material>();
	groudMat->Name = "ground";
	groudMat->MatCBIndex = 0;
	groudMat->DiffuseSrvHeapIndex = 1;
	groudMat->NormalSrvHeapIndex = 2;
	groudMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	groudMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	groudMat->Roughness = 0.6f;

	auto grassMat = std::make_unique<Material>();
	grassMat->Name = "grass";
	grassMat->MatCBIndex = 1;
	grassMat->DiffuseSrvHeapIndex = 3;
	grassMat->NormalSrvHeapIndex = 4;
	grassMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grassMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	grassMat->Roughness = 0.2f;

	auto roadMat = std::make_unique<Material>();
	roadMat->Name = "road";
	roadMat->MatCBIndex = 2;
	roadMat->DiffuseSrvHeapIndex = 5;
	roadMat->NormalSrvHeapIndex = 6;
	roadMat->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.0f);
	roadMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	roadMat->Roughness = 0.8f;

	auto waterBottomMat = std::make_unique<Material>();
	waterBottomMat->Name = "waterBottom";
	waterBottomMat->MatCBIndex = 3;
	waterBottomMat->DiffuseSrvHeapIndex = 7;
	waterBottomMat->NormalSrvHeapIndex = 8;
	waterBottomMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	waterBottomMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	waterBottomMat->Roughness = 0.5f;

	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 4;
	water->DiffuseSrvHeapIndex = 9; 
	water->NormalSrvHeapIndex = 0;//无法线贴图,必须有法线 贴图？？
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);//阿尔法值为0.5
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 1.0f;

	auto sky = std::make_unique<Material>();
	sky->Name = "sky";
	sky->MatCBIndex = 5;
	sky->DiffuseSrvHeapIndex = 10;
	sky->NormalSrvHeapIndex = 0;//无法线贴图
	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->Roughness = 1.0f;
	
	m_materials["ground"] = std::move(groudMat);
	m_materials["grass"] = std::move(grassMat);
	m_materials["road"] = std::move(roadMat);
	m_materials["waterBottom"] = std::move(waterBottomMat);
	m_materials["water"] = std::move(water);
	m_materials["sky"] = std::move(sky);

	UINT matCBIndex = 6;
	UINT srvHeapIndex = m_modelSrvHeapIndex;
	for (UINT i = 0; i < m_pModelImporter->m_meshes.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Name = m_pModelImporter->m_meshes[i].material.Name;
		mat->MatCBIndex = matCBIndex++;
		mat->DiffuseSrvHeapIndex = srvHeapIndex++;
		if (m_pModelImporter->m_meshes[i].material.NormalMapName != "")	//有法线贴图
			mat->NormalSrvHeapIndex = srvHeapIndex++;
		else
			mat->NormalSrvHeapIndex = 0;
		mat->DiffuseAlbedo = m_pModelImporter->m_meshes[i].material.DiffuseAlbedo;
		mat->FresnelR0 = m_pModelImporter->m_meshes[i].material.FresnelR0;
		mat->Roughness = m_pModelImporter->m_meshes[i].material.Roughness;

		m_materials[mat->Name] = std::move(mat);
	}
}

void ShadowDemo::BuildRenderItems()
{		
	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->world, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->texTransform = MathHelper::Identity4x4();
	skyRitem->objCBIndex = 0;
	skyRitem->mat = m_materials["sky"].get();
	skyRitem->geo = m_geometries["terrainGeo"].get();
	skyRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->indexCount = skyRitem->geo->DrawArgs["sphere"].IndexCount;
	skyRitem->startIndexLocation = skyRitem->geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->baseVertexLocation = skyRitem->geo->DrawArgs["sphere"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	m_allRenderItems.push_back(std::move(skyRitem));

	auto groundRitem = std::make_unique<RenderItem>();
	groundRitem->objCBIndex = 1;
	groundRitem->mat = m_materials["ground"].get();
	groundRitem->geo = m_geometries["terrainGeo"].get();
	//纹理变换矩阵，针对地形贴图具体大小做调整
	XMStoreFloat4x4(&groundRitem->texTransform, XMMatrixScaling(.02f, 0.02f, 1.f));
	groundRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	groundRitem->indexCount = groundRitem->geo->DrawArgs["ground"].IndexCount;
	groundRitem->startIndexLocation = groundRitem->geo->DrawArgs["ground"].StartIndexLocation;
	groundRitem->baseVertexLocation = groundRitem->geo->DrawArgs["ground"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Opaque].push_back(groundRitem.get());
	m_allRenderItems.push_back(std::move(groundRitem));
	
	auto grassRitem = std::make_unique<RenderItem>();
	grassRitem->objCBIndex = 2;
	grassRitem->mat = m_materials["grass"].get();
	grassRitem->geo = m_geometries["terrainGeo"].get();
	XMStoreFloat4x4(&grassRitem->texTransform, XMMatrixScaling(0.1f, 0.1f, 1.f));
	grassRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	grassRitem->indexCount = grassRitem->geo->DrawArgs["grass"].IndexCount;
	grassRitem->startIndexLocation = grassRitem->geo->DrawArgs["grass"].StartIndexLocation;
	grassRitem->baseVertexLocation = grassRitem->geo->DrawArgs["grass"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Opaque].push_back(grassRitem.get());
	m_allRenderItems.push_back(std::move(grassRitem));
	
	auto roadRitem = std::make_unique<RenderItem>();
	roadRitem->objCBIndex = 3;
	roadRitem->mat = m_materials["road"].get();
	roadRitem->geo = m_geometries["terrainGeo"].get();
	XMStoreFloat4x4(&roadRitem->texTransform, XMMatrixScaling(0.03f, 0.03f, 1.f));
	roadRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	roadRitem->indexCount = roadRitem->geo->DrawArgs["road"].IndexCount;
	roadRitem->startIndexLocation = roadRitem->geo->DrawArgs["road"].StartIndexLocation;
	roadRitem->baseVertexLocation = roadRitem->geo->DrawArgs["road"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Opaque].push_back(roadRitem.get());
	m_allRenderItems.push_back(std::move(roadRitem));

	auto waterRitem = std::make_unique<RenderItem>();
	waterRitem->objCBIndex = 4;
	waterRitem->mat = m_materials["waterBottom"].get();
	waterRitem->geo = m_geometries["terrainGeo"].get();
	XMStoreFloat4x4(&waterRitem->texTransform, XMMatrixScaling(0.02f, 0.02f, 1.f));
	waterRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	waterRitem->indexCount = waterRitem->geo->DrawArgs["waterBottom"].IndexCount;
	waterRitem->startIndexLocation = waterRitem->geo->DrawArgs["waterBottom"].StartIndexLocation;
	waterRitem->baseVertexLocation = waterRitem->geo->DrawArgs["waterBottom"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Opaque].push_back(waterRitem.get());
	m_allRenderItems.push_back(std::move(waterRitem));
	
	auto wavesRitem = std::make_unique<RenderItem>();
	XMMATRIX wavesWorld = XMMatrixTranslation(0.f, 93.f, 0.f);
	XMStoreFloat4x4(&wavesRitem->world, wavesWorld);
	XMStoreFloat4x4(&wavesRitem->texTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->objCBIndex = 5;
	wavesRitem->mat = m_materials["water"].get();
	wavesRitem->geo = m_geometries["waterGeo"].get();
	wavesRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->indexCount = wavesRitem->geo->DrawArgs["waterGrid"].IndexCount;
	wavesRitem->startIndexLocation = wavesRitem->geo->DrawArgs["waterGrid"].StartIndexLocation;
	wavesRitem->baseVertexLocation = wavesRitem->geo->DrawArgs["waterGrid"].BaseVertexLocation;
	m_wavesRitem = wavesRitem.get();
	m_ritemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
	m_allRenderItems.push_back(std::move(wavesRitem));

	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->world, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(100.0f, 120.f, 0.0f));
	XMStoreFloat4x4(&boxRitem->texTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->objCBIndex = 6;
	boxRitem->mat = m_materials["ground"].get();
	boxRitem->geo = m_geometries["shapeGeo"].get();
	boxRitem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->indexCount = boxRitem->geo->DrawArgs["box"].IndexCount;
	boxRitem->startIndexLocation = boxRitem->geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->baseVertexLocation = boxRitem->geo->DrawArgs["box"].BaseVertexLocation;
	m_ritemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	m_allRenderItems.push_back(std::move(boxRitem));

	UINT objCBIndex = 7;
	for (UINT i = 0; i < m_pModelImporter->m_meshes.size(); ++i)
	{
		std::string submeshName = "sm_" + std::to_string(i);

		auto ritem = std::make_unique<RenderItem>();

		// Reflect to change coordinate system from the RHS the data was exported out as.
		XMMATRIX modelScale = XMMatrixScaling(5.f, 5.f, 5.f);
		XMMATRIX modelRot = XMMatrixRotationZ(MathHelper::Pi / 2);
		XMMATRIX modelOffset = XMMatrixTranslation(25.0f, 115.0f, 0.0f);
		XMStoreFloat4x4(&ritem->world, modelScale*modelRot*modelOffset);

		ritem->texTransform = MathHelper::Identity4x4();
		ritem->objCBIndex = objCBIndex++;
		ritem->mat = m_materials[m_pModelImporter->m_meshes[i].material.Name].get();
		ritem->geo = m_geometries[m_pModelImporter->m_name].get();
		ritem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->indexCount = ritem->geo->DrawArgs[submeshName].IndexCount;
		ritem->startIndexLocation = ritem->geo->DrawArgs[submeshName].StartIndexLocation;
		ritem->baseVertexLocation = ritem->geo->DrawArgs[submeshName].BaseVertexLocation;
		m_ritemLayer[(int)RenderLayer::Opaque].push_back(ritem.get());
		m_allRenderItems.push_back(std::move(ritem));
	}
}

void ShadowDemo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = m_pCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->primitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->objCBIndex*objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->indexCount, 1, ri->startIndexLocation, ri->baseVertexLocation, 0);
	}

}

void ShadowDemo::DrawSceneToShadowMap()
{
	m_pCommandList->RSSetViewports(1, &m_pShadowMap->Viewport());
	m_pCommandList->RSSetScissorRects(1, &m_pShadowMap->ScissorRect());

	// Change to DEPTH_WRITE.
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	m_pCommandList->ClearDepthStencilView(m_pShadowMap->Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	m_pCommandList->OMSetRenderTargets(0, nullptr, false, &m_pShadowMap->Dsv());

	// Bind the pass constant buffer for the shadow map pass.
	auto passCB = m_pCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	m_pCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	m_pCommandList->SetPipelineState(m_PSOs["shadow_opaque"].Get());

	DrawRenderItems(m_pCommandList.Get(), m_ritemLayer[(int)RenderLayer::Opaque]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

//定义一些常用的纹理采样方式
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> ShadowDemo::GetStaticSamplers()
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

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return{
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp, shadow };
}