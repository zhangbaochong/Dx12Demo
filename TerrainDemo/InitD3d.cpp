#include "D3d12Base.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitD3d : public D3d12Base
{
public:
	InitD3d(HINSTANCE hInstance);
	~InitD3d();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
};

/**
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitD3d theApp(hInstance);
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
*/
InitD3d::InitD3d(HINSTANCE hInstance)
	: D3d12Base(hInstance)
{
}

InitD3d::~InitD3d()
{
}

bool InitD3d::Initialize()
{
	if (!D3d12Base::Initialize())
		return false;

	return true;
}

void InitD3d::OnResize()
{
	D3d12Base::OnResize();
}

void InitD3d::Update(const GameTimer& gt)
{

}

void InitD3d::Draw(const GameTimer& gt)
{
	ThrowIfFailed(m_pCommandAllocator->Reset());
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//设置viewport和rect
	m_pCommandList->RSSetViewports(1, &m_screenViewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

	// Clear the back buffer and depth buffer.
	m_pCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Green, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// set render target
	m_pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_pCommandList->Close());

	//添加命令列表到队列中并执行
	ID3D12CommandList* cmdsLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(m_pSwapChain->Present(0, 0));
	m_currBackBuffer = (m_currBackBuffer + 1) % ms_swapChainBufferCount;

	FlushCommandQueue();
}
