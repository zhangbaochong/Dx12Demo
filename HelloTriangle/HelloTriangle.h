#pragma once

#include "DxBase.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class HelloTriangle : public DxBase
{
public:
	HelloTriangle(UINT width, UINT height, std::wstring name);
	
	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Destroy();

private:
	static const UINT m_frameCount = 2;

	//顶点结构
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	D3D12_VIEWPORT m_viewPort;								//视口
	D3D12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_pSwapChain;					//交换链
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12Resource> m_pRenderTargets[m_frameCount];
	ComPtr<ID3D12CommandAllocator>	m_pCommandAllocator;	//命令分配器
	ComPtr<ID3D12CommandQueue>	m_pCommandQueue;			//命令队列
	ComPtr<ID3D12RootSignature> m_pRootSignature;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
	ComPtr<ID3D12PipelineState> m_pPipelineState;			//管线状态
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;		//图像命令列表
	UINT m_rtvDescriptorSize;

	ComPtr<ID3D12Resource> m_vertexBuffer;					//顶点缓冲
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	UINT m_frameIndex;					//fence以帧为单位，可以等待所有针对这一帧的命令.利用ID3D12Fence::SetEventOnCompletion
										//和ID3D12CommandQueue::Signal进行信号操作, 完了之后等待即可,
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_fenceValue;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};

