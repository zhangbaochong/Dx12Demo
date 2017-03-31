#include "HelloTriangle.h"
#include "Win32App.h"


HelloTriangle::HelloTriangle(UINT width, UINT height, std::wstring name):
	DxBase(width,height,name),
	m_frameIndex(0),
	m_viewPort{ 0.f,0.f,static_cast<float>(width),static_cast<float>(height) },
	m_scissorRect{0,0,static_cast<LONG>(width),static_cast<LONG>(height)},
	m_rtvDescriptorSize(0)
{

}

void HelloTriangle::Init()
{
	LoadPipeline();
	LoadAssets();
}


void HelloTriangle::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

	//enable debug layer
#if defined (_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif // (_DEBUG)

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_isUseWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		//创建设备
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_pDevice)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);
		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_pDevice)
		));
	}

	//创建command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	//创建交换链
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = m_frameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(),
		Win32App::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain));

	//不支持全屏转变
	ThrowIfFailed(factory->MakeWindowAssociation(Win32App::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_pSwapChain));

	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();


	//创建 descriptor heaps.
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = m_frameCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pRtvHeap)));
		m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//创建每一帧资源
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
		//对于each frame 创建一个rtv
		for (UINT i = 0; i < m_frameCount; ++i)
		{
			ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])));
			m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator)));
}

//加载资源
void HelloTriangle::LoadAssets()
{
	//创建empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC desc = {};
		desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), 
			error.GetAddressOf()));
		ThrowIfFailed(m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), 
			IID_PPV_ARGS(&m_pRootSignature)));
	}

	//create pipeline state，编译加载shader
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
	
		//vertex input layout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	
		//创建pipeline state object
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));	
	}

	//创建command list
	ThrowIfFailed(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(),
		m_pPipelineState.Get(), IID_PPV_ARGS(&m_pCommandList)));
	ThrowIfFailed(m_pCommandList->Close());

	//创建vertex buffer
	{
		Vertex vertices[] =
		{
			{ { 0.0f, 0.25f * m_aspectRatio, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(vertices);

		ThrowIfFailed(m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)
		));

		//复制三角形data到vertex buffer
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices, sizeof(vertices));
		m_vertexBuffer->Unmap(0, nullptr);

		//初始化vertex buffer view
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	//创建synchronization objects，等待直到assets被加载到gpu
	{
		ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		WaitForPreviousFrame();
	}
}

//更新一帧数据
void HelloTriangle::Update()
{

}

//每一帧渲染
void HelloTriangle::Render()
{
	//record所有命令要渲染的
	PopulateCommandList();

	// 执行命令列表
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 渲染
	ThrowIfFailed(m_pSwapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void HelloTriangle::Destroy()
{
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

//记录要执行的命令列表
void HelloTriangle::PopulateCommandList()
{
	ThrowIfFailed(m_pCommandAllocator->Reset());

	//reset
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get()));

	// Set necessary state.
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	m_pCommandList->RSSetViewports(1, &m_viewPort);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	//清除rtv，设置拓扑结构，设置顶点缓冲，然后渲染
	const float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_pCommandList->DrawInstanced(3, 1, 0, 0);

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex].Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_pCommandList->Close());
}

void HelloTriangle::WaitForPreviousFrame()
{
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), fence));
	m_fenceValue++;

	// 等待直到上一帧结束.
	if (m_pFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_pFence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}