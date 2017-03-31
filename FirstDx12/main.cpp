#include <windows.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <comdef.h>
#include <wrl.h>
#include <string>
#include <d3dcompiler.h>
#include <cassert>
#include "d3dx12.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

using Microsoft::WRL::ComPtr;

HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
LPCWSTR g_name = L"FirstD3D12Demo";
int g_clientWidth = 800;
int g_clientHeight = 600;

ComPtr<IDXGIFactory4> g_pDxgiFactory;					//dxgi工厂
ComPtr<IDXGISwapChain> g_pSwapChain;					//交换链
ComPtr<ID3D12Device> g_pD3dDevice;						//设备
ComPtr<ID3D12CommandQueue> g_pCommandQueue;				//命令队列
ComPtr<ID3D12CommandAllocator> g_pDirectCmdListAlloc;	//命令分配器
ComPtr<ID3D12GraphicsCommandList> g_pCommandList;		//图像命令列表

ComPtr<ID3D12DescriptorHeap> g_pRtvHeap;				//渲染呈现目标描述符

D3D12_VIEWPORT g_screenViewPort;
D3D12_RECT g_scissorRect;

ComPtr<ID3D12Resource> g_pTargetBuffer;

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool InitDirect3D();
void Render();


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
	if (FAILED(InitWindow(hInstance, nShowCmd)))
		return 0;
	if (FAILED(InitDirect3D()))
		return 0;

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else//渲染
		{
			Render();
		}
	}

	return static_cast<int>(msg.wParam);
}

//初始化窗口
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbClsExtra = 0;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = 0;
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wcex.hIconSm = wcex.hIcon;
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = g_name;
	wcex.lpszMenuName = NULL;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	g_hInst = hInstance;
	RECT rc{ 0,0,g_clientWidth,g_clientHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindowEx(WS_EX_APPWINDOW, g_name, g_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, g_hInst, NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	return S_OK;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wPararm, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wPararm, lParam);
	}
	return 0;
}


//初始化direct3d
bool InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();
	}
#endif
	//创建dxgi工厂
	if (FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&g_pDxgiFactory))))
	{
		MessageBox(nullptr, L"create dxgi factory failed!", MB_OK, 0);
		return false;
	}

	//创建设备
	HRESULT hr = D3D12CreateDevice(
		nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&g_pD3dDevice));



	//创建命令分配器
	if (FAILED(g_pD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(
		g_pDirectCmdListAlloc.GetAddressOf()))))
	{
		MessageBox(nullptr, L"create command alloc failed!", MB_OK, 0);
		return false;
	}

	//创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc;
	ZeroMemory(&queueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(g_pD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue))))
	{
		MessageBox(nullptr, L"create command queue failed!", MB_OK, 0);
		return false;
	}
	
	//创建命令列表
	if (FAILED(g_pD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pDirectCmdListAlloc.Get(),
		nullptr, IID_PPV_ARGS(&g_pCommandList))))
	{
		MessageBox(nullptr, L"create command list failed!", MB_OK, 0);
		return false;
	}

	//创建交换链
	g_pSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = g_clientWidth;
	sd.BufferDesc.Height = g_clientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;//?
	sd.OutputWindow = g_hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	if (FAILED(g_pDxgiFactory->CreateSwapChain(
		g_pCommandQueue.Get(),
		&sd,
		g_pSwapChain.GetAddressOf())))
	{
		MessageBox(nullptr, L"create swap chain failed!", MB_OK, 0);
		return false;
	}

	//获取缓冲区
	if (FAILED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&g_pTargetBuffer))))
	{
		MessageBox(nullptr, L"get buffer failed!", MB_OK, 0);
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	g_pD3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(g_pRtvHeap.GetAddressOf()));

	//创建render target view
	g_pD3dDevice->CreateRenderTargetView(g_pTargetBuffer.Get(), nullptr,
		g_pRtvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}


void Render()
{
	g_pDirectCmdListAlloc->Reset();
	g_pCommandList->Reset(g_pDirectCmdListAlloc.Get(), nullptr);

	
	//设置viewport和scissor rect
	g_pCommandList->RSSetViewports(1, &g_screenViewPort);
	g_pCommandList->RSSetScissorRects(1, &g_scissorRect);

	D3D12_RESOURCE_BARRIER Barrier;
	Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource = g_pTargetBuffer.Get();
	Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	g_pCommandList->ResourceBarrier(1, &Barrier);

	FLOAT color[] = { 0.8f,0.5f,1.f,1.f };
	g_pCommandList->ClearRenderTargetView(g_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), color,0, nullptr);
	
	g_pCommandList->ResourceBarrier(1, &Barrier);

	g_pCommandList->Close();

	//执行命令列表
	ID3D12CommandList* const lists = g_pCommandList.Get();
	g_pCommandQueue->ExecuteCommandLists(1, &lists);

	//swap back and front buffer  即呈现画面
	g_pSwapChain->Present(0, 0);

	//重置
	g_pDirectCmdListAlloc->Reset();
	g_pCommandList->Reset(g_pDirectCmdListAlloc.Get(),nullptr);

}

