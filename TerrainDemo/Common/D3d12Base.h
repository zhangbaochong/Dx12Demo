#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Common\d3dUtil.h"
#include "Common\GameTimer.h"
#include "Common\Input.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3d12Base
{
public:
	D3d12Base(HINSTANCE hInstance);
	D3d12Base(const D3d12Base& rhs) = delete;//禁止拷贝
	D3d12Base& operator=(const D3d12Base& rhs) = delete;
	virtual ~D3d12Base();

public:
	static D3d12Base* GetApp();

	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

	bool Get4xMsaaState()const;
	void Set4xMsaaState(bool value);

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	//改变大小
	virtual void OnResize();						
	virtual void Update(const GameTimer& gt) = 0;
	virtual void Draw(const GameTimer& gt) = 0;

	//按键响应  后面封装了Input类，不再使用
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	//初始化
	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

	static D3d12Base* m_app;

	HINSTANCE m_hInstance = nullptr; 
	HWND      m_hwnd = nullptr; 
	bool      m_isAppPaused = false;		// 是否暂停
	bool      m_isMinimized = false;		// 是否最小化
	bool      m_isMaximized = false;		// 是否最大化
	bool      m_isResizing = false;			//是否正在被拖拽
	bool      m_isFullscreenState = false;	// 全屏？

									   // 是否采用多重采样
	bool      m_4xMsaaState = false;    
	UINT      m_4xMsaaQuality = 0;      // quality level of 4X MSAA

									 
	GameTimer m_timer;

	//dxgi工厂
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDxgiFactory;
	//交换链
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
	//设备
	Microsoft::WRL::ComPtr<ID3D12Device> m_pD3dDevice;

	//障碍，保证上一帧渲染完毕
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_currentFence = 0;

	//命令队列
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	//命令分配器
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	//图像命令列表
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	static const int ms_swapChainBufferCount = 2;
	int m_currBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pSwapChainBuffer[ms_swapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDepthStencilBuffer;

	//渲染呈现目标描述符
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDsvHeap;

	//视口
	D3D12_VIEWPORT m_screenViewport;									
	D3D12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbvSrvUavDescriptorSize = 0;

	//驱动类型、format、显示长宽等等
	std::wstring m_mainWndCaption = L"d3d12Base";
	D3D_DRIVER_TYPE m_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int m_clientWidth = 800;
	int m_clientHeight = 600;

};

